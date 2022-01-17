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
		//LocalVectori<uint32_t> ancestral_tris;
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
		bool is_linked_to(uint32_t p_vert_id) const {
			return linked_verts.find(p_vert_id) != -1;
		}

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
			if (edge_vert_neighs[0] != UINT32_MAX) {
				DEV_ASSERT(!edge_vert_neighs_same());
			}
		}

		//		uint32_t count_edge_vert_neighs() const {
		//			uint32_t count = 0;
		//			if (edge_vert_neighs[0] != UINT32_MAX)
		//				count++;
		//			if (edge_vert_neighs[1] != UINT32_MAX)
		//				count++;
		//			return count;
		//		}
		uint32_t get_other_edge_vert_neigh(uint32_t p_first) const {
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
			//error = 0.0;
		}
		bool operator==(const Collapse &p_o) const { return (from == p_o.from) && (to == p_o.to); }

		union {
			struct {
				uint32_t from;
				uint32_t to;
			};
			uint32_t ids[2];
		};
		//real_t error;
	};

	struct CollapseGroup {
		CollapseGroup() {
			error = 0.0;
			size = 0;
		}
		Collapse c[4];

		// number of collapses used
		uint32_t size;

		// 1 = regular collapse
		// 2 = mirror collapse or edge collapse
		// 4 = mirror edge collapse

		// highest error.
		// apply a negative offset for mirror collapses, so they get processed first
		float error;

		// disallow if two collapses contain the same verts
		bool is_invalid() const {
			for (int n = 0; n < size; n++) {
				for (int i = 0; i < 2; i++) {
					uint32_t id = c[n].ids[i];

					for (int m = n + 1; m < size; m++) {
						if ((c[m].ids[0] == id) || (c[m].ids[1] == id))
							return true;
					}
				}
			}
			return false;
		}

		bool contains_collapse(const Collapse &p_c) const {
			for (int n = 0; n < 4; n++) {
				if (c[n] == p_c)
					return true;
			}
			return false;
		}

		bool contains(uint32_t p_id) const {
			for (uint32_t n = 0; n < size; n++) {
				const Collapse &co = c[n];
				if ((co.from == p_id) || (co.to == p_id))
					return true;
			}

			return false;
		}

		void add_collapse(uint32_t p_from, uint32_t p_to, float p_error) {
			Collapse &co = c[size];
			co.from = p_from;
			co.to = p_to;
			error = MAX(error, p_error);
			size++;
			DEV_ASSERT(size <= 4);
		}

		// we actually want the best at the end, so they are cheaper to pop
		bool operator<(const CollapseGroup &p_o) const { return error > p_o.error; }
	};

public:
	void add_attribute(const SpatialDeduplicator::Attribute &p_attr) {
		_deduplicator._attributes.push_back(p_attr);
	}
	void clear_attributes() {
		_deduplicator._attributes.clear();
	}

	// returns number of indices
	uint32_t simplify_occluders(const uint32_t *p_in_inds, uint32_t p_num_in_inds, const Vector3 *p_in_verts, uint32_t p_num_in_verts, uint32_t *r_out_inds, Vector3 *r_out_verts, uint32_t &r_num_out_verts, real_t p_simplification);

	// target fraction is number of target tris / start tris
	// surf error is the allowed error over a surface (0 to 1)
	// edge simplification is the amount of angle in edges allowed to be simplified (0 is no edge simp)
	uint32_t simplify_map(const uint32_t *p_in_inds, uint32_t p_num_in_inds, const Vector3 *p_in_verts, uint32_t p_num_in_verts, uint32_t *r_out_inds, LocalVectori<uint32_t> &r_vert_map, uint32_t &r_num_out_verts, real_t p_tri_target_fraction = 0.01, real_t p_surf_detail = 1.0, real_t p_edge_simplification = 1.0);

