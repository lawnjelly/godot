#pragma once

#include "core/math/aabb.h"
#include "core/math/bvh_abb.h"
#include "core/pooled_list.h"

#define BVH_DEBUG_DRAW
#ifdef BVH_DEBUG_DRAW
#include "scene/3d/immediate_geometry.h"
#endif

#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
//#define BVH_VERBOSE
//#define BVH_VERBOSE_TREE

//#define BVH_VERBOSE_FRAME
//#define BVH_CHECKS
#endif

#ifdef BVH_VERBOSE
#define VERBOSE_PRINT print_line
#else
#define VERBOSE_PRINT(a)
#endif

// whether to update aabbs every change (removal from a leaf)
// or to defer and do the AABB updates for the leaf once per frame.
// The latter may be more efficient (if slightly less accurate)
#define BVH_DEFER_AABB_UPDATES

// really a handle, can be anything
struct BVHHandle {
public:
	uint32_t _data;

	void set_invalid() { _data = -1; }
	bool is_invalid() const { return _data == -1; }
	uint32_t id() const { return _data; }
	void set_id(uint32_t p_id) { _data = p_id; }

	bool operator==(const BVHHandle &p_h) const { return _data == p_h._data; }
	bool operator!=(const BVHHandle &p_h) const { return (*this == p_h) == false; }
};

template <class T, int MAX_CHILDREN, int MAX_ITEMS, bool USE_PAIRS = false>
class BVH_Tree {
	friend class BVH;

#include "bvh_pair.inc"
#include "bvh_structs.inc"

public:
	BVH_Tree() {
		for (int n = 0; n < NUM_TREES; n++) {
			_root_node_id[n] = -1;
		}

		// disallow zero leaf ids (as these are stored as negative numbers)
		uint32_t dummy_leaf_id;
		_leaves.request(dummy_leaf_id);
	}

private:
	bool node_add_child(uint32_t p_node_id, uint32_t p_child_node_id) {
		TNode &tnode = _nodes[p_node_id];
		if (tnode.is_full_of_children())
			return false;
		
		tnode.children[tnode.num_children] = p_child_node_id;
		tnode.num_children += 1;

		// back link in the child to the parent
		TNode &tnode_child = _nodes[p_child_node_id];
		tnode_child.parent_id = p_node_id;

//		uint32_t id;
//		Item *pChild = tnode.request_item(id);
//		pChild->aabb = tnode_child.aabb;
//		pChild->item_ref_id = p_child_node_id;

		return true;
	}

	void node_replace_child(uint32_t p_parent_id, uint32_t p_old_child_id, uint32_t p_new_child_id) {
		TNode &parent = _nodes[p_parent_id];
#ifdef TOOLS_ENABLED
		// node is never a leaf node
		CRASH_COND(parent.is_leaf());
#endif

		int child_num = parent.find_child(p_old_child_id);
#ifdef TOOLS_ENABLED
		CRASH_COND(child_num == -1);
#endif
		parent.children[child_num] = p_new_child_id;

		TNode &new_child = _nodes[p_new_child_id];
		new_child.parent_id = p_parent_id;
	}

	void node_remove_child(uint32_t p_parent_id, uint32_t p_child_id, bool p_prevent_sibling = false) {
		TNode &parent = _nodes[p_parent_id];
#ifdef TOOLS_ENABLED
		// node is never a leaf node
		CRASH_COND(parent.is_leaf());
#endif

		int child_num = parent.find_child(p_child_id);
#ifdef TOOLS_ENABLED
		CRASH_COND(child_num == -1);
#endif

		parent.remove_child_internal(child_num);

		// no need to keep back references for children at the moment

		uint32_t sibling_id; // always a node id, as tnode is never a leaf
		bool sibling_present = false;

		// if there are more children, or this is the root node, don't try and delete
		if (parent.num_children > 1)
		{
			return;
		}
		
		// if there is 1 sibling, it can be moved to be a child of the
		if (parent.num_children == 1) {
			// else there is now a redundant node with one child, which can be removed
			sibling_id = parent.children[0];
			sibling_present = true;
		}
		
		// now there may be no children in this node .. in which case it can be deleted
		// remove node if empty
		// remove link from parent
		uint32_t grandparent_id = parent.parent_id;

		
		// special case for root node
		if (grandparent_id == -1)
		{
			if (sibling_present)
			{
				// change the root node
				change_root_node(sibling_id);
				
				// delete the old root node as no longer needed
				_nodes.free(p_parent_id);
			}
			
			return;
		}
		
		
		
		if (sibling_present) {
			node_replace_child(grandparent_id, p_parent_id, sibling_id);
		} else {
			node_remove_child(grandparent_id, p_parent_id, true);
		}

		// put the node on the free list to recycle
		_nodes.free(p_parent_id);
	}

