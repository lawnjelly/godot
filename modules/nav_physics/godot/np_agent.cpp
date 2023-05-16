#include "np_agent.h"
//#include "../source/navphysics_vector.h"
//#include "../source/navphysics_map.h"
#include "../source/navphysics_loader.h"
#include "../source/navphysics_log.h"
#include "../source/navphysics_map.h"
#include "../source/navphysics_pointf.h"
#include "../source/navphysics_pointi.h"
#include "../source/navphysics_vector.h"
#include "core/fixed_array.h"

#include <core/engine.h>

//#define GODOT_DEBUG_NP_AGENT

Transform NPAgent::_dummy_xform;

void NPAgent::_bind_methods() {
	BIND_ENUM_CONSTANT(PATH_OK);
	BIND_ENUM_CONSTANT(PATH_PENDING);
	BIND_ENUM_CONSTANT(PATH_FINISHED);
	BIND_ENUM_CONSTANT(PATH_FAILED);

	BIND_ENUM_CONSTANT(PATH_RESULT_MOVING);
	BIND_ENUM_CONSTANT(PATH_RESULT_BLOCKED);
	//BIND_ENUM_CONSTANT(PATH_RESULT_REACHED_WAYPOINT);
	BIND_ENUM_CONSTANT(PATH_RESULT_PENDING);
	BIND_ENUM_CONSTANT(PATH_RESULT_FINISHED);
	BIND_ENUM_CONSTANT(PATH_RESULT_FAILED);

	ClassDB::bind_method(D_METHOD("nav_teleport", "position"), &NPAgent::nav_teleport);
	ClassDB::bind_method(D_METHOD("apply_impulse", "impulse"), &NPAgent::apply_impulse);

	ClassDB::bind_method(D_METHOD("get_debug_pos", "which"), &NPAgent::get_debug_pos);

	ClassDB::bind_method(D_METHOD("set_radius", "radius"), &NPAgent::set_radius);
	ClassDB::bind_method(D_METHOD("get_radius"), &NPAgent::get_radius);

	ClassDB::bind_method(D_METHOD("set_friction", "friction"), &NPAgent::set_friction);
	ClassDB::bind_method(D_METHOD("get_friction"), &NPAgent::get_friction);

	ClassDB::bind_method(D_METHOD("set_gravity", "gravity"), &NPAgent::set_gravity);
	ClassDB::bind_method(D_METHOD("get_gravity"), &NPAgent::get_gravity);

	ClassDB::bind_method(D_METHOD("set_modifier_air_friction", "air_friction"), &NPAgent::set_modifier_air_friction);
	ClassDB::bind_method(D_METHOD("get_modifier_air_friction"), &NPAgent::get_modifier_air_friction);

	ClassDB::bind_method(D_METHOD("set_modifier_air", "air"), &NPAgent::set_modifier_air);
	ClassDB::bind_method(D_METHOD("get_modifier_air"), &NPAgent::get_modifier_air);

	ClassDB::bind_method(D_METHOD("set_modifier_uphill", "uphill"), &NPAgent::set_modifier_uphill);
	ClassDB::bind_method(D_METHOD("get_modifier_uphill"), &NPAgent::get_modifier_uphill);

	ClassDB::bind_method(D_METHOD("set_modifier_downhill", "downhill"), &NPAgent::set_modifier_uphill);
	ClassDB::bind_method(D_METHOD("get_modifier_downhill"), &NPAgent::get_modifier_downhill);

	ClassDB::bind_method(D_METHOD("apply_jump", "impulse"), &NPAgent::apply_jump);
	ClassDB::bind_method(D_METHOD("is_on_floor"), &NPAgent::is_on_floor);

	ClassDB::bind_method(D_METHOD("get_mesh_instance_transform"), &NPAgent::get_mesh_instance_transform);

	ClassDB::bind_method(D_METHOD("set_guard_internal_jump_links", "enable"), &NPAgent::set_guard_internal_jump_links);
	ClassDB::bind_method(D_METHOD("get_guard_internal_jump_links"), &NPAgent::get_guard_internal_jump_links);

	ClassDB::bind_method(D_METHOD("set_guard_external_jump_links", "enable"), &NPAgent::set_guard_external_jump_links);
	ClassDB::bind_method(D_METHOD("get_guard_external_jump_links"), &NPAgent::get_guard_external_jump_links);

	ClassDB::bind_method(D_METHOD("set_pathfind_internal_jump_links", "enable"), &NPAgent::set_pathfind_internal_jump_links);
	ClassDB::bind_method(D_METHOD("get_pathfind_internal_jump_links"), &NPAgent::get_pathfind_internal_jump_links);

	ClassDB::bind_method(D_METHOD("set_pathfind_external_jump_links", "enable"), &NPAgent::set_pathfind_external_jump_links);
	ClassDB::bind_method(D_METHOD("get_pathfind_external_jump_links"), &NPAgent::get_pathfind_external_jump_links);

	ClassDB::bind_method(D_METHOD("set_npc", "enable"), &NPAgent::set_npc);
	ClassDB::bind_method(D_METHOD("is_npc"), &NPAgent::is_npc);

	// Pathfinding.
	ClassDB::bind_method(D_METHOD("move_to_agent", "agent"), &NPAgent::move_to_agent);
	ClassDB::bind_method(D_METHOD("get_path_status"), &NPAgent::get_path_status);
	ClassDB::bind_method(D_METHOD("get_path_plan_status"), &NPAgent::get_path_plan_status);
	ClassDB::bind_method(D_METHOD("is_stuck"), &NPAgent::is_stuck);

	ClassDB::bind_method(D_METHOD("has_planner", "plan_type"), &NPAgent::has_planner);

	//	ClassDB::bind_method(D_METHOD("is_path_finished"), &NPAgent::is_path_finished);
	//	ClassDB::bind_method(D_METHOD("is_path_ready"), &NPAgent::is_path_ready);
	ClassDB::bind_method(D_METHOD("get_next_waypoint"), &NPAgent::get_next_waypoint);
	ClassDB::bind_method(D_METHOD("reached_waypoint"), &NPAgent::reached_waypoint);
	ClassDB::bind_method(D_METHOD("iterate_path"), &NPAgent::iterate_path);
	ClassDB::bind_method(D_METHOD("force_repath"), &NPAgent::force_repath);

	ClassDB::bind_method(D_METHOD("get_current_path", "level"), &NPAgent::get_current_path);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "NPC"), "set_npc", "is_npc");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "radius", PROPERTY_HINT_RANGE, "0,1024"), "set_radius", "get_radius");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "friction", PROPERTY_HINT_RANGE, "0,1"), "set_friction", "get_friction");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "gravity", PROPERTY_HINT_RANGE, "0,1"), "set_gravity", "get_gravity");

	ADD_GROUP("Modifiers", "modifier_");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "modifier_air_friction", PROPERTY_HINT_RANGE, "0,1"), "set_modifier_air_friction", "get_modifier_air_friction");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "modifier_air", PROPERTY_HINT_RANGE, "0,1"), "set_modifier_air", "get_modifier_air");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "modifier_uphill", PROPERTY_HINT_RANGE, "-1,1"), "set_modifier_uphill", "get_modifier_uphill");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "modifier_downhill", PROPERTY_HINT_RANGE, "-1,1"), "set_modifier_downhill", "get_modifier_downhill");

	ADD_GROUP("Links", "links_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "links_guard_internal"), "set_guard_internal_jump_links", "get_guard_internal_jump_links");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "links_guard_external"), "set_guard_external_jump_links", "get_guard_external_jump_links");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "links_pathfind_internal"), "set_pathfind_internal_jump_links", "get_pathfind_internal_jump_links");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "links_pathfind_external"), "set_pathfind_external_jump_links", "get_pathfind_external_jump_links");
}

