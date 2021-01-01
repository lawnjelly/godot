#pragma once

#include "bvh_tree.h"

// wrapper for the BVH tree, which can do pairing etc.
//typedef BVHHandle BVHElementID;

//#define USE_BVH_INSTEAD_OF_OCTREE
#define USE_BVH_INSTEAD_OF_OCTREE_FOR_GODOT_PHYSICS
//#define BVH_DEBUG_CALLBACKS

#define BVHTREE_CLASS BVH_Tree<T, 2, 128, USE_PAIRS>

template <class T, bool USE_PAIRS = false>
class BVH_Manager
{
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
		// ignore bound
//	#ifdef BVH_DEBUG_DRAW
//		if (_added_item_count == 1)
//		{
//			h.set_invisible(true);
//		}
//		_added_item_count++;
//	#endif

#ifdef TOOLS_ENABLED
		if (!USE_PAIRS)
		{
			if (p_pairable)
			{
				WARN_PRINT_ONCE("creating pairable item in BVH with USE_PAIRS set to false");
			}
			//CRASH_COND(p_pairable);
		}
#endif

		BVHHandle h = tree.item_add(p_userdata, p_aabb, p_subindex, p_pairable, p_pairable_type, p_pairable_mask);

		if (USE_PAIRS)
		{
			_add_changed_item(h, p_aabb);
		}

//		check_for_collisions();

		return h;
	}

	void move(uint32_t p_handle, const AABB &p_aabb)
	{
		BVHHandle h; h.set(p_handle);
		move (h, p_aabb);
	}
	
	
	void move(BVHHandle p_handle, const AABB &p_aabb)
	{
//		if (p_handle.is_invalid())
//			return;

//#ifdef BVH_DEBUG_DRAW
//		if (p_handle.is_invisible())
//			return;
//#endif

		// returns false if noop
		if (tree.item_move(p_handle, p_aabb))
		{
			if (USE_PAIRS)
			{
				_add_changed_item(p_handle, p_aabb);
			}
		}
	}

	void erase(uint32_t p_handle)
	{
		BVHHandle h; h.set(p_handle);
		erase (h);
	}
	
	void erase(BVHHandle p_handle)
	{
		// call unpair and remove all references to the item
		// before deleting from the tree
		if (USE_PAIRS)
		{
			_remove_changed_item(p_handle);
		}

		tree.item_remove(p_handle);
	}

	// call e.g. once per frame (this does a trickle optimize)
	void update()
	{
		tree.incremental_optimize();
		_check_for_collisions();
	}

	void set_pairable(uint32_t p_handle, bool p_pairable, uint32_t p_pairable_type, uint32_t p_pairable_mask)
	{
		BVHHandle h; h.set(p_handle);
		set_pairable(h, p_pairable, p_pairable_type, p_pairable_mask);
	}
	
	void set_pairable(const BVHHandle &p_handle, bool p_pairable, uint32_t p_pairable_type, uint32_t p_pairable_mask)
	{
		// unpair callback if already paired? NYI
		tree.item_set_pairable(p_handle, p_pairable, p_pairable_type, p_pairable_mask);
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

		tree.cull_aabb(params);

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

		tree.cull_segment(params);

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

		tree.cull_point(params);
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
		tree.cull_convex(params);

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
	void _check_for_collisions()
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

			// use the expanded aabb for pairing
			const AABB &expanded_aabb = tree._pairs[h.id()].expanded_aabb;
			BVH_ABB abb;
			abb.from(expanded_aabb);
			
			// find all the existing paired aabbs that are no longer
			// paired, and send callbacks
			_find_leavers(h, abb);


			// we will redetect all collision pairs from this item
//			tree.pairs_reset_from_item(h);

//			if (!tree._extra[h.id].pairable)
//				continue;

//			int tree_id = h.get_tree();
			uint32_t changed_item_ref_id = h.id();
			
			// it will always be pairable (check is done on adding)
			bool changed_item_pairable = item_is_pairable(h);

			// aabb to abb
			//item_get_AABB(h, bb);
			//params.abb.from(bb);
			
			params.abb = abb;
			

//			tree[tree_id].item_get_AABB(h, bb);

			//int cull_aabb(const AABB &p_aabb, T **p_result_array, int p_result_max, int *p_subindex_array, uint32_t p_mask, bool p_translate_hits = true)

			// get the results into _cull_hits

//				tr.cull_aabb(bb, nullptr, 0, nullptr, 0, false);
				params.result_count_overall = 0; // might not be needed
				tree.cull_aabb(params, false);
//				return params.result_count_overall;

				for (int n=0; n<tree._cull_hits.size(); n++)
				{
					uint32_t ref_id = tree._cull_hits[n];

					// don't collide against ourself
					if (ref_id == changed_item_ref_id)
						continue;

					// if neither are pairable, they should ignore each other
					if (!changed_item_pairable && !tree._extra[ref_id].pairable)
						continue;

					// checkmasks ... NYI

					BVHHandle h_collidee;
					h_collidee.set_id(ref_id);
					//h_collidee.set_pairable(test_tree == 1);

					// find NEW enterers, and send callbacks for them only
					_collide(h, h_collidee);
				}

		}
		_reset();
	}

	// backward compatibility
	bool is_pairable(uint32_t p_handle) const
	{
		BVHHandle h; h.set(p_handle);
		return item_is_pairable(h);
	}
	int get_subindex(uint32_t p_handle) const
	{
		BVHHandle h; h.set(p_handle);
		return item_get_subindex(h);
	}
	
	T * get(uint32_t p_handle) const
	{
		BVHHandle h; h.set(p_handle);
		return item_get_userdata(h);
	}
	
	
	// supplemental funcs
	bool item_is_pairable(BVHHandle p_handle) const {return _get_extra(p_handle).pairable;}
	T * item_get_userdata(BVHHandle p_handle) const {return _get_extra(p_handle).userdata;}
	int item_get_subindex(BVHHandle p_handle) const {return _get_extra(p_handle).subindex;}

	void item_get_AABB(BVHHandle p_handle, AABB &r_aabb)
	{
		BVH_ABB abb;
		item_get_ABB(p_handle, abb);
		abb.to(r_aabb);
	}

	void item_get_ABB(BVHHandle p_handle, BVH_ABB &r_abb)
	{
		tree.item_get_ABB(p_handle, r_abb);
//		const typename BVHTREE_CLASS::ItemRef &ref = _get_ref(p_handle);
//		typename BVHTREE_CLASS::TNode &tnode = tree._nodes[ref.tnode_id];
//		CRASH_COND(!tnode.is_leaf());
//		typename BVHTREE_CLASS::TLeaf * leaf = tree.node_get_leaf(tnode);
		
//		// check this for bugs as this has changed to a reference
//		r_abb = leaf->get_aabb(ref.item_id);
		
//		const typename BVHTREE_CLASS::Item &item = tnode.get_item(ref.item_id);
//		r_abb = item.aabb;
	}


