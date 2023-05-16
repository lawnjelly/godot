#include "navphysics_region.h"

namespace NavPhysics {

void Region::set_transform(const Transform &p_xform) {
	_transform = p_xform;
	// print_line("set transform " + String(Variant(p_xform)));

	_transform_inverse = _transform.affine_inverse();

	// If it is identity transform, we can remove a bunch of calculations per agent
	_transform_identity = _transform.is_equal_approx(Transform());

	// send transform on to child meshes
	for (u32 n = 0; n < _meshes.active_size(); n++) {
		u32 mesh_id = _meshes.get_active(n);

		Mesh &mesh = g_world.get_mesh(mesh_id);
		mesh.set_transform(p_xform, _transform_inverse, _transform_identity);
	}
}

u32 Region::register_mesh(u32 p_mesh_id) {
	u32 slot_id;
	u32 *pid = _meshes.request(slot_id);
	*pid = p_mesh_id;

	// send the current region transform to the new mesh
	if (!is_transform_identity()) {
		Mesh &mesh = g_world.get_mesh(p_mesh_id);
		mesh.set_transform(_transform, _transform_inverse, _transform_identity);
	}

	return slot_id;
}

void Region::unregister_mesh(u32 p_mesh_id, u32 p_mesh_slot_id) {
	NP_NP_ERR_FAIL_COND(_meshes[p_mesh_slot_id] != p_mesh_id);
	_meshes.free(p_mesh_slot_id);
}

/*
PoolVector<Face3> Region::region_get_faces() const {
	for (u32 n = 0; n < _meshes.active_size(); n++) {
		u32 mesh_id = _meshes.get_active(n);

		const Mesh &mesh = g_world.get_mesh(mesh_id);
		return mesh.mesh_get_faces();
	}

	return PoolVector<Face3>();
}
*/

} // namespace NavPhysics
