#pragma once

#include "bvh_tree.h"

// wrapper for the BVH tree, which can do pairing etc.
//typedef BVHHandle BVHElementID;


template <class T, bool use_pairs = false>
class BVH_Manager
{
public:
	BVHHandle create(T *p_userdata, const AABB &p_aabb = AABB(), int p_subindex = 0, bool p_pairable = false, uint32_t p_pairable_type = 0, uint32_t p_pairable_mask = 1)
	{
		return tree.item_add(p_userdata, p_aabb, p_subindex, p_pairable, p_pairable_type, p_pairable_mask);
	}

	void move(BVHHandle p_id, const AABB &p_aabb)
	{
		tree.item_move(p_id, p_aabb);
	}
	void erase(BVHHandle p_id)
	{
		tree.item_remove(p_id);
	}

	// cull tests
	int cull_aabb(const AABB &p_aabb, T **p_result_array, int p_result_max, int *p_subindex_array, uint32_t p_mask)
	{
		return tree.cull_aabb(p_aabb, p_result_array, p_result_max, p_subindex_array, p_mask);
	}

	int cull_segment(const Vector3 &p_from, const Vector3 &p_to, T **p_result_array, int p_result_max, int *p_subindex_array, uint32_t p_mask)
	{
		return tree.cull_segment(p_from, p_to, p_result_array, p_result_max, p_subindex_array, p_mask);
	}

	int cull_convex(const Vector<Plane> &p_convex, T **p_result_array, int p_result_max, uint32_t p_mask)
	{
		if (!p_convex.size())
			return 0;

		int num_results = tree.cull_convex(&p_convex[0], p_convex.size(), p_result_array, p_result_max, p_mask);

		//debug_output_cull_results(p_result_array, num_results);
		return num_results;
	}

	void debug_output_cull_results(T **p_result_array, int p_num_results) const
	{
		for (int n=0; n<p_num_results; n++)
		{
			String sz = itos(n) + " : ";

			uintptr_t val= (uintptr_t) p_result_array[n];

			sz += "0x" + String::num_uint64(val, 16);

			print_line(sz);
		}
	}

#ifdef BVH_DEBUG_DRAW
	void draw_debug(ImmediateGeometry * p_im)
	{
		tree.draw_debug(p_im);
	}
#endif

private:
	BVH_Tree<T, 2, 2> tree;
};