	// this relies on _current_tree being accurate
	void change_root_node(uint32_t p_new_root_id)
	{
		_root_node_id[_current_tree] = p_new_root_id;
		TNode &root = _nodes[p_new_root_id];

		// mark no parent
		root.parent_id = -1;
	}
	
	void node_remove_item(uint32_t p_ref_id, BVH_ABB * r_old_aabb = nullptr) {
		// get the reference
		ItemRef &ref = _refs[p_ref_id];
		uint32_t owner_node_id = ref.tnode_id;
		
		// debug draw special
		if (owner_node_id == -1)
			return;

		TNode &tnode = _nodes[owner_node_id];
		CRASH_COND(!tnode.is_leaf());

		TLeaf *leaf = node_get_leaf(tnode);

		// if the aabb is not determining the corner size, then there is no need to refit!
		// (optimization, as merging AABBs takes a lot of time)		
		const BVH_ABB &old_aabb = leaf->get_aabb(ref.item_id);
		
		// shrink a little to prevent using corner aabbs
		BVH_ABB node_bound = tnode.aabb;
		node_bound.expand(-0.001f);
		bool refit = true;
		
//		if (old_aabb.is_within(node_bound))
		if (node_bound.is_other_within(old_aabb))
			refit = false;
		
		// record the old aabb if required (for incremental remove_and_reinsert)
		if (r_old_aabb)
		{
			*r_old_aabb = old_aabb;
		}
		
		leaf->remove_item_unordered(ref.item_id);

		if (leaf->num_items) {
			// the swapped item has to have its reference changed to, to point to the new item id
			uint32_t swapped_ref_id = leaf->get_item_ref_id(ref.item_id);

			ItemRef &swapped_ref = _refs[swapped_ref_id];

			swapped_ref.item_id = ref.item_id;

			// only have to refit if it is an edge item
			// This is a VERY EXPENSIVE STEP
#ifndef BVH_DEFER_AABB_UPDATES
			if (refit)
				refit_upward(owner_node_id);
#endif
		} else {
			// remove node if empty
			// remove link from parent
			if (tnode.parent_id != -1) {
				uint32_t parent_id = tnode.parent_id;

				node_remove_child(parent_id, owner_node_id);
				refit_upward(parent_id);

				// put the node on the free list to recycle
				_nodes.free(owner_node_id);
			}

			// else if no parent, it is the root node. Do not delete
		}

		ref.tnode_id = -1;
		ref.item_id = -1; // unset
	}

	//public:

	// returns true if needs refit of PARENT tree only, the node itself AABB is calculated
	// within this routine
	bool _node_add_item(uint32_t p_node_id, uint32_t p_ref_id, const BVH_ABB &p_aabb) {
		ItemRef &ref = _refs[p_ref_id];
		ref.tnode_id = p_node_id;

		TNode &node = _nodes[p_node_id];
		CRASH_COND(!node.is_leaf());
		TLeaf &leaf = _leaves[node.get_leaf_id()];
		
		// optimization - we only need to do a refit
		// if the added item is changing the AABB of the node.
		// in most cases it won't.
		bool needs_refit = true;

		// expand bound now
		BVH_ABB expanded = p_aabb;
		expanded.expand();
		
		// the bound will only be valid if there is an item in there already
		if (leaf.num_items)
		{
			//BVH_ABB bound = node.aabb;
			//bound.expand_negative();
			//-0.001f);
			if (node.aabb.is_other_within(expanded))
			{
				// no change to node AABBs
				needs_refit = false;
			}
			else
			{
				node.aabb.merge(expanded);
			}
		}
		else
		{
			// bound of the node = the new aabb
			node.aabb = expanded;
		}

		ref.item_id = leaf.request_item();
		CRASH_COND(ref.item_id == -1);
//		Item *item = leaf.request_item(ref.item_id);

		// set the aabb of the new item
		leaf.get_aabb(ref.item_id) = p_aabb;
		//item->aabb = p_aabb;

		// back reference on the item back to the item reference
		//item->item_ref_id = p_ref_id;
		leaf.get_item_ref_id(ref.item_id) = p_ref_id;
		
#ifdef DEBUG_ENABLED
//		_debug_node_verify_bound(p_node_id);
#endif
		
		return needs_refit;
	}

	uint32_t _create_another_child(uint32_t p_node_id, const BVH_ABB &p_aabb) {
		uint32_t child_node_id;
		TNode *child_node = _nodes.request(child_node_id);
		child_node->clear();

		// may not be necessary
		child_node->aabb = p_aabb;

		node_add_child(p_node_id, child_node_id);

		return child_node_id;
	}