void NPAgent::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			//_nav_update();
		} break;
		case NOTIFICATION_ENTER_TREE: {
			_update_process_mode();
			nav_teleport(get_global_transform().origin);
		} break;
	}
}

Vector3 NPAgent::get_debug_pos(int p_which) const {
	NavPhysics::Agent *agent = NPWORLD.safe_get_body(data.h_agent);
	ERR_FAIL_NULL_V(agent, Vector3());

#ifdef NP_DEV_ENABLED
	Vector3 pos = *(Vector3 *)&agent->debug_pos[p_which];
	return pos;
#else
	return Vector3();
#endif
}

bool NPAgent::has_planner(int p_planner_type) {
	if (data.path.path_id == UINT32_MAX) {
		return false;
	}
	const NavPhysics::Plan &plan = NPWORLD.get_plan_store().get_plan(data.path.path_id);

	switch (p_planner_type) {
		case 0: {
			return plan.has_waypoint_planner();
		} break;
		case 1: {
			return plan.has_zone_planner();
		} break;

		default:
			break;
	}

	return false;
}

bool NPAgent::is_stuck() const {
	if (data.path.path_id == UINT32_MAX) {
		return false;
	}
	const NavPhysics::Plan &plan = NPWORLD.get_plan_store().get_plan(data.path.path_id);
	return plan.is_stuck();
}

