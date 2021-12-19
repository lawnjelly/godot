#pragma once

#include "core/local_vector.h"
#include "core/math/vector3.h"

class MeshSimplify {
public:
	bool simplify(const LocalVectori<uint32_t> &p_inds, const LocalVectori<Vector3> &p_verts, LocalVectori<uint32_t> &r_inds);

private:
	const LocalVectori<Vector3> *_verts = nullptr;
};
