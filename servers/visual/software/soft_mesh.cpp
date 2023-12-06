#include "soft_mesh.h"
#include "scene/resources/mesh_data_tool.h"
#include "servers/visual/visual_server_globals.h"
#include "soft_surface.h"

void SoftMesh::create(const RID &p_mesh) {
	rid = p_mesh;

	int num_surfs = VSG::storage->mesh_get_surface_count(rid);
	data.surfaces.resize(num_surfs);

	//	MeshDataTool mdt;
	//	mdt.create_from_surface(

	//	virtual Array mesh_surface_get_arrays(RID p_mesh, int p_surface) const;

	for (int n = 0; n < num_surfs; n++) {
		Array arrays = VisualServer::get_singleton()->mesh_surface_get_arrays(rid, n);

		if (!arrays.size()) {
			continue;
		}

		PoolVector<Vector3> vertices = arrays[VS::ARRAY_VERTEX];

		Surface &surf = data.surfaces[n];
		surf.positions = vertices;

		PoolVector<int> indices = arrays[VS::ARRAY_INDEX];
		surf.indices = indices;

		DEV_ASSERT((indices.size() % 3) == 0);

		// Check inds
		for (uint32_t i = 0; i < surf.indices.size(); i++) {
			DEV_ASSERT(surf.indices[i] < surf.positions.size());
		}

		surf.uvs = (PoolVector<Vector2>)arrays[VS::ARRAY_TEX_UV];

		//DEV_ASSERT(surf.uvs.size() == surf.positions.size());
		if (surf.uvs.size() != surf.positions.size()) {
			WARN_PRINT_ONCE("SoftMesh num UVs does not match num verts.");
			surf.uvs.resize(surf.positions.size());
			surf.uvs.fill(Vector2());
		}

		//		RID rid_material = p_instance->material_override;

		//		if (!rid_material.is_valid()) {
		//			if (n < p_instance->materials.size()) {
		//				rid_material = p_instance->materials[n];
		//			}
		//		}

		//		if (!rid_material.is_valid()) {
		RID rid_material = VisualServer::get_singleton()->mesh_surface_get_material(rid, n);
		//		}

		if (rid_material.is_valid()) {
			surf.soft_material_id = g_soft_rend.materials.find_or_create_material(rid_material);
		}
	}
}

SoftMeshInstance::~SoftMeshInstance() {
	if (_mesh_id != UINT32_MAX) {
		g_soft_rend.meshes->unref_mesh(_mesh_id);
		_mesh_id = UINT32_MAX;
	}
}

void SoftMeshInstance::create(VisualServerScene::Instance *p_instance) {
	//VisualServerScene::InstanceGeometryData *geom = static_cast<VisualServerScene::InstanceGeometryData *>(p_instance->base_data);
	RID mesh_rid = p_instance->base;

	_mesh_id = g_soft_rend.meshes->find_or_create(mesh_rid);
	const SoftMesh &mesh = g_soft_rend.meshes->get(_mesh_id);

	surface_material_ids.resize(mesh.data.surfaces.size());

	for (uint32_t n = 0; n < surface_material_ids.size(); n++) {
		RID rid_material = p_instance->material_override;

		if (!rid_material.is_valid()) {
			if (n < p_instance->materials.size()) {
				rid_material = p_instance->materials[n];
			}
		}

		if (!rid_material.is_valid()) {
			// Use the mesh material
			surface_material_ids[n] = mesh.data.surfaces[n].soft_material_id;
		} else {
			surface_material_ids[n] = g_soft_rend.materials.find_or_create_material(rid_material);
		}
	}
}
