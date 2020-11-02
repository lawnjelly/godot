#pragma once

#include "core/math/aabb.h"
#include "core/pooled_list.h"

#define BVH_DEBUG_DRAW
#ifdef BVH_DEBUG_DRAW
#include "scene/3d/immediate_geometry.h"
#endif

#if defined (TOOLS_ENABLED) && defined (DEBUG_ENABLED)
//#define BVH_VERBOSE
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
	struct ItemRef
	{
		uint32_t tnode_id;
		uint32_t item_id; // in the tnode
	};

	// this is an item OR a child node depending on whether a leaf node
	struct Item
	{
		AABB aabb;
		uint32_t item_ref_id;
	};

	// tree node
	struct TNode
	{
		uint32_t parent_tnode_id_p1;
		//		uint16_t parent_child_number; // which child this is of the parent node
	private:
		uint16_t _leaf;
	public:
		// items can be child nodes or items (if we are a leaf)
		uint16_t num_items;

		AABB aabb;
		Item items[MAX_ITEMS];

		bool is_leaf() const {return _leaf != 0;}
		void set_leaf(bool b) {_leaf = (uint16_t) b;}

		void clear()
		{
			num_items = 0;
			parent_tnode_id_p1 = 0;
			set_leaf(true);

			// other members are not blanked for speed .. they may be uninitialized
		}

		bool is_full_of_items() const {return num_items >= MAX_ITEMS;}
		bool is_full_of_children() const {return num_items >= MAX_CHILDREN;}

		Item * request_item(uint32_t &id)
		{
			if (num_items < MAX_ITEMS)
			{
				id = num_items;
				return &items[num_items++];
			}
			return nullptr;
		}

		const Item &get_item(int id) const {return items[id];}
		Item &get_item(int id) {return items[id];}

		int find_child(uint32_t p_child_node_id)
		{
			CRASH_COND(is_leaf());

			for (int n=0; n<num_items; n++)
			{
				if (items[n].item_ref_id == p_child_node_id)
					return n;
			}

			// not found
			return -1;
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
#ifdef BVH_CHECKS
				WARN_PRINT_ONCE("BVH_Tree::TNode AABB is undefined");
#endif
				aabb = AABB();
				return;
			}

			// items
			aabb = items[0].aabb;
			for (int n=1; n<num_items; n++)
			{
				aabb.merge_with(items[n].aabb);
			}
		}
	};


	// instead of using linked list we maintain
	// item references (for quick lookup)
	PooledList<ItemRef, true> _refs;
	PooledList<TNode, true> _nodes;
	int _root_node_id;

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

	void refit_upward(uint32_t p_node_id)
	{
		TNode &tnode = _nodes[p_node_id];

		// update children AABBs
		if (!tnode.is_leaf())
		{
			for (int n=0; n<tnode.num_items; n++)
			{
				Item &item = tnode.get_item(n);
				const TNode &tchild = _nodes[item.item_ref_id];

				item.aabb = tchild.aabb;
			}
		}

		// update overall aabb from the children
		tnode.update_aabb_internal();

		if (tnode.parent_tnode_id_p1)
		{
			refit_upward(	tnode.parent_tnode_id_p1-1);
		}
	}

	AABB * refit_downward(uint32_t p_node_id)
	{
		TNode &tnode = _nodes[p_node_id];

		// do children first
		if (!tnode.is_leaf())
		{
			for (int n=0; n<tnode.num_items; n++)
			{
				Item &item = tnode.get_item(n);
				item.aabb = *refit_downward(item.item_ref_id);
			}
		}

		tnode.update_aabb_internal();

		return &tnode.aabb;
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
	void create_root_node()
	{
		// if there is no root node, create one
		if (_root_node_id == -1)
		{
			uint32_t root_node_id;
			TNode * node = _nodes.request(root_node_id);
			node->clear();
			_root_node_id = root_node_id;
		}
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


	// either choose an existing node to add item to, or create a new node and return this
	uint32_t recursive_choose_item_add_node(uint32_t p_node_id, const AABB &p_aabb)
	{
		TNode &tnode = _nodes[p_node_id];

		// if not a leaf node
		if (!tnode.is_leaf())
		{
			// first choice is, if there are not max children, create another child node
			if (tnode.num_items < MAX_CHILDREN)
			{
				return _create_another_child(p_node_id, p_aabb);
			}

			// there are children already
			float best_goodness_fit = -FLT_MAX;
			int best_child = -1;

			// find the best child and recurse into that
			for (int n=0; n<tnode.num_items; n++)
			{

						float fit = _goodness_of_fit_merge(p_aabb, tnode.get_item(n).aabb);
						if (fit > best_goodness_fit)
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

	void split_leaf(uint32_t p_node_id)
	{
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



		// move each item to a child node
		for (int n=0; n<tnode.num_items; n++)
		{
			int child_node_id = child_ids[n % MAX_CHILDREN];
			const Item &source_item = tnode.get_item(n);
			_node_add_item(child_node_id, source_item.item_ref_id, source_item.aabb);
		}

		// now remove all items from the parent and replace with the child nodes
		tnode.num_items = MAX_CHILDREN;
		for (int n=0; n<MAX_CHILDREN; n++)
		{
			// store the child node ids, but not the AABB as not calculated yet
			tnode.items[n].item_ref_id = child_ids[n];
		}

		refit_all();
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
#ifdef BVH_VERBOSE
		VERBOSE_PRINT("item_add BEFORE");
		_recursive_print_tree();
#endif

		// handle to be filled with the new item ref
		BVH_Handle handle;

		// ref id easier to pass around than handle
		uint32_t ref_id;

		// this should never fail
		ItemRef * ref = _refs.request(ref_id);

		// assign to handle to return
		handle.id = ref_id;

		create_root_node();

		// we must choose where to add to tree
		ref->tnode_id = recursive_choose_item_add_node(_root_node_id, p_aabb);

		_node_add_item(ref->tnode_id, ref_id, p_aabb);

		refit_all();
//		recursive_node_update_aabb_upward(ref->tnode_id);

		VERBOSE_PRINT("item_add " + itos(_refs.size()) + " refs,\t" + itos(_nodes.size()) + " nodes ");

#ifdef BVH_VERBOSE
		VERBOSE_PRINT("item_add AFTER");
		_recursive_print_tree();
#endif

		return handle;
	}


	void item_move(BVH_Handle p_handle, const AABB &p_aabb)
	{
		uint32_t ref_id = p_handle.id;

		// get the reference
		ItemRef &ref = _refs[ref_id];

		TNode &tnode = _nodes[ref.tnode_id];

//		int id = tnode.find_item(p_ref_id.id);
//		CRASH_COND(id == -1)

		Item &item = tnode.get_item(ref.item_id);
		item.aabb = p_aabb;

		//refit_all();
		refit_upward(ref.tnode_id);
		//recursive_node_update_aabb_upward(ref.tnode_id);
	}

	void item_remove(BVH_Handle p_handle)
	{
		uint32_t ref_id = p_handle.id;

		// get the reference
		//ItemRef &ref = refs[ref_id];

		// remove the item from the node
		node_remove_item(ref_id);

		// remove the item reference
		_refs.free(ref_id);

		refit_all();
	}

	void refit_all()
	{
		refit_downward(_root_node_id);
	}


#ifdef BVH_DEBUG_DRAW
	void draw_debug(ImmediateGeometry * p_im)
	{
		if (_root_node_id == -1)
			return;

#ifdef BVH_VERBOSE
		_recursive_print_tree(_root_node_id);
#endif


		p_im->clear();
		//im->begin(Mesh::PRIMITIVE_TRIANGLES, NULL);
		p_im->begin(Mesh::PRIMITIVE_LINES, NULL);

		_debug_draw_node(p_im, _root_node_id);
		//im->add_vertex(m_DebugPortalLightPlanes[n]);
		p_im->end();
	}

	void _debug_draw_AABB(ImmediateGeometry * p_im, const AABB &aabb)
	{
		Vector3 mins, maxs;
		mins = aabb.position;
		maxs = mins + aabb.size;

		Vector3 pts[8];
		pts[0] = Vector3(mins.x, mins.y, mins.z);
		pts[1] = Vector3(mins.x, maxs.y, mins.z);
		pts[2] = Vector3(maxs.x, maxs.y, mins.z);
		pts[3] = Vector3(maxs.x, mins.y, mins.z);

		pts[4] = Vector3(mins.x, mins.y, maxs.z);
		pts[5] = Vector3(mins.x, maxs.y, maxs.z);
		pts[6] = Vector3(maxs.x, maxs.y, maxs.z);
		pts[7] = Vector3(maxs.x, mins.y, maxs.z);


		p_im->add_vertex(pts[0]);
		p_im->add_vertex(pts[1]);
		p_im->add_vertex(pts[1]);
		p_im->add_vertex(pts[2]);
		p_im->add_vertex(pts[2]);
		p_im->add_vertex(pts[3]);
		p_im->add_vertex(pts[3]);
		p_im->add_vertex(pts[0]);

		p_im->add_vertex(pts[4+0]);
		p_im->add_vertex(pts[4+1]);
		p_im->add_vertex(pts[4+1]);
		p_im->add_vertex(pts[4+2]);
		p_im->add_vertex(pts[4+2]);
		p_im->add_vertex(pts[4+3]);
		p_im->add_vertex(pts[4+3]);
		p_im->add_vertex(pts[4+0]);


		p_im->add_vertex(pts[0]);
		p_im->add_vertex(pts[4+0]);
		p_im->add_vertex(pts[1]);
		p_im->add_vertex(pts[4+1]);
		p_im->add_vertex(pts[2]);
		p_im->add_vertex(pts[4+2]);
		p_im->add_vertex(pts[3]);
		p_im->add_vertex(pts[4+3]);

	}

	void _debug_draw_node(ImmediateGeometry * p_im, uint32_t p_node_id)
	{
		TNode &tnode = _nodes[p_node_id];

		_debug_draw_AABB(p_im, tnode.aabb);

		// children?
		if (!tnode.is_leaf())
		{
			for (int n=0; n<tnode.num_items; n++)
			{
				_debug_draw_node(p_im, tnode.items[n].item_ref_id);
			}
		}
	}

#endif

#ifdef BVH_VERBOSE
	void _recursive_print_tree() const
	{
		if (_root_node_id != -1)
			_recursive_print_tree(_root_node_id);
	}

	void _recursive_print_tree(uint32_t p_node_id, int depth = 0) const
	{
		const TNode &tnode = _nodes[p_node_id];

		String sz = "";
		for (int n=0; n<depth; n++)
		{
			sz += "\t";
		}
		if (tnode.is_leaf())
			sz += "L ";
		else
			sz += "N ";

		sz += "(";
		sz += itos(tnode.num_items);
		sz += ") ";

		sz += String(tnode.aabb);
		print_line(sz);

		if (!tnode.is_leaf())
		{
			for (int n=0; n<tnode.num_items; n++)
			{
				const Item &item = tnode.get_item(n);
				_recursive_print_tree(item.item_ref_id, depth+1);
			}
		}

	}
#endif



};

#undef VERBOSE_PRINT
