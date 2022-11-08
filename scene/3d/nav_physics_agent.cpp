#include "nav_physics_agent.h"
#include "core/engine.h"
#include "scene/3d/immediate_geometry.h"
#include "scene/3d/label_3d.h"
#include "servers/nav_physics/nav_physics_server.h"
#include "servers/navigation_server.h"

int NavPhysicsAgent::Data::ticks_per_second = 60;
real_t NavPhysicsAgent::Data::tick_duration = 1.0 / 60.0;

void NavPhysicsAgent::_bind_methods() {
	BIND_ENUM_CONSTANT(NavPhysics::AGENT_STATE_CLEAR);
	BIND_ENUM_CONSTANT(NavPhysics::AGENT_STATE_COLLIDING);
	BIND_ENUM_CONSTANT(NavPhysics::AGENT_STATE_PENDING_COLLIDING);
	BIND_ENUM_CONSTANT(NavPhysics::AGENT_STATE_BLOCKED_BY_NARROWING);

	BIND_ENUM_CONSTANT(PATH_RESULT_SUCCESS);
	BIND_ENUM_CONSTANT(PATH_RESULT_STUCK);

	ClassDB::bind_method(D_METHOD("set_navigation_map", "navigation_map"), &NavPhysicsAgent::set_navigation_map);
	ClassDB::bind_method(D_METHOD("get_navigation_map"), &NavPhysicsAgent::get_navigation_map);

	ClassDB::bind_method(D_METHOD("agent_teleport", "position"), &NavPhysicsAgent::agent_teleport);
	ClassDB::bind_method(D_METHOD("agent_add_force", "force"), &NavPhysicsAgent::agent_add_force);
	ClassDB::bind_method(D_METHOD("agent_jump", "impulse", "allow_air_jump"), &NavPhysicsAgent::agent_jump, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("agent_is_jumping"), &NavPhysicsAgent::agent_is_jumping);
	ClassDB::bind_method(D_METHOD("agent_get_push"), &NavPhysicsAgent::agent_get_push);
	ClassDB::bind_method(D_METHOD("agent_choose_random_location"), &NavPhysicsAgent::agent_choose_random_location);

	ClassDB::bind_method(D_METHOD("path_move_to", "destination"), &NavPhysicsAgent::path_move_to);
	ClassDB::bind_method(D_METHOD("path_finished"), &NavPhysicsAgent::path_finished);
	ClassDB::bind_method(D_METHOD("get_path_result"), &NavPhysicsAgent::get_path_result);
	ClassDB::bind_method(D_METHOD("path_stop"), &NavPhysicsAgent::path_stop);

	ClassDB::bind_method(D_METHOD("set_physics_friction", "friction"), &NavPhysicsAgent::set_physics_friction);
	ClassDB::bind_method(D_METHOD("get_physics_friction"), &NavPhysicsAgent::get_physics_friction);

	ClassDB::bind_method(D_METHOD("set_physics_gravity", "gravity"), &NavPhysicsAgent::set_physics_gravity);
	ClassDB::bind_method(D_METHOD("get_physics_gravity"), &NavPhysicsAgent::get_physics_gravity);

	ClassDB::bind_method(D_METHOD("set_path_speed", "speed"), &NavPhysicsAgent::set_path_speed);
	ClassDB::bind_method(D_METHOD("get_path_speed"), &NavPhysicsAgent::get_path_speed);

	ClassDB::bind_method(D_METHOD("set_path_waypoint_threshold", "distance"), &NavPhysicsAgent::set_path_waypoint_threshold);
	ClassDB::bind_method(D_METHOD("get_path_waypoint_threshold"), &NavPhysicsAgent::get_path_waypoint_threshold);

	ClassDB::bind_method(D_METHOD("get_distance_to_next_waypoint"), &NavPhysicsAgent::path_get_distance_to_next_waypoint);
	ClassDB::bind_method(D_METHOD("get_next_waypoint"), &NavPhysicsAgent::path_get_next_waypoint);

	ClassDB::bind_method(D_METHOD("set_avoidance_enabled", "enabled"), &NavPhysicsAgent::set_avoidance_enabled);
	ClassDB::bind_method(D_METHOD("get_avoidance_enabled"), &NavPhysicsAgent::get_avoidance_enabled);

	ClassDB::bind_method(D_METHOD("set_avoidance_radius", "radius"), &NavPhysicsAgent::set_avoidance_radius);
	ClassDB::bind_method(D_METHOD("get_avoidance_radius"), &NavPhysicsAgent::get_avoidance_radius);

	ClassDB::bind_method(D_METHOD("set_avoidance_ignore_y", "ignore"), &NavPhysicsAgent::set_avoidance_ignore_y);
	ClassDB::bind_method(D_METHOD("get_avoidance_ignore_y"), &NavPhysicsAgent::get_avoidance_ignore_y);

	ClassDB::bind_method(D_METHOD("set_avoidance_neighbor_dist", "neighbor_dist"), &NavPhysicsAgent::set_avoidance_neighbor_dist);
	ClassDB::bind_method(D_METHOD("get_avoidance_neighbor_dist"), &NavPhysicsAgent::get_avoidance_neighbor_dist);

	ClassDB::bind_method(D_METHOD("set_avoidance_max_neighbors", "max_neighbors"), &NavPhysicsAgent::set_avoidance_max_neighbors);
	ClassDB::bind_method(D_METHOD("get_avoidance_max_neighbors"), &NavPhysicsAgent::get_avoidance_max_neighbors);

	ClassDB::bind_method(D_METHOD("set_avoidance_time_horizon", "time_horizon"), &NavPhysicsAgent::set_avoidance_time_horizon);
	ClassDB::bind_method(D_METHOD("get_avoidance_time_horizon"), &NavPhysicsAgent::get_avoidance_time_horizon);

	ClassDB::bind_method(D_METHOD("set_avoidance_max_speed", "max_speed"), &NavPhysicsAgent::set_avoidance_max_speed);
	ClassDB::bind_method(D_METHOD("get_avoidance_max_speed"), &NavPhysicsAgent::get_avoidance_max_speed);

	ClassDB::bind_method(D_METHOD("set_avoidance_ignore_narrowings", "enabled"), &NavPhysicsAgent::set_avoidance_ignore_narrowings);
	ClassDB::bind_method(D_METHOD("get_avoidance_ignore_narrowings"), &NavPhysicsAgent::get_avoidance_ignore_narrowings);

	ClassDB::bind_method(D_METHOD("_navphysics_done", "new_floor_position", "avoidance", "state"), &NavPhysicsAgent::_navphysics_done);

	ClassDB::bind_method(D_METHOD("set_debug_direction_node", "node"), &NavPhysicsAgent::set_debug_direction_node);

	ADD_GROUP("Physics", "physics");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "physics_friction", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_physics_friction", "get_physics_friction");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "physics_gravity", PROPERTY_HINT_RANGE, "0.0,100.0"), "set_physics_gravity", "get_physics_gravity");
	ADD_GROUP("Path", "path");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "path_speed"), "set_path_speed", "get_path_speed");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "path_waypoint_threshold", PROPERTY_HINT_RANGE, "0.0,100.0"), "set_path_waypoint_threshold", "get_path_waypoint_threshold");
	ADD_GROUP("Avoidance", "avoidance");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "avoidance_enabled"), "set_avoidance_enabled", "get_avoidance_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "avoidance_ignore_narrowings"), "set_avoidance_ignore_narrowings", "get_avoidance_ignore_narrowings");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "avoidance_radius", PROPERTY_HINT_RANGE, "0.1,100,0.01"), "set_avoidance_radius", "get_avoidance_radius");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "avoidance_neighbor_dist", PROPERTY_HINT_RANGE, "0.1,10000,0.01"), "set_avoidance_neighbor_dist", "get_avoidance_neighbor_dist");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "avoidance_max_neighbors", PROPERTY_HINT_RANGE, "1,1000,1"), "set_avoidance_max_neighbors", "get_avoidance_max_neighbors");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "avoidance_time_horizon", PROPERTY_HINT_RANGE, "0.01,100,0.01"), "set_avoidance_time_horizon", "get_avoidance_time_horizon");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "avoidance_max_speed", PROPERTY_HINT_RANGE, "0.1,10000,0.01"), "set_avoidance_max_speed", "get_avoidance_max_speed");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "avoidance_ignore_y"), "set_avoidance_ignore_y", "get_avoidance_ignore_y");

	NavPhysicsAgent::Data::ticks_per_second = Engine::get_singleton()->get_iterations_per_second();
	NavPhysicsAgent::Data::tick_duration = 1.0 / Engine::get_singleton()->get_iterations_per_second();
}

