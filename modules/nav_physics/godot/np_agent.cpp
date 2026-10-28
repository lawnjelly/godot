#include "np_agent.h"
#include "np_point.h"
//#include "thirdparty/navphysics/navphysics_vector.h"
//#include "thirdparty/navphysics/navphysics_map.h"
#include "core/fixed_array.h"
#include "thirdparty/navphysics/navphysics_loader.h"
#include "thirdparty/navphysics/navphysics_log.h"
#include "thirdparty/navphysics/navphysics_map.h"
#include "thirdparty/navphysics/navphysics_mesh_instance.h"
#include "thirdparty/navphysics/navphysics_pointf.h"
#include "thirdparty/navphysics/navphysics_pointi.h"
#include "thirdparty/navphysics/navphysics_vector.h"

#include <core/engine.h>

//#define GODOT_DEBUG_NP_AGENT

Transform NPAgent::_dummy_xform;

void NPAgent::_bind_methods() {
	BIND_ENUM_CONSTANT(UP_ALIGN_MESH);
	BIND_ENUM_CONSTANT(UP_ALIGN_MESH_TANGENT);
	BIND_ENUM_CONSTANT(UP_ALIGN_WORLD_UP);

	BIND_ENUM_CONSTANT(TEST_RESULT_CLEAR);
	BIND_ENUM_CONSTANT(TEST_RESULT_FAILED);
	BIND_ENUM_CONSTANT(TEST_RESULT_INVALID_PARAMETER);
	BIND_ENUM_CONSTANT(TEST_RESULT_DIFFERENT_MESH);
	BIND_ENUM_CONSTANT(TEST_RESULT_INVALID_MESH);
	BIND_ENUM_CONSTANT(TEST_RESULT_BLOCKED_WALL);

	BIND_ENUM_CONSTANT(PATH_OK);
	BIND_ENUM_CONSTANT(PATH_PENDING);
	BIND_ENUM_CONSTANT(PATH_FINISHED);
	BIND_ENUM_CONSTANT(PATH_FAILED);

	BIND_ENUM_CONSTANT(ON_AIR);
	BIND_ENUM_CONSTANT(ON_FLOOR);
	BIND_ENUM_CONSTANT(ON_AGENT);

	BIND_ENUM_CONSTANT(PATH_RESULT_MOVING);
	BIND_ENUM_CONSTANT(PATH_RESULT_BLOCKED);
	//BIND_ENUM_CONSTANT(PATH_RESULT_REACHED_WAYPOINT);
	BIND_ENUM_CONSTANT(PATH_RESULT_PENDING);
	BIND_ENUM_CONSTANT(PATH_RESULT_FINISHED);
	BIND_ENUM_CONSTANT(PATH_RESULT_FAILED);

	ClassDB::bind_method(D_METHOD("ray_test_point", "point"), &NPAgent::ray_test_point);
	ClassDB::bind_method(D_METHOD("ray_test_agent", "agent"), &NPAgent::ray_test_agent);

	ClassDB::bind_method(D_METHOD("nav_teleport", "position"), &NPAgent::nav_teleport);
	ClassDB::bind_method(D_METHOD("apply_impulse", "impulse"), &NPAgent::apply_impulse);

	ClassDB::bind_method(D_METHOD("get_debug_pos", "which"), &NPAgent::get_debug_pos);

	ClassDB::bind_method(D_METHOD("set_radius", "radius"), &NPAgent::set_radius);
	ClassDB::bind_method(D_METHOD("get_radius"), &NPAgent::get_radius);

	ClassDB::bind_method(D_METHOD("set_height", "height"), &NPAgent::set_height);
	ClassDB::bind_method(D_METHOD("get_height"), &NPAgent::get_height);

	ClassDB::bind_method(D_METHOD("set_friction", "friction"), &NPAgent::set_friction);
	ClassDB::bind_method(D_METHOD("get_friction"), &NPAgent::get_friction);

	ClassDB::bind_method(D_METHOD("set_gravity", "gravity"), &NPAgent::set_gravity);
	ClassDB::bind_method(D_METHOD("get_gravity"), &NPAgent::get_gravity);

	ClassDB::bind_method(D_METHOD("set_up_align", "up_align"), &NPAgent::set_up_align);
	ClassDB::bind_method(D_METHOD("get_up_align"), &NPAgent::get_up_align);

	ClassDB::bind_method(D_METHOD("set_modifier_air_friction", "air_friction"), &NPAgent::set_modifier_air_friction);
	ClassDB::bind_method(D_METHOD("get_modifier_air_friction"), &NPAgent::get_modifier_air_friction);

	ClassDB::bind_method(D_METHOD("set_modifier_air_control", "air_control"), &NPAgent::set_modifier_air_control);
	ClassDB::bind_method(D_METHOD("get_modifier_air_control"), &NPAgent::get_modifier_air_control);

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

	ClassDB::bind_method(D_METHOD("set_yaw", "yaw"), &NPAgent::set_yaw);
	ClassDB::bind_method(D_METHOD("get_yaw"), &NPAgent::get_yaw);

	ClassDB::bind_method(D_METHOD("set_yaw_movement_restriction", "restriction"), &NPAgent::set_yaw_movement_restriction);
	ClassDB::bind_method(D_METHOD("get_yaw_movement_restriction"), &NPAgent::get_yaw_movement_restriction);

	ClassDB::bind_method(D_METHOD("set_air_yaw_movement_restriction", "restriction"), &NPAgent::set_air_yaw_movement_restriction);
	ClassDB::bind_method(D_METHOD("get_air_yaw_movement_restriction"), &NPAgent::get_air_yaw_movement_restriction);

	// Physics
	ClassDB::bind_method(D_METHOD("set_physics_enabled", "enable"), &NPAgent::set_physics_enabled);
	ClassDB::bind_method(D_METHOD("is_physics_enabled"), &NPAgent::is_physics_enabled);

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
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "height", PROPERTY_HINT_RANGE, "0,1024"), "set_height", "get_height");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "friction", PROPERTY_HINT_RANGE, "0,1"), "set_friction", "get_friction");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "gravity", PROPERTY_HINT_RANGE, "0,1"), "set_gravity", "get_gravity");

	ADD_PROPERTY(PropertyInfo(Variant::REAL, "yaw_movement_restriction", PROPERTY_HINT_RANGE, "0,1"), "set_yaw_movement_restriction", "get_yaw_movement_restriction");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "air_yaw_movement_restriction", PROPERTY_HINT_RANGE, "0,1"), "set_air_yaw_movement_restriction", "get_air_yaw_movement_restriction");

	ADD_PROPERTY(PropertyInfo(Variant::INT, "up_align", PROPERTY_HINT_ENUM, "Mesh,Mesh Tangent,World"), "set_up_align", "get_up_align");

	ADD_GROUP("Modifiers", "modifier_");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "modifier_air_friction", PROPERTY_HINT_RANGE, "0,1"), "set_modifier_air_friction", "get_modifier_air_friction");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "modifier_air_control", PROPERTY_HINT_RANGE, "0,1"), "set_modifier_air_control", "get_modifier_air_control");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "modifier_uphill", PROPERTY_HINT_RANGE, "-1,1"), "set_modifier_uphill", "get_modifier_uphill");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "modifier_downhill", PROPERTY_HINT_RANGE, "-1,1"), "set_modifier_downhill", "get_modifier_downhill");

	ADD_GROUP("Physics", "physics_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "physics_enabled"), "set_physics_enabled", "is_physics_enabled");

	ADD_GROUP("Links", "links_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "links_guard_internal"), "set_guard_internal_jump_links", "get_guard_internal_jump_links");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "links_guard_external"), "set_guard_external_jump_links", "get_guard_external_jump_links");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "links_pathfind_internal"), "set_pathfind_internal_jump_links", "get_pathfind_internal_jump_links");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "links_pathfind_external"), "set_pathfind_external_jump_links", "get_pathfind_external_jump_links");
}

void NPAgent::_agent_callback(const NavPhysics::AgentCallbackData &p_data) {
	//Transform tr = get_transform();
	// tr.origin = *(Vector3 *)&p_data.position_world_space;

	data.impulse_mesh_space = *(Vector3 *)&p_data.impulse_mesh_space;
	data.impulse_world_space = *(Vector3 *)&p_data.impulse_world_space;

	Transform tr = get_mesh_instance_transform();

	const uint32_t TRANSITION_TICKS = MAX(Engine::get_singleton()->get_iterations_per_second() / 2, 1);

	// When we change mesh, we need a sudden change in yaw to resync,
	// because yaw is measured in mesh space.
	if (p_data.changing_mesh) {
		//print_line("changing mesh");
		data.mesh_instance_id_prev = data.mesh_instance_id_curr;
		data.mesh_instance_id_curr = get_mesh_instance_object_id();

		// TODO : scale this according to time, and TPS.
		data.mesh_instance_transition_ticks = TRANSITION_TICKS;

		if (data.mesh_instance_id_prev) {
			NPMeshInstance *mi = Object::cast_to<NPMeshInstance>(ObjectDB::get_instance(data.mesh_instance_id_prev));
			if (mi) {
				data.mesh_instance_transition_start_basis = mi->get_global_transform().basis;
			} else {
				data.mesh_instance_transition_start_basis = Basis();
			}
		} else {
			data.mesh_instance_transition_start_basis = Basis();
		}

		// Get the previous yaw direction into mesh space.
		Vector3 new_yaw_direction = tr.basis.xform_normal(data.yaw_direction_world_space);
		Vector2 yaw_dir = Vector2(new_yaw_direction.x, new_yaw_direction.z);
		data.yaw = yaw_dir.angle();
	}

	// Calculate yaw
	//_default_update_yaw();

	//data.vel.zero();

	// Get the forward vector for the mesh instance.
	tr.origin = *(Vector3 *)&p_data.position_world_space;

	Basis yaw_basis = Basis(Vector3(0, (Math_PI / 2) - data.yaw, 0));

	switch (data.up_align) {
		case UP_ALIGN_WORLD_UP: {
			tr.basis = yaw_basis;
		} break;
		default: {
			// Mesh align
			tr.basis *= yaw_basis;
		} break;
	}

	// Store the yaw direction in case we change mesh next tick, so we have a frame of reference.
	data.yaw_direction_world_space = tr.basis.xform_normal(Vector3(0, 0, 1));

#if 0
	String sz = "mesh impulse " + String(Variant(data.impulse_mesh_space));
	sz += ", world impulse " + String(Variant(data.impulse_world_space));
	sz += ", yaw : " + itos(Math::rad2deg(data.yaw)) + ", yaw_dir : " + String(Variant(data.yaw_direction_world_space));
	print_line(sz);
#endif

	// Special case, if we are in a transition.
	if (data.mesh_instance_transition_ticks) {
		data.mesh_instance_transition_ticks--;
		if (data.mesh_instance_transition_ticks) {
			float blend_fraction = data.mesh_instance_transition_ticks / (float)TRANSITION_TICKS;

			switch (data.up_align) {
				case UP_ALIGN_MESH: {
					// Get the up vectors from the old mesh and the new one.
					Vector3 prev_up = data.mesh_instance_transition_start_basis.get_axis(1).normalized();
					Vector3 curr_up = tr.basis.get_axis(1).normalized();

					// Construct a quat rotation to go between them, then use slerp to get a smooth blend
					// from the prev_up orientation to the curr_up orientation.
					Quat quat = Quat(curr_up, prev_up);
					quat = Quat().slerp(quat, blend_fraction);

					tr.basis = quat * tr.basis;
				} break;
				case UP_ALIGN_WORLD_UP: {
					/*
					data.up_align_capsule_offset = (data.height * 0.5f) * blend_fraction;
					Vector3 test_pt = Vector3();
					test_pt = tr.xform(test_pt);
					tr.translate(Vector3(0, -data.up_align_capsule_offset, 0));
					Vector3 test_pt2 = Vector3();
					test_pt2 = tr.xform(test_pt2);

					print_line(String(Variant(test_pt)) + " to " + String(Variant(test_pt2)));
					*/
				} break;
				default:
					break;
			}
		}
	}

	//tr.basis = Basis(Vector3(0, (Math_PI / 2) - data.yaw, 0));
	_nav_physics_update_transform(tr);
}

void NPAgent::_nav_physics_update_transform(const Transform &p_xform) {
	set_transform(p_xform);

	if (!physics.rid_body.is_valid()) {
		return;
	}
	PhysicsServer *ps = PhysicsServer::get_singleton();
	ERR_FAIL_NULL(ps);

	Transform tr = get_global_transform();
	tr = tr * physics.shape_xform;

	ps->body_set_state(physics.rid_body, PhysicsServer::BODY_STATE_TRANSFORM, tr);
}

void NPAgent::set_up_align(UpAlign p_up_align) {
	data.up_align = p_up_align;

	NavPhysics::Agent *agent = NPWORLD.safe_get_body(data.h_agent);
	ERR_FAIL_NULL(agent);
	agent->world_space_gravity = p_up_align != UP_ALIGN_MESH;
}

void NPAgent::set_physics_enabled(bool p_enable) {
	if (p_enable == physics.enabled) {
		return;
	}

	physics.enabled = p_enable;

	if (!is_inside_tree()) {
		return;
	}
	_update_physics_enabled();
}

void NPAgent::_update_physics_shape() {
	if (!physics.rid_body.is_valid()) {
		return;
	}

	// Logic should prevent creating physics body when we are in the editor.
	DEV_ASSERT(!Engine::get_singleton()->is_editor_hint());

	// Assuming single threaded here.
	DEV_ASSERT(physics.enabled);
	DEV_ASSERT(is_inside_tree());

	PhysicsServer *ps = PhysicsServer::get_singleton();
	ERR_FAIL_NULL(ps);

	DEV_ASSERT(physics.rid_shape.is_valid());

	Dictionary capsule_data;
	capsule_data["radius"] = data.radius;

	// In Godot the "height" of a cylinder is the height
	// of the middle section, not the overall height.
	// This is confusing, so we standardize the agent height as being "real height",
	// which can be no lower in practice than twice the radius (i.e. a sphere).
	float mid_height = data.height - (data.radius * 2);
	mid_height = MAX(mid_height, 0.0f);

	capsule_data["height"] = mid_height;
	ps->shape_set_data(physics.rid_shape, capsule_data);

	Transform &tr = physics.shape_xform;

	// Rotate around x axis 90 degrees, and push up by half the height of the agent.
	tr.origin = Vector3(0, data.height * 0.5f, 0);
	tr.basis = Basis(Vector3(1, 0, 0), Math::deg2rad(90.0f), Vector3(1, 1, 1));
}

void NPAgent::_update_physics_enabled(bool p_exiting_tree) {
	DEV_ASSERT(is_inside_tree());

	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}

	PhysicsServer *ps = PhysicsServer::get_singleton();
	ERR_FAIL_NULL(ps);

	if (physics.enabled && !p_exiting_tree) {
		DEV_CHECK(!physics.rid_body.is_valid());
		if (!physics.rid_body.is_valid()) {
			physics.rid_body = ps->body_create(PhysicsServer::BODY_MODE_KINEMATIC);
			if (get_world().is_valid()) {
				RID rid_space = get_world()->get_space();
				ps->body_set_space(physics.rid_body, rid_space);
			}

			DEV_ASSERT(!physics.rid_shape.is_valid());
			physics.rid_shape = ps->shape_create(PhysicsServer::SHAPE_CAPSULE);
			_update_physics_shape();

			ps->body_add_shape(physics.rid_body, physics.rid_shape, Transform());
			ps->body_set_collision_layer(physics.rid_body, 1);
			ps->body_set_collision_mask(physics.rid_body, 1);

			_nav_physics_update_transform(get_transform());
		}
	} else {
		if (physics.rid_body.is_valid()) {
			ps->free(physics.rid_body);
			physics.rid_body = RID();
		}
		if (physics.rid_shape.is_valid()) {
			ps->free(physics.rid_shape);
			physics.rid_shape = RID();
		}
	}
}