int NPAgent::get_path_plan_status() const {
	if (data.path.path_id == UINT32_MAX) {
		return 0;
	}
	const NavPhysics::Plan &plan = NPWORLD.get_plan_store().get_plan(data.path.path_id);
	return plan.get_plan_status();
}

NPAgent::PathStatus NPAgent::get_path_status() const {
	if (data.path.path_id == UINT32_MAX) {
		return PATH_FINISHED;
	}
	const NavPhysics::Plan &plan = NPWORLD.get_plan_store().get_plan(data.path.path_id);
	return (NPAgent::PathStatus)plan.get_status();
}

void NPAgent::force_repath() {
	if (data.path.path_id == UINT32_MAX) {
		return;
	}

	NavPhysics::Plan &plan = NPWORLD.get_plan_store().get_plan(data.path.path_id);
	plan.force_repath();
}

Vector<Vector3> NPAgent::get_current_path(int p_level) const {
	ERR_FAIL_COND_V(data.path.path_id == UINT32_MAX, Vector<Vector3>());
	NavPhysics::Plan &plan = NPWORLD.get_plan_store().get_plan(data.path.path_id);

	const uint32_t max_points = 1024;

	uint32_t num_points = 0;
	FixedArray<Vector3, max_points, true> temp;
	num_points = plan.fill_current_world_path(p_level, (NavPhysics::FPoint3 *)temp.ptr(), max_points);

	if (num_points) {
		temp.resize(num_points);
		return temp;
	}

	return Vector<Vector3>();
}

#if 0
bool NPAgent::reached_waypoint() {
	return data.path.reached_waypoint;
	
//	ERR_FAIL_COND_V(data.path.path_id == UINT32_MAX, false);
//	NavPhysics::Plan &plan = NPWORLD.get_plan_store().get_plan(data.path.path_id);
//	if (plan.reached_next_waypoint()) {
//		data.waypoint.clear();
//		return true;
//	}
//	data.waypoint.clear();
//	return false;
}
#endif

NPAgent::PathResult NPAgent::iterate_path() {
	if (data.path.path_id == UINT32_MAX) {
		return PATH_RESULT_FINISHED;
	}

	// Blocked temporarily?
	NavPhysics::Agent *agent = NPWORLD.safe_get_body(data.h_agent);
	ERR_FAIL_NULL_V(agent, PATH_RESULT_FAILED);

	if (agent->blocking_zone_id != UINT32_MAX) {
		return PATH_RESULT_BLOCKED;
	}

	//ERR_FAIL_COND_V(data.path.path_id == UINT32_MAX, PATH_RESULT_FINISHED);
	NavPhysics::Plan &plan = NPWORLD.get_plan_store().get_plan(data.path.path_id);

	Vector3 agent_pos = get_global_transform().origin;
	NPAgent::PathResult res = (NPAgent::PathResult)plan.iterate_move(*(NavPhysics::FPoint3 *)&agent_pos, *(NavPhysics::FPoint3 *)&data.waypoint.pos, data.waypoint.poly_id);
	data.waypoint.reached_waypoint = plan.reached_waypoint();

	if ((res == PATH_RESULT_FINISHED) || (res == PATH_RESULT_FAILED)) {
		free_current_path();
	}

	return res;
}