	/*
	// after adding a node after a split, it may still be worth exchanging with one of the cousins
	void _try_exchange_cousin(uint32_t p_ref_id) {
		ItemRef &ref = _refs[p_ref_id];

		uint32_t node_id = ref.tnode_id;
		TNode &tnode = _nodes[node_id];

		// no parent
		if (!tnode.parent_tnode_id_p1)
			return;

		uint32_t parent_id = tnode.parent_tnode_id_p1 - 1;
		TNode &parent = _nodes[parent_id];

		// find the other sibling
		uint32_t sibling_id = -1;
		for (int n = 0; n < parent.num_items; n++) {
			if (parent.get_item(n).item_ref_id != node_id) {
				sibling_id = parent.get_item(n).item_ref_id;
				break;
			}
		}

		// no sibling found
		if (sibling_id == -1)
			return;

		// now we will try an exchange with each cousin
		TNode &sibling = _nodes[sibling_id];
		if (!sibling.is_leaf())
			return;

		// calculate the aabb of the node without the item
		BVH_ABB aabb_node;
		aabb_node.set_to_max_opposite_extents();

		for (int n = 0; n < tnode.num_items; n++) {
			if (n == ref.item_id)
				continue;

			aabb_node.merge(tnode.get_item(n).aabb);
		}

		float area_node_orig = tnode.aabb.get_area();
		float area_node_without = aabb_node.get_area();
		float area_node_saved = area_node_orig - area_node_without;

		const BVH_ABB &aabb_item = tnode.get_item(ref.item_id).aabb;

		// area of sibling before
		float area_sibling_orig = sibling.aabb.get_area();

		for (int n = 0; n < sibling.num_items; n++) {
			if (_should_exchange_cousin(aabb_node, aabb_item, area_node_orig, area_sibling_orig, area_node_saved, sibling, n)) {
				return;
			}
		}
	}

	bool _should_exchange_cousin(const BVH_ABB &aabb_without, const BVH_ABB &aabb_item, float area_node_orig, float area_sibling_orig, float area_node_saved, TNode &sibling, int child_num) {
		// calculate the aabb of the sibling without
		BVH_ABB aabb_sibling;
		aabb_sibling.set_to_max_opposite_extents();

		for (int n = 0; n < sibling.num_items; n++) {
			if (n == child_num)
				continue;

			aabb_sibling.merge(sibling.get_item(n).aabb);
		}

		// mockup adding the item in exchange for item 'child_num'
		//float area_sibling_without = _aabb_size(aabb_sibling);
		aabb_sibling.merge(aabb_item);
		float area_sibling_with = aabb_sibling.get_area();

		float area_sibling_change = area_sibling_with - area_sibling_orig;

		// mockup adding the sibling item to the original node
		BVH_ABB aabb_node = aabb_without;
		aabb_node.merge(sibling.get_item(child_num).aabb);
		float area_node_with = aabb_node.get_area();

		float area_node_change = area_node_with - area_node_orig;

		// if there was an overall drop in area, they should be exchanged
		float overall_change = area_sibling_change + area_node_change;
		if (overall_change < 0.0f)
			return true;

		return false;
	}

	uint32_t _create_sibling(uint32_t p_node_id, const BVH_ABB &p_aabb) {
		VERBOSE_PRINT("_create_sibling");

		TNode &tnode = _nodes[p_node_id];
		uint32_t old_parent_id_p1 = tnode.parent_tnode_id_p1;

		// create the new common parent
		uint32_t new_parent_id;
		TNode *new_parent = _nodes.request(new_parent_id);
		new_parent->clear(false);

		// it is possible we are creating a new root node
		if (!old_parent_id_p1)
		//		if (p_node_id == _root_node_id)
		{
			// is a new root to one of the trees
			for (int n = 0; n < NUM_TREES; n++) {
				if (_root_node_id[n] == p_node_id)
					_root_node_id[n] = new_parent_id;
			}
			//_root_node_id = new_parent_id;
		} else {
			// if there was an old parent, remove the node as a child, and add the new parent as a child
			//CRASH_COND(old_parent_id_p1 == 0);
			TNode &t_old_parent = _nodes[old_parent_id_p1 - 1];
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
	*/

#include "bvh_cull.inc"
#include "bvh_debug.inc"
#include "bvh_logic.inc"
#include "bvh_misc.inc"
#include "bvh_public.inc"
#include "bvh_split.inc"
#include "bvh_refit.inc"
};

#undef VERBOSE_PRINT
