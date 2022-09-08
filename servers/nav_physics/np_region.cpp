#include "np_region.h"
#include "np_mesh.h"

namespace NavPhysics {

void Region::set_transform(const Transform &p_xform) {
	_transform = p_xform;
	// print_line("set transform " + String(Variant(p_xform)));

	_transform_inverse = _transform.affine_inverse();

	// If it is identity transform, we can remove a bunch of calculations per agent
	_transform_identity = _transform.is_equal_approx(Transform());

	// send transform on to child meshes
	for (uint32_t n = 0; n < _meshes.active_size(); n++) {
		uint32_t mesh_id = _meshes.get_active(n);

		Mesh &mesh = g_world.get_mesh(mesh_id);
		mesh.set_transform(p_xform, _transform_inverse, _transform_identity);
	}
}

uint32_t Region::register_mesh(uint32_t p_mesh_id) {
	uint32_t slot_id;
	uint32_t *pid = _meshes.request(slot_id);
	*pid = p_mesh_id;

	// send the current region transform to the new mesh
	if (!is_transform_identity()) {
		Mesh &mesh = g_world.get_mesh(p_mesh_id);
		mesh.set_transform(_transform, _transform_inverse, _transform_identity);
	}

	return slot_id;
}

void Region::unregister_mesh(uint32_t p_mesh_id, uint32_t p_mesh_slot_id) {
	ERR_FAIL_COND(_meshes[p_mesh_slot_id] != p_mesh_id);
	_meshes.free(p_mesh_slot_id);
}

} //namespace NavPhysics
