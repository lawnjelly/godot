#pragma once

#include "core/object.h"
#include "core/reference.h"
#include "np_defines.h"

class NavigationMesh;

// Note parameters are tick rate dependent, account for this before calling.
class NavPhysicsServer : public Object {
	GDCLASS(NavPhysicsServer, Object);

	static NavPhysicsServer *singleton;

protected:
	static void _bind_methods();

public:
	static NavPhysicsServer *get_singleton();

	// Maps
	np_handle map_create();
	void map_free(np_handle p_map);

	// Regions - regions can contain multiple meshes of different agent sizes
	np_handle region_create();
	void region_free(np_handle p_region);
	void region_set_transform(np_handle p_region, const Transform &p_xform);

	// Purely for debugging views
	PoolVector<Face3> region_get_faces(np_handle p_region) const;

	// Meshes
	np_handle mesh_create();
	void mesh_free(np_handle p_mesh);
	bool mesh_load(np_handle p_mesh, const Ref<NavigationMesh> &p_navmesh);
	void mesh_set_map(np_handle p_mesh, np_handle p_map);
	void mesh_set_region(np_handle p_mesh, np_handle p_region);

	// Bodies
	np_handle body_create();
	void body_free(np_handle p_body);
	void body_set_map(np_handle p_body, np_handle p_map);
	void body_set_enabled(np_handle p_body, bool p_enabled);
	void body_teleport(np_handle p_body, const Vector3 &p_position);
	void body_add_impulse(np_handle p_body, const Vector3 &p_impulse);
	void body_set_params(np_handle p_body, real_t p_friction, bool p_ignore_narrowings);
	void body_set_callback(np_handle p_body, Object *p_receiver);
	bool body_get_info(np_handle p_body, NavPhysics::BodyInfo &r_body_info);
	const NavPhysics::TraceResult &body_trace(np_handle p_body, const Vector3 &p_destination, bool p_trace_navmesh = true, bool p_trace_obstacles = false);
	//const Vector3 &body_iterate(np_handle p_body);

	void tick_update(real_t p_delta);

	NavPhysicsServer();
	~NavPhysicsServer();
};