void NPAgent::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			_update_physics_enabled();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			_update_physics_enabled(true);
		} break;
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

ObjectID NPAgent::get_mesh_instance_object_id() {
	NavPhysics::MeshInstance *mi = NPWORLD.safe_get_body_mesh_instance(data.h_agent);
	if (!mi) {
		return 0;
	}
	return (ObjectID)mi->get_user_data();
}

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

double NPAgent::_scale_friction_param(double p_friction, double p_delta) const {
	if (p_friction <= 0) {
		return 1;
	}
	if (p_friction >= 1) {
		return 0;
	}

	// More means 0.5 maps to more friction, and vice versa.
	const double friction_mapping = 8; // 5

	double f = 1 - p_friction;
	f = Math::pow(f, friction_mapping);
	f = 1 - f;
	f = Math::pow(1 - f, p_delta);
	return f;
}

void NPAgent::_update_params() {
	NavPhysics::Agent *agent = NPWORLD.safe_get_body(data.h_agent);
	ERR_FAIL_NULL(agent);

	// Calculations to give consistent parameters at different physics TPS.
	// float speedMultiplier = pow(1.0 - fractionLossPerSecond, deltaTime)

	int tps = Engine::get_singleton()->get_iterations_per_second();
	double delta = 1.0 / tps;
	double tps_linear_adjustment = (60.0 / tps) * (60.0 / tps);
//	double time_ratio = tps / 60.0;

// Takes in the value calibrated at 60tps, and spits out the value relative to our tps.
//#define NPAGENT_EXP_ADJUSTMENT(a) (Math::pow(Math::pow(a, 60.0), delta))
#define NPAGENT_LINEAR_ADJUSTMENT(a) (a * tps_linear_adjustment)
	//#define NPAGENT_FRICTION_ADJUSTMENT(a) (Math::pow(a, time_ratio))
	//#define NPAGENT_FRICTION_ADJUSTMENT(a) (Math::pow(1.0 - a, delta))

	//#define NPAGENT_FRICTION_ADJUSTMENT(friction_per_second) (Math::pow(friction_per_second, delta))

	agent->radius = data.radius;
	agent->height = data.height;

	double gravity = Math::pow((double)data.gravity, 2.0);
	gravity *= (1.0 / 60.0);
	agent->gravity = NPAGENT_LINEAR_ADJUSTMENT(gravity);

	agent->set_friction(_scale_friction_param(data.friction, delta), false);
	agent->set_friction(_scale_friction_param(data.air_friction, delta), true);

	agent->uphill_modifier = data.uphill;
	agent->downhill_modifier = data.downhill;

	agent->callback.user_data = (uint64_t)this;

	agent->guard_internal_jump_links = data.guard_internal_jump_links;
	agent->guard_external_jump_links = data.guard_external_jump_links;
	agent->pathfind_internal_jump_links = data.pathfind_internal_jump_links;
	agent->pathfind_external_jump_links = data.pathfind_external_jump_links;

	agent->is_npc = data.is_npc;

	agent->set_yaw_movement_restriction(data.yaw_movement_restriction, false);
	agent->set_yaw_movement_restriction(data.air_yaw_movement_restriction, true);

	//#undef NPAGENT_LINEAR_ADJUSTMENT
	//#undef NPAGENT_EXP_ADJUSTMENT
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

NPAgent::TestResult NPAgent::ray_test_agent(const Node *p_np_agent) const {
	const NPAgent *np_agent = Object::cast_to<NPAgent>(p_np_agent);
	ERR_FAIL_NULL_V_MSG(np_agent, TEST_RESULT_INVALID_PARAMETER, "Parameter must be an NPAgent");

	NavPhysics::Agent *agent_A = NPWORLD.safe_get_body(np_agent->data.h_agent);
	ERR_FAIL_NULL_V(agent_A, TEST_RESULT_INVALID_PARAMETER);

	NavPhysics::Agent *agent_B = NPWORLD.safe_get_body(data.h_agent);
	ERR_FAIL_NULL_V(agent_B, TEST_RESULT_FAILED);

	ERR_FAIL_COND_V(agent_A->get_mesh_instance_id() != agent_B->get_mesh_instance_id(), TEST_RESULT_DIFFERENT_MESH);
	ERR_FAIL_COND_V(agent_B->get_mesh_instance_id() == UINT32_MAX, TEST_RESULT_INVALID_MESH);

	NavPhysics::MeshInstance &mi = NPWORLD.get_mesh_instance(agent_B->get_mesh_instance_id());

	return mi.get_mesh().ray_test(agent_A->pos, agent_A->poly_id, agent_B->pos, agent_B->poly_id) ? TEST_RESULT_CLEAR : TEST_RESULT_BLOCKED_WALL;
}

NPAgent::TestResult NPAgent::ray_test_point(const Node *p_np_point) const {
	const NPPoint *point = Object::cast_to<NPPoint>(p_np_point);
	ERR_FAIL_NULL_V_MSG(point, TEST_RESULT_INVALID_PARAMETER, "Parameter must be an NPPoint");

	NavPhysics::POI *poi = point->get_poi();
	ERR_FAIL_NULL_V(poi, TEST_RESULT_FAILED);

	NavPhysics::Agent *agent = NPWORLD.safe_get_body(data.h_agent);
	ERR_FAIL_NULL_V(agent, TEST_RESULT_FAILED);

	ERR_FAIL_COND_V(agent->get_mesh_instance_id() == UINT32_MAX, TEST_RESULT_INVALID_MESH);

	NavPhysics::MeshInstance &mi = NPWORLD.get_mesh_instance(agent->get_mesh_instance_id());
	ERR_FAIL_COND_V(mi.get_handle() != poi->handle_mesh_instance, TEST_RESULT_DIFFERENT_MESH);

	return mi.get_mesh().ray_test(agent->pos, agent->poly_id, poi->local_pos, poi->poly_id) ? TEST_RESULT_CLEAR : TEST_RESULT_BLOCKED_WALL;
}

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

	// Make sure the mesh instance ID is up to date, and there is no transition set.
	data.mesh_instance_id_prev = data.mesh_instance_id_curr;
	data.mesh_instance_id_curr = get_mesh_instance_object_id();
	data.mesh_instance_transition_ticks = 0;
}

