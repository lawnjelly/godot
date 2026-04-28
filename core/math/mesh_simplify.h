#pragma once

#include "core/local_vector.h"
#include "core/math/vector3.h"
#include <core/math/aabb.h>
#include <core/math/vector3i.h>

class MeshSimplify {
	struct InputData {
		LocalVector<uint32_t> indices;
		LocalVector<Vector3> positions;
	} input_data;

	struct Face {
		uint32_t v[3] = {};
	};

	struct Edge {
		uint32_t v[2] = {};
		double cost = 0;
	};

	struct Data {
		uint32_t grid_size = 65535;

		AABB bound;
		double bound_extent = 0;

		// Quantized positions in integer space.
		// Removes float error from the math.
		LocalVector<Vector3i> positions;
		LocalVector<Face> faces;
		LocalVector<Edge> edges;

		Vector3i find_grid_pos(const Vector3 &p_pos) const;
	} data;

public:
	void declare_indices(const Span<int> &p_indices);
	void declare_positions(const Span<Vector3> &p_positions);

	bool simplify_mesh();
};
