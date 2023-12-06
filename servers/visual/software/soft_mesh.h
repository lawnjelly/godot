#pragma once

#include "core/local_vector.h"
#include "servers/visual/visual_server_scene.h"

class SoftMaterial;

class SoftMesh {
	RID rid;

public:
	struct Surface {
		LocalVector<Vector3> positions;
		//LocalVector<Vector3> xformed_positions;
		//LocalVector<Plane> xformed_hpositions;
		//LocalVector<Vector3> normals;
		//LocalVector<Vector3> xformed_normals;
		LocalVector<Vector2> uvs;
		LocalVector<int> indices;

		uint32_t soft_material_id = UINT32_MAX;
	};

	struct Data {
		LocalVector<Surface> surfaces;
	} data;

	void create(SoftRend &r_renderer, const RID &p_mesh);
	const RID &get_rid() const { return rid; }
};

class SoftMeshInstance {
	uint32_t _mesh_id = UINT32_MAX;
	LocalVector<uint32_t> surface_material_ids;

public:
	void create(SoftRend &r_renderer, VisualServerScene::Instance *p_instance);
	uint32_t get_mesh_id() const { return _mesh_id; }
	uint32_t get_surface_material_id(uint32_t p_surface) { return surface_material_ids[p_surface]; }
};

class SoftMeshes {
	LocalVector<SoftMesh> _meshes;

public:
	uint32_t find_or_create(SoftRend &r_renderer, RID p_mesh) {
		for (uint32_t n = 0; n < _meshes.size(); n++) {
			if (_meshes[n].get_rid() == p_mesh) {
				return n;
			}
		}
		uint32_t id = _meshes.size();
		_meshes.resize(id + 1);
		_meshes[id].create(r_renderer, p_mesh);
		return id;
	}
	SoftMesh &get(uint32_t p_id) {
		return _meshes[p_id];
	}
	const SoftMesh &get(uint32_t p_id) const {
		return _meshes[p_id];
	}
};
