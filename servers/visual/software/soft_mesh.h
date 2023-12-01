#pragma once

#include "core/local_vector.h"
#include "servers/visual/visual_server_scene.h"

class SoftMaterial;

class SoftMesh {
	friend class SoftRend;

	struct Surface {
		LocalVector<Vector3> positions;
		LocalVector<Vector3> xformed_positions;
		LocalVector<Plane> xformed_hpositions;
		//LocalVector<Vector3> normals;
		//LocalVector<Vector3> xformed_normals;
		LocalVector<Vector2> uvs;
		LocalVector<int> indices;

		uint32_t soft_material_id = UINT32_MAX;
	};

	struct Data {
		LocalVector<Surface> surfaces;
	} data;

public:
	void create(SoftRend &r_renderer, VisualServerScene::Instance *p_instance);
};