void NavPhysicsAgent::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POST_ENTER_TREE: {
			// need to use POST_ENTER_TREE cause with normal ENTER_TREE not all required Nodes are ready.
			// cannot use READY as ready does not get called if Node is readded to SceneTree
#ifdef TOOLS_ENABLED
			if (!Engine::get_singleton()->is_editor_hint()) {
				set_physics_process_internal(true);
			}
#else
			set_physics_process_internal(true);
#endif
#ifdef TOOLS_ENABLED
			if (!Engine::get_singleton()->is_editor_hint() && debug.label) {
				debug.label->set_owner(get_owner());
			}
#endif

			if (debug.imm) {
				// Set as top level for some reason resets the transform to the parent when
				// entering the tree. We need to counteract this as we want the immediate geometry
				// in world space.
				debug.imm->set_transform(Transform());
			}
		} break;
		case NOTIFICATION_EXIT_TREE: {
			path.clear();
			set_physics_process_internal(false);
		} break;
		case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			_update_path();
			_iterate_nav_physics();
		} break;
	}
}

NavPhysicsAgent::NavPhysicsAgent() {
	data.body = NavPhysicsServer::get_singleton()->body_create();

	data.agent = NavigationServer::get_singleton()->agent_create();
	//NavigationServer::get_singleton()->navphysics_set_enabled(data.agent, true);
	//NavigationServer::get_singleton()->agent_set_callback(data.agent, this, "_navphysics_done");

	set_avoidance_neighbor_dist(avoidance.neighbor_dist);
	set_avoidance_max_neighbors(avoidance.max_neighbours);
	set_avoidance_time_horizon(avoidance.time_horizon);
	set_avoidance_radius(avoidance.radius);
	set_avoidance_max_speed(avoidance.max_speed);
	set_avoidance_ignore_y(data.avoidance_ignore_y);

	set_physics_gravity(data.gravity);
	set_physics_friction(data.friction);
	set_path_waypoint_threshold(0.2f);

	NavPhysicsServer::get_singleton()->body_set_callback(data.body, this);

	if (Engine::get_singleton()->is_editor_hint()) {
		debug.enabled = false;
	}

	if (debug.enabled) {
		//		debug.label = memnew(Label3D);
		//		add_child(debug.label);
		//		debug.label->set_text("Testing 123");
		//		debug.label->translate(Vector3(0, 3, 0));
		//		debug.label->set_scale(Vector3(3, 3, 3));
		//		debug.label->set_billboard_mode(SpatialMaterial::BillboardMode::BILLBOARD_FIXED_Y);

		debug.imm = memnew(ImmediateGeometry);
		add_child(debug.imm);
		debug.imm->set_as_toplevel(true);
	}
}

