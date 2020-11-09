#pragma once

#include "bvh_tree.h"

// wrapper for the BVH tree, which can do pairing etc.
//typedef BVHHandle BVHElementID;

#define USE_BVH_INSTEAD_OF_OCTREE
#define BVHTREE_CLASS BVH_Tree<T, 2, 2>

template <class T, bool USE_PAIRS = false>
class BVH_Manager
{
	struct Pair
	{
		bool operator==(const Pair &p) const {return (handle[0] == p.handle[0]) && (handle[1] == p.handle[1]);}
		void sort()
		{
			if (handle[1]._data < handle[0]._data)
			{
				BVHHandle t = handle[1];
				handle[1] = handle[0];
				handle[0] = t;
			}
		}
		BVHHandle handle[2];
		uint32_t version; // add 1 each tick / frame
		void *ud; // userdata for the pair (needed for client, not sure what used for)
	};

	class PairHashtable
	{
		int _hash(const Pair &p) const
		{
			unsigned int h = p.handle[0]._data + p.handle[1]._data;
			h %= 128;
			return (int) h;
		}
	public:
		enum {NUM_BINS = 128};
		LocalVector<Pair, uint32_t, true> bins[NUM_BINS];

		Pair * add(const Pair &p)
		{
//			Pair p;
//			p.handle[0] = a;
//			p.handle[1] = b;
			int bin = _hash(p);

			int s = bins[bin].size();
			for (int n=0; n<s; n++)
			{
				// already exists
				if (bins[bin][n] == p)
				{
					// update the version
					bins[bin][n].version = p.version;
					return nullptr;
				}
			}

			bins[bin].push_back(p);
			return &bins[bin][s];
		}
	} _pairs_hashtable;

	uint32_t _version;

public:
	typedef void *(*PairCallback)(void *, BVHHandle, T *, int, BVHHandle, T *, int);
	typedef void (*UnpairCallback)(void *, BVHHandle, T *, int, BVHHandle, T *, int, void *);

	void set_pair_callback(PairCallback p_callback, void *p_userdata)
	{
		pair_callback = p_callback;
		pair_callback_userdata = p_userdata;
	}
	void set_unpair_callback(UnpairCallback p_callback, void *p_userdata)
	{
		unpair_callback = p_callback;
		unpair_callback_userdata = p_userdata;
	}



	BVHHandle create(T *p_userdata, const AABB &p_aabb = AABB(), int p_subindex = 0, bool p_pairable = false, uint32_t p_pairable_type = 0, uint32_t p_pairable_mask = 1)
	{
		uint32_t tree_id = 0;

		// which tree are we adding to?
		if (p_pairable)
			tree_id = 1;

		BVHHandle h;
		h.clear();
		h.set_pairable(p_pairable);

		// ignore bound
	#ifdef BVH_DEBUG_DRAW
		if (_added_item_count == 1)
		{
			h.set_invisible(true);
		}
		_added_item_count++;
	#endif


		uint32_t ref_id = tree[tree_id].item_add(p_userdata, p_aabb, p_subindex, p_pairable, p_pairable_type, p_pairable_mask, h.is_invisible());

//		BVHHandle h;
		h.set_id(ref_id);
	//	h.set_pairable(p_pairable);

		if (USE_PAIRS)
		{
			_add_changed_item(h);
		}

//		check_for_collisions();

		return h;
	}

	void move(BVHHandle p_handle, const AABB &p_aabb)
	{
//		if (p_handle.is_invalid())
//			return;

#ifdef BVH_DEBUG_DRAW
		if (p_handle.is_invisible())
			return;
#endif

		tree[p_handle.get_tree()].item_move(p_handle, p_aabb);
		if (USE_PAIRS)
			_add_changed_item(p_handle);
	}

	void erase(BVHHandle p_handle)
	{
		// call unpair and remove all references to the item
		// before deleting from the tree
		if (USE_PAIRS)
		{
			_remove_changed_item(p_handle);
		}

		tree[p_handle.get_tree()].item_remove(p_handle);
	}

