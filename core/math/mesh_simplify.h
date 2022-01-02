#pragma once

#include "core/local_vector.h"
#include "core/math/aabb.h"
#include "core/math/plane.h"
#include "core/math/spatial_deduplicator.h"
#include "core/math/vector3.h"

#define GODOT_MESH_SIMPLIFY_USE_DIRTY_VERTS
#define GODOT_MESH_SIMPLIFY_OPTIMIZED_NEIGHS

class MeshSimplify {
	SpatialDeduplicator _deduplicator;

	struct Edge {
		Edge() {
			a = UINT32_MAX;
			b = UINT32_MAX;
		}
		void sort() {
			if (b > a) {
				SWAP(a, b);
			}
		}
		bool operator==(const Edge &p_o) const { return (a == p_o.a) && (b == p_o.b); }
		uint32_t a, b;
	};

	struct Tri {
		Tri() {
			for (int n = 0; n < 3; n++) {
				corn[n] = UINT32_MAX;
				neigh[n] = UINT32_MAX;
			}
			aabb_volume = 0.0;
		}
		// corner indices
		uint32_t corn[3];

		Edge edge[3];
		Plane plane;

		// neighboursing triangles
		uint32_t neigh[3];
		uint32_t num_neighs = 0;

		bool active = true;

		// quicker detection of mirror verts by
		// quick rejection
		bool has_mirror = false;

		// for detecting mirroring
		AABB aabb;
		real_t aabb_volume;
	};

	struct Vert {
		Vert() {
			edge_vert_neighs[0] = UINT32_MAX;
			edge_vert_neighs[1] = UINT32_MAX;
			mirror_vert = UINT32_MAX;
		}

		void link_tri(uint32_t p_id) {
			int64_t res = tris.find(p_id);
			if (res == -1) {
				tris.push_back(p_id);
			}
		}

		void add_edge_vert_neigh(uint32_t p_vert_id) {
			edge_vert = true;
			// already present?
			if (has_edge_vert_neigh(p_vert_id)) {
				return;
			}

			// first neighbour?
			if (edge_vert_neighs[0] == UINT32_MAX) {
				edge_vert_neighs[0] = p_vert_id;
				check_edge_vert_neighs();
				return;
			}

			// should only be two neighbours possible,
			// except edge t junctions, and this is a special case
			if (edge_vert_neighs[1] == UINT32_MAX) {
				edge_vert_neighs[1] = p_vert_id;
				check_edge_vert_neighs();
				return;
			}

			// special case, more than 2 edge neighbours,
			// lock the vert
			locked = true;
			edge_vert_neighs[0] = UINT32_MAX;
			edge_vert_neighs[1] = UINT32_MAX;
		}

		Vector3 pos;

		// If mirroring is detected on an axis,
		// a vert can have a mirror which can also be merged
		// at the same time to give symmetrical LODs
		uint32_t mirror_vert;

		// List of triangles that use this vertex
		LocalVectori<uint32_t> tris;

		// ancestors
		LocalVectori<uint32_t> ancestral_tris;
		LocalVectori<uint32_t> ancestral_verts;

		// list of vertices that this vertex is already registered to collapse
		// to on the heap
		LocalVectori<uint32_t> heap_collapse_to;
		//LocalVectori<uint32_t> heap_collapse_from;

		// List of verts that share the same position
		// (these will usually be on another edge, and separated
		// as a result of UVs or normals).
		// When collapsing edge verts, we will only do if we can
		// also similarly collapse the linked vert, to preserve the shared
		// edge between the two zones (otherwise you get an ugly seam
		// like Blender decimate).
		LocalVectori<uint32_t> linked_verts;

		// A vertex is active until it has been collapsed
		bool active = false;

		// if a vert has more than 2 edge neighbours
		// (i.e. at a t junction of edges)
		// lock it to prevent collapse, we want to preserve these cases
		bool locked = false;

		// If we are an edge vert, we will have neighbouring edge verts.
		// We can use these to determine whether a collapse is allowed along the edge...
		// A straight line is ok to collapse, but e.g. a right angle will change the outline too much.
		bool edge_vert = false;

		// Colinear is ok for collapsing to neighbouring edge vert, but non colinear is not
		bool edge_colinear = false;

