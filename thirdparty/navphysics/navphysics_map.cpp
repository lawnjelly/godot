// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#include "navphysics_map.h"

#include "navphysics_mesh.h"
#include "navphysics_mesh_funcs.h"
#include "navphysics_mesh_instance.h"
#include "navphysics_region.h"

namespace NavPhysics {

World g_world;

np_agent_callback World::agent_callback = nullptr;

Map::~Map() {
	clear();
}

u32 Map::register_mesh_instance(u32 p_mesh_instance_id) {
	u32 slot_id;
	u32 *pid = _mesh_instances.request(slot_id);
	*pid = p_mesh_instance_id;
	return slot_id;
}

void Map::unregister_mesh_instance(u32 p_mesh_instance_id, u32 p_mesh_slot_id) {
	NP_ERR_FAIL_COND(_mesh_instances[p_mesh_slot_id] != p_mesh_instance_id);
	_mesh_instances.free(p_mesh_slot_id);
}

void Map::register_body(u32 p_body_id) {
	_sap.add_item(p_body_id);
}

void Map::unregister_body(u32 p_body_id) {
	_sap.remove_item(p_body_id);
}

bool Map::calculate_vertical_overlap_push(Agent &p_agent_a, Agent &p_agent_b) const {
	// Height overlap.
	freal a_bottom = p_agent_a.fpos3.y;
	freal b_bottom = p_agent_b.fpos3.y;

	Agent *lower = nullptr;
	Agent *upper = nullptr;

	if (a_bottom >= b_bottom) {
		// A higher than B
		lower = &p_agent_b;
		upper = &p_agent_a;
	} else {
		// B higher than A
		lower = &p_agent_a;
		upper = &p_agent_b;
	}

	freal bottom = upper->fpos3.y;
	freal top = lower->fpos3.y + lower->height;
	freal overlap = top - bottom;

	if (overlap <= 0) {
		// No collision on the y axis.
		// Still mark as on an agent if the upper is within a certain small threshold,
		// to prevent vibration float error preventing jumps off agents.
		if (overlap >= -0.1f) {
			upper->set_on_agent();
		}

		return false;
	}

	freal radius = lower->radius;
	NP_DEV_ASSERT(radius > 0);

	if (overlap < radius) {
		upper->set_on_agent();
#if 0
		freal push = overlap / radius;
		push *= 0.01f;
		push = 0;
		upper->agent_height = lower->agent_height + lower->height;
#endif

		if (lower->is_on_floor()) {
			upper->jump_velocity = MAX(lower->jump_velocity, upper->jump_velocity);
		} else {
			// Average the jump velocities.
			// This isn't really any hard rules, just something that looks good.
			freal average_jump_vel = (lower->jump_velocity + upper->jump_velocity) * 0.5f;

			upper->jump_velocity = MAX(average_jump_vel, upper->jump_velocity);
			lower->jump_velocity = average_jump_vel;
		}
	}

	return true;
}

void Map::tick_update(freal p_delta) {
	_sap.update();

	for (u32 n = 0; n < _sap.get_num_intersections(); n++) {
		const NavPhysics::SAP::Intersection &i = _sap.get_intersection(n);

		Agent &agent_a = g_world.get_body(i.agent_id_a);
		Agent &agent_b = g_world.get_body(i.agent_id_b);

		if (!calculate_vertical_overlap_push(agent_a, agent_b)) {
			// No collision on the y axis.
			continue;
		}

		// Debugging, stop moving on agents.
		if (agent_a.on_agent || agent_b.on_agent) {
			continue;
		}

		freal radii = agent_a.radius + agent_b.radius;

		// Should be already checked in the SAP code.
		NP_DEV_ASSERT(radii > 0);

		/////////////////////////////////////////////
		// 2D check first.
		FPoint2 offset = agent_b.fpos3.xz() - agent_a.fpos3.xz();
		freal prox = Math::sqrt_real(i.dist_squared);

		// SAP should guarantee this
#ifdef NP_DEV_ENABLED
		NP_ERR_CONTINUE(prox > radii);
#endif

		// Scaled 0 with max overlap, 1 with no overlap.
		freal overlap_fraction = (prox / radii);

		// Apply some damping.
		overlap_fraction *= overlap_fraction;

		overlap_fraction = 1 - overlap_fraction;

		// scale the push apart force
		overlap_fraction *= 0.01f; // 0.3

		if (prox > 0.001f) {
			// Normalize offset and scale by overlap_fraction.
			offset *= overlap_fraction / prox;
		} else {
			// choose random vector to push apart
			offset = FPoint2::make(overlap_fraction, 0);
		}

		FPoint3 push = FPoint3::make(offset.x, 0, offset.y);

		if (agent_a.priority > agent_b.priority) {
			agent_a.avoidance_fvel3 -= push * 2;
			agent_b.avoidance_fvel3 += push;
		} else {
			agent_a.avoidance_fvel3 -= push;
			agent_b.avoidance_fvel3 += push * 2;
		}

		agent_a.fvel3 *= 0.5f;
		agent_b.fvel3 *= 0.5f;

		agent_a.state = AGENT_STATE_PENDING_COLLIDING;
		agent_b.state = AGENT_STATE_PENDING_COLLIDING;
	}
}

/*
void NavPhysics::Map::unload_mesh_callback(u32 p_mesh_id) {
	// remove this mesh from any agents
	for (u32 n = 0; n < _agents.active_size(); n++) {
		Agent &agent = _agents.get_active(n);
		if (agent.get_mesh_id() == p_mesh_id) {
			agent.set_mesh_id(UINT32_MAX);
		}
	}
}
*/

u32 Map::find_best_fit_agent_mesh(Agent &r_agent, const FPoint3 &p_world_pos, u32 p_ignore_mesh_id, const JumpFinderData *p_jump_data) const {
	freal best_fit = FLT_MAX;
	u32 best_mesh_instance_id = UINT32_MAX;

	AgentStatus::suggested_poly_id = UINT32_MAX;

	// If we are exploring connections outside of meshes,
	// we only want to find a new mesh if it is "perfect",
	// i.e. we will land on it on a poly.
	// We don't want to find meshes that are a way away,
	// which we may want to find when teleporting.
	if (p_ignore_mesh_id != UINT32_MAX) {
		best_fit = 1000;
	}

	if (!_mesh_instances.active_size()) {
		log("teleport before mesh instances loaded");
	}

	freal max_step_up = 0;
	freal max_drop = 0;

	if (p_ignore_mesh_id != UINT32_MAX) {
		MeshInstance &mesh_instance = g_world.get_mesh_instance(p_ignore_mesh_id);
		max_step_up = mesh_instance.get_mesh().get_mesh_params().exit_max_step_up;
		max_drop = mesh_instance.get_mesh().get_mesh_params().exit_max_drop;
	}

	for (u32 n = 0; n < _mesh_instances.active_size(); n++) {
		u32 mi_id = _mesh_instances.get_active(n);

		if (mi_id == p_ignore_mesh_id) {
			continue;
		}

		MeshInstance &mesh_instance = g_world.get_mesh_instance(mi_id);
		if (!mesh_instance.is_active()) {
			continue;
		}

		if (p_ignore_mesh_id == UINT32_MAX) {
			max_step_up = mesh_instance.get_mesh().get_mesh_params().exit_max_step_up;
			max_drop = mesh_instance.get_mesh().get_mesh_params().exit_max_drop;
		}

		u32 poly_id = UINT32_MAX;
		freal fit = 0;

		if (p_jump_data) {
			fit = mesh_instance.find_agent_jump_fit(r_agent, *p_jump_data, poly_id, max_step_up, max_drop, best_fit);
		} else {
			fit = mesh_instance.find_agent_fit(r_agent, p_world_pos, poly_id, max_step_up, max_drop);
		}

		if (fit < best_fit) {
			best_fit = fit;
			best_mesh_instance_id = mi_id;
			//log(String("NavPhysics::Map::find_best_fit_agent_mesh best MeshInstance ID ") + n);

			AgentStatus::jump_mesh_instance_id = best_mesh_instance_id;
			AgentStatus::suggested_poly_id = poly_id;
		}
	}

	//	best_mesh_instance_id = 1;
	NP_LOG(String("find_best_fit_agent_mesh : ") + best_mesh_instance_id);
	return best_mesh_instance_id;
}

bool Map::update_agent_mesh(Agent &r_agent, bool p_teleport_if_changed) {
	//Agent &agent = _agents[p_agent_id];

	// no mesh yet?
	if (r_agent.get_mesh_instance_id() == UINT32_MAX) {
		// find one (SLOW)

		u32 best_mesh_instance_id = find_best_fit_agent_mesh(r_agent, r_agent.fpos3, UINT32_MAX);

		if (best_mesh_instance_id == UINT32_MAX) {
			// Can't iterate, no decent meshs for this agent.
			// Could ERR here, but it may happen in normal circumstances sometimes
			// and not be an error.
			return false;
		}
		NP_LOG(String("Agent ") + String(r_agent.agent_id) + " is on mesh " + String(best_mesh_instance_id) + ".");
		r_agent.set_mesh_instance_id(best_mesh_instance_id);

		// teleport
		if (p_teleport_if_changed) {
			body_teleport(r_agent, r_agent.agent_id, r_agent.fpos3_teleport);
			//g_world.body_teleport(p_agent_id, r_agent.fpos3_teleport);
			//navphysics_teleport(p_agent_id, agent.fpos3_teleport);
			//		Mesh *mesh = _meshes[agent.mesh_id];
			//		FPoint3 pos_local = mesh->get_transform().xform(agent.fpos3);
			//		agent.fpos = FPoint2(pos_local.x, pos_local.z);
			//		mesh->teleport_agent(agent);
		}
	}
	return true;
}

bool Map::iterate_agent(u32 p_agent_id, IterateResult &r_result) {
	AgentStatus::reset();
	Agent &agent = g_world.get_body(p_agent_id);
	NP_DEV_ASSERT(agent.agent_id == p_agent_id);

	// Initialize the agent state each tick. This may already have been set to colliding by the agent - agent collision detection,
	// which happens before iterate_agent().
	agent.state = (agent.state != AGENT_STATE_PENDING_COLLIDING) ? AGENT_STATE_CLEAR : AGENT_STATE_COLLIDING;

	if (!update_agent_mesh(agent, true)) {
		return false;
	}

	NP_DEV_ASSERT(agent.get_mesh_instance_id() != UINT32_MAX);
	MeshInstance &mesh_instance = g_world.get_mesh_instance(agent.get_mesh_instance_id());

	// We will do all the transforming necessary to enter and exit mesh space here
	// in the map, to keep the mesh code as clean and simple as possible.
	if (false) {
#if 0
		//	if (mesh->is_transform_identity()) {
		// If using identity transform, we can save some calculations
		agent.fpos = FPoint2::make(agent.fpos3.x, agent.fpos3.z);
		agent.fvel = FPoint2::make(agent.fvel3.x, agent.fvel3.z);
		//mesh_instance.iterate_agent(agent);
		agent.fpos3 = FPoint3::make(agent.fpos.x, agent.floor_height, agent.fpos.y);
#endif
	} else {
		//		agent.fpos3 = FPoint3(agent.fpos.x, agent.height, agent.fpos.y);
		//		agent.fpos3 = mesh->get_transform_inverse().xform(agent.fpos3);
		//	FPoint3 pos_before = agent.fpos3;

		//agent.fpos3 = mesh->get_transform().xform(agent.fpos3);

		// An agent may exit a mesh instance on a single iteration,
		// so we need to be able to change mesh instance and fulfill
		// the remainder of an existing move.
		Mesh::MoveInfo move_info;
		move_info.map_id = _map_id;
		move_info.agent_id = agent.agent_id;
		move_info.agent = &agent;

		{
#if 1
			// Cap avoidance strength.
			f32 avoid_sl = agent.avoidance_fvel3.length_squared();
			if (avoid_sl > 0.00001f) {
				f32 l = Math::sqrt32(avoid_sl);
				const f32 max_avoid = 0.1f;
				l = MIN(l, max_avoid);

				FPoint3 avel = agent.avoidance_fvel3.normalized() * l;

				// Apply avoidance.
				agent.fvel3 += avel;
			}
#else
			agent.fvel3 += agent.avoidance_fvel3;
#endif
		}

		// Get the velocity into local navmesh space
		agent.fvel3 = mesh_instance.get_transform_inverse().basis.xform(agent.fvel3);
		mesh_instance.iterate_agent(agent, move_info);

		if (!AgentStatus::changing_mesh()) {
			//			NP_DEV_ASSERT(agent.fpos3.isfinite());
			//			agent.fpos3 = mesh_instance.get_transform().xform(agent.fpos3);
			//			NP_DEV_ASSERT(agent.fpos3.isfinite());
			mesh_instance.refresh_world_space_agent_position(agent, false);
		}
		// Changing mesh instance?
		else {
			NP_DEV_ASSERT(agent.get_mesh_instance_id() != UINT32_MAX);

			// Get the velocity into world space from the old mesh coordinate space.
			// As it needs to be reapplied in the new mesh local space
			// so the two coordinate spaces of the previous and new mesh match.
			//agent.fvel3 = mesh_instance.get_transform().basis.xform(FPoint3(agent.fvel));

			NP_LOG(String("Changing mesh instance to ") + move_info.new_mesh_instance_id + ", new world space vel: " + agent.fvel3 + ", new world space pos: " + agent.fpos3);

			body_teleport_to_agent_status_jump_target(agent, p_agent_id);

			MeshInstance &mesh_instance_new = g_world.get_mesh_instance(agent.get_mesh_instance_id());

			// This makes sure the agent height is correct.
			if (AgentStatus::is_in_jump_link()) {
				agent.force_off_floor();
				//mesh_instance_new._iterate_agent_on_jump_link(agent, move_info, move_info.remaining_velocity);
			}

			mesh_instance_new.iterate_agent_housekeeping(agent);
			mesh_instance_new.refresh_world_space_agent_position(agent, true);
		}

		r_result.velocity = agent.fvel3;
		r_result.position = agent.fpos3;
		agent.fvel3.zero();

		NP_LOG(String("ITERATE final pos ") + agent.fpos3);
	}

	return true;
}

//u32 NavPhysics::Map::add_agent() {
//	u32 id = UINT32_MAX;
//	Agent *agent = _agents.request(id);
//	agent->blank();
//	return id;
//}
//bool NavPhysics::Map::remove_agent(u32 p_agent_id) {
//	_agents.free(p_agent_id);
//	return true;
//}

void NavPhysics::Map::clear() {
	//	for (u32 n = 0; n < _meshes.active_size(); n++) {
	//		Mesh *mesh = _meshes.get_active(n);
	//		if (mesh) {
	//			memdelete(mesh);
	//			_meshes.get_active(n) = nullptr;
	//		}
	//	}

	_mesh_instances.clear();
}

//void NavPhysics::Map::navphysics_set_params(u32 p_agent_id, freal p_friction) {
//	Agent &agent = _agents[p_agent_id];
//	agent.friction = CLAMP(p_friction, 0.0f, 1.0f);
//}

//void NavPhysics::Map::navphysics_add_impulse(u32 p_agent_id, const FPoint3 &p_impulse) {
//	Agent &agent = _agents[p_agent_id];
//	agent.fvel3 += p_impulse;
//}

void Map::body_teleport_to_agent_status_jump_target(Agent &r_agent, u32 p_agent_id) {
	//AgentStatus::debug_print("body_teleport_to_agent_status_jump_target");

	r_agent.set_mesh_instance_id(AgentStatus::jump_mesh_instance_id);

	r_agent.pos = AgentStatus::jump_target_cross_pos;
	NP_DEV_ASSERT(r_agent.pos != IPoint2(INT32_MAX, INT32_MAX));

	r_agent.agent_height = AgentStatus::jump_target_local_height;
	r_agent.vel = AgentStatus::jump_target_vel;
	NP_DEV_ASSERT(r_agent.vel != IPoint2(INT32_MAX, INT32_MAX));

	MeshInstance &meshi = g_world.get_mesh_instance(r_agent.get_mesh_instance_id());
	const Mesh &mesh = meshi.get_mesh();

	// Compare the result of the world space velocity.
	// If the mesh instance has rotated since the jump, we will use
	// the recalculated local velocity from the the world space.
	//	IPoint2 world_vel = meshi.world_space_velocity_to_local(mesh, AgentStatus::jump_target_world_space_vel);
	//	u64 difference = (world_vel - r_agent.vel).length_squared();
	//	if (difference > 0) {
	//		r_agent.vel = world_vel;
	//	}

	r_agent.poly_id = AgentStatus::jump_cross_only ? AgentStatus::suggested_poly_id : UINT32_MAX;

	// Make sure it starts in off ground state, so it can settle to a lower mesh,
	// rather than instantaneously dropping down when changing meshes.
	// Note this might cause some client logic confusion .. may need looking at.
	// NYI
	r_agent.on_floor = false;

	mesh.refresh_local_agent_position_from_fixed_point(r_agent);

	//meshi.teleport_agent(r_agent);
	//meshi._agent_enter_poly(r_agent, new_poly_id, true);

	// Transform the location back into world space.
	//r_agent.fpos3 = meshi.get_transform().xform(r_agent.fpos3);
}

void Map::body_teleport(Agent &r_agent, u32 p_agent_id, const FPoint3 &p_pos) {
	//Agent &agent = _agents[p_agent_id];
	r_agent.fpos3_teleport = p_pos;
	r_agent.fpos3 = p_pos;

	NP_LOG(String("teleporting to ") + p_pos + ", mesh id was: " + r_agent.get_mesh_instance_id());

	NP_DEV_ASSERT(r_agent.agent_id == p_agent_id);
	if (update_agent_mesh(r_agent, false)) {
		MeshInstance &meshi = g_world.get_mesh_instance(r_agent.get_mesh_instance_id());
		NP_LOG(String("\tmesh id is now: ") + r_agent.get_mesh_instance_id());

		// transform
		//FPoint3 pos_local = meshi.get_transform().xform(r_agent.fpos3_teleport);
		const Transform &tr = meshi.get_transform_inverse();
		NP_LOG(String("\tmeshi xform ") + tr);

		FPoint3 pos_local = tr.xform(r_agent.fpos3_teleport);

		NP_LOG(String("\tpos_local is ") + pos_local);

		r_agent.fpos = FPoint2::make(pos_local.x, pos_local.z);
		//r_agent.fpos3 = r_agent.fpos3_teleport;
		r_agent.fpos3 = pos_local;

		// Calculate the new agent height from world space to local space
		r_agent.agent_height = pos_local.y;

		// Make sure it starts in off ground state, so it can settle to a lower mesh,
		// rather than instantaneously dropping down when changing meshes.
		// Note this might cause some client logic confusion .. may need looking at.
		// NYI
		r_agent.on_floor = false;

		//NP_ERR_FAIL_NULL(mesh);
		meshi.teleport_agent(r_agent);

		// Transform the location back into world space.
		r_agent.fpos3 = meshi.get_transform().xform(r_agent.fpos3);
	} else {
		NP_ERR_FAIL_MSG("Agent mesh not found.");
	}
}

//////////////////////////////////////////////////////////////////////////

NavPhysics::Agent *World::safe_get_body(np_handle p_body, u32 *r_id) {
	NP_ERR_FAIL_COND_V(!p_body, nullptr);
	u32 revision;
	u32 id = _meshes.handle_to_id(p_body, revision);
	if (r_id) {
		*r_id = id;
	}
	NavPhysics::Agent &agent = _agents[id];
	NP_ERR_FAIL_COND_V(agent.revision != revision, nullptr);
	return &agent;
}

bool World::safe_link_body(np_handle p_body, np_handle p_map) {
	u32 agent_id;
	Agent *agent = safe_get_body(p_body, &agent_id);
	NP_ERR_FAIL_NULL_V(agent, false);

	Map *map = safe_get_map(p_map);
	map->register_body(agent_id);

	agent->map = p_map;
	return true;
}

bool World::safe_unlink_body(np_handle p_body, np_handle p_map) {
	u32 agent_id;
	Agent *agent = safe_get_body(p_body, &agent_id);
	NP_ERR_FAIL_NULL_V(agent, false);

	Map *map = safe_get_map(p_map);
	map->unregister_body(agent_id);

	agent->map = 0;
	return true;
}

void World::safe_set_mesh_instance_active(np_handle p_mesh_instance, bool p_active) {
	u32 mi_id;
	MeshInstance *mi = safe_get_mesh_instance(p_mesh_instance, &mi_id);
	NP_ERR_FAIL_NULL(mi);
	mi->set_active(p_active);
}

bool World::safe_link_mesh_instance(np_handle p_mesh_instance, np_handle p_map) {
	u32 mi_id;
	MeshInstance *mi = safe_get_mesh_instance(p_mesh_instance, &mi_id);
	NP_ERR_FAIL_NULL_V(mi, false);

	Map *map = safe_get_map(p_map);
	u32 slot = map->register_mesh_instance(mi_id);
	mi->link_map(slot);
	return true;
}

bool World::safe_unlink_mesh_instance(np_handle p_mesh_instance, np_handle p_map) {
	u32 mi_id;
	MeshInstance *mi = safe_get_mesh_instance(p_mesh_instance, &mi_id);
	NP_ERR_FAIL_NULL_V(mi, false);

	Map *map = safe_get_map(p_map);
	map->unregister_mesh_instance(mi_id, mi->get_map_slot());

	mi->link_map(UINT32_MAX);
	return true;
}

bool World::safe_link_mesh(np_handle p_mesh_instance, np_handle p_mesh) {
	MeshInstance *mi = safe_get_mesh_instance(p_mesh_instance);
	NP_ERR_FAIL_NULL_V(mi, false);

	if (!p_mesh) {
		mi->link_mesh(UINT32_MAX);
		return true;
	}

	u32 mesh_id;
	safe_get_mesh(p_mesh, &mesh_id);
	mi->link_mesh(mesh_id);

	return true;
}

np_handle World::get_mesh_instance_handle(u32 p_id) const {
	return _mesh_instances.pool[p_id].object->get_handle();
}

np_handle World::safe_body_create() {
	u32 id = UINT32_MAX;
	NavPhysics::Agent *agent = _agents.request(id);
	if (agent) {
		agent->blank();

		//#ifdef NP_DEV_ENABLED
		agent->agent_id = id;
		//#endif
		// kind of semi random
		agent->priority = id;

		if (!agent->revision) {
			// special case, zero is reserved
			agent->revision = 1;
		}
		return _meshes.id_to_handle(id, agent->revision);
	}
	return 0;
}

np_handle World::safe_region_create() {
	return _regions.safe_object_create();
}

void World::safe_region_free(np_handle p_region) {
	_regions.safe_object_free(p_region);
}

void World::safe_body_free(np_handle p_body) {
	NP_ERR_FAIL_COND(!p_body);
	u32 revision;
	u32 id = _meshes.handle_to_id(p_body, revision);
	NavPhysics::Agent &agent = _agents[id];
	NP_ERR_FAIL_COND(agent.revision != revision);
	_meshes.wrapped_increment_revision(agent.revision);
	_agents.free(id);
}

void World::set_timestep(freal p_delta) {
	Mesh::_timestep = p_delta;
	Mesh::_inverse_timestep = 1.0 / p_delta;

	// Rounded, approx.
	Mesh::_ticks_per_sec = (Mesh::_inverse_timestep + 0.5);
}

void World::set_agent_callback(np_agent_callback p_callback) {
	agent_callback = p_callback;
}

void World::tick_update(u64 p_tick, freal p_delta) {
	Mesh::_tick = p_tick;

	if (!agent_callback) {
		return;
	}

	// Update pathfinding.
	get_plan_store().iterate();

	// do agent-agent bouncing
	for (u32 n = 0; n < _maps.pool.active_size(); n++) {
		Container<Map> &map = _maps.pool.get_active(n);
		if (map.object) {
			map.object->tick_update(p_delta);
		}
	}

	//	Variant::CallError responseCallError;
	//	Variant returned_position;
	//	Variant returned_state;
	//	Variant returned_avoidance;

	//	StringName callback_func_name = "_navphysics_done";

	Map::IterateResult res;

	for (u32 n = 0; n < _agents.active_size(); n++) {
		u32 agent_id = _agents.get_active_id(n);
		Agent &agent = _agents[agent_id];

		if (!agent.map) {
			continue;
		}

		NavPhysics::Map *map = NavPhysics::g_world.safe_get_map(agent.map);
		NP_ERR_CONTINUE(!map);

		if (map->iterate_agent(agent_id, res)) {
			if (agent.callback.user_data) {
				agent_callback(agent.callback.user_data, res.position, res.velocity);
			}
		}

#if 0		
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

		}
#endif
		// blank for next time
		agent.avoidance_fvel3 = FPoint3();
	}
}

