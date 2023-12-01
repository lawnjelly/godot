#include "soft_mesh.h"
#include "scene/resources/mesh_data_tool.h"
#include "servers/visual/visual_server_globals.h"
#include "soft_surface.h"

void SoftMesh::create(SoftRend &r_renderer, VisualServerScene::Instance *p_instance) {
	//VisualServerScene::InstanceGeometryData *geom = static_cast<VisualServerScene::InstanceGeometryData *>(p_instance->base_data);
	int num_surfs = VSG::storage->mesh_get_surface_count(p_instance->base);
	data.surfaces.resize(num_surfs);

	//	MeshDataTool mdt;
	//	mdt.create_from_surface(

	//	virtual Array mesh_surface_get_arrays(RID p_mesh, int p_surface) const;

	for (int n = 0; n < num_surfs; n++) {
		Array arrays = VisualServer::get_singleton()->mesh_surface_get_arrays(p_instance->base, n);

		if (!arrays.size()) {
			continue;
		}

		PoolVector<Vector3> vertices = arrays[VS::ARRAY_VERTEX];

		Surface &surf = data.surfaces[n];
		surf.positions = vertices;

		PoolVector<int> indices = arrays[VS::ARRAY_INDEX];
		surf.indices = indices;

		surf.uvs = (PoolVector<Vector2>)arrays[VS::ARRAY_TEX_UV];

		RID rid_material = p_instance->material_override;

		if (!rid_material.is_valid()) {
			if (n < p_instance->materials.size()) {
				rid_material = p_instance->materials[n];
			}
		}

		if (!rid_material.is_valid()) {
			rid_material = VisualServer::get_singleton()->mesh_surface_get_material(p_instance->base, n);
		}

		if (rid_material.is_valid()) {
			surf.soft_material_id = r_renderer.materials.find_or_create_material(rid_material);
		}
	}
}
