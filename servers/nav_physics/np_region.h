#pragma once

#include "core/math/transform.h"

namespace NavPhysics {

// A region (with a single transform, representing a single node in the scene tree)
// can hold multiple Meshes for different Agent sizes.
class Region {
	Transform _transform;
	Transform _transform_inverse;
	bool _transform_identity = true;

	TrackedPooledList<uint32_t> _meshes;

	// For agent to agent collision detection, we maintain a scaling
	// for an AABB formed by all the registered meshes.
	// This enables us to maintain a simple orientated grid to quickly find agent neighbours.
	Vector2 _float_to_fp_scale;
	Vector2 _float_to_fp_offset;

	Vector2 _fp_to_float_scale;
	Vector2 _fp_to_float_offset;

public:
	void set_transform(const Transform &p_xform);
	const Transform &get_transform() const { return _transform; }
	const Transform &get_transform_inverse() const { return _transform_inverse; }
	bool is_transform_identity() const { return _transform_identity; }

	uint32_t register_mesh(uint32_t p_mesh_id);
	void unregister_mesh(uint32_t p_mesh_id, uint32_t p_mesh_slot_id);
};

} //namespace NavPhysics
