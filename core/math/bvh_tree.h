#pragma once

#include "core/math/aabb.h"
#include "core/pooled_list.h"

#define BVH_DEBUG_DRAW
#ifdef BVH_DEBUG_DRAW
#include "scene/3d/immediate_geometry.h"
#endif

#if defined (TOOLS_ENABLED) && defined (DEBUG_ENABLED)
#define BVH_VERBOSE
#define BVH_VERBOSE_TREE
//#define BVH_VERBOSE_FRAME
#define BVH_CHECKS
#endif

#ifdef BVH_VERBOSE
#define VERBOSE_PRINT print_line
#else
#define VERBOSE_PRINT(a)
#endif

// really a handle, can be anything
struct BVH_Handle
{
	uint32_t id;
};


template <class T, int MAX_CHILDREN, int MAX_ITEMS>
class BVH_Tree
{
#include "bvh_structs.inc"

public:
	BVH_Tree()
	{
		_root_node_id = -1;
	}

private:
	bool node_add_child(uint32_t p_node_id, uint32_t p_child_node_id)
	{
		TNode &tnode = _nodes[p_node_id];
		if (tnode.is_full_of_children())
			return false;

		TNode &tnode_child = _nodes[p_child_node_id];
		// back link in the child to the parent
		tnode_child.parent_tnode_id_p1 = p_node_id + 1;

		uint32_t id;
		Item * pChild = tnode.request_item(id);
		pChild->aabb = tnode_child.aabb;
		pChild->item_ref_id = p_child_node_id;

		return true;
	}


	void node_remove_child(uint32_t p_node_id, uint32_t p_child_node_id)
	{
		TNode &tnode = _nodes[p_node_id];

		int child_num = tnode.find_child(p_child_node_id);
		CRASH_COND(child_num == -1);

		tnode.remove_item_internal(child_num);

		// no need to keep back references for children at the moment

		if (tnode.num_items)
			return;

		// now there may be no children in this node .. in which case it can be deleted
		// remove node if empty
		// remove link from parent
		if (tnode.parent_tnode_id_p1)
		{
			// the node number is stored +1 based (i.e. 1 is 0, so that 0 indicates NULL)
			uint32_t parent_node_id = tnode.parent_tnode_id_p1-1;

			node_remove_child(parent_node_id, p_node_id);

			// put the node on the free list to recycle
			_nodes.free(p_node_id);
		}
	}

	void node_remove_item(uint32_t p_ref_id)
	{
		// get the reference
		ItemRef &ref = _refs[p_ref_id];
		uint32_t owner_node_id = ref.tnode_id;

		TNode &tnode = _nodes[owner_node_id];
		tnode.remove_item_internal(ref.item_id);

		if (tnode.num_items)
		{
			// the swapped item has to have its reference changed to, to point to the new item id
			uint32_t swapped_ref_id = tnode.items[ref.item_id].item_ref_id;

			ItemRef &swapped_ref = _refs[swapped_ref_id];

			swapped_ref.item_id = ref.item_id;

			refit_downward(_root_node_id);
			//recursive_node_update_aabb_upward(owner_node_id);
		}
		else
		{
			// remove node if empty
			// remove link from parent
			if (tnode.parent_tnode_id_p1)
			{
				// the node number is stored +1 based (i.e. 1 is 0, so that 0 indicates NULL)
				uint32_t parent_node_id = tnode.parent_tnode_id_p1-1;

				node_remove_child(parent_node_id, owner_node_id);


				refit_downward(_root_node_id);
//				recursive_node_update_aabb_upward(parent_node_id);

				// put the node on the free list to recycle
				_nodes.free(owner_node_id);
			}

			// else if no parent, it is the root node. Do not delete
		}

		ref.tnode_id = -1;
		ref.item_id = -1; // unset
	}

public:

