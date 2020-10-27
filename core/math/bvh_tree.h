#pragma once

#include "core/math/aabb.h"
#include "core/pooled_list.h"

#if defined (TOOLS_ENABLED) && defined (DEBUG_ENABLED)
#define VERBOSE_PRINT print_line
#else
#define VERBOSE_PRINT(a)
#endif

// really a handle, can be anything
struct BVH_Handle
{
	uint32_t id;
};


template <class T, int MAX_CHILDREN, int MAX_ITEMS_PER_NODE>
class BVH_Tree
{
	struct ItemRef
	{
		uint32_t tnode_id;
		uint32_t item_id; // in the tnode
	};

	struct Item
	{
		AABB aabb;
		uint32_t item_ref_id;
	};

	// tree node
	struct TNode
	{
		void clear()
		{
			num_items = 0;
			num_children = 0;
			active_child_flags = 0;
			parent_tnode_id_p1 = 0;

			// other members are not blanked for speed .. they may be uninitialized
		}

		bool is_full_of_items() const {return num_items >= MAX_ITEMS_PER_NODE;}
		bool is_full_of_children() const {return num_children >= MAX_CHILDREN;}

		Item * request_item(uint32_t &id)
		{
			if (num_items < MAX_ITEMS_PER_NODE)
			{
				id = num_items;
				return &items[num_items++];
			}
			return nullptr;
		}

		Item &get_item(int id)
		{
			return items[id];
		}

		// ALSO NEEDS the reference changing in the tree, cannot be used on its own
		void remove_item_internal(uint32_t item_id)
		{
			CRASH_COND(item_id >= num_items)
			num_items--;
			// swap with last .. unordered
			items[item_id] = items[num_items];
		}

		void update_aabb_internal()
		{
			// should there even be nodes with no children?
			// the 'blank' aabb will screw up parent aabbs
			if (!num_items)
			{
				aabb = AABB();
				return;
			}

			// items
			aabb = items[0].aabb;
			for (int n=1; n<num_items; n++)
			{
				aabb.merge_with(items[n].aabb);
			}

			// children
			uint32_t mask = 1;
			for (int n=0; n<MAX_CHILDREN; n++)
			{
				if (active_child_flags & mask)
				{
					aabb.merge_with(child_aabbs[n]);
				}
				mask <<= 1;
			}
		}

		void remove_child_number(uint32_t p_child_number)
		{
			CRASH_COND(!num_children);
			CRASH_COND(p_child_number >= MAX_CHILDREN);
			uint32_t mask = 1 << p_child_number;
			// it should be active
			CRASH_COND(!(active_child_flags & mask));
			active_child_flags |= ~mask;
			num_children--;
		}

		uint32_t active_child_flags; // this limits number of children
		uint32_t num_items;
		uint32_t parent_tnode_id_p1;
		uint16_t parent_child_number; // which child this is of the parent node
		uint16_t num_children;

		AABB aabb;
		AABB child_aabbs[MAX_CHILDREN];
		uint32_t child_tnode_ids[MAX_CHILDREN];
		Item items[MAX_ITEMS_PER_NODE];
	};


	// instead of using linked list we maintain
	// item references (for quick lookup)
	PooledList<ItemRef, true> refs;
	PooledList<TNode, true> nodes;
	int _root_node_id;

public:
	BVH_Tree()
	{
		_root_node_id = -1;
	}

private:
	bool node_add_child(uint32_t p_node_id, uint32_t p_child_node_id)
	{
		TNode &tnode = nodes[p_node_id];
		if (tnode.num_children >= MAX_CHILDREN)
			return false;

		TNode &tnode_child = nodes[p_child_node_id];
		// back link in the child to the parent
		tnode_child.parent_tnode_id_p1 = p_node_id + 1;

		// children
		uint32_t mask = 1;
		for (int n=0; n<MAX_CHILDREN; n++)
		{
			if (!(tnode.active_child_flags & mask))
			{
				// mark slot as used
				tnode.active_child_flags |= mask;
				tnode.child_aabbs[n] = tnode_child.aabb;
				tnode.child_tnode_ids[n] = p_child_node_id; // plus one based index
				tnode.num_children++;

				// make a back link .. the slot used in the parent
				tnode_child.parent_child_number = n;
				return true;
			}
			mask <<= 1;
		}

		return false;
	}

	void recursive_node_update_aabb_upward(uint32_t p_node_id)
	{
		uint32_t parent_node_p1 = node_update_aabb(p_node_id);

		if (parent_node_p1)
			recursive_node_update_aabb_upward(parent_node_p1-1);
	}

	// return parent node p1
	uint32_t node_update_aabb(uint32_t p_node_id)
	{
		TNode &tnode = nodes[p_node_id];
		tnode.update_aabb_internal();

		// inform parent of new aabb
		if (tnode.parent_tnode_id_p1)
		{
			// the node number is stored +1 based (i.e. 1 is 0, so that 0 indicates NULL)
			TNode &tnode_parent = nodes[tnode.parent_tnode_id_p1-1];
			CRASH_COND(tnode.parent_child_number >= MAX_CHILDREN);
			tnode_parent.child_aabbs[tnode.parent_child_number] = tnode.aabb;
		}
		return tnode.parent_tnode_id_p1;
	}

