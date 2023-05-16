#include "np_map.h"
#include "core/engine.h"
#include "np_loader.h"
#include "np_mesh.h"
#include "np_region.h"

namespace NavPhysics {

World g_world;

NavPhysics::Map::~Map() {
	clear();
}

uint32_t NavPhysics::Map::register_mesh(uint32_t p_mesh_id) {
	uint32_t slot_id;
	uint32_t *pid = _meshes.request(slot_id);
	*pid = p_mesh_id;
	return slot_id;
}

void NavPhysics::Map::unregister_mesh(uint32_t p_mesh_id, uint32_t p_mesh_slot_id) {
	ERR_FAIL_COND(_meshes[p_mesh_slot_id] != p_mesh_id);
	_meshes.free(p_mesh_slot_id);
}

void NavPhysics::Map::register_body(uint32_t p_body_id) {
	_sap.add_item(p_body_id);
}

void NavPhysics::Map::unregister_body(uint32_t p_body_id) {
	_sap.remove_item(p_body_id);
}

void NavPhysics::Map::tick_update(real_t p_delta) {
	_sap.update();

	for (uint32_t n = 0; n < _sap.get_num_intersections(); n++) {
		const NavPhysics::SAP::Intersection &i = _sap.get_intersection(n);

		Agent &agent_a = g_world.get_body(i.agent_id_a);
		Agent &agent_b = g_world.get_body(i.agent_id_b);

		Vector3 offset = agent_b.fpos3 - agent_a.fpos3;
		real_t prox = offset.length();
		real_t radii = agent_a.radius + agent_b.radius;

		// User could have chosen zero radius for both
		if (radii <= 0) {
			continue;
		}

		// SAP should guarantee this
		ERR_CONTINUE(prox > radii);

		real_t overlap_fraction = 1.0 - (prox / radii);

		// scale the push apart force
		overlap_fraction *= 0.3;

		if (prox > 0) {
			offset *= overlap_fraction / prox;
		} else {
			// choose random vector to push apart
			offset = Vector3(overlap_fraction, 0, 0);
		}

		if (agent_a.priority > agent_b.priority) {
			agent_a.avoidance_fvel3 -= offset * 2;
			agent_b.avoidance_fvel3 += offset;
			// agent_a.fvel3 -= offset * 2;
			// agent_b.fvel3 += offset;
		} else {
			agent_a.avoidance_fvel3 -= offset;
			agent_b.avoidance_fvel3 += offset * 2;
		}

		agent_a.state = AGENT_STATE_PENDING_COLLIDING;
		agent_b.state = AGENT_STATE_PENDING_COLLIDING;
	}
}

/*
void NavPhysics::Map::unload_mesh_callback(uint32_t p_mesh_id) {
	// remove this mesh from any agents
	for (uint32_t n = 0; n < _agents.active_size(); n++) {
		Agent &agent = _agents.get_active(n);
		if (agent.get_mesh_id() == p_mesh_id) {
			agent.set_mesh_id(UINT32_MAX);
		}
	}
}
*/

bool NavPhysics::Map::update_agent_mesh(Agent &r_agent, uint32_t p_agent_id) {
	//Agent &agent = _agents[p_agent_id];

	// no mesh yet?
	if (r_agent.get_mesh_id() == UINT32_MAX) {
		// find one (SLOW)

		real_t best_fit = FLT_MAX;
		uint32_t best_mesh_id = UINT32_MAX;

		if (!_meshes.active_size()) {
			print_line("teleport before meshs loaded");
		}

		for (uint32_t n = 0; n < _meshes.active_size(); n++) {
			uint32_t mesh_id = _meshes.get_active(n);

			Mesh &mesh = g_world.get_mesh(mesh_id);

			//if (mesh) {
			real_t fit = mesh.find_agent_fit(r_agent);
			if (fit < best_fit) {
				best_fit = fit;
				best_mesh_id = mesh_id;
			}
			//}
		}

		if (best_mesh_id == UINT32_MAX) {
			// Can't iterate, no decent meshs for this agent.
			// Could ERR here, but it may happen in normal circumstances sometimes
			// and not be an error.
			return false;
		}
		print_line("Agent " + itos(p_agent_id) + " is on mesh " + itos(best_mesh_id) + ".");
		r_agent.set_mesh_id(best_mesh_id);

		// teleport
		body_teleport(r_agent, p_agent_id, r_agent.fpos3_teleport);
		//g_world.body_teleport(p_agent_id, r_agent.fpos3_teleport);
		//navphysics_teleport(p_agent_id, agent.fpos3_teleport);
		//		Mesh *mesh = _meshes[agent.mesh_id];
		//		Vector3 pos_local = mesh->get_transform().xform(agent.fpos3);
		//		agent.fpos = Vector2(pos_local.x, pos_local.z);
		//		mesh->teleport_agent(agent);
	}
	return true;
}

void NavPhysics::Map::iterate_agent(uint32_t p_agent_id) {
	Agent &agent = g_world.get_body(p_agent_id);

	// Initialize the agent state each tick. This may already have been set to colliding by the agent - agent collision detection,
	// which happens before iterate_agent().
	agent.state = (agent.state != AGENT_STATE_PENDING_COLLIDING) ? AGENT_STATE_CLEAR : AGENT_STATE_COLLIDING;

	if (!update_agent_mesh(agent, p_agent_id)) {
		return;
	}

	DEV_ASSERT(agent.get_mesh_id() != UINT32_MAX);
	Mesh &mesh = g_world.get_mesh(agent.get_mesh_id());

	// We will do all the transforming necessary to enter and exit mesh space here
	// in the map, to keep the mesh code as clean and simple as possible.
	if (false) {
		//	if (mesh->is_transform_identity()) {
		// If using identity transform, we can save some calculations
		agent.fpos = Vector2(agent.fpos3.x, agent.fpos3.z);
		agent.fvel = Vector2(agent.fvel3.x, agent.fvel3.z);
		mesh.iterate_agent(agent);
		agent.fpos3 = Vector3(agent.fpos.x, agent.height, agent.fpos.y);
	} else {
		//		agent.fpos3 = Vector3(agent.fpos.x, agent.height, agent.fpos.y);
		//		agent.fpos3 = mesh->get_transform_inverse().xform(agent.fpos3);
		//	Vector3 pos_before = agent.fpos3;

		//agent.fpos3 = mesh->get_transform().xform(agent.fpos3);

		// apply avoidance
		agent.fvel3 += agent.avoidance_fvel3;
		agent.fvel3 = mesh.get_transform_inverse().xform(agent.fvel3);
		//agent.fvel3 = mesh->get_transform().xform(agent.fvel3);
		//agent.fpos = Vector2(agent.fpos3.x, agent.fpos3.z);
		agent.fvel = Vector2(agent.fvel3.x, agent.fvel3.z);
		mesh.iterate_agent(agent);
		agent.fpos3 = Vector3(agent.fpos.x, agent.height, agent.fpos.y);

		//		if (Engine::get_singleton()->get_physics_frames() %2)
		//		{
		//		agent.fpos3 = mesh->get_transform_inverse().xform(agent.fpos3);
		//		}
		//		else
		//		{
		agent.fpos3 = mesh.get_transform().xform(agent.fpos3);
		//		}

		//agent.fvel3 = agent.fpos3 - pos_before;
		agent.fvel3 = Vector3();
	}
}

//uint32_t NavPhysics::Map::add_agent() {
//	uint32_t id = UINT32_MAX;
//	Agent *agent = _agents.request(id);
//	agent->blank();
//	return id;
//}
//bool NavPhysics::Map::remove_agent(uint32_t p_agent_id) {
//	_agents.free(p_agent_id);
//	return true;
//}

void NavPhysics::Map::clear() {
	//	for (uint32_t n = 0; n < _meshes.active_size(); n++) {
	//		Mesh *mesh = _meshes.get_active(n);
	//		if (mesh) {
	//			memdelete(mesh);
	//			_meshes.get_active(n) = nullptr;
	//		}
	//	}

	_meshes.clear();
}

//void NavPhysics::Map::navphysics_set_params(uint32_t p_agent_id, real_t p_friction) {
//	Agent &agent = _agents[p_agent_id];
//	agent.friction = CLAMP(p_friction, 0.0f, 1.0f);
//}

//void NavPhysics::Map::navphysics_add_impulse(uint32_t p_agent_id, const Vector3 &p_impulse) {
//	Agent &agent = _agents[p_agent_id];
//	agent.fvel3 += p_impulse;
//}

void NavPhysics::Map::body_teleport(Agent &r_agent, uint32_t p_agent_id, const Vector3 &p_pos) {
	//Agent &agent = _agents[p_agent_id];
	r_agent.fpos3_teleport = p_pos;

	if (update_agent_mesh(r_agent, p_agent_id)) {
		Mesh &mesh = g_world.get_mesh(r_agent.get_mesh_id());

		// transform
		Vector3 pos_local = mesh.get_transform().xform(r_agent.fpos3_teleport);
		r_agent.fpos = Vector2(pos_local.x, pos_local.z);
		r_agent.fpos3 = r_agent.fpos3_teleport;

		//ERR_FAIL_NULL(mesh);
		mesh.teleport_agent(r_agent);
	} else {
		ERR_FAIL_MSG("Agent mesh not found.");
	}
}

//void NavPhysics::Map::body_teleport(Agent &r_agent, uint32_t p_agent_id, const Vector3 &p_pos) {
//	//Agent &agent = _agents[p_agent_id];
//	r_agent.fpos3_teleport = p_pos;

//	if (update_agent_mesh(r_agent, p_agent_id)) {
//		Mesh *mesh = _meshes[agent.get_mesh_id()];

//		// transform
//		Vector3 pos_local = mesh->get_transform().xform(agent.fpos3_teleport);
//		agent.fpos = Vector2(pos_local.x, pos_local.z);
//		agent.fpos3 = agent.fpos3_teleport;

//		ERR_FAIL_NULL(mesh);
//		mesh->teleport_agent(agent);
//	} else {
//		ERR_FAIL_MSG("Agent mesh not found.");
//	}
//}

//void NavPhysics::Map::set_mesh_transform(uint32_t p_id, const Transform &p_xform) {
//	Mesh *mesh = get_mesh(p_id);
//	if (mesh) {
//		mesh->set_transform(p_xform);
//	}
//}

//Mesh *NavPhysics::Map::get_mesh(uint32_t p_id) {
//	ERR_FAIL_COND_V(p_id >= _meshes.pool_reserved_size(), nullptr);
//	return _meshes[p_id];
//}

//bool NavPhysics::Map::load(const NavMap &p_navmap) {
//	Loader loader;
//	return loader.load(p_navmap, *this);
//}

//////////////////////////////////////////////////////////////////////////

NavPhysics::Agent *World::safe_get_body(np_handle p_body, uint32_t *r_id) {
	ERR_FAIL_COND_V(!p_body, nullptr);
	uint32_t revision;
	uint32_t id = handle_to_id(p_body, revision);
	if (r_id) {
		*r_id = id;
	}
	NavPhysics::Agent &agent = _agents[id];
	ERR_FAIL_COND_V(agent.revision != revision, nullptr);
	return &agent;
}

NavPhysics::Mesh *World::safe_get_mesh(np_handle p_mesh, uint32_t *r_id) {
	ERR_FAIL_COND_V(!p_mesh, nullptr);
	uint32_t revision;
	uint32_t id = handle_to_id(p_mesh, revision);
	if (r_id) {
		*r_id = id;
	}
	MeshContainer &mesh = _meshes[id];
	ERR_FAIL_COND_V(mesh.revision != revision, nullptr);
	return mesh.mesh;
}

NavPhysics::Region *World::safe_get_region(np_handle p_region, uint32_t *r_id) {
	ERR_FAIL_COND_V(!p_region, nullptr);
	uint32_t revision;
	uint32_t id = handle_to_id(p_region, revision);
	if (r_id) {
		*r_id = id;
	}
	RegionContainer &region = _regions[id];
	ERR_FAIL_COND_V(region.revision != revision, nullptr);
	return region.region;
}

NavPhysics::Map *World::safe_get_map(np_handle p_map, uint32_t *r_id) {
	ERR_FAIL_COND_V(!p_map, nullptr);
	uint32_t revision;
	uint32_t id = handle_to_id(p_map, revision);
	if (r_id) {
		*r_id = id;
	}
	MapContainer &map = _maps[id];
	ERR_FAIL_COND_V(map.revision != revision, nullptr);
	return map.map;
}

np_handle World::safe_body_create() {
	uint32_t id = UINT32_MAX;
	NavPhysics::Agent *agent = _agents.request(id);
	if (agent) {
		agent->blank();

#ifdef DEV_ENABLED
		agent->agent_id = id;
#endif
		// kind of semi random
		agent->priority = id;

		if (!agent->revision) {
			// special case, zero is reserved
			agent->revision = 1;
		}
		return id_to_handle(id, agent->revision);
	}
	return 0;
}

np_handle World::safe_mesh_create() {
	uint32_t id = UINT32_MAX;
	MeshContainer *mesh = _meshes.request(id);
	if (mesh) {
		DEV_CHECK(!mesh->mesh);
		mesh->mesh = memnew(Mesh);
		if (!mesh->revision) {
			// special case, zero is reserved
			mesh->revision = 1;
		}
		return id_to_handle(id, mesh->revision);
	}
	return 0;
}

np_handle World::safe_region_create() {
	uint32_t id = UINT32_MAX;
	RegionContainer *region = _regions.request(id);
	if (region) {
		DEV_CHECK(!region->region);
		region->region = memnew(Region);
		if (!region->revision) {
			// special case, zero is reserved
			region->revision = 1;
		}
		return id_to_handle(id, region->revision);
	}
	return 0;
}

np_handle World::safe_map_create() {
	uint32_t id = UINT32_MAX;
	MapContainer *map = _maps.request(id);
	if (map) {
		DEV_CHECK(!map->map);
		map->map = memnew(Map);
		if (!map->revision) {
			// special case, zero is reserved
			map->revision = 1;
		}
		return id_to_handle(id, map->revision);
	}
	return 0;
}

void World::safe_body_free(np_handle p_body) {
	ERR_FAIL_COND(!p_body);
	uint32_t revision;
	uint32_t id = handle_to_id(p_body, revision);
	NavPhysics::Agent &agent = _agents[id];
	ERR_FAIL_COND(agent.revision != revision);
	wrapped_increment_revision(agent.revision);
	_agents.free(id);
}

void World::safe_mesh_free(np_handle p_mesh) {
	ERR_FAIL_COND(!p_mesh);
	uint32_t revision;
	uint32_t id = handle_to_id(p_mesh, revision);
	MeshContainer &mesh = _meshes[id];
	ERR_FAIL_COND(mesh.revision != revision);
	wrapped_increment_revision(mesh.revision);
	if (mesh.mesh) {
		memfree(mesh.mesh);
		mesh.mesh = nullptr;
	}
	_meshes.free(id);
}

void World::safe_region_free(np_handle p_region) {
	ERR_FAIL_COND(!p_region);
	uint32_t revision;
	uint32_t id = handle_to_id(p_region, revision);
	RegionContainer &region = _regions[id];
	ERR_FAIL_COND(region.revision != revision);
	wrapped_increment_revision(region.revision);
	if (region.region) {
		memfree(region.region);
		region.region = nullptr;
	}
	_regions.free(id);
}

void World::safe_map_free(np_handle p_map) {
	ERR_FAIL_COND(!p_map);
	uint32_t revision;
	uint32_t id = handle_to_id(p_map, revision);
	MapContainer &map = _maps[id];
	ERR_FAIL_COND(map.revision != revision);
	wrapped_increment_revision(map.revision);
	if (map.map) {
		memfree(map.map);
		map.map = nullptr;
	}
	_maps.free(id);
}

void World::tick_update(real_t p_delta) {
	// do agent-agent bouncing
	for (uint32_t n = 0; n < _maps.active_size(); n++) {
		MapContainer &map = _maps.get_active(n);
		if (map.map) {
			map.map->tick_update(p_delta);
		}
	}

	Variant::CallError responseCallError;
	Variant returned_position;
	Variant returned_state;
	Variant returned_avoidance;

	StringName callback_func_name = "_navphysics_done";

	for (uint32_t n = 0; n < _agents.active_size(); n++) {
		uint32_t agent_id = _agents.get_active_id(n);
		Agent &agent = _agents[agent_id];

		if (!agent.map) {
			continue;
		}

		NavPhysics::Map *map = NavPhysics::g_world.safe_get_map(agent.map);
		ERR_CONTINUE(!map);

		map->iterate_agent(agent_id);

		// result is stored in agent.fpos3
		if (agent.callback.receiver) {
			int argc = 3;
			returned_position = agent.fpos3;
			returned_avoidance = agent.avoidance_fvel3;
			returned_state = agent.state;

			const Variant *vp[3] = { &returned_position, &returned_avoidance, &returned_state };

			// This will crash if the client does not keep the callback object up to date.
			// This is a sacrifice for call speed versus using e.g. ObjectID lookups.
			agent.callback.receiver->call(callback_func_name, vp, argc, responseCallError);

			// blank for next time
			agent.avoidance_fvel3 = Vector3();
		}
	}
}

void World::clear() {
	_agents.clear();

	for (uint32_t n = 0; n < _meshes.active_size(); n++) {
		MeshContainer &mesh = _meshes.get_active(n);
		if (mesh.mesh) {
			memdelete(mesh.mesh);
			mesh.mesh = nullptr;
		}
	}
	_meshes.clear();

	for (uint32_t n = 0; n < _maps.active_size(); n++) {
		MapContainer &map = _maps.get_active(n);
		if (map.map) {
			memdelete(map.map);
			map.map = nullptr;
		}
	}
	_maps.clear();
}

World::World() {
}

World::~World() {
	clear();
}

} //namespace NavPhysics
