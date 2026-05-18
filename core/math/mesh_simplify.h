#pragma once

#include "core/local_vector.h"
#include "core/math/vector3.h"
#include <core/math/aabb.h>
#include <core/math/vector3i.h>

class MeshSimplify {
#if 0
	struct Quadric {
		// Symmetric 4x4 matrix stored as:
		// A  B  C  D
		//    E  F  G
		//       H  I
		//          J
		double a, b, c, d; // first row
		double e, f, g; // second row (symmetric)
		double h, i; // third row
		double j; // bottom right

		Quadric() :
				a(0), b(0), c(0), d(0), e(0), f(0), g(0), h(0), i(0), j(0) {}
	};
#endif

	struct InputData {
		LocalVector<uint32_t> indices;
		LocalVector<Vector3> positions;
	} input_data;

	struct Tri {
		Tri() {
			for (int n = 0; n < 3; n++) {
				corn[n] = UINT32_MAX;
				neigh[n] = UINT32_MAX;
				edge[n] = UINT32_MAX;
			}
		}
		// corner indices
		uint32_t corn[3];
		uint32_t edge[3];

		// neighbouring triangles
		uint32_t neigh[3];
		uint32_t num_neighs = 0;
	};

	struct Edge {
		uint32_t a, b;
		double cost;

		// List of triangles using this edge.
		// If there is only 1, it must be a mesh edge,
		// therefore having different rules for collapse.
		LocalVector<uint32_t> tris;

		Edge() {
			a = UINT32_MAX;
			b = UINT32_MAX;
			cost = 0;
		}
		void sort() {
			if (b > a) {
				SWAP(a, b);
			}
		}
		bool operator==(const Edge &p_o) const { return (a == p_o.a) && (b == p_o.b); }

		void link_tri(uint32_t p_id) {
			int64_t res = tris.find(p_id);
			if (res == -1) {
				tris.push_back(p_id);
			}
		}
	};

	struct Vert {
		Vector3i position;

		// List of triangles that use this vertex
		LocalVector<uint32_t> tris;

		// ancestors
		//LocalVector<uint32_t> ancestral_verts;

		// list of vertices that this vertex is already registered to collapse
		// to on the heap
		//LocalVector<uint32_t> heap_collapse_to;

		// List of verts that share the same position
		// (these will usually be on another edge, and separated
		// as a result of UVs or normals).
		// When collapsing edge verts, we will only do if we can
		// also similarly collapse the linked vert, to preserve the shared
		// edge between the two zones (otherwise you get an ugly seam
		// like Blender decimate).
		LocalVector<uint32_t> linked_verts;
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

		// The max displacement of the points merged to this vertex so far.
		// This prevents "creep", where a vertex merges slowly a large displacement
		// than would be possible over a single merge
		real_t displacement = 0;

		// Some helpful edge funcs.
		bool edge_vert_neighs_same() const { return edge_vert_neighs[0] == edge_vert_neighs[1]; }
		void check_edge_vert_neighs() {
			if (edge_vert_neighs[0] != UINT32_MAX) {
				DEV_ASSERT(!edge_vert_neighs_same());
			}
		}

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

		Vert() {
			edge_vert_neighs[0] = UINT32_MAX;
			edge_vert_neighs[1] = UINT32_MAX;
		}
	};

	struct Data {
		uint32_t grid_size = 65535;

		AABB bound;
		double bound_extent = 0;

		// Quantized positions in integer space.
		// Removes float error from the math.
		LocalVector<Vert> verts;
		LocalVector<Tri> tris;
		LocalVector<Edge> edges;

		Vector3i find_grid_pos(const Vector3 &p_pos) const;
	} data;

	void _create_tris();
	uint32_t _create_edge(uint32_t p_corn_a, uint32_t p_corn_b, uint32_t p_triangle_id);

	int32_t _triangle_which_side(const Vector3i &p_a, const Vector3i &p_b, const Vector3i &p_c, const Vector3i &p_test) const;
	bool _is_triangle_degenerate(const uint32_t p_inds[3]) const;

public:
	void declare_indices(const Span<int> &p_indices);
	void declare_positions(const Span<Vector3> &p_positions);

	bool simplify_mesh();
};