void World::clear() {
	_agents.clear();

	_mesh_instances.clear();
	_meshes.clear();
	_regions.clear();
	_maps.clear();
}

np_handle World::safe_get_agent_mesh_instance_handle(np_handle p_body) {
	Agent *agent = safe_get_body(p_body);
	NP_ERR_FAIL_NULL_V(agent, UINT32_MAX);

	uint32_t mesh_instance_id = agent->get_mesh_instance_id();
	if (mesh_instance_id == UINT32_MAX) {
		return UINT32_MAX;
	}

	return get_mesh_instance(mesh_instance_id).get_handle();
}

const NavPhysics::Transform &World::safe_get_agent_mesh_instance_transform(np_handle p_body) {
	static Transform dummy_xform;

	Agent *agent = safe_get_body(p_body);
	NP_ERR_FAIL_NULL_V(agent, dummy_xform);
	uint32_t mesh_instance_id = agent->get_mesh_instance_id();

	if (mesh_instance_id != UINT32_MAX) {
		const NavPhysics::MeshInstance &mi = get_mesh_instance(mesh_instance_id);
		return mi.get_transform();
	}

	// Doesn't yet have a mesh instance assigned.
	return dummy_xform;
}

bool World::safe_toggle_mesh_wall_connection(np_handle p_mesh, const FPoint3 &p_from, const FPoint3 &p_to, bool p_external_or_internal) {
	Mesh *mesh = safe_get_mesh(p_mesh);

	if (mesh) {
		MeshFuncs mf;
		mf.editor_toggle_wall_connection(*mesh, p_from, p_to, p_external_or_internal);
		return true;
	}

	return false;
}

World::World() {
	_default_map = safe_map_create();
}

World::~World() {
	if (_default_map) {
		safe_map_free(_default_map);
		_default_map = 0;
	}

	clear();
}

} // namespace NavPhysics