Vector3 NPAgent::get_next_waypoint() {
	return data.waypoint.pos;
#if 0
	ERR_FAIL_COND_V(data.path.path_id == UINT32_MAX, Vector3());
	NavPhysics::Plan &plan = NPWORLD.get_plan_store().get_plan(data.path.path_id);
	ERR_FAIL_COND_V(!plan.get_next_world_pos(*(NavPhysics::FPoint3 *)&data.waypoint.pos, data.waypoint.poly_id), Vector3());

	// Detect getting stuck.
	uint32_t tps = Engine::get_singleton()->get_iterations_per_second();
	uint32_t tick = Engine::get_singleton()->get_physics_frames();

	if ((tick % (tps * 2)) == 0) {
		Vector3 pos = get_global_transform().origin;

		float dist = (pos - data.path.historical_pos_global).length();

		if (dist < 0.5f) {
			// We are stuck!
			print_line("Stuck!");

			if (!plan.plan_repath()) {
				free_current_path();
				print_line("Repath failed.");
			}
		}

		data.path.historical_pos_global = pos;
	}

	return data.waypoint.pos;
#endif
}

bool NPAgent::move_to_agent(Node *p_agent) {
	free_current_path();

	NPAgent *agent = Object::cast_to<NPAgent>(p_agent);
	ERR_FAIL_NULL_V(agent, false);

	data.path.path_id = NPWORLD.get_plan_store().request();
	NavPhysics::Plan &plan = NPWORLD.get_plan_store().get_plan(data.path.path_id);
	if (!plan.plan_path_agent_agent(data.h_agent, agent->data.h_agent)) {
		free_current_path();
		return false;
	}
	return true;
}

const Transform &NPAgent::get_mesh_instance_transform() const {
	return *(Transform *)(&NPWORLD.safe_get_agent_mesh_instance_transform(data.h_agent));
}

void NPAgent::_update_params() {
	float friction_multiplier = 1.0 - (data.friction * data.friction);
	// print_line("Setting friction_multiplier to " + rtos(friction_multiplier));

	NavPhysics::Agent *agent = NPWORLD.safe_get_body(data.h_agent);
	ERR_FAIL_NULL(agent);

	agent->radius = data.radius;
	agent->friction = 1 - friction_multiplier;
	agent->gravity = data.gravity;

	agent->air_friction_modifier = data.air_friction;
	agent->uphill_modifier = data.uphill;
	agent->downhill_modifier = data.downhill;

	agent->callback.user_data = (uint64_t)this;

	agent->guard_internal_jump_links = data.guard_internal_jump_links;
	agent->guard_external_jump_links = data.guard_external_jump_links;
	agent->pathfind_internal_jump_links = data.pathfind_internal_jump_links;
	agent->pathfind_external_jump_links = data.pathfind_external_jump_links;

	agent->is_npc = data.is_npc;
}

void NPAgent::_update_process_mode() {
	return;

	if (!Engine::get_singleton()->is_editor_hint()) {
		set_physics_process_internal(true);
	}
}

#if 0
void NPAgent::_nav_update() {
	// HACK for now
	NPWORLD.tick_update(Engine::get_singleton()->get_physics_frames(), get_physics_process_delta_time());

	u32 agent_id;
	NavPhysics::Agent *agent = NPWORLD.safe_get_body(data.h_agent, &agent_id);
	ERR_FAIL_NULL(agent);

	agent->fvel3 = *(NavPhysics::FPoint3 *)&data.vel;
	if (data.jump_vel != 0) {
		agent->apply_jump(data.jump_vel);
		data.jump_vel = 0;
	}

	NPWORLD.safe_get_default_map()->iterate_agent(agent_id);

	//Transform tr = get_transform();
	Transform tr;
	tr.origin = *(Vector3 *)&agent->fpos3;
	tr.basis = Basis(Vector3(0, agent->worldspace_yaw, 0));

#ifdef GODOT_DEBUG_NP_AGENT
	print_line(String(Variant(tr.origin)));
#endif
	set_transform(tr);

	data.vel = *(Vector3 *)&agent->fvel3;

	//data.vel *= data.friction_multiplier;
}
#endif

void NPAgent::nav_teleport(const Vector3 &p_pos) {
#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
#endif
	u32 agent_id;
	NavPhysics::Agent *agent = NPWORLD.safe_get_body(data.h_agent, &agent_id);
	ERR_FAIL_NULL(agent);

	NPWORLD.safe_get_default_map()->body_teleport(*agent, agent_id, *(const NavPhysics::FPoint3 *)&p_pos);
}

