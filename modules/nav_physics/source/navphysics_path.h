#pragma once

#include "navphysics_defines.h"
#include "navphysics_pointi.h"
#include "navphysics_pooled_list.h"
#include "navphysics_vector.h"
#include <functional>

namespace NavPhysics {

enum PathStatus : u32 {
	PATH_OK,
	PATH_PENDING,
	PATH_FINISHED,
	PATH_FAILED,
};

enum PathResult : u32 {
	PATH_RESULT_MOVING,
	PATH_RESULT_BLOCKED,
	PATH_RESULT_PENDING,
	PATH_RESULT_FINISHED,
	PATH_RESULT_FAILED,
};

// Scale smallest to largest.

// * Waypoints - individual locations on the mesh instance in local space.
// * Zone - parts of a mesh instance separated by bottlenecks.
// * Links - paths that cover more than one mesh instance, with the external link to cross.
// * Signs -  grand waypoints, between multiple mesh instances (optional).

struct WayPoint {
	//IPoint2 local_pos;
	FPoint3 local_pos3;
	u32 poly_id = UINT32_MAX;
	//u32 entry_wall_id = UINT32_MAX;

	// Linked list, so we can use a pool with no allocations.
	u32 next_id = UINT32_MAX;

	void create() {
		next_id = UINT32_MAX;
	}

	//bool operator==(const WayPoint &p_o) const { return local_pos == p_o.local_pos; }

	static PooledList<WayPoint> _pool_waypoints;
};

struct ZoneWayPoint {
	u32 zone_id = UINT32_MAX;

	// The number of the link based on the zone.
	// This is likely to be a low number as it is added to the first zone_link_id.
	u32 zone_link_id = UINT32_MAX;

	// Direction of the link, from the narrowing to zone, or vice versa.
	//	bool from_narrowing = false;
	//	u32 zone_link_id = UINT32_MAX;

	//	// Narrowing, or zone
	//	u32 client_id = UINT32_MAX;
	//	ZoneType type = ZONE_TYPE_UNDEFINED;

	bool operator==(const ZoneWayPoint &p_o) const {
		//return client_id == p_o.client_id && type == p_o.type;
		return zone_id == p_o.zone_id;
	}
};

struct ZonePoint {
	ZoneWayPoint wp;
	IPoint2 local_pos;
	FPoint3 local_pos3;
	u32 poly_id = UINT32_MAX;

	// Linked list, so we can use a pool with no allocations.
	u32 next_id = UINT32_MAX;

	void create() {
		next_id = UINT32_MAX;
	}

	static PooledList<ZonePoint> _pool_zonepoints;
};

struct LinkPoint {
	np_handle mesh_instance;
	u32 wall_id;
};

struct SignPoint {
	np_handle mesh_instance;
};

template <class T, PooledList<T> &_pool>
class Path {
public:
	struct Data {
		np_handle mesh_instance = 0;

		u32 curr_point_id = UINT32_MAX;
		u32 prev_point_id = UINT32_MAX;
	} data;

	np_handle get_mesh_instance() const { return data.mesh_instance; }
	u32 get_first_point_id() const { return data.curr_point_id; }

	const T *get_point(u32 p_id) const {
		if (p_id == UINT32_MAX) {
			return nullptr;
		}
		return &_pool[p_id];
	}
	const T *get_previous_point() const {
		return get_point(data.prev_point_id);
	}

	const T *get_current_point() const {
		return get_point(data.curr_point_id);
	}
	const T *get_next_point() const {
		const T *curr = get_current_point();
		if (!curr) {
			return nullptr;
		}
		return get_point(curr->next_id);
	}
	const T *get_next_next_point() const {
		const T *next = get_next_point();
		if (!next) {
			return nullptr;
		}
		return get_point(next->next_id);
	}

	bool is_empty() const {
		return this->data.curr_point_id == UINT32_MAX;
	}

