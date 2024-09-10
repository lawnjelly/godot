#include "np_mesh.h"
#include "../source/navphysics_loader.h"
#include "../source/navphysics_log.h"
#include "../source/navphysics_map.h"
#include "../source/navphysics_pointf.h"
#include "../source/navphysics_pointi.h"
#include "../source/navphysics_vector.h"

NPMesh::NPMesh() {
	data.h_mesh = NavPhysics::g_world.safe_mesh_create();
}

NPMesh::~NPMesh() {
	if (data.h_mesh) {
		NavPhysics::g_world.safe_mesh_free(data.h_mesh);
		data.h_mesh = 0;
	}
}

bool NPMesh::load(const Vector3 *p_verts, uint32_t p_num_verts, const uint32_t *p_indices, uint32_t p_num_indices) {
	NavPhysics::Loader loader;
	NavPhysics::Loader::SourceMeshData source;

	NavPhysics::Vector<NavPhysics::FPoint3> np_verts;
	NavPhysics::Vector<u32> np_inds;

	for (uint32_t n = 0; n < p_num_verts; n++) {
		const Vector3 &pt = p_verts[n];
		np_verts.push_back(NavPhysics::FPoint3::make(pt.x, pt.y, pt.z));
	}

	for (uint32_t n = 0; n < p_num_indices; n++) {
		np_inds.push_back(p_indices[n]);
	}

	NavPhysics::Vector<u32> poly_num_inds;
	poly_num_inds.resize(p_num_indices / 3);

	for (u32 n = 0; n < poly_num_inds.size(); n++) {
		poly_num_inds[n] = 3;
	}

	source.num_verts = np_verts.size();
	source.verts = np_verts.ptr();
	source.num_indices = np_inds.size();
	source.indices = np_inds.ptr();
	source.num_polys = poly_num_inds.size();
	source.poly_num_indices = poly_num_inds.ptr();

	NavPhysics::Mesh *mesh = NavPhysics::g_world.safe_get_mesh(data.h_mesh);

	loader.load_mesh(source, *mesh);

	return true;
}
