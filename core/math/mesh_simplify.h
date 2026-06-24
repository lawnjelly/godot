#pragma once

#include "core/local_vector.h"
#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include <core/math/aabb.h>
#include <core/math/plane_64.h>
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
		LocalVector<Vector2> uvs;
		LocalVector<Vector2> uv2s;
	} input_data;

	struct Tri {
		Tri() {
			for (int n = 0; n < 3; n++) {
				corn[n] = UINT32_MAX;
				edge_ids[n] = UINT32_MAX;
			}
		}
		// corner indices
		uint32_t corn[3];
		uint32_t edge_ids[3];
		bool active = true;

		Plane_64 plane;
	};

	struct Edge {
		uint32_t a = UINT32_MAX;
		uint32_t b = UINT32_MAX;
		uint32_t vertex_to_collapse_to = UINT32_MAX;
		uint32_t version = 0;

		double cost = 0;
		bool active = true;

		bool is_seam_or_boundary = false;
		uint32_t triangle_count = 0;

		uint32_t get_collapse_from() const { return vertex_to_collapse_to == a ? b : a; }
		uint32_t get_readable_cost() const { return cost / 100000000; }
		//uint32_t get_readable_cost() const { return cost / 10; }

		String info() const;
		void sort() {
			if (a > b) {
				SWAP(a, b);
			}
		}
		void check_sorted() const {
			DEV_ASSERT(a < b);
		}
		bool operator==(const Edge &p_o) const { return (a == p_o.a) && (b == p_o.b); }
	};

	struct SortedEdge {
		uint32_t edge_id;
		double cost = 0;
		uint32_t version;

		SortedEdge(uint32_t p_id, double p_cost, uint32_t p_version) {
			edge_id = p_id;
			cost = p_cost;
			version = p_version;
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

		// Use Hughes Hoppe's version of storing UV mapping
		// as quadrics, similar to position.
		Quadric Qu;
		Quadric Qv;

		Vector2 uv;
		Vector2 uv2;

		// A vertex is active until it has been collapsed
		bool active = false;
		bool is_seam_or_boundary = false;

		// Edges that use this vertex.
		LocalVector<uint32_t> edges;
		LocalVector<uint32_t> seam_neighbour_verts;

		Vector3_64 pos() const {
			return Vector3_64(position.x, position.y, position.z);
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
	void _detect_seam_edges();

	void _triangle_calculate_plane(uint32_t p_tri_id);
	void _initialize_vertex_quadrics();
	void _evaluate_edge_collapse(uint32_t p_edge_id);
	double _compute_quadric_error(const Vector3i &p_pos, const Quadric &Q);
	double _compute_attribute_error(const Vector3i &p_pos, double p_attr, const Quadric &Qa);

	uint32_t _create_edge(uint32_t p_corn_a, uint32_t p_corn_b, uint32_t p_triangle_id);

	int32_t _triangle_which_side(const Vector3i &p_a, const Vector3i &p_b, const Vector3i &p_c, const Vector3i &p_test) const;
	bool _is_triangle_degenerate(const uint32_t p_inds[3]) const;
	//bool _can_collapse_edge(const Edge& edge) const;
	bool _can_collapse(uint32_t kept, uint32_t deleted) const;
	bool _is_triangle_degenerate_from_positions(const Vector3i p[3]) const;

	void _validate_and_rebuild();

	void _debug_log_input_data();

public:
	void declare_indices(const Span<int> &p_indices);
	void declare_positions(const Span<Vector3> &p_positions);
	void declare_uvs(const Span<Vector2> &p_uvs);
	void declare_uv2s(const Span<Vector2> &p_uvs);

	bool simplify_mesh();

	Span<uint32_t> get_simplified_remapped_indices() {
		return Span<uint32_t>(data.output_remapped_indices.ptr(), data.output_remapped_indices.size());
	}
};