private:

	void _unpair(BVHHandle p_from, BVHHandle p_to)
	{
		typename BVHTREE_CLASS::ItemPairs &pairs_from = tree._pairs[p_from.id()];
		typename BVHTREE_CLASS::ItemPairs &pairs_to = tree._pairs[p_to.id()];

		void * ud_from = pairs_from.remove_pair_to(p_to);
		void * ud_to = pairs_to.remove_pair_to(p_from);

		// callback
		if (unpair_callback) {
			BVHHandle ha, hb;
			ha = p_from;
			hb = p_to;
			tree._sort_handles(ha, hb);

			typename BVHTREE_CLASS::ItemExtra &exa = tree._extra[ha.id()];
			typename BVHTREE_CLASS::ItemExtra &exb = tree._extra[hb.id()];

			// the user data will be stored in the LOWER handle
			void * ud = ud_from;
			if (p_to.id() < p_from.id())
				ud = ud_to;

			unpair_callback(pair_callback_userdata, ha, exa.userdata, exa.subindex, hb, exb.userdata, exb.subindex, ud);
#ifdef BVH_DEBUG_CALLBACKS
			print_line("Unpair callback : " + itos (ha.id()) + " to " + itos(hb.id()));
#endif

		}
	}

	void _find_leavers_process_pair(typename BVHTREE_CLASS::ItemPairs &p_pairs_from, const BVH_ABB &p_abb_from, BVHHandle p_from, BVHHandle p_to)
	{
		BVH_ABB abb_to;
		item_get_ABB(p_to, abb_to);

		// do they overlap?
		if (p_abb_from.intersects(abb_to))
			return;

		_unpair(p_from, p_to);

	}

	// find all the existing paired aabbs that are no longer
	// paired, and send callbacks
	void _find_leavers(BVHHandle p_handle, const BVH_ABB &expanded_abb_from)
	{
		typename BVHTREE_CLASS::ItemPairs &p_from = tree._pairs[p_handle.id()];

		BVH_ABB abb_from = expanded_abb_from;
		//item_get_ABB(p_handle, abb_from);

		// remove from pairing list for every partner
		if (!p_from.extended())
		{
			for (int n=0; n<p_from.num_pairs; n++)
			{
				BVHHandle h_to = p_from.pairs[n].handle;
				_find_leavers_process_pair(p_from, abb_from, p_handle, h_to);
			}
		}
		else
		{
			for (int n=0; n<p_from.extended_pairs.size(); n++)
			{
				BVHHandle h_to = p_from.extended_pairs[n].handle;
				_find_leavers_process_pair(p_from, abb_from, p_handle, h_to);
			}
		}

	}

	// find NEW enterers, and send callbacks for them only
	// handle a and b
	void _collide(BVHHandle p_ha, BVHHandle p_hb)
	{
		// only have to do this oneway, lower ID then higher ID
		tree._sort_handles(p_ha, p_hb);

		typename BVHTREE_CLASS::ItemPairs &p_from = tree._pairs[p_ha.id()];
		typename BVHTREE_CLASS::ItemPairs &p_to = tree._pairs[p_hb.id()];

		// does this pair exist already?
		// or only check the one with lower number of pairs for greater speed
		if (p_from.num_pairs <= p_to.num_pairs)
		{
			if (p_from.contains_pair_to(p_hb))
				return;
		}
		else
		{
			if (p_to.contains_pair_to(p_ha))
				return;
		}

		// callback
		void * callback_userdata = nullptr;

		if (pair_callback) {
			const typename BVHTREE_CLASS::ItemExtra &exa = _get_extra(p_ha);
			const typename BVHTREE_CLASS::ItemExtra &exb = _get_extra(p_hb);

			callback_userdata = pair_callback(pair_callback_userdata, p_ha, exa.userdata, exa.subindex, p_hb, exb.userdata, exb.subindex);

#ifdef BVH_DEBUG_CALLBACKS
			print_line("Pair callback : " + itos (p_ha.id()) + " to " + itos(p_hb.id()));
#endif
		}

		// new pair! .. only really need to store the userdata on the lower handle, but both have storage so...
		p_from.add_pair_to(p_hb, callback_userdata);
		p_to.add_pair_to(p_ha, callback_userdata);


		/*
		Pair p;
		p.handle[0] = collider;
		p.handle[1] = collidee;
		p.sort();
		p.version = _tick;
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
		*/
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

		typename BVHTREE_CLASS::ItemPairs &p_from = tree._pairs[p_handle.id()];

		// remove from pairing list for every partner
		if (!p_from.extended())
		{
			for (int n=0; n<p_from.num_pairs; n++)
			{
				BVHHandle h_to = p_from.pairs[n].handle;
				_unpair(p_handle, h_to);
			}
		}
		else
		{
			for (int n=0; n<p_from.extended_pairs.size(); n++)
			{
				BVHHandle h_to = p_from.extended_pairs[n].handle;
				_unpair(p_handle, h_to);
			}
		}


		/*
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
		*/
	}