	void set_pairable(BVHHandle &r_handle, bool p_pairable, uint32_t p_pairable_type, uint32_t p_pairable_mask)
	{
		// does it need to change tree?
		if (p_pairable != r_handle.is_pairable())
		{
			AABB aabb;
			item_get_AABB(r_handle, aabb);

			// copy the extra data
			typename BVHTREE_CLASS::ItemExtra ex = tree[r_handle.get_tree()]._extra[r_handle.id()];

			// remove the old
			tree[r_handle.get_tree()].item_remove(r_handle);

			// create the new
			r_handle = create(ex.userdata, aabb, ex.subindex, p_pairable, p_pairable_type, p_pairable_mask);

			return;
		}

		// no change of tree
		uint32_t ref_id = r_handle.id();

		// get the reference
		typename BVHTREE_CLASS::ItemExtra &extra = tree[r_handle.get_tree()]._extra[ref_id];
		extra.pairable = p_pairable;
		extra.pairable_mask = p_pairable_mask;
		extra.pairable_type = p_pairable_type;
	}


	// cull tests
	int cull_aabb(const AABB &p_aabb, T **p_result_array, int p_result_max, int *p_subindex_array = nullptr, uint32_t p_mask = 0xFFFFFFFF)
	{
		typename BVHTREE_CLASS::CullParams params;

		params.result_count_overall = 0;
		params.result_max = p_result_max;
		params.result_array = p_result_array;
		params.subindex_array = p_subindex_array;
		params.mask = p_mask;

		params.abb.from(p_aabb);

		tree[0].cull_aabb(params);
		tree[1].cull_aabb(params);

		return params.result_count_overall;
//		return tree.cull_aabb(p_aabb, p_result_array, p_result_max, p_subindex_array, p_mask);
	}

	int cull_segment(const Vector3 &p_from, const Vector3 &p_to, T **p_result_array, int p_result_max, int *p_subindex_array = nullptr, uint32_t p_mask = 0xFFFFFFFF)
	{
		typename BVHTREE_CLASS::CullParams params;

		params.result_count_overall = 0;
		params.result_max = p_result_max;
		params.result_array = p_result_array;
		params.subindex_array = p_subindex_array;
		params.mask = p_mask;

		params.segment.from = p_from;
		params.segment.to = p_to;

		tree[0].cull_segment(params);
		tree[1].cull_segment(params);

		return params.result_count_overall;
		//		return tree.cull_segment(p_from, p_to, p_result_array, p_result_max, p_subindex_array, p_mask);
	}

	int cull_point(const Vector3 &p_point, T **p_result_array, int p_result_max, int *p_subindex_array = nullptr, uint32_t p_mask = 0xFFFFFFFF)
	{
		typename BVHTREE_CLASS::CullParams params;

		params.result_count_overall = 0;
		params.result_max = p_result_max;
		params.result_array = p_result_array;
		params.subindex_array = p_subindex_array;
		params.mask = p_mask;

		params.point = p_point;

		tree[0].cull_point(params);
		tree[1].cull_point(params);
		return params.result_count_overall;
//		return tree.cull_point(p_point, p_result_array, p_result_max, p_subindex_array, p_mask);
	}