void NavPhysicsAgent::set_debug_direction_node(Node *p_node) {
	debug.direction_node = Object::cast_to<Spatial>(p_node);

	if (debug.direction_node && (debug.direction_node->get_parent() != this)) {
		debug.direction_node = nullptr;
		ERR_FAIL_MSG("Direction node must be a child of NavPhysicsAgent.");
	}
}

NavPhysicsAgent::~NavPhysicsAgent() {
	if (data.agent.is_valid()) {
		NavigationServer::get_singleton()->free(data.agent);
		data.agent = RID();
	}

	NavPhysicsServer::get_singleton()->body_free(data.body);
}

void NavPhysicsAgent::set_physics_friction(real_t p_friction) {
	data.friction = p_friction;
	_refresh_parameters();
}

void NavPhysicsAgent::set_path_speed(real_t p_speed) {
	ERR_FAIL_COND(p_speed < 0);
	ERR_FAIL_COND(p_speed > 1000);
	path.speed = p_speed;
}

void NavPhysicsAgent::set_path_waypoint_threshold(real_t p_distance) {
	ERR_FAIL_COND(p_distance < 0);
	ERR_FAIL_COND(p_distance > 1000);
	path.waypoint_threshold = p_distance;
	path.waypoint_threshold_squared = p_distance * p_distance;
}

