#pragma once

#include "scene/3d/spatial.h"
#include "servers/nav_physics/np_defines.h"

class Label3D;
class ImmediateGeometry;

class NavPhysicsAgent : public Spatial {
	GDCLASS(NavPhysicsAgent, Spatial);

public:
	enum PathResult : unsigned int {
		PATH_RESULT_SUCCESS,
		PATH_RESULT_STUCK,
	};

private:
	struct Data {
		RID agent;
		RID map;
		np_handle body = 0;

		Vector3 floor_pos;
		Vector3 avoidance_vec;
		//Vector3 impulse; // current impulse total this tick
		Vector3 prev_move;
		bool jumping : 1;
		bool started : 1;
		//bool impulse_set : 1;
		bool avoidance : 1;
		bool avoidance_ignore_y : 1;
		bool path_auto_follow : 1;
		NavPhysics::AgentState state : 4;

		real_t jump_height = 0.0f;
		real_t jump_vel = 0.0f;
		real_t gravity = 4.0f;
		real_t friction = 0.5f;
		real_t scaled_gravity = 0.1f;
		Data() {
			jumping = false;
			started = false;
			//impulse_set = false;
			avoidance = false;
			avoidance_ignore_y = true;
			path_auto_follow = true;
			state = NavPhysics::AGENT_STATE_CLEAR;
		}
		static int ticks_per_second;
		static real_t tick_duration;
	} data;

	struct Avoidance {
		real_t neighbor_dist = 50.0f;
		uint32_t max_neighbours = 10;
		real_t time_horizon = 5.0f;
		real_t radius = 1.0f;
		real_t max_speed = 10.0f;
		bool ignore_narrowings = false;
	} avoidance;

	struct Path {
		struct PathPoint {
			bool shifted = false;
		};
		LocalVector<Vector3> pts;
		LocalVector<PathPoint> path_pts;
		uint32_t curr = 0;
		void clear() {
			pts.clear();
			path_pts.clear();
			curr = 0;
		}
		bool is_finished() const { return curr >= pts.size(); }
		const Vector3 &get_current_waypoint() const {
			DEV_ASSERT(!is_finished());
			return pts[curr];
		}

		// path following behavior
		real_t speed = 6.0f;
		real_t waypoint_threshold = 0.2f;
		real_t waypoint_threshold_squared = 1.0f;

		// periodic trace to the next waypoint, to check for being stuck
		// and needing a repath if blocked...
		uint32_t trace_timeout = 0;

		// when blocked by a narrowing, stop pathfollowing for a set interval, to
		// allow being pushed aside more easily
		uint32_t blocked_timeout = 0;

		Vector3 stuck_record;
		uint32_t stuck_timeout = 0;

		PathResult result : 2;
		bool repathing : 1;
		Path() {
			result = PATH_RESULT_SUCCESS;
			repathing = false;
		}

	} path;

	struct Debug {
		bool enabled = false;
		uint32_t checksum = 0;
		Label3D *label = nullptr;
		Spatial *direction_node = nullptr;
		ImmediateGeometry *imm = nullptr;
	} debug;

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_navigation_map(RID p_navigation_map);
	RID get_navigation_map() const;

	// SETTINGS

	// Physics
	void set_physics_gravity(real_t p_gravity);
	real_t get_physics_gravity() const { return data.gravity; }
	void set_physics_friction(real_t p_friction);
	real_t get_physics_friction() const { return data.friction; }

	// Pathfinding
	void set_path_speed(real_t p_speed);
	real_t get_path_speed() const { return path.speed; }
	void set_path_waypoint_threshold(real_t p_distance);
	real_t get_path_waypoint_threshold() const { return path.waypoint_threshold; }
	void set_path_auto_follow(bool p_enabled) { data.path_auto_follow = p_enabled; }
	bool get_path_auto_follow() const { return data.path_auto_follow; }
	PathResult get_path_result() const { return path.result; }

	// Avoidance
	void set_avoidance_enabled(bool p_enabled);
	bool get_avoidance_enabled() const { return data.avoidance; }
	void set_avoidance_neighbor_dist(real_t p_distance);
	real_t get_avoidance_neighbor_dist() const { return avoidance.neighbor_dist; }
	void set_avoidance_max_neighbors(int p_max_neighbors);
	int get_avoidance_max_neighbors() const { return avoidance.max_neighbours; }
	void set_avoidance_time_horizon(real_t p_time_horizon);
	real_t get_avoidance_time_horizon() const { return avoidance.time_horizon; }
	void set_avoidance_radius(real_t p_radius);
	real_t get_avoidance_radius() const { return avoidance.radius; }
	void set_avoidance_max_speed(real_t p_speed);
	real_t get_avoidance_max_speed() const { return avoidance.max_speed; }
	void set_avoidance_ignore_y(bool p_ignore_y);
	bool get_avoidance_ignore_y() const { return data.avoidance_ignore_y; }
	void set_avoidance_ignore_narrowings(bool p_enabled);
	bool get_avoidance_ignore_narrowings() const { return avoidance.ignore_narrowings; }

	void set_debug_direction_node(Node *p_node);

	// FUNCS

	// Physics
	const Vector3 &agent_get_position() const;
	void agent_teleport(const Vector3 &p_position);
	void agent_add_force(const Vector3 &p_force);
	bool agent_jump(real_t p_impulse, bool p_allow_air_jump = false);
	bool agent_is_jumping() const { return data.jumping; }
	const Vector3 &agent_get_push() const;
	Vector3 agent_choose_random_location() const;

	// Pathfinding
	bool path_move_to(const Vector3 &p_destination);
	void path_stop();
	bool path_finished() const { return path.is_finished(); }

	real_t path_get_distance_to_next_waypoint() const;
	const Vector3 &path_get_next_waypoint() const;

	void _navphysics_done(const Vector3 &p_floor_pos, const Vector3 &p_avoidance, NavPhysics::AgentState p_state);

	NavPhysicsAgent();
	virtual ~NavPhysicsAgent();

private:
	void _refresh_map();
	void _refresh_parameters();
	void _update_path(int p_depth = 0);
	void _debug_update_next_waypoint_display();
	void _iterate_nav_physics();
};

VARIANT_ENUM_CAST(NavPhysics::AgentState);
VARIANT_ENUM_CAST(NavPhysicsAgent::PathResult);