		// The neighbouring verts on either side if we follow this edge
		uint32_t edge_vert_neighs[2];

		// Some helpful edge funcs.
		bool edge_vert_neighs_same() const { return edge_vert_neighs[0] == edge_vert_neighs[1]; }
		void check_edge_vert_neighs() {
			DEV_ASSERT(!edge_vert_neighs_same());
		}

		//		uint32_t count_edge_vert_neighs() const {
		//			uint32_t count = 0;
		//			if (edge_vert_neighs[0] != UINT32_MAX)
		//				count++;
		//			if (edge_vert_neighs[1] != UINT32_MAX)
		//				count++;
		//			return count;
		//		}
		uint32_t get_other_edge_vert_neigh(uint32_t p_first) {
			if (edge_vert_neighs[0] == p_first) {
				return edge_vert_neighs[1];
			}
			DEV_ASSERT(edge_vert_neighs[1] == p_first);
			return edge_vert_neighs[0];
		}
		void exchange_edge_vert_neigh(uint32_t p_from, uint32_t p_to) {
			if (edge_vert_neighs[0] == p_from) {
				edge_vert_neighs[0] = p_to;
				if (edge_vert_neighs_same()) {
					edge_vert_neighs[0] = UINT32_MAX;
				}
				check_edge_vert_neighs();
				return;
			}
			DEV_ASSERT(edge_vert_neighs[1] == p_from);
			edge_vert_neighs[1] = p_to;
			if (edge_vert_neighs_same()) {
				edge_vert_neighs[1] = UINT32_MAX;
			}
			check_edge_vert_neighs();
		}
		void set_other_edge_vert_neigh(uint32_t p_keep, uint32_t p_change) {
			if (p_keep == p_change) {
				;
			}

			if (edge_vert_neighs[0] == p_keep) {
				edge_vert_neighs[1] = p_change;
				if (edge_vert_neighs_same()) {
					edge_vert_neighs[1] = UINT32_MAX;
				}
				check_edge_vert_neighs();
				return;
			}
			DEV_ASSERT(edge_vert_neighs[1] == p_keep);
			edge_vert_neighs[0] = p_change;
			if (edge_vert_neighs_same()) {
				edge_vert_neighs[0] = UINT32_MAX;
			}
			check_edge_vert_neighs();
		}
		bool has_edge_vert_neigh(uint32_t p_vert_id) const {
			return (edge_vert_neighs[0] == p_vert_id) || (edge_vert_neighs[1] == p_vert_id);
		}

		// We only need to check dirty vertices for collapsing from..
		// i.e. vertices that have recently had neighbours collapsed.
		// If we have already checked a vertex for collapsing and no neighbours
		// have changed, no need to check again in a new iteration.
#ifdef GODOT_MESH_SIMPLIFY_USE_DIRTY_VERTS
		bool dirty = true;
#endif

		// The max displacement of the points merged to this vertex so far.
		// This prevents "creep", where a vertex merges slowly a large displacement
		// than would be possible over a single merge
		real_t displacement = 0.0;
	};

	struct Collapse {
		Collapse() {
			from = UINT32_MAX;
			to = UINT32_MAX;
			error = 0.0;
		}

		// we actually want the best at the end, so they are cheaper to pop
		bool operator<(const Collapse &p_o) const { return error > p_o.error; }
		uint32_t from;
		uint32_t to;
		real_t error;
	};

public:
	void add_attribute(const SpatialDeduplicator::Attribute &p_attr) {
		_deduplicator._attributes.push_back(p_attr);
	}
	void clear_attributes() {
		_deduplicator._attributes.clear();
	}

	// returns number of indices
	uint32_t simplify(const uint32_t *p_inds, uint32_t p_num_inds, const Vector3 *p_verts, uint32_t p_num_verts, uint32_t *r_inds, Vector3 *r_deduped_verts, uint32_t &r_num_deduped_verts, real_t p_threshold = 0.01);

	uint32_t simplify_map(const uint32_t *p_in_inds, uint32_t p_num_in_inds, const Vector3 *p_in_verts, uint32_t p_num_in_verts, uint32_t *r_out_inds, LocalVectori<uint32_t> &r_vert_map, uint32_t &r_num_out_verts, real_t p_threshold = 0.01);

private:
	bool _simplify();
	bool _simplify_vert(uint32_t p_vert_from_id);
	bool _simplify_vert_primary(uint32_t p_vert_from_id, uint32_t p_vert_to_id);

