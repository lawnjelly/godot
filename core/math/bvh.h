#pragma once

#include "bvh_tree.h"

// wrapper for the BVH tree, which can do pairing etc.
//#define OctreeElementID BVHElementID
typedef BVH_Handle BVHElementID;


template <class T, bool use_pairs = false>
class BVH_Manager
{
public:
	BVHElementID create(T *p_userdata, const AABB &p_aabb = AABB(), int p_subindex = 0, bool p_pairable = false, uint32_t p_pairable_type = 0, uint32_t pairable_mask = 1)
	{
		return tree.item_add(p_aabb);
	}

	void move(BVHElementID p_id, const AABB &p_aabb)
	{
		tree.item_move(p_id, p_aabb);
	}
	void erase(BVHElementID p_id)
	{
		tree.item_remove(p_id);
	}

private:
	BVH_Tree<T, 2, 8> tree;
};