void NavPhysicsAgent::set_physics_gravity(real_t p_gravity) {
	ERR_FAIL_COND(p_gravity < 0);
	ERR_FAIL_COND(p_gravity > 100);
	data.gravity = p_gravity;
	data.scaled_gravity = p_gravity / Data::ticks_per_second;
}

void NavPhysicsAgent::_refresh_parameters() {
	ERR_FAIL_COND(!data.agent.is_valid());
	real_t timestep = Data::tick_duration;

	// get the friction into a reasonable range
	real_t friction_per_second = Math::pow(data.friction, 6.0f);
	real_t friction_per_tick = Math::pow(friction_per_second, timestep);
	// print_line("setting friction per tick " + String(Variant(friction_per_tick)));
	NavPhysicsServer::get_singleton()->body_set_params(data.body, friction_per_tick, avoidance.ignore_narrowings);
}

void NavPhysicsAgent::set_avoidance_ignore_narrowings(bool p_enabled) {
	avoidance.ignore_narrowings = p_enabled;
	_refresh_parameters();
}

void NavPhysicsAgent::set_avoidance_enabled(bool p_enabled) {
	data.avoidance = p_enabled;
}

void NavPhysicsAgent::set_avoidance_neighbor_dist(real_t p_distance) {
	avoidance.neighbor_dist = p_distance;
	NavigationServer::get_singleton()->agent_set_neighbor_dist(data.agent, avoidance.neighbor_dist);
}

void NavPhysicsAgent::set_avoidance_max_neighbors(int p_max_neighbors) {
	avoidance.max_neighbours = p_max_neighbors;
	NavigationServer::get_singleton()->agent_set_max_neighbors(data.agent, avoidance.max_neighbours);
}

void NavPhysicsAgent::set_avoidance_time_horizon(real_t p_time_horizon) {
	avoidance.time_horizon = p_time_horizon;
	NavigationServer::get_singleton()->agent_set_time_horizon(data.agent, avoidance.time_horizon);
}

void NavPhysicsAgent::set_avoidance_radius(real_t p_radius) {
	avoidance.radius = p_radius;
	NavigationServer::get_singleton()->agent_set_radius(data.agent, avoidance.radius);
}

void NavPhysicsAgent::set_avoidance_max_speed(real_t p_speed) {
	avoidance.max_speed = p_speed;
	NavigationServer::get_singleton()->agent_set_max_speed(data.agent, avoidance.max_speed * Data::tick_duration);
}

void NavPhysicsAgent::set_avoidance_ignore_y(bool p_ignore_y) {
	data.avoidance_ignore_y = p_ignore_y;
	NavigationServer::get_singleton()->agent_set_ignore_y(data.agent, data.avoidance_ignore_y);
}

void NavPhysicsAgent::_refresh_map() {
	if (data.map.is_valid()) {
		return;
	}

	// no navigation node found in parent nodes, use default navigation map from world resource
	data.map = get_world()->get_navigation_map();
	set_navigation_map(data.map);
}

