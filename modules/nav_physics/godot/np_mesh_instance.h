#pragma once

#include "../source/navphysics_defines.h"
#include "np_mesh.h"
#include "scene/3d/spatial.h"

class NPMeshInstance : public Spatial {
	GDCLASS(NPMeshInstance, Spatial);

	struct Data {
		np_handle h_mesh_instance = 0;
		Ref<NPMesh> mesh;
	} data;

	void resource_changed(RES res);
	void _update_server();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_mesh(const Ref<NPMesh> &p_mesh);
	Ref<NPMesh> get_mesh() const;

	String get_configuration_warning() const;

	NPMeshInstance();
	~NPMeshInstance();
};