	bool increment_current_point() {
		if (this->data.curr_point_id == UINT32_MAX) {
			reset();
			return false;
		}
		u32 next_id = _pool[this->data.curr_point_id].next_id;

		// Free previous waypoint
		if (this->data.prev_point_id != UINT32_MAX) {
			_pool.free(this->data.prev_point_id);
		}

		this->data.prev_point_id = this->data.curr_point_id;
		this->data.curr_point_id = next_id;

		bool finished = this->data.curr_point_id == UINT32_MAX;
		if (finished) {
			reset();
		}
		return !finished;
	}
	void reset(bool p_clear_data = true) {
		// Delete any current points?
		if (p_clear_data) {
			u32 id = this->data.prev_point_id;
			if (id == UINT32_MAX) {
				id = this->data.curr_point_id;
			}

			while (id != UINT32_MAX) {
				u32 next_id = _pool[id].next_id;

				// Free current waypoint
				_pool.free(id);
				id = next_id;
			}
		}

		this->data.curr_point_id = UINT32_MAX;
		this->data.prev_point_id = UINT32_MAX;
	}
};

#if 0
class Path_WayPoint : public Path<WayPoint> {
	friend class Plan;

//	struct Data {
//	} data;

public:


};

class Path_ZonePoint : public Path<ZonePoint, ZonePoint::_pool_zonepoints> {
	struct Data {
		Vector<ZonePoint> points;
	} data;

public:
	u32 size() const { return data.points.size(); }
	const ZonePoint &get_point(u32 p_id) { return data.points[p_id]; }
};

class Path_LinkPoint : public Path<LinkPoint> {
	struct Data {
		Vector<LinkPoint> points;
	} data;

public:
	u32 size() const { return data.points.size(); }
	const LinkPoint &get_point(u32 p_id) { return data.points[p_id]; }
};

class Path_SignPoint : public Path<SignPoint> {
	struct Data {
		Vector<SignPoint> points;
	} data;

public:
	u32 size() const { return data.points.size(); }
	const SignPoint &get_point(u32 p_id) { return data.points[p_id]; }
};
#endif

class Plan {
public:
	enum PlanStatus {
		PLAN_STATUS_NONE,
		PLAN_STATUS_WAYPOINTS,
		PLAN_STATUS_ZONEPOINTS,
	};

private:
	enum InstructionType : u32 {
		IT_NONE,
		IT_AGENT_AGENT,
	};

	struct MarkPoint {
		IPoint2 pt_local;
		FPoint3 pt_local3;
		u32 poly_id = UINT32_MAX;
		np_handle mesh_instance = UINT32_MAX;

		void reset() {
			poly_id = UINT32_MAX;
			mesh_instance = UINT32_MAX;
		}
	};

	struct FullPath {
		Path<WayPoint, WayPoint::_pool_waypoints> waypoints;
		Path<ZonePoint, ZonePoint::_pool_zonepoints> zonepoints;
		PathStatus status = PATH_FAILED;
		PlanStatus plan_status = PLAN_STATUS_NONE;
		void reset() {
			waypoints.reset();
			zonepoints.reset();
			status = PATH_FAILED;
		}
	};

	struct Data {
		u32 waypoint_planner_id = UINT32_MAX;
		u32 zone_planner_id = UINT32_MAX;

		u64 requested_waypoint_planner_issue = 0;
		u64 requested_zone_planner_issue = 0;

		bool planned_waypoint_path = false;
		bool planned_zone_path = false;

		// Store two waypoint paths.
		// The one currently being followed, and the next one being calculated.
		// This allows us to follow a hierarchical path without a pause at each
		// zone to calculate new waypoints - they can be calculated in advance.

		FullPath fullpath;
		FullPath fullpath_next;