	void _node_add_item(uint32_t p_node_id, uint32_t p_ref_id, const AABB &p_aabb)
	{
		ItemRef &ref = _refs[p_ref_id];
		ref.tnode_id = p_node_id;

		TNode &node = _nodes[p_node_id];
		Item * item = node.request_item(ref.item_id);

		// set the aabb of the new item
		item->aabb = p_aabb;

		// back reference on the item back to the item reference
		item->item_ref_id = p_ref_id;
	}

	uint32_t _create_another_child(uint32_t p_node_id, const AABB &p_aabb)
	{
		uint32_t child_node_id;
		TNode * child_node = _nodes.request(child_node_id);
		child_node->clear();

		// may not be necessary
		child_node->aabb = p_aabb;

		node_add_child(p_node_id, child_node_id);

		return child_node_id;
	}

	uint32_t _create_sibling(uint32_t p_node_id, const AABB &p_aabb)
	{
		VERBOSE_PRINT("_create_sibling");

		TNode &tnode = _nodes[p_node_id];
		uint32_t old_parent_id_p1 = tnode.parent_tnode_id_p1;

		// create the new common parent
		uint32_t new_parent_id;
		TNode * new_parent = _nodes.request(new_parent_id);
		new_parent->clear(false);

		// it is possible we are creating a new root node
		if (p_node_id == _root_node_id)
		{
			_root_node_id = new_parent_id;
		}
		else
		{
			// if there was an old parent, remove the node as a child, and add the new parent as a child
			CRASH_COND(old_parent_id_p1 == 0);
			TNode &t_old_parent = _nodes[old_parent_id_p1-1];
			int child_num = t_old_parent.find_child(p_node_id);
			CRASH_COND(child_num == -1);

			// exchange the child (very cheap)
			Item &item = t_old_parent.get_item(child_num);
			item.item_ref_id = new_parent_id;
		}

		// either a parent node or root
		new_parent->parent_tnode_id_p1 = tnode.parent_tnode_id_p1;

		// add the node as first child to the new parent
		node_add_child(new_parent_id, p_node_id);

		// add a new sibling to hold the new aabb
		return _create_another_child(new_parent_id, p_aabb);
	}


	// either choose an existing node to add item to, or create a new node and return this
	uint32_t recursive_choose_item_add_node(uint32_t p_node_id, const AABB &p_aabb)
	{
		TNode &tnode = _nodes[p_node_id];

		/*
		// if they don't overlap, and the node is not a non leaf which is non full
		if (tnode.num_items && !p_aabb.intersects(tnode.aabb))
		{
			if (!tnode.is_leaf() || !tnode.is_full_of_children())
			{
			}
			else
			{
				// create sibling to this node
				return _create_sibling(p_node_id, p_aabb);
			}
		}
		*/

		// if not a leaf node
		if (!tnode.is_leaf())
		{
			// first choice is, if there are not max children, create another child node
			if (!tnode.is_full_of_children())
			{
				return _create_another_child(p_node_id, p_aabb);
			}

			// there are children already
			float best_goodness_fit = FLT_MAX;
			int best_child = -1;

			// find the best child and recurse into that
			for (int n=0; n<tnode.num_items; n++)
			{

						float fit = _goodness_of_fit_merge(p_aabb, tnode.get_item(n).aabb);
						if (fit < best_goodness_fit)
						{
							best_goodness_fit = fit;
							best_child = n;
						}
			}

			CRASH_COND(best_child == -1);
			return recursive_choose_item_add_node(tnode.get_item(best_child).item_ref_id, p_aabb);
		} // if not a leaf

		// if we got to here, must be a leaf

		// is it full?
		if (!tnode.is_full_of_items())
			return p_node_id;

		// if it is full, and a leaf node, we will split the leaf into children, then
		// recurse into the children
		split_leaf(p_node_id);

		return recursive_choose_item_add_node(p_node_id, p_aabb);
	}