	void node_remove_item(uint32_t p_ref_id)
	{
		// get the reference
		ItemRef &ref = refs[p_ref_id];
		uint32_t owner_node_id = ref.tnode_id;

		TNode &tnode = nodes[owner_node_id];
		tnode.remove_item_internal(ref.item_id);

		if (tnode.num_items)
		{
			// the swapped item has to have its reference changed to, to point to the new item id
			uint32_t swapped_ref_id = tnode.items[ref.item_id].item_ref_id;

			ItemRef &swapped_ref = refs[swapped_ref_id];

			swapped_ref.item_id = ref.item_id;

			recursive_node_update_aabb_upward(owner_node_id);
		}
		else
		{
			// remove node if empty
			// remove link from parent
			if (tnode.parent_tnode_id_p1)
			{
				// the node number is stored +1 based (i.e. 1 is 0, so that 0 indicates NULL)
				uint32_t parent_node_id = tnode.parent_tnode_id_p1-1;

				TNode &tnode_parent = nodes[parent_node_id];
				CRASH_COND(tnode.parent_child_number >= MAX_CHILDREN);
				tnode_parent.remove_child_number(tnode.parent_child_number);

				recursive_node_update_aabb_upward(parent_node_id);
			}

			// put the node on the free list to recycle
			nodes.free(owner_node_id);
		}

		ref.tnode_id = -1;
		ref.item_id = -1; // unset
	}

public:
	void create_root_node()
	{
		// if there is no root node, create one
		if (_root_node_id == -1)
		{
			uint32_t root_node_id;
			TNode * node = nodes.request(root_node_id);
			node->clear();
			_root_node_id = root_node_id;
		}
	}

	// either choose an existing node to add item to, or create a new node and return this
	uint32_t recursive_choose_item_add_node(uint32_t p_node_id, const AABB &p_aabb)
	{
		TNode &tnode = nodes[p_node_id];

		// is it full?
		if (!tnode.is_full_of_items())
			return p_node_id;

		// try children
		if (tnode.num_children == 0)
		{
			// add a child
			uint32_t child_node_id;
			TNode * node = nodes.request(child_node_id);
			node->clear();

			// add a link from the parent
			node_add_child(p_node_id, child_node_id);

			// return the child node as the one to add to
			return child_node_id;
		}

		// there are children already
		float best_goodness_fit = -FLT_MAX;
		int best_child = -1;

		uint32_t mask = 1;
		for (int n=0; n<MAX_CHILDREN; n++)
		{
			if (tnode.active_child_flags & mask)
			{
				float fit = _goodness_of_fit_merge(p_aabb, tnode.child_aabbs[n]);
				if (fit > best_goodness_fit)
				{
					best_goodness_fit = fit;
					best_child = n;
				}
			}
			mask <<= 1;
		}

		CRASH_COND(best_child == -1);

		return recursive_choose_item_add_node(tnode.child_tnode_ids[best_child], p_aabb);
	}

	float _aabb_size(const AABB &p_aabb) const
	{
		return p_aabb.size.x * p_aabb.size.y * p_aabb.size.z;
	}

	float _goodness_of_fit_merge(const AABB &p_a, const AABB &p_b) const
	{
		AABB merged = p_a.merge(p_b);
		float m_size = _aabb_size(merged);
		float a_size = _aabb_size(p_a);
		float b_size = _aabb_size(p_b);

		// higher is better
		return -(m_size - (a_size + b_size));
	}

	BVH_Handle item_add(const AABB &p_aabb)
	{
		// handle to be filled with the new item ref
		BVH_Handle handle;

		// ref id easier to pass around than handle
		uint32_t ref_id;

		// this should never fail
		ItemRef * ref = refs.request(ref_id);

		// assign to handle to return
		handle.id = ref_id;

		create_root_node();

		// we must choose where to add to tree
		ref->tnode_id = recursive_choose_item_add_node(_root_node_id, p_aabb);
		TNode &node = nodes[ref->tnode_id];

		// add to tree
//		uint32_t new_node_id;
//		TNode * node = nodes.request(new_node_id);
//		ref->tnode_id = new_node_id; // store in the reference
//		node->clear();

		// request a new item on the node, and return the item id to the reference
		Item * item = node.request_item(ref->item_id);

		// set the aabb of the new item
		item->aabb = p_aabb;

		// back reference on the item back to the item reference
		item->item_ref_id = ref_id;

		recursive_node_update_aabb_upward(ref->tnode_id);

		VERBOSE_PRINT("item_add " + itos(refs.size()) + " refs,\t" + itos(nodes.size()) + " nodes ");

		return handle;
	}

	void item_move(BVH_Handle p_handle, const AABB &p_aabb)
	{
		uint32_t ref_id = p_handle.id;

		// get the reference
		ItemRef &ref = refs[ref_id];

		TNode &tnode = nodes[ref.tnode_id];

//		int id = tnode.find_item(p_ref_id.id);
//		CRASH_COND(id == -1)

		Item &item = tnode.get_item(ref.item_id);
		item.aabb = p_aabb;

		recursive_node_update_aabb_upward(ref.tnode_id);
	}

	void item_remove(BVH_Handle p_handle)
	{
		uint32_t ref_id = p_handle.id;

		// get the reference
		//ItemRef &ref = refs[ref_id];

		// remove the item from the node
		node_remove_item(ref_id);

		// remove the item reference
		refs.free(ref_id);
	}
};

#undef VERBOSE_PRINT
