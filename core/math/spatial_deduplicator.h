#pragma once

#include "core/local_vector.h"
#include "core/math/vector3.h"

#define SPATIAL_DEDUPLICATOR_USE_REFERENCE_IMPL

class SpatialDeduplicator {
	real_t _epsilon = 0.01;

public:
	bool deduplicate(const uint32_t *p_inds, uint32_t p_num_inds, const Vector3 *p_verts, uint32_t p_num_verts, LocalVectori<Vector3> &r_verts, LocalVectori<uint32_t> &r_out_inds, real_t p_epsilon = 0.01);
};