		//		Path<WayPoint, WayPoint::_pool_waypoints> waypoints;
		//		Path<WayPoint, WayPoint::_pool_waypoints> waypoints_next;
		//		Path<ZonePoint, ZonePoint::_pool_zonepoints> zonepoints;
		//		PathStatus status = PATH_FAILED;
		//		PathStatus status_next = PATH_FAILED;
		bool reached_waypoint = false;

		// When waiting for repath, we continue on the
		// current path but wait until we get planners from the queue.
		bool waiting_for_repath = false;

		// Which tick we will check next whether we haven't moved sufficiently
		// and have got stuck...
		u64 next_stuck_tick = 0;

		float historical_distance_to_waypoint = 0;

		// This potentially changes as we progress through the path
		// towards the destination.
		np_handle current_mesh_instance = UINT32_MAX;

		// Keep the master data for the start and end point,
		// and the polys etc here.
		MarkPoint start;
		MarkPoint destination;

		void reset(bool p_repath = false);
		void free_waypoint_planner_request();
		void free_zone_planner_request();

		void set_both_status(PathStatus p_status) {
			fullpath.status = p_status;
			fullpath_next.status = p_status;
		}
	} data;

	struct Instruction {
		InstructionType type = IT_NONE;
		np_handle agent_from = UINT32_MAX;
		np_handle agent_to = UINT32_MAX;
	} instruction;

	bool request_waypoint_planner();
	void free_waypoint_planner();
	bool request_zone_planner();
	void free_zone_planner();

	bool _plan_waypoint_path(FullPath &p_zonepoints_source);
	bool _plan_zone_path();

	bool _shift_pending_zone_path();
	bool _shift_pending_waypoint_path();
	bool reached_next_zonepoint();
	bool reached_next_waypoint();

	bool _setup_agent_agent(bool p_repath = false);

	// During a repath, the agent will likely have moved
	// closer to the destination while waiting to get a planner.
	// So we should use the latest position to start the pathfind from,
	// not the position when we first got stuck.
	bool _repath_refresh_start_from_agent(bool p_ensure_same_zone);
	static bool path_active(PathStatus p_ps) {
		return (p_ps == PATH_OK) || (p_ps == PATH_PENDING);
	}

public:
	void create();
	void destroy();
	void reset(bool p_repath = false);

	void iterate_pathfind();
	PathResult iterate_move(const FPoint3 &p_agent_pos, FPoint3 &r_waypoint_pos, u32 &r_waypoint_poly_id);

	//bool get_next_world_pos(FPoint3 &r_pos, u32 &r_poly_id) const;

	PathStatus get_status() const { return data.fullpath.status; }
	PlanStatus get_plan_status() const { return data.fullpath_next.plan_status; }
	bool is_stuck() const { return data.waiting_for_repath; }
	bool has_waypoint_planner() const { return data.waypoint_planner_id != UINT32_MAX; }
	bool has_zone_planner() const { return data.zone_planner_id != UINT32_MAX; }

	void force_repath();

	// Just a helper for debug.
	bool reached_waypoint() const { return data.reached_waypoint; }

	u32 fill_current_world_path(u32 p_level, FPoint3 *r_points, u32 p_max_points) const;

	bool plan_path(np_handle p_mesh_instance_from, IPoint2 p_pos_from, np_handle p_mesh_instance_to, IPoint2 p_pos_to);
	bool plan_path_agent_agent(np_handle p_agent_from, np_handle p_agent_to, bool p_repath = false);
	bool plan_repath();
};

class PlanStore {
	TrackedPooledList<Plan> plans;

public:
	u32 request() {
		u32 id;
		Plan *plan = plans.request(id);
		plan->create();
		return id;
	}
	void free(u32 p_plan_id) {
		get_plan(p_plan_id).destroy();
		plans.free(p_plan_id);
	}
	Plan &get_plan(u32 p_plan_id) {
		return plans[p_plan_id];
	}
	const Plan &get_plan(u32 p_plan_id) const {
		return plans[p_plan_id];
	}

	void iterate();
};

} //namespace NavPhysics