void NavPhysicsAgent::set_navigation_map(RID p_navigation_map) {
	data.map = p_navigation_map;
	ERR_FAIL_COND(!data.agent.is_valid());
	NavigationServer::get_singleton()->agent_set_map(data.agent, data.map);
	np_handle np_map = NavigationServer::get_singleton()->map_get_navphysics_map(data.map);
	NavPhysicsServer::get_singleton()->body_set_map(data.body, np_map);
	_refresh_parameters();
}

RID NavPhysicsAgent::get_navigation_map() const {
	return data.map;
}

Vector3 NavPhysicsAgent::agent_choose_random_location() const {
	ERR_FAIL_COND_V(!data.body, Vector3());
	return NavPhysicsServer::get_singleton()->body_choose_random_location(data.body);
}

const Vector3 &NavPhysicsAgent::agent_get_push() const {
	return data.avoidance_vec;
}

const Vector3 &NavPhysicsAgent::agent_get_position() const {
	return data.floor_pos;
}

void NavPhysicsAgent::path_stop() {
	path.clear();
}

bool NavPhysicsAgent::path_move_to(const Vector3 &p_destination) {
	ERR_FAIL_COND_V(!data.map.is_valid(), false);
	path.pts = NavigationServer::get_singleton()->map_get_path(data.map, data.floor_pos, p_destination, true);
	path.path_pts.clear();
	path.path_pts.resize(path.pts.size());

	path.curr = 0;

	// For detecting paths that have got stuck
	path.result = PATH_RESULT_SUCCESS;
	path.stuck_record = data.floor_pos;
	path.stuck_timeout = Engine::get_singleton()->get_physics_frames() + (Engine::get_singleton()->get_iterations_per_second() * 5);
	path.repathing = false;
	path.trace_timeout = Engine::get_singleton()->get_physics_frames() + (Engine::get_singleton()->get_iterations_per_second() * 4);
	return true;
}

real_t NavPhysicsAgent::path_get_distance_to_next_waypoint() const {
	if (!path.is_finished()) {
		return (path.get_current_waypoint() - data.floor_pos).length_squared();
	}
	return 0;
}

const Vector3 &NavPhysicsAgent::path_get_next_waypoint() const {
	static Vector3 fail_pos;
	ERR_FAIL_COND_V(path.is_finished(), fail_pos);
	return path.get_current_waypoint();
}

void NavPhysicsAgent::_debug_update_next_waypoint_display() {
	if (debug.imm && !path_finished()) {
		ImmediateGeometry &g = *debug.imm;

		g.clear();
		g.begin(Mesh::PRIMITIVE_LINES);
		g.set_normal(Vector3(0, 1, 0));
		g.set_uv(Vector2(0, 0));

		g.add_vertex(data.floor_pos);
		g.add_vertex(path_get_next_waypoint());

		g.end();
	}
}

