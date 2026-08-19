#pragma once

#include "scene/3d/spatial.h"
#include "thirdparty/navphysics/navphysics_defines.h"

class NPAgentClass {
public:
};

class NPAgent : public Spatial {
	GDCLASS(NPAgent, Spatial);

public:
	enum PathResult : u32 {
		PATH_RESULT_MOVING,
		PATH_RESULT_BLOCKED,
		PATH_RESULT_PENDING,
		PATH_RESULT_FINISHED,
		PATH_RESULT_FAILED,
	};

private:
	friend class NPMap;

	struct PathInfo {
		// Current path being followed.
		u32 path_id = UINT32_MAX;

		// Detect getting stuck by comparing
		// current pos with historical position.
		//Vector3 historical_pos_global;

		//bool reached_waypoint = false;
	};

	struct Waypoint {
		Vector3 pos;
		uint32_t poly_id = UINT32_MAX;
		bool reached_waypoint = false;

		bool is_set() const { return poly_id != UINT32_MAX; }
		void clear() {
			pos = Vector3();
			poly_id = UINT32_MAX;
			reached_waypoint = false;
		}
	};

	struct Data {
		np_handle h_agent = 0;
		Vector3 vel;

		float yaw = 0;

		float jump_vel = 0;
		float friction = 0.2;
		float gravity = 0.02;
		float radius = 0.5;

		// Modifiers
		float uphill = -0.2;
		float downhill = -0.2;
		float air = 0.3;
		float air_friction = 0.2;

		PathInfo path;
		Waypoint waypoint;

		bool guard_internal_jump_links : 1;
		bool pathfind_internal_jump_links : 1;
		bool guard_external_jump_links : 1;
		bool pathfind_external_jump_links : 1;

		bool is_npc : 1;

		Data() {
			guard_internal_jump_links = false;
			guard_external_jump_links = false;
			pathfind_internal_jump_links = true;
			pathfind_external_jump_links = true;
			is_npc = true;
		}
	} data;

	static Transform _dummy_xform;

	void _update_process_mode();
	//void _nav_update();
	void _update_params();
	float _shift_yaw(float p_from, float p_to, float p_max_change) const;
	void update_yaw();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void nav_teleport(const Vector3 &p_pos);
	void apply_impulse(const Vector3 &p_impulse);
	void apply_jump(float p_impulse);
	bool is_on_floor() const;

	bool get_guard_internal_jump_links() const { return data.guard_internal_jump_links; }
	bool get_guard_external_jump_links() const { return data.guard_external_jump_links; }
	bool get_pathfind_internal_jump_links() const { return data.pathfind_internal_jump_links; }
	bool get_pathfind_external_jump_links() const { return data.pathfind_external_jump_links; }
	bool is_npc() const { return data.is_npc; }

	void set_guard_internal_jump_links(bool p_enable);
	void set_guard_external_jump_links(bool p_enable);
	void set_pathfind_internal_jump_links(bool p_enable);
	void set_pathfind_external_jump_links(bool p_enable);
	void set_npc(bool p_enable);

	void set_radius(float p_radius);
	float get_radius() const { return data.radius; }

	void set_friction(float p_friction);
	float get_friction() const { return data.friction; }

	void set_gravity(float p_gravity);
	float get_gravity() const { return data.gravity; }

	void set_modifier_uphill(float p_value);
	float get_modifier_uphill() const { return data.uphill; }

	void set_modifier_downhill(float p_value);
	float get_modifier_downhill() const { return data.downhill; }

	void set_modifier_air(float p_value);
	float get_modifier_air() const { return data.air; }

	void set_modifier_air_friction(float p_value);
	float get_modifier_air_friction() const { return data.air_friction; }

	Vector3 get_debug_pos(int p_which) const;

	const Transform &get_mesh_instance_transform() const;
	bool move_to_agent(Node *p_agent);

	enum PathStatus {
		PATH_OK,
		PATH_PENDING,
		PATH_FINISHED,
		PATH_FAILED,
	};

	PathStatus get_path_status() const;
	int get_path_plan_status() const;
	bool is_stuck() const;
	bool has_planner(int p_planner_type);

	Vector3 get_next_waypoint();
	bool reached_waypoint() const { return data.waypoint.reached_waypoint; }

	PathResult iterate_path();
	void force_repath();

	// Intended for debugging visuals, not for path following.
	Vector<Vector3> get_current_path(int p_level) const;

	void free_current_path();

	NPAgent();
	~NPAgent();
};

VARIANT_ENUM_CAST(NPAgent::PathStatus);
VARIANT_ENUM_CAST(NPAgent::PathResult);
