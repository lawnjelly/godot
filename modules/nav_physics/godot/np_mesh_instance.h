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

	struct DebugData {
		RID debug_polys;
	} debug_data;

	void resource_changed(RES res);
	void _update_server();
	void _update_visibility();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_mesh(const Ref<NPMesh> &p_mesh);
	Ref<NPMesh> get_mesh() const;
	Vector3 choose_random_location() const;

	bool refresh_debug_geometry(bool p_show);

	String get_configuration_warning() const;

	NPMeshInstance();
	~NPMeshInstance();
};

class NPMap : public Node {
	GDCLASS(NPMap, Node);

protected:
	void _notification(int p_what);
	static void _bind_methods();
	static void _agent_callback(uint64_t p_user_data, const NavPhysics::FPoint3 &p_position, const NavPhysics::FPoint3 &p_velocity);

public:
	NPMap();
};