void NavPhysicsAgent::_update_path(int p_depth) {
	if (p_depth >= 15) {
		// debugging
		//		for (uint32_t n = 0; n < path.pts.size(); n++) {
		//			print_line(itos(n) + " : " + String(Variant(path.pts[n])) + " " + String(Variant(path.path_pts[n].shifted)));
		//		}
		print_line("depth exceeded");
	}

	if (path.is_finished()) {
		return;
	}

	if (Engine::get_singleton()->get_physics_frames() < path.blocked_timeout) {
		if (debug.direction_node && !debug.direction_node->is_visible()) {
			debug.direction_node->set_visible(false);
		}

		return;
	}

	const Vector3 &next = path.get_current_waypoint();

	// stuck detection
	uint32_t tick = Engine::get_singleton()->get_physics_frames();
	if (tick >= path.stuck_timeout) {
		real_t dist = (data.floor_pos - path.stuck_record).length();

		// if we haven't moved far
		if (dist < 3) {
			// try a repath rather than report stuck, if we can
			if (!path.repathing) {
				NavPhysics::TraceResult res = NavPhysicsServer::get_singleton()->body_trace(data.body, next);
				if (res.mesh.hit) {
					// repath!
					print_line("repath");
					path_move_to(path.pts[path.pts.size() - 1]);
					path.repathing = true;
					return;
				}
			}

			path.clear();
			path.result = PATH_RESULT_STUCK;
			return;
		}

		path.stuck_record = data.floor_pos;
		path.stuck_timeout = tick + (Engine::get_singleton()->get_iterations_per_second() * 5);

		// if we haven't repathed for some time, allow another one to occur if we get stuck again
		path.repathing = false;
	}

	_debug_update_next_waypoint_display();

	// Periodically trace to the next waypoint .. in case we have been nudged off course by an obstacle.
	// We may be trapped against geometry.
	if (tick >= path.trace_timeout) {
		NavPhysics::TraceResult res = NavPhysicsServer::get_singleton()->body_trace(data.body, next);
		if (res.mesh.hit) {
			bool backtrack = false;

			// before repathing, see if we can simply backtrack to the old path as we have been nudged off
			if (path.curr) {
				Vector3 backtrack_pt = Geometry::get_closest_point_to_segment(data.floor_pos, &path.pts[path.curr - 1]);

				res = NavPhysicsServer::get_singleton()->body_trace(data.body, backtrack_pt);
				if (!res.mesh.hit) {
					//DEV_ASSERT(backtrack_pt.x > -18.0f);
					path.pts[path.curr - 1] = backtrack_pt;
					path.curr -= 1;
					backtrack = true;
					print_line("backtrack");
				}
			}

			if (!backtrack) {
				// repath!
				print_line("repath");
				path_move_to(path.pts[path.pts.size() - 1]);
				return;
			}
		}

		path.trace_timeout = tick + (Engine::get_singleton()->get_iterations_per_second() * 2);
	}

	Vector3 offset = next - data.floor_pos;

	// reached?
	Vector2 segment[2];
	segment[1] = Vector2(data.floor_pos.x, data.floor_pos.z);
	segment[0] = segment[1] - Vector2(data.prev_move.x, data.prev_move.z);
	Vector2 closest_point = Geometry::get_closest_point_to_segment_2d(Vector2(next.x, next.z), segment);
	Vector2 closest_offset = closest_point - Vector2(next.x, next.z);

	real_t dist = closest_offset.length_squared();
	if (dist <= path.waypoint_threshold_squared) {
		path.curr += 1;
		if (path.curr >= path.pts.size()) {
			path.clear();
		}
		_update_path(p_depth + 1);
		return;
	}

	if (data.path_auto_follow) {
		// if we are in agent - agent collision, add a conventional
		// offset to the left hand side of the "road", to allow queues of agents
		// to pass more easily
		if (data.state == NavPhysics::AGENT_STATE_COLLIDING) {
			Vector2 off2 = Vector2(offset.x, offset.z);
			float length = off2.length();

			// if this is the first collision, shift the waypoint to the left
			if (!path.path_pts[path.curr].shifted) {
				path.path_pts[path.curr].shifted = true;

				if (path.curr) {
					Vector3 curr_heading = next - path.pts[path.curr - 1];

					if (curr_heading.length_squared() > 0.01f) {
						// to the left
						Vector3 side_vec = Vector3(-curr_heading.z, 0, curr_heading.x);

						// normalize and scale
						side_vec *= 2.0f / length;

						// BUG : TRACE IS GOING FROM YOUR BODY, NOT THE WAYPOINT
						NavPhysics::TraceResult res = NavPhysicsServer::get_singleton()->body_dual_trace(data.body, next, next + side_vec);

						if (!res.mesh.first_trace_hit) {
							//						if (res.mesh.hit_point.x < -18.0f)
							//						{
							//							res = NavPhysicsServer::get_singleton()->body_trace(data.body, next + side_vec);
							//						}

							// change the current next waypoint
							path.pts[path.curr] = res.mesh.hit_point;

							// rerun this routine
							_update_path(p_depth + 1);
							return;
						}
					}
				}
			}

			// only apply if not near a waypoint
			if (length > (path.waypoint_threshold * 4)) { // 4

				Vector2 avoid2 = Vector2(data.avoidance_vec.x, data.avoidance_vec.z);
				float angle_to_avoid = off2.angle_to(avoid2);
				if (Math::abs(angle_to_avoid) < Math::deg2rad(45.0f)) {
					float angle = off2.angle();
					angle -= Math::deg2rad(60.0f); // 45
					off2.set_rotation(angle);
					off2 *= length;
					offset.x = off2.x;
					offset.z = off2.y;
				}
			}
		}

		offset.normalize();

		offset *= path.speed / Data::ticks_per_second;
		agent_add_force(offset);
	}
}