void NPAgent::apply_jump(float p_impulse) {
#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
#endif
	data.jump_vel += p_impulse;

	if (data.jump_vel != 0) {
		// Send to NavPhysics.
		u32 agent_id;
		NavPhysics::Agent *agent = NPWORLD.safe_get_body(data.h_agent, &agent_id);
		ERR_FAIL_NULL(agent);

		agent->apply_jump(data.jump_vel);
		data.jump_vel = 0;
	}
}

bool NPAgent::is_on_floor() const {
#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		return false;
	}
#endif
	u32 agent_id;
	NavPhysics::Agent *agent = NPWORLD.safe_get_body(data.h_agent, &agent_id);
	ERR_FAIL_NULL_V(agent, false);

	return agent->is_on_floor();
}

void NPAgent::apply_impulse(const Vector3 &p_impulse) {
#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
#endif

	Vector3 impulse = is_on_floor() ? p_impulse : p_impulse * data.air;

	// Send to NavPhysics.
	u32 agent_id;
	NavPhysics::Agent *agent = NPWORLD.safe_get_body(data.h_agent, &agent_id);
	ERR_FAIL_NULL(agent);
	agent->fvel3 += *(NavPhysics::FPoint3 *)&impulse;
}

NPAgent::NPAgent() {
	data.h_agent = NPWORLD.safe_body_create();

	NPWORLD.safe_link_body(data.h_agent, NPWORLD.get_handle_default_map());

	_update_params();
}

void NPAgent::set_guard_internal_jump_links(bool p_enable) {
	data.guard_internal_jump_links = p_enable;
	_update_params();
}

void NPAgent::set_guard_external_jump_links(bool p_enable) {
	data.guard_external_jump_links = p_enable;
	_update_params();
}

void NPAgent::set_pathfind_internal_jump_links(bool p_enable) {
	data.pathfind_internal_jump_links = p_enable;
	_update_params();
}

void NPAgent::set_npc(bool p_enable) {
	data.is_npc = p_enable;
	_update_params();
}

void NPAgent::set_pathfind_external_jump_links(bool p_enable) {
	data.pathfind_external_jump_links = p_enable;
	_update_params();
}

void NPAgent::set_radius(float p_radius) {
	data.radius = p_radius;
	_update_params();
}

void NPAgent::set_friction(float p_friction) {
	data.friction = p_friction;
	_update_params();
}

void NPAgent::set_gravity(float p_gravity) {
	data.gravity = p_gravity;
	_update_params();
}

void NPAgent::set_modifier_uphill(float p_value) {
	data.uphill = p_value;
	_update_params();
}

void NPAgent::set_modifier_downhill(float p_value) {
	data.downhill = p_value;
	_update_params();
}

void NPAgent::set_modifier_air(float p_value) {
	data.air = p_value;
	_update_params();
}

void NPAgent::set_modifier_air_friction(float p_value) {
	data.air_friction = p_value;
	_update_params();
}

void NPAgent::free_current_path() {
	if (data.path.path_id != UINT32_MAX) {
		NPWORLD.get_plan_store().free(data.path.path_id);
		data.path.path_id = UINT32_MAX;
	}
}

float NPAgent::_shift_yaw(float p_from, float p_to, float p_max_change) const {
	float difference = fmod(p_to - p_from, (float)Math_TAU);
	float distance = fmod(2.0f * difference, (float)Math_TAU) - difference;

	if (distance >= 0) {
		distance = MIN(distance, p_max_change);
	} else {
		distance = MAX(distance, -p_max_change);
	}

	return p_from + distance;
}

void NPAgent::update_yaw() {
	Vector2 vel = Vector2(data.vel.x, data.vel.z);
	if (vel.length_squared() > 0) {
		data.yaw = _shift_yaw(data.yaw, vel.angle(), 0.1f);
	}
}

NPAgent::~NPAgent() {
	free_current_path();

	if (data.h_agent) {
		NPWORLD.safe_unlink_body(data.h_agent, NavPhysics::g_world.get_handle_default_map());

		NPWORLD.safe_body_free(data.h_agent);
		data.h_agent = 0;
	}
}
