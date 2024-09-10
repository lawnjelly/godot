#pragma once

namespace NavPhysics {

// A region (with a single transform, representing a single node in the scene tree)
// can hold multiple Meshes for different Agent sizes.
class Region {
	Transform _transform;
	Transform _transform_inverse;
	bool _transform_identity = true;

	TrackedPooledList<u32> _meshes;

	// For agent to agent collision detection, we maintain a scaling
	// for an AABB formed by all the registered meshes.
	// This enables us to maintain a simple orientated grid to quickly find agent neighbours.
	FPoint2 _f32_to_fp_scale;
	FPoint2 _f32_to_fp_offset;

	FPoint2 _fp_to_f32_scale;
	FPoint2 _fp_to_f32_offset;

public:
	void set_transform(const Transform &p_xform);
	const Transform &get_transform() const { return _transform; }
	const Transform &get_transform_inverse() const { return _transform_inverse; }
	bool is_transform_identity() const { return _transform_identity; }

	u32 register_mesh(u32 p_mesh_id);
	void unregister_mesh(u32 p_mesh_id, u32 p_mesh_slot_id);

	//	PoolVector<Face3> region_get_faces() const;
};

} // namespace NavPhysics