void NavPhysicsAgent::agent_teleport(const Vector3 &p_position) {
	data.started = true;
	_refresh_map();
	ERR_FAIL_COND(!data.map.is_valid());
	ERR_FAIL_COND(!data.agent.is_valid());
	NavPhysicsServer::get_singleton()->body_teleport(data.body, p_position);
}

void NavPhysicsAgent::agent_add_force(const Vector3 &p_force) {
	if (!data.started) {
		return;
	}
	ERR_FAIL_COND(!data.map.is_valid());
	ERR_FAIL_COND(!data.agent.is_valid());
	Vector3 impulse = p_force / Data::ticks_per_second;

	NavPhysicsServer::get_singleton()->body_add_impulse(data.body, impulse);

#ifdef TOOLS_ENABLED
	if (debug.direction_node) {
		real_t l = p_force.length();

		if (l > 0.01f) {
			// assuming y axis aligned for now
			real_t angle = Vector2(p_force.x, p_force.z).angle();
			Transform tr;
			//tr.translate(Vector3(l, 0, 0));
			tr.rotate(Vector3(0, 1, 0), -angle);

			if (!debug.direction_node->is_visible()) {
				debug.direction_node->set_visible(true);
			}

			debug.direction_node->set_transform(tr);
			//debug.direction_node->set_rotation(Vector3(0, -angle, 0));
			//debug.direction_node->set_translation(Vector3(0, 0, l));
		}
	}
#endif
}

// Using an impulse rather than force because in most cases a jump will be instantaneous,
// and the impulse should not vary according to tick rate.
bool NavPhysicsAgent::agent_jump(real_t p_impulse, bool p_allow_air_jump) {
	if (p_impulse >= 0.0f) {
		if (data.jumping && !p_allow_air_jump) {
			return false;
		}
		data.jumping = true;
		data.jump_height = data.floor_pos.y;
		data.jump_vel = p_impulse;
	} else {
		if (!data.jumping) {
			return false;
		}
		data.jump_vel += p_impulse;
	}

	return true;
}

