#pragma once

#include "core/local_vector.h"
#include "core/math/vector3.h"

class MeshSimplify {
	struct Data {
		LocalVector<uint32_t> indices;
		LocalVector<Vector3> positions;
	} data;

public:
	void declare_indices(const Span<int> &p_indices);
	void declare_positions(const Span<Vector3> &p_positions);

	bool simplify_mesh();
};
