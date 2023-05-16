#pragma once

#include "np_ticker.h"
#include "scene/3d/spatial.h"
#include "thirdparty/navphysics/navphysics_defines.h"

class NPPoint;

class NPAgentClass {
public:
};

class NPAgent : public Spatial {
	GDCLASS(NPAgent, Spatial);

public:
	void _agent_callback(const NavPhysics::AgentCallbackData &p_data);

	enum PathResult : u32 {
		PATH_RESULT_MOVING,
		PATH_RESULT_BLOCKED,
		PATH_RESULT_PENDING,
		PATH_RESULT_FINISHED,
		PATH_RESULT_FAILED,
	};

	enum TestResult : u32 {
		TEST_RESULT_CLEAR,
		TEST_RESULT_FAILED,
		TEST_RESULT_INVALID_PARAMETER,
		TEST_RESULT_DIFFERENT_MESH,
		TEST_RESULT_MESH_UNSET,
		TEST_RESULT_BLOCKED_WALL,
	};

	// The orientation of the capsule when on a wall (like a wall climb or spiderman).
	enum UpAlign : u32 {
		UP_ALIGN_MESH, // Up is the current mesh instance up vector.
		UP_ALIGN_MESH_TANGENT, // Up is the closest tangent on the wall that points upward in world space.
		UP_ALIGN_WORLD_UP, // Up is always world space up (simple wall climbing).
	};

	enum Param : u32 {
		PARAM_RADIUS,
		PARAM_HEIGHT,
		PARAM_FRICTION,
		PARAM_GRAVITY,
		PARAM_YAW_MOVEMENT_RESTRICTION,
		PARAM_AIR_FRICTION,
		PARAM_AIR_CONTROL,
		PARAM_AIR_YAW_MOVEMENT_RESTRICTION,
		PARAM_MAX_STEP_UP,
		PARAM_MAX_DROP_DOWN,
		PARAM_MODIFIER_UPHILL,
		PARAM_MODIFIER_DOWNHILL,

		PARAM_MAX,
	};

	enum BoolParam {

		BOOL_PARAM_IS_NPC,
		BOOL_PARAM_PHYSICS_ENABLED,
		BOOL_PARAM_GUARD_INTERNAL_JUMP_LINKS,
		BOOL_PARAM_GUARD_EXTERNAL_JUMP_LINKS,
		BOOL_PARAM_PATHFIND_INTERNAL_JUMP_LINKS,
		BOOL_PARAM_PATHFIND_EXTERNAL_JUMP_LINKS,
		BOOL_PARAM_MAX,
	};
	static_assert(BOOL_PARAM_MAX <= 32, "Increase bit depth of bool param storage.");

	bool guard_internal_jump_links : 1;
	bool pathfind_internal_jump_links : 1;
	bool guard_external_jump_links : 1;
	bool pathfind_external_jump_links : 1;

private:
	friend class NPTicker;

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
		np_handle h_agent;

		// Returned data from the NavPhysics.
		Vector3 impulse_mesh_space;
		Vector3 impulse_world_space;

		// Scene side retains master control of yaw,
		// in order to specify vehicles, and determine
		//rate of turn.
		// Yaw is in MESH space, not world space,
		// therefore needs to change suddenly sometimes when changing mesh.
		float yaw = 0;
		Vector3 yaw_direction_world_space;

		// Up alignment.
		UpAlign up_align = UP_ALIGN_MESH;
		//float up_align_capsule_offset = 0;

		// When transitioning between mesh instances,
		// the agent keeps track of the previous and current,
		// and blends across the basis between them for a few ticks so the orientation looks smooth.
		ObjectID mesh_instance_id_prev = 0;
		ObjectID mesh_instance_id_curr = 0;
		Basis mesh_instance_transition_start_basis;
		uint32_t mesh_instance_transition_ticks = 0;

		float params[PARAM_MAX] = {};
		uint32_t bool_params = 0;

		PathInfo path;
		Waypoint waypoint;

		bool is_active_desired : 1;
		bool is_active_state : 1;

