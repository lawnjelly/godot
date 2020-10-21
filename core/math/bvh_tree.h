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



template <class T, int NUM_CHILDREN, int MAX_ITEMS_PER_NODE>
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
			active_child_flags = 0;
			parent_tnode_id = -1;
		}
		Item * request_item(uint32_t &id)
//		Item * request_item()
		{
			if (num_items < MAX_ITEMS_PER_NODE)
			{
				id = num_items;
				return &items[num_items++];
			}
			return nullptr;
		}
//		int find_item(uint32_t ref_id)
//		{
//			for (int n=0; n<num_items; n++)
//			{
//				if (items[n].item_ref_id == ref_id)
//					return n;
//			}
//			return -1;
//		}
		Item &get_item(int id)
		{
			return items[id];
		}
		// ALSO NEEDS the reference changing in the tree, cannot be used on its own
		void remove_item_internal(uint32_t item_id)
		{
//			int id = find_item(ref_id);
			CRASH_COND(item_id >= num_items)

			num_items--;
			// swap with last .. unordered
			items[item_id] = items[num_items];
		}
		void recalc_aabb()
		{
			if (!num_items)
			{
				aabb = AABB();
				return;
			}
			aabb = items[0].aabb;
			for (int n=1; n<num_items; n++)
			{
				aabb.merge_with(items[n].aabb);
			}
		}

		uint32_t active_child_flags; // this limits number of children
		uint32_t num_items;
		int parent_tnode_id;
		AABB aabb;
		AABB child_aabbs[NUM_CHILDREN];
		Item items[MAX_ITEMS_PER_NODE];
	};


	// instead of using linked list we maintain
	// item references (for quick lookup)
	PooledList<ItemRef, true> refs;
	PooledList<TNode, true> nodes;

private:
	void node_remove_item(uint32_t p_ref_id)
	{
		// get the reference
		ItemRef &ref = refs[p_ref_id];

		TNode &tnode = nodes[ref.tnode_id];
		tnode.remove_item_internal(ref.item_id);

		if (tnode.num_items)
		{
			// the swapped item has to have its reference changed to, to point to the new item id
			uint32_t swapped_ref_id = tnode.items[ref.item_id].item_ref_id;

			ItemRef &swapped_ref = refs[swapped_ref_id];

			swapped_ref.item_id = ref.item_id;
		}

		ref.item_id = -1; // unset
	}

public:

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

		// add to tree
		TNode * node = nodes.request(ref->tnode_id);
		node->clear();

		// request a new item on the node, and return the item id to the reference
		Item * item = node->request_item(ref->item_id);

		// set the aabb of the new item
		item->aabb = p_aabb;

		// back reference on the item back to the item reference
		item->item_ref_id = ref_id;

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
	}

	void item_remove(BVH_Handle p_handle)
	{
		uint32_t ref_id = p_handle.id;

		// get the reference
		//ItemRef &ref = refs[ref_id];

		// remove the item from the node
		node_remove_item(ref_id);

		// remove node? NYI

		// remove the item reference
		refs.free(ref_id);
	}
};

#undef VERBOSE_PRINT