void NavPhysicsAgent::_iterate_nav_physics() {
	if (!data.started) {
		return;
	}

	// if using avoidance, we need to use a stale velocity
	if (data.avoidance) {
		NavigationServer::get_singleton()->agent_set_position(data.agent, data.floor_pos);
		//NavigationServer::get_singleton()->agent_set_target_velocity(data.agent, data.impulse * Data::ticks_per_second);
		NavigationServer::get_singleton()->agent_set_velocity(data.agent, data.prev_move * Data::ticks_per_second);

		Vector3 avoid_vel = NavigationServer::get_singleton()->agent_force_process_avoidance(data.agent, Data::tick_duration);

		//Vector3 change = data.prev_move - avoid_vel;
		//print_line("avoid vel " + String(avoid_vel));
		//Vector3 change = avoid_vel;

		if (avoid_vel.length_squared() > 0) {
			avoid_vel /= Data::ticks_per_second;

			NavPhysicsServer::get_singleton()->body_add_impulse(data.body, avoid_vel);
			//data.impulse_set = true;
			//data.impulse += avoid_vel;
		}
	}

	// defer sending the impulse in one lot, just in case we add several forces in a tick
	//	if (data.impulse_set) {
	//		NavPhysicsServer::get_singleton()->body_add_impulse(data.body, data.impulse);
	//	}

	/*
	// keep a record of the move since the last position
	data.prev_move = data.floor_pos;
	data.floor_pos = NavPhysicsServer::get_singleton()->body_iterate(data.body);
	data.prev_move = data.floor_pos - data.prev_move;

	//	if (data.impulse_set) {
	//		data.impulse = Vector3();
	//		data.impulse_set = false;
	//	}

	Vector3 pos = data.floor_pos;

	if (data.jumping) {
		data.jump_height += data.jump_vel;
		data.jump_vel -= data.scaled_gravity;

		if (data.jump_height <= pos.y) {
			data.jumping = false;
		} else {
			pos.y = data.jump_height;
		}
	}

	Vector3 old_pos = get_global_translation();
	if (!old_pos.is_equal_approx(pos)) {
		set_global_translation(pos);
	}
	*/
}

void NavPhysicsAgent::_navphysics_done(const Vector3 &p_floor_pos, const Vector3 &p_avoidance, NavPhysics::AgentState p_state) {
	// keep a record of the move since the last position
	data.prev_move = data.floor_pos;
	data.floor_pos = p_floor_pos;
	data.prev_move = data.floor_pos - data.prev_move;
	data.state = p_state;
	data.avoidance_vec = p_avoidance;

	if (p_state == NavPhysics::AGENT_STATE_BLOCKED_BY_NARROWING) {
		uint32_t tick = Engine::get_singleton()->get_physics_frames();
		if (path.blocked_timeout <= tick) {
			path.blocked_timeout = tick + Engine::get_singleton()->get_iterations_per_second();
		}
	}

	Vector3 pos = data.floor_pos;

	if (data.jumping) {
		data.jump_height += data.jump_vel;
		data.jump_vel -= data.scaled_gravity;

		if (data.jump_height <= pos.y) {
			data.jumping = false;
		} else {
			pos.y = data.jump_height;
		}
	}

	Vector3 old_pos = get_global_translation();
	if (!old_pos.is_equal_approx(pos)) {
		set_global_translation(pos);
	}

	if (debug.enabled && debug.label) {
		NavPhysics::BodyInfo info;
		if (NavPhysicsServer::get_singleton()->body_get_info(data.body, info)) {
			// Only change the text of the label when there has been a change.
			// For this we use a checksum.
			uint32_t checksum = info.checksum();

			// also apply the state to the checksum, as we are displaying this also.
			checksum += data.state;

			if (checksum != debug.checksum) {
				debug.checksum = checksum;

				String s = "Poly " + itos(info.p_poly_id);

				switch (data.state) {
					case NavPhysics::AGENT_STATE_BLOCKED_BY_NARROWING: {
						s += " [B]\n";
					} break;
					case NavPhysics::AGENT_STATE_COLLIDING: {
						s += " [C]\n";
					} break;
					default:
						s += "\n";
						break;
				}

				if (info.p_narrowing_id != UINT32_MAX) {
					s += "N " + itos(info.p_narrowing_id) + " [ " + itos(info.p_narrowing_used) + " ( " + itos(info.p_narrowing_available) + " ) ]\n";
				}
				if (info.p_blocking_narrowing_id != UINT32_MAX) {
					debug.label->set_modulate(Color(1, 0.5, 0.5, 1));
					s += "B " + itos(info.p_blocking_narrowing_id) + " [ " + itos(info.p_blocking_narrowing_used) + " ]\n";
				} else {
					debug.label->set_modulate(Color(1, 1, 1, 1));
				}

				debug.label->set_text(s);
			}
		}
	}
}
