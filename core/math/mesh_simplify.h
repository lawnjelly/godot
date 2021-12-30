#pragma once

#include "core/local_vector.h"
#include "core/math/plane.h"
#include "core/math/spatial_deduplicator.h"
#include "core/math/vector3.h"

// The optional callback can test any triangle before allowing a merge, allowing testing normals, UVs etc to prevent
// merging where the change in the attribute is too high.
// This could alternatively be written as a template, which would be more efficient but require everything in the header.
//typedef bool (*MeshSimplifyCallback)(void *p_userdata, const uint32_t p_tri_from[3], const uint32_t p_tri_to[3]);

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
		}
		// corner indices
		uint32_t corn[3];

		Edge edge[3];

		Plane plane;

		// neighboursing triangles
		uint32_t neigh[3];
		uint32_t num_neighs = 0;

		bool active = true;
	};

	struct Vert {
		Vert() {
			edge_vert_neighs[0] = UINT32_MAX;
			edge_vert_neighs[1] = UINT32_MAX;
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
		LocalVectori<uint32_t> tris;
		LocalVectori<uint32_t> linked_verts;
		bool active = false;

		// if a vert has more than 2 edge neighbours
		// (i.e. at a t junction of edges)
		// lock it to prevent collapse
		bool locked = false;

		// If we are an edge vert, we will have neighbouring edge verts.
		// We can use these to determine whether a collapse is allowed along the edge...
		// A straight line is ok to collapse, but e.g. a right angle will change the outline too much.
		bool edge_vert = false;
		bool edge_colinear = false;
		uint32_t edge_vert_neighs[2];

		bool edge_vert_neighs_same() const { return edge_vert_neighs[0] == edge_vert_neighs[1]; }
		void check_edge_vert_neighs() {
			DEV_ASSERT(!edge_vert_neighs_same());
		}

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
	bool _simplify_vert(uint32_t p_vert_id);
	bool _tri_simplify_linked_merge_vert(uint32_t p_vert_from_id, uint32_t p_vert_to_id);
	void _finalize_merge(uint32_t p_vert_from_id, uint32_t p_vert_to_id, real_t p_max_displacement);

	void _create_tris(const uint32_t *p_inds, uint32_t p_num_inds);
	void _establish_neighbours_for_tri(int p_tri_id);
	void _establish_neighbours(int p_tri0, int p_tri1);

	uint32_t _choose_vert_to_merge(uint32_t p_start_from) const;
	bool _find_vert_to_merge_to(uint32_t p_vert_from);
	void _adjust_tri(uint32_t p_tri_id, uint32_t p_vert_from, uint32_t p_vert_to);
	void _resync_tri(uint32_t p_tri_id);
	bool _calculate_plane(uint32_t p_corns[3], Plane &r_plane) const;
	bool _allow_collapse(uint32_t p_tri_id, uint32_t p_vert_from, uint32_t p_vert_to, real_t &r_max_displacement) const;
	uint32_t _find_or_add(uint32_t p_val, LocalVectori<uint32_t> &r_list);
	void _delete_triangle(uint32_t p_tri_id);

	bool _is_reciprocal_edge(uint32_t p_vert_a, uint32_t p_vert_b) const;
	bool _check_merge_allowed(uint32_t p_vert_from, uint32_t p_vert_to) const;

	//	void _deduplicate_verts(const uint32_t *p_in_inds, uint32_t p_num_in_inds, const Vector3 *p_in_verts, uint32_t p_num_in_verts, LocalVectori<uint32_t> &r_vert_map, uint32_t &r_num_out_verts, LocalVectori<Vector3> &r_deduped_verts, LocalVectori<uint32_t> &r_deduped_verts_source, LocalVectori<uint32_t> &r_deduped_inds);
	void _optimize_vertex_cache(uint32_t *r_inds, uint32_t p_num_inds, uint32_t p_num_verts) const;

	void _debug_verify_vert(uint32_t p_vert_id);
	void _debug_verify_verts();

	LocalVectori<Vert> _verts;
	LocalVectori<Tri> _tris;

	LocalVectori<uint32_t> _merge_vert_ids;
	real_t _threshold_dist = 0.01;

	// This temporary triangle list is used multiple times,
	// and is stored on the object instead of recreating each time
	// to save on allocations.
	LocalVectori<uint32_t> _possible_tris;

	//MeshSimplifyCallback _callback = nullptr;
	//void *_callback_userdata = nullptr;

	bool _try_simplify_merge_vert(Vert &p_vert_from, uint32_t p_vert_from_id, uint32_t p_vert_to_id, real_t &r_max_displacement) const {
		r_max_displacement = 0.0;

		// edge vert? can only collapse to a neighbouring edge vert
		if (p_vert_from.edge_vert) {
			if ((p_vert_from.edge_vert_neighs[0] != p_vert_to_id) && (p_vert_from.edge_vert_neighs[1] != p_vert_to_id)) {
				return false;
			}
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
