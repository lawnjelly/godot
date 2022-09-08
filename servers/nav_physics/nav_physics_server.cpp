#include "nav_physics_server.h"
#include "core/engine.h"
#include "np_loader.h"
#include "np_map.h"
#include "np_mesh.h"
#include "np_region.h"
#include "np_structs.h"

NavPhysicsServer *NavPhysicsServer::singleton = nullptr;
NavPhysics::TraceResult NavPhysicsServer::_trace_result;

NavPhysicsServer *NavPhysicsServer::get_singleton() {
	return singleton;
}

NavPhysicsServer::NavPhysicsServer() {
	ERR_FAIL_COND(singleton != nullptr);
	singleton = this;
}

NavPhysicsServer::~NavPhysicsServer() {
	singleton = nullptr;
}

void NavPhysicsServer::tick_update(real_t p_delta) {
#ifdef TOOLS_ENABLED
	if (!Engine::get_singleton()->is_editor_hint()) {
		NavPhysics::g_world.tick_update(p_delta);
	}
#else
	NavPhysics::g_world.tick_update(p_delta);
#endif
}

void NavPhysicsServer::_bind_methods() {
}

np_handle NavPhysicsServer::map_create() {
	return NavPhysics::g_world.safe_map_create();
}

void NavPhysicsServer::map_free(np_handle p_map) {
	NavPhysics::g_world.safe_map_free(p_map);
}

np_handle NavPhysicsServer::region_create() {
	return NavPhysics::g_world.safe_region_create();
}

void NavPhysicsServer::region_free(np_handle p_region) {
	NavPhysics::g_world.safe_region_free(p_region);
}

void NavPhysicsServer::region_set_transform(np_handle p_region, const Transform &p_xform) {
	NavPhysics::Region *region = NavPhysics::g_world.safe_get_region(p_region);
	ERR_FAIL_NULL(region);
	region->set_transform(p_xform);
}

PoolVector<Face3> NavPhysicsServer::region_get_faces(np_handle p_region) const {
	NavPhysics::Region *region = NavPhysics::g_world.safe_get_region(p_region);
	ERR_FAIL_NULL_V(region, PoolVector<Face3>());

	return region->region_get_faces();
}

np_handle NavPhysicsServer::mesh_create() {
	return NavPhysics::g_world.safe_mesh_create();
}

void NavPhysicsServer::mesh_free(np_handle p_mesh) {
	NavPhysics::g_world.safe_mesh_free(p_mesh);
}

void NavPhysicsServer::mesh_set_region(np_handle p_mesh, np_handle p_region) {
	uint32_t mesh_id = UINT32_MAX;
	NavPhysics::Mesh *mesh = NavPhysics::g_world.safe_get_mesh(p_mesh, &mesh_id);
	ERR_FAIL_NULL(mesh);

	uint32_t new_region_id = UINT32_MAX;
	NavPhysics::Region *new_region = nullptr;
	if (p_region) {
		new_region = NavPhysics::g_world.safe_get_region(p_region, &new_region_id);
	}

	uint32_t old_slot_id = UINT32_MAX;
	uint32_t old_region_id = mesh->get_region_id(old_slot_id);
	if (old_region_id == new_region_id) {
		// NOOP
		return;
	}

	// detach from old region
	if (old_region_id != UINT32_MAX) {
		NavPhysics::Region &old_region = NavPhysics::g_world.get_region(old_region_id);
		old_region.unregister_mesh(mesh->get_mesh_id(), old_slot_id);
		mesh->set_region_id(UINT32_MAX, UINT32_MAX);
	}

	// attach to new region
	if ((new_region_id != UINT32_MAX) && new_region) {
		uint32_t new_slot_id = new_region->register_mesh(mesh_id);
		mesh->set_region_id(new_region_id, new_slot_id);
	}
}

void NavPhysicsServer::mesh_set_map(np_handle p_mesh, np_handle p_map) {
	uint32_t mesh_id = UINT32_MAX;
	NavPhysics::Mesh *mesh = NavPhysics::g_world.safe_get_mesh(p_mesh, &mesh_id);
	ERR_FAIL_NULL(mesh);

	uint32_t new_map_id = UINT32_MAX;
	NavPhysics::Map *new_map = nullptr;
	if (p_map) {
		new_map = NavPhysics::g_world.safe_get_map(p_map, &new_map_id);
	}

	uint32_t old_slot_id = UINT32_MAX;
	uint32_t old_map_id = mesh->get_map_id(old_slot_id);
	if (old_map_id == new_map_id) {
		// NOOP
		return;
	}

	// detach from old map
	if (old_map_id != UINT32_MAX) {
		NavPhysics::Map &old_map = NavPhysics::g_world.get_map(old_map_id);
		old_map.unregister_mesh(mesh->get_mesh_id(), old_slot_id);
		mesh->set_map_id(UINT32_MAX, UINT32_MAX);
	}

	// attach to new map
	if ((new_map_id != UINT32_MAX) && new_map) {
		uint32_t new_slot_id = new_map->register_mesh(mesh_id);
		mesh->set_map_id(new_map_id, new_slot_id);
	}
}

bool NavPhysicsServer::mesh_load(np_handle p_mesh, const Ref<NavigationMesh> &p_navmesh) {
	NavPhysics::Mesh *mesh = NavPhysics::g_world.safe_get_mesh(p_mesh);
	ERR_FAIL_NULL_V(mesh, false);

	NavPhysics::Loader loader;
	loader.load_mesh(p_navmesh, *mesh);

	return true;
}

// Currently this is now done indirectly through regions, to allow multiple maps per region.
// This function can be re-instated if necessary - the navigation design is currently subject to  change.