	bool _tri_simplify_linked_merge_vert(uint32_t p_vert_from_id, uint32_t p_vert_to_id);
	void _finalize_merge(uint32_t p_vert_from_id, uint32_t p_vert_to_id, real_t p_max_displacement);

	void _add_collapse(uint32_t p_vert_from_id, uint32_t p_vert_to_id, real_t p_max_displacement);
	void _apply_collapses();

	void _create_tris(const uint32_t *p_inds, uint32_t p_num_inds);
	void _establish_neighbours_for_tri(int p_tri_id);
	void _establish_neighbours(int p_tri0, int p_tri1);

	uint32_t _choose_vert_to_merge(uint32_t p_start_from) const;
	bool _find_vert_to_merge_to(uint32_t p_vert_from);
	void _adjust_tri(uint32_t p_tri_id, uint32_t p_vert_from, uint32_t p_vert_to);
	void _resync_tri(uint32_t p_tri_id);
	bool _calculate_plane(uint32_t p_corns[3], Plane &r_plane) const;
	bool _allow_collapse(uint32_t p_tri_id, uint32_t p_vert_from, uint32_t p_vert_to, real_t &r_max_displacement, bool p_test_displacement = true) const;
	uint32_t _find_or_add(uint32_t p_val, LocalVectori<uint32_t> &r_list);
	void _delete_triangle(uint32_t p_tri_id);

	bool _is_reciprocal_edge(uint32_t p_vert_a, uint32_t p_vert_b) const;
	bool _check_merge_allowed(uint32_t p_vert_from, uint32_t p_vert_to) const;

	void _optimize_vertex_cache(uint32_t *r_inds, uint32_t p_num_inds, uint32_t p_num_verts) const;
	void _detect_mirror_verts();
	bool _detect_mirror_tris(const Tri &p_a, const Tri &p_b, int &r_axis, real_t &r_midpoint) const;
	void _find_mirror_verts(const Tri &p_a, const Tri &p_b, int p_axis, real_t p_midpoint);

	void _debug_verify_vert(uint32_t p_vert_id);
	void _debug_verify_verts();

	// NEW
	void _create_heap();
	void _evaluate_collapse(uint32_t p_vert_from_id, uint32_t p_vert_to_id);
	bool _evaluate_collapses_from_vertex(uint32_t p_vert_from_id);
	void _reevaluate_from_changed_vertex(uint32_t p_deleted_vert, uint32_t p_central_vert);
	void _heap_remove(uint32_t p_vert_id);
	bool _new_simplify();

	LocalVectori<Vert> _verts;
	LocalVectori<Tri> _tris;

	LocalVectori<uint32_t> _merge_vert_ids;
	real_t _threshold_dist = 0.01;
	bool _mirror_verts_only = false;

	// pending list of collapses, so we can backtrack and undo
	// collapses in the case of mirror verts
	LocalVectori<Collapse> _collapses;

	// sorted heap of collapses
	LocalVectori<Collapse> _heap;

	// This temporary triangle list is used multiple times,
	// and is stored on the object instead of recreating each time
	// to save on allocations.
	LocalVectori<uint32_t> _possible_tris;

	bool _try_simplify_merge_vert(Vert &p_vert_from, uint32_t p_vert_from_id, uint32_t p_vert_to_id, real_t &r_max_displacement) const {
		const Vert &vert_to = _verts[p_vert_to_id];

		// disallow if edge verts and not linked
		if (p_vert_from.edge_vert || vert_to.edge_vert) {
			if (!_is_reciprocal_edge(p_vert_from_id, p_vert_to_id))
				return false;
		}

		r_max_displacement = 0.0;

		// mirror vert only?
		if (_mirror_verts_only) {
			if (vert_to.mirror_vert == UINT32_MAX)
				return false;
		}

		// is this suitable for collapse?
		for (int n = 0; n < p_vert_from.tris.size(); n++) {
			if (!_allow_collapse(p_vert_from.tris[n], p_vert_from_id, p_vert_to_id, r_max_displacement)) {
				return false;
			}
		}

		return true;
	}
};