		bool set_bool_param(uint32_t p_shift, bool p_set_or_clear = true) {
			bool old_state = get_bool_param(p_shift);
			if (p_set_or_clear == old_state) {
				return false;
			}

			if (p_set_or_clear) {
				bool_params |= 1 << p_shift;
			} else {
				bool_params &= ~(1 << p_shift);
			}
			return true;
		}
		bool get_bool_param(uint32_t p_shift) const {
			return bool_params & (1 << p_shift);
		}

		Data() {
			set_bool_param(BOOL_PARAM_PATHFIND_INTERNAL_JUMP_LINKS);
			set_bool_param(BOOL_PARAM_PATHFIND_EXTERNAL_JUMP_LINKS);

			set_bool_param(BOOL_PARAM_IS_NPC);

			is_active_desired = true;
			is_active_state = false;

			params[PARAM_RADIUS] = 0.5;
			params[PARAM_HEIGHT] = 2.0;
			params[PARAM_FRICTION] = 0.5;
			params[PARAM_GRAVITY] = 0.5;

			params[PARAM_MODIFIER_UPHILL] = -0.2;
			params[PARAM_MODIFIER_DOWNHILL] = -0.2;

			params[PARAM_AIR_CONTROL] = 0.3;
			params[PARAM_AIR_FRICTION] = 0.2;

			params[PARAM_YAW_MOVEMENT_RESTRICTION] = 0;
			params[PARAM_AIR_YAW_MOVEMENT_RESTRICTION] = 0;

			params[PARAM_MAX_STEP_UP] = 0.5;
			params[PARAM_MAX_DROP_DOWN] = 1.0;
		}
	} data;

	struct Physics {
		RID rid_body;
		RID rid_shape;

		// The collision shape may need some local xform
		// in order to line up with the agent global_xform
		// (which is the base of the agent).
		Transform shape_xform;
	} physics;

	static Transform _dummy_xform;

	void _update_process_mode();
	//void _nav_update();
	double _scale_friction_param(double p_friction, double p_delta) const;
	void _update_params();
	void _update_active(bool p_exiting_tree = false);
	void _update_physics_shape();
	float _shift_yaw(float p_from, float p_to, float p_max_change) const;
	void _default_update_yaw();
	void _nav_physics_update_transform(const Transform &p_xform);
	void _update_physics_enabled(bool p_exiting_tree = false);

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	enum FloorStatus : uint32_t {
		ON_AIR = 0,
		ON_FLOOR,
		ON_AGENT,
	};

	// Utility functions.
	TestResult ray_test_point(const Node *p_np_point) const;
	TestResult ray_test_agent(const Node *p_np_agent) const;

	void nav_teleport(const Vector3 &p_pos);
	void apply_impulse(const Vector3 &p_impulse);
	void apply_jump(const Vector3 &p_impulse);
	FloorStatus is_on_floor() const;

	void set_param(Param p_param, float p_value);
	float get_param(Param p_param) const { return data.params[p_param]; }

	void set_bool_param(BoolParam p_param, bool p_enable);
	bool get_bool_param(BoolParam p_param) const;

	void set_enabled(bool p_enable);
	bool is_enabled() const { return data.is_active_desired; }

	// In mesh space.
	// YAW can either be handled automatically by the agent (to follow movement),
	// or set by the user in order to restrict movement (e.g. for vehicles).
	void set_yaw(float p_yaw);
	void shift_yaw(float p_yaw, float p_max_shift);
	float get_yaw() const { return data.yaw; }

	void set_up_align(UpAlign p_up_align);
	UpAlign get_up_align() const { return data.up_align; }

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

	// If the agent is not currently assigned to a mesh instance, returns ObjectID().
	ObjectID get_mesh_instance_object_id();

	// Intended for debugging visuals, not for path following.
	Vector<Vector3> get_current_path(int p_level) const;

	void free_current_path();

	NPAgent();
	~NPAgent();
};

VARIANT_ENUM_CAST(NPAgent::UpAlign);
VARIANT_ENUM_CAST(NPAgent::TestResult);
VARIANT_ENUM_CAST(NPAgent::PathStatus);
VARIANT_ENUM_CAST(NPAgent::FloorStatus);
VARIANT_ENUM_CAST(NPAgent::PathResult);
VARIANT_ENUM_CAST(NPAgent::Param);
VARIANT_ENUM_CAST(NPAgent::BoolParam);