void NPAgent::apply_jump(float p_impulse) {
#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
#endif
	//data.jump_velocity += p_impulse;

	if (p_impulse != 0) {
		// Send to NavPhysics.
		u32 agent_id;
		NavPhysics::Agent *agent = NPWORLD.safe_get_body(data.h_agent, &agent_id);
		ERR_FAIL_NULL(agent);

		// NavPhysics takes a TPS agnostic jump velocity.
		float jump = p_impulse * NPTicker::tps_data.multiplier_jump;

		agent->apply_jump(jump);
		//data.jump_velocity = 0;
	}
}

NPAgent::FloorStatus NPAgent::is_on_floor() const {
#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		return ON_AIR;
	}
#endif
	u32 agent_id;
	NavPhysics::Agent *agent = NPWORLD.safe_get_body(data.h_agent, &agent_id);
	ERR_FAIL_NULL_V(agent, ON_AIR);

	if (agent->is_on_floor()) {
		return ON_FLOOR;
	}
	if (agent->is_on_agent()) {
		return ON_AGENT;
	}
	return ON_AIR;
}

void NPAgent::apply_impulse(const Vector3 &p_impulse) {
#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
#endif

	Vector3 impulse = (is_on_floor() != ON_AIR) ? p_impulse : p_impulse * data.air_control;

	// Fudge for TPS and delta combined.
	// Navphysics takes a raw impulse to add to velocity,
	// it is TPS agnostic.
	impulse *= NPTicker::tps_data.multiplier_impulse;

	// Send to NavPhysics.
	NavPhysics::Agent *agent = NPWORLD.safe_get_body(data.h_agent);
	ERR_FAIL_NULL(agent);
	agent->apply_impulse(*(NavPhysics::FPoint3 *)&impulse);
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

void NPAgent::set_height(float p_height) {
	if (p_height == data.height) {
		return;
	}
	data.height = p_height;
	_update_params();
	_update_physics_shape();
}

void NPAgent::set_radius(float p_radius) {
	data.radius = p_radius;
	_update_params();
	_update_physics_shape();
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

void NPAgent::set_modifier_air_control(float p_value) {
	data.air_control = p_value;
	_update_params();
}

void NPAgent::set_yaw_movement_restriction(float p_value) {
	data.yaw_movement_restriction = p_value;
	_update_params();
}

void NPAgent::set_air_yaw_movement_restriction(float p_value) {
	data.air_yaw_movement_restriction = p_value;
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

void NPAgent::shift_yaw(float p_yaw, float p_max_shift) {
	set_yaw(_shift_yaw(data.yaw, p_yaw, p_max_shift));
}

void NPAgent::set_yaw(float p_yaw) {
	if (p_yaw == data.yaw) {
		return;
	}

	data.yaw = p_yaw;
	if (data.h_agent) {
		// This could probably be combined with setting impulse for efficiency,
		// but calling separately is easiest conceptually for users.
		NavPhysics::Agent *agent = NPWORLD.safe_get_body(data.h_agent);
		ERR_FAIL_NULL(agent);

		agent->set_yaw(p_yaw);
	}
}

void NPAgent::_default_update_yaw() {
	Vector2 vel = Vector2(data.impulse_mesh_space.x, data.impulse_mesh_space.z);
	if (vel.length_squared() > 0) {
		shift_yaw(vel.angle(), 6.0f / Engine::get_singleton()->get_iterations_per_second());
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
