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
		void sort()
		{
			if (ref_id[1] < ref_id[0])
			{
				uint32_t t = ref_id[1];
				ref_id[1] = ref_id[0];
				ref_id[0] = t;
			}
		}
		uint32_t ref_id[2];
		uint32_t version; // add 1 each tick / frame
		void *ud; // userdata for the pair (needed for client, not sure what used for)
	};

	class PairHashtable
	{
		int _hash(const Pair &p) const
		{
			int h = p.ref_id[0] + p.ref_id[1];
			h %= 128;
			return h;
		}
	public:
		enum {NUM_BINS = 128};
		LocalVector<Pair, uint32_t, true> bins[NUM_BINS];

		Pair * add(uint32_t a, uint32_t b)
		{
			Pair p;
			p.ref_id[0] = a;
			p.ref_id[1] = b;
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
		BVHHandle h = tree.item_add(p_userdata, p_aabb, p_subindex, p_pairable, p_pairable_type, p_pairable_mask);
		if (USE_PAIRS)
			_add_changed_item(h);
		return h;
	}

	void move(BVHHandle p_id, const AABB &p_aabb)
	{
		tree.item_move(p_id, p_aabb);
		if (USE_PAIRS)
			_add_changed_item(p_id);
	}
	void erase(BVHHandle p_id)
	{
		// call unpair and remove all references to the item
		// before deleting from the tree
		if (USE_PAIRS)
			_remove_changed_item(p_id);

		tree.item_remove(p_id);
	}

	void set_pairable(BVHHandle p_handle, bool p_pairable, uint32_t p_pairable_type, uint32_t p_pairable_mask)
	{
		uint32_t ref_id = p_handle.id;

		// get the reference
		//ItemRef &ref = _refs[ref_id];
		typename BVHTREE_CLASS::ItemExtra &extra = tree._extra[ref_id];
		extra.pairable = p_pairable;
		extra.pairable_mask = p_pairable_mask;
		extra.pairable_type = p_pairable_type;
	}


	// cull tests
	int cull_aabb(const AABB &p_aabb, T **p_result_array, int p_result_max, int *p_subindex_array = nullptr, uint32_t p_mask = 0xFFFFFFFF)
	{
		return tree.cull_aabb(p_aabb, p_result_array, p_result_max, p_subindex_array, p_mask);
	}

	int cull_segment(const Vector3 &p_from, const Vector3 &p_to, T **p_result_array, int p_result_max, int *p_subindex_array = nullptr, uint32_t p_mask = 0xFFFFFFFF)
	{
		return tree.cull_segment(p_from, p_to, p_result_array, p_result_max, p_subindex_array, p_mask);
	}

	int cull_point(const Vector3 &p_point, T **p_result_array, int p_result_max, int *p_subindex_array = nullptr, uint32_t p_mask = 0xFFFFFFFF)
	{
		return tree.cull_point(p_point, p_result_array, p_result_max, p_subindex_array, p_mask);
	}

	int cull_convex(const Vector<Plane> &p_convex, T **p_result_array, int p_result_max, uint32_t p_mask = 0xFFFFFFFF)
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

	// do this after moving etc.
	void check_for_collisions()
	{
		AABB bb;

		for (int n=0; n<changed_items.size(); n++)
		{
			const BVHHandle &h = changed_items[n];

			if (!tree._extra[h.id].pairable)
				continue;

			tree.item_get_AABB(h, bb);

			//int cull_aabb(const AABB &p_aabb, T **p_result_array, int p_result_max, int *p_subindex_array, uint32_t p_mask, bool p_translate_hits = true)

			// get the results into _cull_hits
			tree.cull_aabb(bb, nullptr, 0, nullptr, 0, false);

			for (int n=0; n<tree._cullhits; n++)
			{
				uint32_t ref_id = tree._cullhits[n];

				if (ref_id == h.id)
					continue;

				if (!tree._extra[ref_id].pairable)
					continue;

				// checkmasks ... NYI

				_collide(h.id, ref_id);
			}

		}

		_reset();
	}

	// supplemental funcs
	bool item_is_pairable(BVHHandle p_handle) const
	{
		uint32_t ref_id = p_handle.id;
		return tree._extra[ref_id].pairable;
	}

	T * item_get_userdata(BVHHandle p_handle) const
	{
		uint32_t ref_id = p_handle.id;
		return tree._extra[ref_id].userdata;
	}

	void item_get_AABB(BVHHandle p_handle, AABB &r_aabb) const
	{
		uint32_t ref_id = p_handle.id;
		typename BVHTREE_CLASS::ItemRef &ref = tree._refs[ref_id];
		typename BVHTREE_CLASS::TNode &tnode = tree._nodes[ref.tnode_id];
		typename BVHTREE_CLASS::Item &item = tnode.get_item(ref.item_id);
		item.aabb.to(r_aabb);
	}

	int item_get_subindex(BVHHandle p_handle) const
	{
		uint32_t ref_id = p_handle.id;
		return tree._extra[ref_id].subindex;
	}

private:
	void _collide(uint32_t ref_id_a, uint32_t ref_id_b)
	{
		Pair p;
		p.ref_id[0] = ref_id_a;
		p.ref_id[1] = ref_id_b;
		p.sort();
		p.version = _version;

		Pair * p_added_pair = _pairs_hashtable.add(p);
		if (p_added_pair)
		{
			// callback start collision
			if (pair_callback) {
				BVHHandle ha, hb;
				ha.id = p.ref_id[0];
				hb.id = p.ref_id[1];

				typename BVHTREE_CLASS::ItemExtra &exa = tree._extra[ha.id];
				typename BVHTREE_CLASS::ItemExtra &exb = tree._extra[hb.id];

				p_added_pair->ud = pair_callback(pair_callback_userdata, ha, exa.userdata, exa.subindex, hb, exb.userdata, exb.subindex);
			}

		}
	}

	// pairs that were not colliding in this version
	void _remove_stale_pairs()
	{
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
	}

	// if we remove an item, we need to immediately remove the pairs, to prevent reading the pair after deletion
	void _remove_pairs_containing(uint32_t p_ref_id)
	{
		for (int b=0; b<_pairs_hashtable.NUM_BINS; b++)
		{
			for (int n=0; n<_pairs_hashtable.bins[b].size(); n++)
			{
				const Pair &p = _pairs_hashtable.bins[b][n];
				if ((p.ref_id[0] == p_ref_id) || (p.ref_id[1] == p_ref_id))
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
	}


#ifdef BVH_DEBUG_DRAW
	void draw_debug(ImmediateGeometry * p_im)
	{
		tree.draw_debug(p_im);
	}
#endif

private:
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
			if (changed_items[n].id == p_handle.id)
				return;
		}

		changed_items.push_back(p_handle);
	}

	void _remove_changed_item(BVHHandle p_handle)
	{
		// only if uses pairing
		if (!item_is_pairable(p_handle))
			return;

		_remove_pairs_containing(p_handle.id);

		// remove from changed items (not very efficient yet)
		for (int n=0; n<changed_items.size(); n++)
		{
			if (changed_items[n].id == p_handle.id)
			{
				changed_items.remove_unordered(n);
			}
		}
	}

	PairCallback pair_callback;
	UnpairCallback unpair_callback;
	void *pair_callback_userdata;
	void *unpair_callback_userdata;

	BVHTREE_CLASS tree;

	// for collision pairing,
	// maintain a list of all items moved etc on each frame / tick
	LocalVector<BVHHandle, uint32_t, true> changed_items;

public:
	BVH_Manager()
	{
		_version = 0;
		pair_callback = nullptr;
		unpair_callback = nullptr;
		pair_callback_userdata = nullptr;
		unpair_callback_userdata = nullptr;
	}
};


