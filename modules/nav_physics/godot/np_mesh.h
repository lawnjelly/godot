#pragma once

#include "../source/navphysics_defines.h"
#include "core/resource.h"

class NavigationMeshInstance;

class NPMesh : public Resource {
	//class NPMesh : public Reference {
	//	GDCLASS(NPMesh, Reference);
	GDCLASS(NPMesh, Resource);
	OBJ_SAVE_TYPE(NPMesh);
	RES_BASE_EXTENSION("npmesh");

	struct Data {
		np_handle h_mesh = 0;
	} data;

public:
	NPMesh();
	~NPMesh();

	bool load(const Vector3 *p_verts, uint32_t p_num_verts, const uint32_t *p_indices, uint32_t p_num_indices);
	bool load(const NavigationMeshInstance &p_nav_mesh_instance);
};