#ifdef BVH_DEBUG_DRAW
public:
	void draw_debug(ImmediateGeometry * p_im)
	{
		tree.draw_debug(p_im);
	}
#endif

private:
	const typename BVHTREE_CLASS::ItemExtra &_get_extra(BVHHandle p_handle) const
	{
		return tree._extra[p_handle.id()];
	}
	const typename BVHTREE_CLASS::ItemRef &_get_ref(BVHHandle p_handle) const
	{
		return tree._refs[p_handle.id()];
	}


	void _reset()
	{
		changed_items.clear();
		_tick++;
	}

	void _add_changed_item(BVHHandle p_handle, const AABB &aabb)
	{
		//return;
		
		// only if uses pairing
		// no .. non pairable items seem to be able to pair with pairable
//		if (!tree.item_is_pairable(p_handle))
//			return;

		// aabb check with expanded aabb. This greatly decreases processing
		// at the cost of slightly less accurate pairing checks
		AABB &expanded_aabb = tree._pairs[p_handle.id()].expanded_aabb;
		if (expanded_aabb.encloses(aabb))
			return;
		
		uint32_t &last_updated_tick = tree._extra[p_handle.id()].last_updated_tick;

		if (last_updated_tick == _tick)
			return; // already on changed list
		
		// mark as on list
		last_updated_tick = _tick;

		// new expanded aabb
		expanded_aabb = aabb;
		expanded_aabb.grow_by(0.1f);
		
		
		// todo .. add bitfield for fast check
//		for (int n=0; n<changed_items.size(); n++)
//		{
//			// already on list
//			if (changed_items[n] == p_handle)
//				return;
//		}

		changed_items.push_back(p_handle);
	}

	void _remove_changed_item(BVHHandle p_handle)
	{
		// only if uses pairing
		if (!tree.item_is_pairable(p_handle))
			return;
		
		// Care has to be taken here for items that are deleted. The ref ID
		// could be reused on the same tick for new items. This is probably
		// rare but should be taken into consideration

		// only if uses pairing
//		if (!item_is_pairable(p_handle))
//			return;

		// callbacks
		_remove_pairs_containing(p_handle);

		// remove from changed items (not very efficient yet)
		for (int n=0; n<changed_items.size(); n++)
		{
			if (changed_items[n] == p_handle)
			{
				changed_items.remove_unordered(n);
			}
		}

		// reset the last updated tick (may not be necessary but just in case)
		tree._extra[p_handle.id()].last_updated_tick = 0;
	}

	PairCallback pair_callback;
	UnpairCallback unpair_callback;
	void *pair_callback_userdata;
	void *unpair_callback_userdata;

	BVHTREE_CLASS tree;

	// for collision pairing,
	// maintain a list of all items moved etc on each frame / tick
	LocalVector<BVHHandle, uint32_t, true> changed_items;
	uint32_t _tick;

#ifdef BVH_DEBUG_DRAW
	int _added_item_count;
#endif

public:
	BVH_Manager()
	{
		_tick = 1; // start from 1 so items with 0 indicate never updated
		pair_callback = nullptr;
		unpair_callback = nullptr;
		pair_callback_userdata = nullptr;
		unpair_callback_userdata = nullptr;

#ifdef BVH_DEBUG_DRAW
		_added_item_count = 0;
#endif
	}
};