	int cull_convex(const Vector<Plane> &p_convex, T **p_result_array, int p_result_max, uint32_t p_mask = 0xFFFFFFFF)
	{
		if (!p_convex.size())
			return 0;

		Vector<Vector3> convex_points = Geometry::compute_convex_mesh_points(&p_convex[0], p_convex.size());
		if (convex_points.size() == 0)
			return 0;

		typename BVHTREE_CLASS::CullParams params;
		params.result_count_overall = 0;
		params.result_max = p_result_max;
		params.result_array = p_result_array;
		params.subindex_array = nullptr;
		params.mask = p_mask;

		params.hull.planes = &p_convex[0];
		params.hull.num_planes = p_convex.size();
		params.hull.points = &convex_points[0];
		params.hull.num_points = convex_points.size();


//		int num_results = tree.cull_convex(&p_convex[0], p_convex.size(), p_result_array, p_result_max, p_mask);
		tree[0].cull_convex(params);
		tree[1].cull_convex(params);

		return params.result_count_overall;
		//debug_output_cull_results(p_result_array, num_results);
//		return num_results;
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

	// do this after moving etc.
	void check_for_collisions()
	{
		AABB bb;


		typename BVHTREE_CLASS::CullParams params;

		params.result_count_overall = 0;
		params.result_max = 0;
		params.result_array = nullptr;
		params.subindex_array = nullptr;
		params.mask = 0xFFFFFFFF;


		for (int n=0; n<changed_items.size(); n++)
		{
			const BVHHandle &h = changed_items[n];

//			if (!tree._extra[h.id].pairable)
//				continue;

			int tree_id = h.get_tree();
			uint32_t changed_item_ref_id = h.id();
			bool changed_item_pairable = h.is_pairable();

			item_get_AABB(h, bb);

			// aabb to abb
			params.abb.from(bb);

//			tree[tree_id].item_get_AABB(h, bb);

			//int cull_aabb(const AABB &p_aabb, T **p_result_array, int p_result_max, int *p_subindex_array, uint32_t p_mask, bool p_translate_hits = true)

			// get the results into _cull_hits
			for (int test_tree=0; test_tree<2; test_tree++)
			{
				BVHTREE_CLASS &tr = tree[test_tree];

//				tr.cull_aabb(bb, nullptr, 0, nullptr, 0, false);
				params.result_count_overall = 0; // might not be needed
				tr.cull_aabb(params, false);
//				return params.result_count_overall;

				for (int n=0; n<tr._cull_hits.size(); n++)
				{
					uint32_t ref_id = tr._cull_hits[n];

					// don't collide against ourself
					if ((ref_id == changed_item_ref_id) && (test_tree == tree_id))
						continue;

					// if neither are pairable, they should ignore each other
					if (!changed_item_pairable && !tr._extra[ref_id].pairable)
						continue;

					// checkmasks ... NYI

					BVHHandle h_collidee;
					h_collidee.set_all(ref_id);
					h_collidee.set_pairable(test_tree == 1);

					_collide(h, h_collidee);
				}
			} // for tree

		}

		_reset();
	}

	// supplemental funcs
	bool item_is_pairable(BVHHandle p_handle) const
	{
		return p_handle.is_pairable();
//		uint32_t ref_id = p_handle.id;
//		return tree._extra[ref_id].pairable;
	}

	T * item_get_userdata(BVHHandle p_handle) const {return _get_extra(p_handle).userdata;}

	void item_get_AABB(BVHHandle p_handle, AABB &r_aabb) const
	{
		uint32_t ref_id = p_handle.id();
		uint32_t tree_id = p_handle.get_tree();

		const typename BVHTREE_CLASS::ItemRef &ref = tree[tree_id]._refs[ref_id];
		const typename BVHTREE_CLASS::TNode &tnode = tree[tree_id]._nodes[ref.tnode_id];
		const typename BVHTREE_CLASS::Item &item = tnode.get_item(ref.item_id);
		item.aabb.to(r_aabb);
	}

	int item_get_subindex(BVHHandle p_handle) const {return _get_extra(p_handle).subindex;}

private:
	void _collide(BVHHandle collider, BVHHandle collidee)
	{
		Pair p;
		p.handle[0] = collider;
		p.handle[1] = collidee;
		p.sort();
		p.version = _version;
		p.ud = 0;

		Pair * p_added_pair = _pairs_hashtable.add(p);
		if (p_added_pair)
		{
			// callback start collision
			if (pair_callback) {
				const BVHHandle &ha = p_added_pair->handle[0];
				const BVHHandle &hb = p_added_pair->handle[1];

				const typename BVHTREE_CLASS::ItemExtra &exa = _get_extra(ha);
				const typename BVHTREE_CLASS::ItemExtra &exb = _get_extra(hb);

				p_added_pair->ud = pair_callback(pair_callback_userdata, ha, exa.userdata, exa.subindex, hb, exb.userdata, exb.subindex);
			}

		}

	}

	// pairs that were not colliding in this version
	void _remove_stale_pairs()
	{
		/*
		for (int b=0; b<_pairs_hashtable.NUM_BINS; b++)
		{
			for (int n=0; n<_pairs_hashtable.bins[b].size(); n++)
			{
				const Pair &p = _pairs_hashtable.bins[b][n];
				if (p.version != _version)
				{
					// callback
					if (unpair_callback) {
						BVHHandle ha, hb;
						ha.id = p.ref_id[0];
						hb.id = p.ref_id[1];

						typename BVHTREE_CLASS::ItemExtra &exa = tree._extra[ha.id];
						typename BVHTREE_CLASS::ItemExtra &exb = tree._extra[hb.id];

						unpair_callback(pair_callback_userdata, ha, exa.userdata, exa.subindex, hb, exb.userdata, exb.subindex, p.ud);
					}

					_pairs_hashtable.bins[b].remove_unordered(n);
				} // version outdated
			}
		}
		*/
	}

	// if we remove an item, we need to immediately remove the pairs, to prevent reading the pair after deletion
	void _remove_pairs_containing(BVHHandle p_handle)
	{
		for (int b=0; b<_pairs_hashtable.NUM_BINS; b++)
		{
			for (int n=0; n<_pairs_hashtable.bins[b].size(); n++)
			{
				const Pair &p = _pairs_hashtable.bins[b][n];
				if ((p.handle[0] == p_handle) || (p.handle[1] == p_handle))
				{
					// callback
					if (unpair_callback) {
						const BVHHandle &ha = p.handle[0];
						const BVHHandle &hb = p.handle[1];

						const typename BVHTREE_CLASS::ItemExtra &exa = _get_extra(ha);
						const typename BVHTREE_CLASS::ItemExtra &exb = _get_extra(hb);

						unpair_callback(pair_callback_userdata, ha, exa.userdata, exa.subindex, hb, exb.userdata, exb.subindex, p.ud);
					}

					_pairs_hashtable.bins[b].remove_unordered(n);
				} // version outdated
			}
		}
	}


#ifdef BVH_DEBUG_DRAW
public:
	void draw_debug(ImmediateGeometry * p_im)
	{
		tree[0].draw_debug(p_im);
		//tree[0].draw_debug(p_im);
	}
#endif

private:
	const typename BVHTREE_CLASS::ItemExtra &_get_extra(BVHHandle p_handle) const
	{
		return tree[p_handle.get_tree()]._extra[p_handle.id()];
	}
	const typename BVHTREE_CLASS::ItemExtra &_get_ref(BVHHandle p_handle) const
	{
		return tree[p_handle.get_tree()]._refs[p_handle.id()];
	}


	void _reset()
	{
		changed_items.clear();
		_version++;
	}

	void _add_changed_item(BVHHandle p_handle)
	{
		// only if uses pairing
		if (!item_is_pairable(p_handle))
			return;

		// todo .. add bitfield for fast check
		for (int n=0; n<changed_items.size(); n++)
		{
			// already on list
			if (changed_items[n] == p_handle)
				return;
		}

		changed_items.push_back(p_handle);
	}

	void _remove_changed_item(BVHHandle p_handle)
	{
		// only if uses pairing
//		if (!item_is_pairable(p_handle))
//			return;

		_remove_pairs_containing(p_handle);

		// remove from changed items (not very efficient yet)
		for (int n=0; n<changed_items.size(); n++)
		{
			if (changed_items[n] == p_handle)
			{
				changed_items.remove_unordered(n);
			}
		}
	}

	PairCallback pair_callback;
	UnpairCallback unpair_callback;
	void *pair_callback_userdata;
	void *unpair_callback_userdata;

	// unpaired, and paired trees
	BVHTREE_CLASS tree[2];

	// for collision pairing,
	// maintain a list of all items moved etc on each frame / tick
	LocalVector<BVHHandle, uint32_t, true> changed_items;

#ifdef BVH_DEBUG_DRAW
	int _added_item_count;
#endif

public:
	BVH_Manager()
	{
		_version = 0;
		pair_callback = nullptr;
		unpair_callback = nullptr;
		pair_callback_userdata = nullptr;
		unpair_callback_userdata = nullptr;

#ifdef BVH_DEBUG_DRAW
		_added_item_count = 0;
#endif
	}
};