	void _split_leaf_sort_groups(const TNode &tnode, int &num_a, int &num_b, uint8_t * group_a, uint8_t * group_b)
	{
		AABB groupb_aabb;
		_set_aabb_to_opposites(groupb_aabb);
		for (int n=0; n<num_b; n++)
		{
			int which = group_b[n];
			groupb_aabb.merge_with(tnode.get_item(which).aabb);
		}
		AABB groupb_aabb_new;

		AABB rest_aabb;

		float best_size = FLT_MAX;
		int best_candidate = -1;

		// find most likely from a to move into b
		for (int check = 0; check < num_a; check++)
		{
			_set_aabb_to_opposites(rest_aabb);
			groupb_aabb_new = groupb_aabb;

			// find aabb of all the rest
			for (int rest=0; rest< num_a; rest++)
			{
				if (rest == check)
					continue;

				int which = group_a[rest];
				const Item &item = tnode.get_item(which);
				rest_aabb.merge_with(item.aabb);
			}

			groupb_aabb_new.merge_with(tnode.get_item(group_a[check]).aabb);

			// now compare the sizes
			float size = _aabb_size(groupb_aabb_new) + _aabb_size(rest_aabb);
			if (size < best_size)
			{
				best_size = size;
				best_candidate = check;
			}
		}

		// we should now have the best, move it from group a to group b
		group_b[num_b++] = group_a[best_candidate];

		// remove best candidate from group a
		num_a--;
		group_a[best_candidate] = group_a[num_a];

	}


	void split_leaf(uint32_t p_node_id)
	{
		VERBOSE_PRINT("split_leaf");

		// first create child leaf nodes
		uint32_t * child_ids = (uint32_t *) alloca(sizeof (uint32_t) * MAX_CHILDREN);

		for (int n=0; n<MAX_CHILDREN; n++)
		{
			TNode * child_node = _nodes.request(child_ids[n]);
			child_node->clear();

			// back link to parent
			child_node->parent_tnode_id_p1 = p_node_id + 1;
		}

		CRASH_COND(MAX_ITEMS < MAX_CHILDREN);
		TNode &tnode = _nodes[p_node_id];

		// mark as no longer a leaf node
		tnode.set_leaf(false);

		// 2 groups, A and B, and assign children to each to split equally
		uint8_t * group_a = (uint8_t *) alloca(sizeof (uint8_t) * MAX_CHILDREN);
		uint8_t * group_b = (uint8_t *) alloca(sizeof (uint8_t) * MAX_CHILDREN);

		int num_a = tnode.num_items;
		int num_b = 0;

		// setup - start with all in group a
		for (int n=0; n<tnode.num_items; n++)
		{
			group_a[n] = n;
		}

		int num_moves = num_a / 2;
		for (int m=0; m<num_moves; m++)
		{
			_split_leaf_sort_groups(tnode, num_a, num_b, group_a, group_b);
		}
		// now there should be equal numbers in both groups
		for (int n=0; n<num_a; n++)
		{
			int which = group_a[n];
			const Item &source_item = tnode.get_item(which);
			_node_add_item(child_ids[0], source_item.item_ref_id, source_item.aabb);
		}
		for (int n=0; n<num_b; n++)
		{
			int which = group_b[n];
			const Item &source_item = tnode.get_item(which);
			_node_add_item(child_ids[1], source_item.item_ref_id, source_item.aabb);
		}

		/*
		// move each item to a child node
		for (int n=0; n<tnode.num_items; n++)
		{
			int child_node_id = child_ids[n % MAX_CHILDREN];
			const Item &source_item = tnode.get_item(n);
			_node_add_item(child_node_id, source_item.item_ref_id, source_item.aabb);
		}
		*/

		// now remove all items from the parent and replace with the child nodes
		tnode.num_items = MAX_CHILDREN;
		for (int n=0; n<MAX_CHILDREN; n++)
		{
			// store the child node ids, but not the AABB as not calculated yet
			tnode.items[n].item_ref_id = child_ids[n];
		}

		refit_all();
	}


#include "bvh_misc.inc"
#include "bvh_public.inc"
#include "bvh_debug.inc"

};

#undef VERBOSE_PRINT
