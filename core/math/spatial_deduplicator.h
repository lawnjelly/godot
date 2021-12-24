#pragma once

#include "core/local_vector.h"
#include "core/math/vector2.h"
#include "core/math/vector3.h"

#define SPATIAL_DEDUPLICATOR_USE_REFERENCE_IMPL

class SpatialDeduplicator {
	real_t _epsilon = 0.01;

public:
	struct Attribute {
		enum Type {
			AT_NORMAL,
			AT_UV,
			AT_POSITION,
		};
		Type type = AT_NORMAL;
		real_t epsilon_dedup = 0.0;
		real_t epsilon_merge = 0.0;
		union {
			const Vector3 *vec3s = nullptr;
			const Vector2 *vec2s;
		};
	};

	// As well as position, vertices can prevent merging based on
	// multiple optional custom attributes.
	// These will be tested prior to merging.
	LocalVectori<Attribute> _attributes;

	bool deduplicate_verts_only(const uint32_t *p_in_inds, uint32_t p_num_in_inds, const Vector3 *p_in_verts, uint32_t p_num_in_verts, LocalVectori<Vector3> &r_out_verts, LocalVectori<uint32_t> &r_out_inds, real_t p_epsilon = 0.01);

	bool deduplicate_map(const uint32_t *p_in_inds, uint32_t p_num_in_inds, const Vector3 *p_in_verts, uint32_t p_num_in_verts, LocalVectori<uint32_t> &r_vert_map, uint32_t &r_num_out_verts, LocalVectori<uint32_t> &r_out_inds, real_t p_epsilon = 0.01);
};
