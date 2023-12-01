#pragma once

#include "core/local_vector.h"
#include "servers/visual/visual_server_scene.h"
#include "soft_renderer.h"

class SoftMaterial;

class SoftMesh {
	RID rid;
	uint32_t refcount = 0;

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

	void create_from_mesh(const RID &p_mesh);
	const RID &get_rid() const { return rid; }

	void ref() { refcount++; }
	bool unref() {
		if (refcount) {
			refcount--;
			return !refcount;
		}
		DEV_ASSERT(0);
		return true;
	}
};

class SoftMeshInstance {
	uint32_t _mesh_id = UINT32_MAX;
	LocalVector<uint32_t> surface_material_ids;

	void _create_from_mesh(VisualServerScene::Instance *p_instance, const RID &p_mesh_rid);

public:
	void create_from_mesh(VisualServerScene::Instance *p_instance);
	void create_from_multimesh(VisualServerScene::Instance *p_instance);
	uint32_t get_mesh_id() const { return _mesh_id; }
	uint32_t get_surface_material_id(uint32_t p_surface) { return surface_material_ids[p_surface]; }
	~SoftMeshInstance();
};

class SoftMeshes {
	TrackedPooledList<SoftMesh> _meshes;

public:
	uint32_t find_or_create(RID p_mesh) {
		for (uint32_t n = 0; n < _meshes.active_size(); n++) {
			uint32_t id = _meshes.get_active_id(n);
			if (_meshes[id].get_rid() == p_mesh) {
				// Add another reference.
				_meshes[id].ref();
				return id;
			}
		}
		uint32_t id;
		SoftMesh *mesh = _meshes.request(id);
		mesh->create_from_mesh(p_mesh);
		mesh->ref();

		//print_line("create mesh " + itos(id) + ", total meshes " + itos(_meshes.active_size()));
		return id;
	}
	void unref_mesh(uint32_t p_id) {
		SoftMesh &mesh = get(p_id);
		if (mesh.unref()) {
			_meshes.free(p_id);
		}
	}
	SoftMesh &get(uint32_t p_id) {
		return _meshes[p_id];
	}
	const SoftMesh &get(uint32_t p_id) const {
		return _meshes[p_id];
	}
};
