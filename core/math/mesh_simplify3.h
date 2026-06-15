#pragma once

#if 0

#include "core/local_vector.h"
#include "core/math/vector3.h"
#include <core/math/aabb.h>
#include <core/math/vector3i.h>

class MeshSimplify {
	// Symmetric 4x4 matrix,
	// can be stored as just half to save memory / calcs.
	struct Quadric {
		double m[4][4] = {}; // Initialized to all zeros

		// Add two quadric matrices together
		Quadric operator+(const Quadric &other) const {
			Quadric res;

			const double *a = &m[0][0];
			const double *b = &other.m[0][0];
			double *c = &res.m[0][0];

			for (uint32_t n = 0; n < 16; n++) {
				*c++ = *a++ + *b++;
			}
			return res;
		}
	};

	struct InputData {
		LocalVector<uint32_t> indices;
		LocalVector<Vector3> positions;
	} input_data;

	struct Tri {
		Tri() {
			for (int n = 0; n < 3; n++) {
				corn[n] = UINT32_MAX;
				//neigh[n] = UINT32_MAX;
//#define STORE_EDGES_IN_TRIS
#ifdef STORE_EDGES_IN_TRIS
				edge[n] = UINT32_MAX;
#endif
			}
		}
		// corner indices
		uint32_t corn[3];

#ifdef STORE_EDGES_IN_TRIS
		uint32_t edge[3];
#endif

		// neighbouring triangles
		//uint32_t neigh[3];
		//uint32_t num_neighs = 0;

		bool active = true;

		Plane plane;
	};

	struct Edge {
		uint32_t a = UINT32_MAX;
		uint32_t b = UINT32_MAX;
		double cost = 0;
		uint32_t vertex_to_collapse_to = UINT32_MAX;
		bool active = true;

		void sort() {
			if (b > a) {
				SWAP(a, b);
			}
		}
		bool operator==(const Edge &p_o) const { return (a == p_o.a) && (b == p_o.b); }

//#define LINK_EDGES_TO_TRIS
#ifdef LINK_EDGES_TO_TRIS
		// List of triangles using this edge.
		// If there is only 1, it must be a mesh edge,
		// therefore having different rules for collapse.
		LocalVector<uint32_t> tris;

		void link_tri(uint32_t p_id) {
			int64_t res = tris.find(p_id);
			if (res == -1) {
				tris.push_back(p_id);
			}
		}
#endif
	};

	struct SortedEdge {
		uint32_t edge_id;
		double cost = 0;

		SortedEdge(uint32_t p_id, double p_cost) {
			edge_id = p_id;
			cost = p_cost;
		}

		// Overload the less-than operator for std::priority_queue
		bool operator<(const SortedEdge &o) const {
			// Invert the operator: higher cost means "less priority" (lower in the queue)
			return this->cost > o.cost;
		}
	};

	struct Vert {
		Vector3i position;
		Quadric Q;

		// A vertex is active until it has been collapsed
		bool active = false;

		// List of triangles that use this vertex
#ifdef LINK_VERTS_TO_TRIS
		LocalVector<uint32_t> tris;
#endif

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
#if 0
		LocalVector<uint32_t> linked_verts;
		bool is_linked_to(uint32_t p_vert_id) const {
			return linked_verts.find(p_vert_id) != -1;
		}


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

#ifdef LINK_VERTS_TO_TRIS
		void link_tri(uint32_t p_id) {
			int64_t res = tris.find(p_id);
			if (res == -1) {
				tris.push_back(p_id);
			}
		}
#endif
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
#endif
		Vector3 pos() const {
			return Vector3(position.x, position.y, position.z);
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

		// Remapped to the original vertices.
		LocalVector<uint32_t> output_remapped_indices;
	} data;

	void _create_tris();
	void _triangle_calculate_plane(uint32_t p_tri_id);
	void _initialize_vertex_quadrics();
	void _evaluate_edge_collapse(uint32_t p_edge_id);
	double _compute_quadric_error(const Vector3i &p_pos, const Quadric &Q);

	uint32_t _create_edge(uint32_t p_corn_a, uint32_t p_corn_b, uint32_t p_triangle_id);
	double plane_coord(const Plane &p_plane, uint32_t p_coord) const {
		switch (p_coord) {
			case 0: {
				return p_plane.normal.x;
			} break;
			case 1: {
				return p_plane.normal.y;
			} break;
			case 2: {
				return p_plane.normal.z;
			} break;
			case 3: {
				return p_plane.d;
			} break;
			default: {
				DEV_ASSERT(0);
			} break;
		}
		return 0;
	}

	int32_t _triangle_which_side(const Vector3i &p_a, const Vector3i &p_b, const Vector3i &p_c, const Vector3i &p_test) const;
	bool _is_triangle_degenerate(const uint32_t p_inds[3]) const;

public:
	void declare_indices(const Span<int> &p_indices);
	void declare_positions(const Span<Vector3> &p_positions);

	bool simplify_mesh();

	Span<uint32_t> get_simplified_remapped_indices() {
		return Span<uint32_t>(data.output_remapped_indices.ptr(), data.output_remapped_indices.size());
	}
};

#endif