private:
	void _finalize_merge(uint32_t p_vert_from_id, uint32_t p_vert_to_id);

	//void _add_collapse(uint32_t p_vert_from_id, uint32_t p_vert_to_id, real_t p_max_displacement);

	void _create_tris(const uint32_t *p_inds, uint32_t p_num_inds);
	void _establish_neighbours_for_tri(int p_tri_id);
	void _establish_neighbours(int p_tri0, int p_tri1);

	uint32_t _choose_vert_to_merge(uint32_t p_start_from) const;
	bool _find_vert_to_merge_to(uint32_t p_vert_from);
	void _adjust_tri(uint32_t p_tri_id, uint32_t p_vert_from, uint32_t p_vert_to);
	void _resync_tri(uint32_t p_tri_id);
	bool _calculate_plane(uint32_t p_corns[3], Plane &r_plane) const;
	bool _allow_collapse(uint32_t p_tri_id, uint32_t p_vert_test, uint32_t p_vert_from, uint32_t p_vert_to, real_t &r_dist) const;
	void _delete_triangle(uint32_t p_tri_id);

	bool _is_reciprocal_edge(uint32_t p_vert_a, uint32_t p_vert_b) const;

	void _optimize_vertex_cache(uint32_t *r_inds, uint32_t p_num_inds, uint32_t p_num_verts) const;
	void _detect_mirror_verts();
	bool _detect_mirror_tris(const Tri &p_a, const Tri &p_b, int &r_axis, real_t &r_midpoint) const;
	void _find_mirror_verts(const Tri &p_a, const Tri &p_b, int p_axis, real_t p_midpoint);

	void _debug_verify_vert(uint32_t p_vert_id);
	void _debug_verify_verts();

	// NEW
	void _create_heap();
	bool _evaluate_collapse(uint32_t p_vert_from_id, uint32_t p_vert_to_id, CollapseGroup &r_cg, bool p_second_edge = false);
	void _evaluate_collapse_group(uint32_t p_vert_from_id, uint32_t p_vert_to_id);
	bool _evaluate_collapses_from_vertex(uint32_t p_vert_from_id);
	void _reevaluate_from_changed_vertex(uint32_t p_deleted_vert, uint32_t p_central_vert);
	void _heap_remove(uint32_t p_vert_id);
	void _destroy_collapse_group(const CollapseGroup &p_cg);
	//uint32_t _heap_find_collapse(uint32_t p_vert_from_id, uint32_t p_vert_to_id);

	bool _new_simplify();
	bool _collapse_matching_edge(const Collapse &p_collapse);

	bool _cg_creates_flipped_tris(const CollapseGroup &p_cg);

	bool _find_second_edge_from_to(uint32_t p_vert_from_id, uint32_t p_vert_to_id, uint32_t &r_vert_from_id, uint32_t &r_vert_to_id) const;
	bool _are_verts_linked(uint32_t p_id_a, uint32_t p_id_b) const;

	// after collapsing to a vertex, and adding an ancestral vert,
	// the cost of collapsing this vert to others has changed and needs recalculating
	void _recalculate_collapse_errors_from_vert(uint32_t p_vert_merge_id);
	bool _evaluate_collapse_metric(uint32_t p_vert_from_id, uint32_t p_vert_to_id, real_t &r_error) const;

	void _debug_print_collapse_group(const CollapseGroup &p_cg, int p_tabs = 0, String p_title = String()) const;

	LocalVectori<Vert> _verts;
	LocalVectori<Tri> _tris;
	uint32_t _active_tri_count = 0;

	LocalVectori<uint32_t> _merge_vert_ids;
	bool _mirror_verts_only = false;
	real_t _edge_simplification = 1.0;
	real_t _allowed_surface_error = 1.0;

	// pending list of collapses, so we can backtrack and undo
	// collapses in the case of mirror verts
	//LocalVectori<Collapse> _collapses;

	// sorted heap of collapses
	LocalVectori<CollapseGroup> _heap;

	// we don't add new collapses directly to the heap..
	// this saves us doing lots of sorting, and also makes the verts
	// mirroring more likely to work correctly.
	LocalVectori<CollapseGroup> _heap_pending;

	void _consolidate_heap();

	// This temporary triangle list is used multiple times,
	// and is stored on the object instead of recreating each time
	// to save on allocations.
	LocalVectori<uint32_t> _possible_tris;

	bool dot_edge_test(const Vector3 &p_a, const Vector3 &p_b, const Vector3 &p_c) const {
		if (_edge_simplification > 0.0) {
			// never allow edge
			if (_edge_simplification == 1.0)
				return false;

			Vector3 a = p_b - p_a;
			Vector3 b = p_c - p_b;

			real_t la = a.length();
			real_t lb = b.length();

			// always pass if points are close together, we want to get rid of this edge vert
			if ((la < 0.001) || (lb < 0.001))
				return true;

			// normalize
			a *= 1.0 / la;
			b *= 1.0 / lb;

			real_t dot = a.dot(b);
			if (dot < _edge_simplification)
				return false;
		}

		// always allow edge
		return true;
	}

	// Adapted from https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
	bool dist_point_from_line(const Vector3 &p_pt, const Vector3 &p_a, const Vector3 &p_b, real_t &r_dist) const {
		Vector3 n = p_b - p_a;
		real_t n_l = n.length();

		// line is undefined in direction, because the two points are too close / zero..
		// just return distance to the points.
		if (n_l < 0.0001) {
			r_dist = (p_pt - p_a).length();
			return false;
		}

		Vector3 p_min_a = p_pt - p_a;
		Vector3 cross = p_min_a.cross(n);
		real_t l = cross.length();

		r_dist = l / n_l;
		return true;
	}

	/*
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
	*/
};