// void NavPhysicsServer::mesh_set_transform(np_handle p_mesh, const Transform &p_xform) {
// NavPhysics::Mesh *mesh = NavPhysics::g_world.safe_get_mesh(p_mesh);
// ERR_FAIL_NULL(mesh);
// mesh->set_transform(p_xform);
// }

np_handle NavPhysicsServer::body_create() {
	return NavPhysics::g_world.safe_body_create();
}

void NavPhysicsServer::body_free(np_handle p_body) {
	NavPhysics::g_world.safe_body_free(p_body);
}

void NavPhysicsServer::body_set_map(np_handle p_body, np_handle p_map) {
	uint32_t id = UINT32_MAX;
	NavPhysics::Agent *agent = NavPhysics::g_world.safe_get_body(p_body, &id);
	ERR_FAIL_NULL(agent);

	if (agent->map == p_map) {
		// NOOP
		return;
	}

	if (agent->map) {
		NavPhysics::Map *map = NavPhysics::g_world.safe_get_map(agent->map);
		if (map) {
			map->unregister_body(id);
		}
	}

	agent->map = p_map;

	if (agent->map) {
		NavPhysics::Map *map = NavPhysics::g_world.safe_get_map(agent->map);
		if (map) {
			map->register_body(id);
		}
	}
}

void NavPhysicsServer::body_set_enabled(np_handle p_body, bool p_enabled) {
}

const NavPhysics::TraceResult &NavPhysicsServer::body_dual_trace(np_handle p_body, const Vector3 &p_intermediate_destination, const Vector3 &p_final_destination) {
	_trace_result.blank();
	_trace_result.mesh.first_trace_hit = false;
	_trace_result.mesh.hit_point = p_final_destination;

	NavPhysics::Agent *agent = NavPhysics::g_world.safe_get_body(p_body);
	ERR_FAIL_NULL_V(agent, _trace_result);

	const NavPhysics::Mesh &mesh = NavPhysics::g_world.get_mesh(agent->get_mesh_id());
	mesh.body_dual_trace(*agent, p_intermediate_destination, _trace_result);

	return _trace_result;
}

Vector3 NavPhysicsServer::body_choose_random_location(np_handle p_body) const {
	const NavPhysics::Agent *agent = NavPhysics::g_world.safe_get_body(p_body);
	ERR_FAIL_NULL_V(agent, Vector3());
	ERR_FAIL_COND_V(agent->get_mesh_id() == UINT32_MAX, Vector3());

	const NavPhysics::Mesh &mesh = NavPhysics::g_world.get_mesh(agent->get_mesh_id());
	return mesh.choose_random_location();
}

const NavPhysics::TraceResult &NavPhysicsServer::body_trace(np_handle p_body, const Vector3 &p_destination, bool p_trace_navmesh, bool p_trace_obstacles) {
	_trace_result.blank();
	_trace_result.mesh.hit_point = p_destination;

	const NavPhysics::Agent *agent = NavPhysics::g_world.safe_get_body(p_body);
	ERR_FAIL_NULL_V(agent, _trace_result);

	if (p_trace_navmesh) {
		const NavPhysics::Mesh &mesh = NavPhysics::g_world.get_mesh(agent->get_mesh_id());
		mesh.body_trace(*agent, _trace_result);
	}

	if (p_trace_obstacles) {
		// NYI
	}

	return _trace_result;
}

bool NavPhysicsServer::body_get_info(np_handle p_body, NavPhysics::BodyInfo &r_body_info) {
	r_body_info.blank();

	NavPhysics::Agent *agent = NavPhysics::g_world.safe_get_body(p_body);
	ERR_FAIL_NULL_V(agent, false);

	const NavPhysics::Mesh &mesh = NavPhysics::g_world.get_mesh(agent->get_mesh_id());
	mesh.agent_get_info(*agent, r_body_info);

	return true;
}

void NavPhysicsServer::body_teleport(np_handle p_body, const Vector3 &p_position) {
	NavPhysics::Agent *agent = NavPhysics::g_world.safe_get_body(p_body);
	ERR_FAIL_NULL(agent);
	agent->fpos3_teleport = p_position;
	agent->set_mesh_id(UINT32_MAX);
}

void NavPhysicsServer::body_add_impulse(np_handle p_body, const Vector3 &p_impulse) {
	NavPhysics::Agent *agent = NavPhysics::g_world.safe_get_body(p_body);
	ERR_FAIL_NULL(agent);
	agent->fvel3 += p_impulse;
}

void NavPhysicsServer::body_set_params(np_handle p_body, real_t p_friction, bool p_ignore_narrowings) {
	NavPhysics::Agent *agent = NavPhysics::g_world.safe_get_body(p_body);
	ERR_FAIL_NULL(agent);
	agent->friction = CLAMP(p_friction, 0.0f, 1.0f);
	agent->ignore_narrowings = p_ignore_narrowings;
}

void NavPhysicsServer::body_set_callback(np_handle p_body, Object *p_receiver) {
	NavPhysics::Agent *agent = NavPhysics::g_world.safe_get_body(p_body);
	ERR_FAIL_NULL(agent);
	agent->callback.receiver = p_receiver;
}

//const Vector3 &NavPhysicsServer::body_iterate(np_handle p_body) {
//	static Vector3 fail_vec;

//	uint32_t agent_id = 0;
//	NavPhysics::Agent *agent = NavPhysics::g_world.safe_get_body(p_body, &agent_id);
//	ERR_FAIL_NULL_V(agent, fail_vec);

//	NavPhysics::Map *map = NavPhysics::g_world.safe_get_map(agent->map);
//	ERR_FAIL_NULL_V(map, fail_vec);

//	map->iterate_agent(agent_id);
//	return agent->fpos3;
//}
