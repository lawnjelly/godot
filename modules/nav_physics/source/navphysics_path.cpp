#include "navphysics_path.h"
#include "navphysics_map.h"
#include "navphysics_mesh_instance.h"
#include "navphysics_planner.h"
#include "navphysics_transform.h"
#include "navphysics_zone_planner.h"

// #define NAVPHYSICS_PATH_VERBOSE

namespace NavPhysics {

PooledList<WayPoint> WayPoint::_pool_waypoints;
PooledList<ZonePoint> ZonePoint::_pool_zonepoints;

u32 Plan::fill_current_world_path(u32 p_level, FPoint3 *r_points, u32 p_max_points) const {
	if (get_status() != PATH_OK) {
		return 0;
	}
	const Transform &tr = NPWORLD.safe_get_mesh_instance(data.current_mesh_instance)->get_transform();
	u32 num_points = 0;

	if (p_level == 0) {
		const WayPoint *wp = data.fullpath.waypoints.get_current_point();
		while (wp) {
			FPoint3 &dest = r_points[num_points];
			dest = tr.xform(wp->local_pos3);

			num_points++;
			if (num_points >= p_max_points) {
				break;
			}
			wp = data.fullpath.waypoints.get_point(wp->next_id);
		}
	} else {
		const ZonePoint *wp = data.fullpath.zonepoints.get_current_point();
		while (wp) {
			FPoint3 &dest = r_points[num_points];
			dest = tr.xform(wp->local_pos3);
			num_points++;
			if (num_points >= p_max_points) {
				break;
			}
			wp = data.fullpath.zonepoints.get_point(wp->next_id);
		}
	}

	return num_points;
}

PathResult Plan::iterate_move(const FPoint3 &p_agent_pos, FPoint3 &r_waypoint_pos, u32 &r_waypoint_poly_id) {
	switch (get_status()) {
		case PATH_PENDING: {
			return PATH_RESULT_PENDING;
		}
		case PATH_FINISHED: {
			return PATH_RESULT_FINISHED;
		} break;
		default:
			break;
	}

	if (get_status() != PATH_OK) {
		return PATH_RESULT_FINISHED;
	}

	data.reached_waypoint = false;

	const Transform &tr = NPWORLD.safe_get_mesh_instance(data.current_mesh_instance)->get_transform();
	const WayPoint *wp = data.fullpath.waypoints.get_current_point();
	NP_ERR_FAIL_NULL_V(wp, PATH_RESULT_FINISHED);

	r_waypoint_pos = tr.xform(wp->local_pos3);
	r_waypoint_poly_id = wp->poly_id;

	// Have we reached the waypoint?
	const WayPoint *wp_prev = data.fullpath.waypoints.get_previous_point();

	bool reached = true;

	if (wp_prev) {
		FPoint3 segment[2];
		segment[0] = tr.xform(wp_prev->local_pos3);
		segment[1] = p_agent_pos;

		FPoint3 closest_pt = MeshFuncs::get_closest_point_to_segment(r_waypoint_pos, segment);
		float dist_to_segment = closest_pt.distance_to(r_waypoint_pos);
		reached = dist_to_segment < 0.5f;
	}

	float dist = (r_waypoint_pos - p_agent_pos).length();

	if (reached) {
		//if (dist < 0.5f) {
		// Finished the path?
		if (!reached_next_waypoint()) {
			r_waypoint_pos.zero();
			r_waypoint_poly_id = UINT32_MAX;
#ifdef NAVPHYSICS_PATH_VERBOSE
			log("Finished!");
#endif
			return PATH_RESULT_FINISHED;
		}

		// Call recursively.
		PathResult res = iterate_move(p_agent_pos, r_waypoint_pos, r_waypoint_poly_id);

		//		if (res == PATH_RESULT_MOVING) {
		data.reached_waypoint = true;
		//res = PATH_RESULT_REACHED_WAYPOINT;
		data.historical_distance_to_waypoint = 0;
		//		}

		return res;
	}

	// Detect stuck.
	if (Mesh::_tick >= data.next_stuck_tick) {
		//	if ((Mesh::_tick % (Mesh::_ticks_per_sec * 2)) == 0) {
		data.next_stuck_tick = Mesh::_tick + Mesh::_ticks_per_sec * 2;

		if ((data.historical_distance_to_waypoint != 0) && !data.waiting_for_repath) {
			float change = dist - data.historical_distance_to_waypoint;
			if (change < 0.5f) {
				// Stuck.
#ifdef NAVPHYSICS_PATH_VERBOSE
				log("Stuck!");
#endif
				data.waiting_for_repath = true;
				plan_repath();
#if 0
				plan_repath();
				// Call recursively.
				return iterate_move(p_agent_pos, r_waypoint_pos, r_waypoint_poly_id);
#endif
			}
		}

		data.historical_distance_to_waypoint = dist;
	}

	return PATH_RESULT_MOVING;
}

void Plan::force_repath() {
	if (!data.waiting_for_repath) {
		data.waiting_for_repath = true;
		plan_repath();
	}
}

#if 0
bool Plan::get_next_world_pos(FPoint3 &r_pos, u32 &r_poly_id) const {
	if (get_status() != PATH_OK) {
		//if (!data.ready || data.finished || data.waypoints.is_empty()) {
		r_pos.zero();
		r_poly_id = UINT32_MAX;
		return false;
	}

	const Transform &tr = NPWORLD.safe_get_mesh_instance(data.current_mesh_instance)->get_transform();
	//u32 c = data.waypoints.get_current_point();
	const WayPoint *wp = data.waypoints.get_current_point();
	NP_ERR_FAIL_NULL_V(wp, false);

	//r_pos = tr.xform(data.waypoints.get_point(c).local_pos3);
	r_pos = tr.xform(wp->local_pos3);
	r_poly_id = wp->poly_id;
	return true;
}
#endif

bool Plan::_shift_pending_zone_path() {
	FullPath &dest = data.fullpath;
	FullPath &source = data.fullpath_next;

	dest.zonepoints.reset();

	if (!source.zonepoints.is_empty()) {
		dest.zonepoints = source.zonepoints;
		source.zonepoints.reset(false);
		data.set_both_status(PATH_PENDING);
		return true;
	}

	log("no zone path to shift, finished");
	data.fullpath.status = PATH_FINISHED;
	data.fullpath_next.status = PATH_FINISHED;

	return false;
}

bool Plan::_shift_pending_waypoint_path() {
	FullPath &dest = data.fullpath;
	FullPath &source = data.fullpath_next;

	dest.waypoints.reset();

	if (!source.waypoints.is_empty()) {
		//log("shifting existing waypoint path");
		dest.waypoints = source.waypoints;
		source.waypoints.reset(false);
		data.fullpath.status = PATH_OK;
		data.fullpath_next.status = PATH_PENDING;
		//log("shifting waypoint path");
		return true;
	}

	log("no waypoint path to shift, finished");
	data.fullpath.status = PATH_FINISHED;
	data.fullpath_next.status = PATH_FINISHED;

	return false;
}

bool Plan::reached_next_zonepoint() {
	// Try to move on to the next zone point.
	if (data.fullpath.zonepoints.increment_current_point() && data.fullpath.zonepoints.get_next_point()) {
		// Is there a ready made path ready to move across?
		if (_shift_pending_waypoint_path()) {
			return true;
		}
	}

	// log("No more zonepoints, path finished.");
	data.fullpath.status = PATH_FINISHED;
	data.fullpath_next.status = PATH_FINISHED;
	return false;
}

bool Plan::reached_next_waypoint() {
	if (!data.fullpath.waypoints.increment_current_point()) {
		return reached_next_zonepoint();
	}
	return true;
}

bool Plan::plan_repath() {
	if (instruction.type == IT_AGENT_AGENT) {
		return plan_path_agent_agent(instruction.agent_from, instruction.agent_to, true);
		// Re-establish start and destination.
		//		if (!_setup_agent_agent()) {
		//			return false;
		//		}
	}

	return true;
}

// During a repath, the agent will likely have moved
// closer to the destination while waiting to get a planner.
// So we should use the latest position to start the pathfind from,
// not the position when we first got stuck.
bool Plan::_repath_refresh_start_from_agent(bool p_ensure_same_zone) {
	Agent *agent = NPWORLD.safe_get_body(instruction.agent_from);
	NP_ERR_FAIL_NULL_V(agent, false);

	np_handle h_mi_from = NPWORLD.get_mesh_instance_handle(agent->get_mesh_instance_id());

	if (h_mi_from != data.current_mesh_instance) {
		NP_WARN_PRINT("Repathing has changed mesh instance, not yet supported, backtracking.");
		return false;
	}

	MeshInstance *mi_from = NPWORLD.safe_get_mesh_instance(h_mi_from);
	if (!mi_from) {
		return false;
	}

	const Mesh &mesh_from = mi_from->get_mesh();

	// Only allow changing within the same zone, otherwise it is too complicated,
	// and we will simply backtrack.
	if (p_ensure_same_zone) {
		if (agent->poly_id != UINT32_MAX) {
			u32 new_zone_id = mesh_from.get_poly_extra(agent->poly_id).zone_id;
			u32 old_zone_id = mesh_from.get_poly_extra(data.start.poly_id).zone_id;

			if (new_zone_id != old_zone_id) {
				return false;
			}

		} else {
			return false;
		}
	}

	data.start.poly_id = agent->poly_id;
	data.start.pt_local = agent->pos;
	data.start.pt_local3 = mesh_from.local_point_to_point3(data.start.pt_local, data.start.poly_id);
	return true;
}

bool Plan::_setup_agent_agent(bool p_repath) {
	Agent *agent = NPWORLD.safe_get_body(instruction.agent_from);
	NP_ERR_FAIL_NULL_V(agent, false);

	Agent *agent_to = NPWORLD.safe_get_body(instruction.agent_to);
	NP_ERR_FAIL_NULL_V(agent_to, false);

	if (agent->get_mesh_instance_id() != agent_to->get_mesh_instance_id()) {
		return false;
	}
	if ((agent->poly_id == UINT32_MAX) || (agent_to->poly_id == UINT32_MAX)) {
		return false;
	}

	np_handle h_mi_from = NPWORLD.get_mesh_instance_handle(agent->get_mesh_instance_id());
	np_handle h_mi_to = NPWORLD.get_mesh_instance_handle(agent_to->get_mesh_instance_id());

	MeshInstance *mi_from = NPWORLD.safe_get_mesh_instance(h_mi_from);
	if (!mi_from) {
		return false;
	}

	MeshInstance *mi_to = NPWORLD.safe_get_mesh_instance(h_mi_to);
	if (!mi_to) {
		return false;
	}

	const Mesh &mesh_from = mi_from->get_mesh();
	const Mesh &mesh_to = mi_to->get_mesh();

	data.current_mesh_instance = h_mi_from;

	data.start.mesh_instance = h_mi_from;
	data.start.poly_id = agent->poly_id;
	data.start.pt_local = agent->pos;
	data.start.pt_local3 = mesh_from.local_point_to_point3(data.start.pt_local, data.start.poly_id);

	if (!p_repath) {
		data.destination.mesh_instance = h_mi_to;
		data.destination.poly_id = agent_to->poly_id;
		data.destination.pt_local = agent_to->pos;
		data.destination.pt_local3 = mesh_to.local_point_to_point3(data.destination.pt_local, data.destination.poly_id);
	}

	return true;
}

bool Plan::plan_path_agent_agent(np_handle p_agent_from, np_handle p_agent_to, bool p_repath) {
	reset(p_repath);

	if (p_repath) {
		data.fullpath_next.status = PATH_PENDING;
	} else {
		data.set_both_status(PATH_PENDING);
	}

	//data.status = PATH_PENDING;
	//data.status_next = PATH_PENDING;

	data.fullpath_next.plan_status = PLAN_STATUS_ZONEPOINTS;

	instruction.type = IT_AGENT_AGENT;
	instruction.agent_from = p_agent_from;
	instruction.agent_to = p_agent_to;

	// For now and debugging we will turn off refinding new
	// position of agent at repath.
	if (!_setup_agent_agent(p_repath)) {
		data.set_both_status(PATH_FAILED);
		return false;
	}
	return true;
}

void Plan::Data::reset(bool p_repath) {
	if (!p_repath) {
		fullpath.reset();
		reached_waypoint = false;
		waiting_for_repath = false;

		start.reset();
		destination.reset();
		current_mesh_instance = UINT32_MAX;
	}

	fullpath_next.reset();
	//	waypoints.reset();
	//	waypoints_next.reset();

	free_waypoint_planner_request();
	free_zone_planner_request();

	//status = PATH_FAILED;
	//status_next = PATH_FAILED;

	planned_waypoint_path = false;
	planned_zone_path = false;

	//	zonepoints.reset();

	historical_distance_to_waypoint = 0;
}

bool Plan::_plan_zone_path() {
	if (data.waiting_for_repath) {
		_repath_refresh_start_from_agent(false);
	}

	FullPath &fp = data.fullpath_next;

	fp.zonepoints.reset();
	//data.zonepoints.reset();

	fp.status = PATH_FAILED;
	data.planned_zone_path = true;

	NP_DEV_ASSERT(data.zone_planner_id != UINT32_MAX);
	ZonePlanner &planner = ZonePlanner::_pool_planners[data.zone_planner_id];

	fp.status = planner.pathfind_pos(data.start.mesh_instance, data.start.pt_local, data.destination.pt_local, fp.zonepoints.data.curr_point_id, data.start.poly_id, data.destination.poly_id);

	if (fp.status != PATH_FAILED) {
		return true;
	}
	return false;
}

bool Plan::_plan_waypoint_path(FullPath &p_zonepoints_source) {
	if (data.waiting_for_repath) {
		_repath_refresh_start_from_agent(true);
	}

	FullPath &zp_path = p_zonepoints_source;
	FullPath &wp_path = data.fullpath_next;

	wp_path.waypoints.reset();
	wp_path.status = PATH_FAILED;

	//	data.waypoints_next.reset();
	//	data.status_next = PATH_FAILED;
	data.planned_waypoint_path = true;

	NP_DEV_ASSERT(data.waypoint_planner_id != UINT32_MAX);
	Planner &planner = Planner::_pool_planners[data.waypoint_planner_id];

	// The source zonepoints depend on whether we are on the first segment or not.
	bool on_first_segment = zp_path.waypoints.is_empty();

	// The path is defined by the current zone points, there should be at least two.
	const ZonePoint *curr = on_first_segment ? zp_path.zonepoints.get_current_point() : zp_path.zonepoints.get_next_point();
	const ZonePoint *next = on_first_segment ? zp_path.zonepoints.get_next_point() : zp_path.zonepoints.get_next_next_point();

	// ToDo: Maybe this should be assert.
	if ((!curr) || (!next)) {
		wp_path.status = PATH_FINISHED;
		return false;
	}

	NP_DEV_ASSERT(data.current_mesh_instance != UINT32_MAX);

	wp_path.status = planner.pathfind_pos(data.current_mesh_instance, curr->local_pos, next->local_pos, wp_path.waypoints.data.curr_point_id, curr->poly_id, next->poly_id);

	if (wp_path.status != PATH_FAILED) {
		return true;
	}
	return false;
}

bool Plan::plan_path(np_handle p_mesh_instance_from, IPoint2 p_pos_from, np_handle p_mesh_instance_to, IPoint2 p_pos_to) {
#if 0
	data.waypoints.reset();
	data.mesh_instance = p_mesh_instance_from;

	free_waypoint_planner();
	request_waypoint_planner();

	Planner &planner = Planner::_pool_planners[data.waypoint_planner_id];

	//Planner planner;
	//return planner.pathfind_pos(p_mesh_instance_from, p_pos_from, p_pos_to, data.waypoints.data.points);
	return planner.pathfind_pos(p_mesh_instance_from, p_pos_from, p_pos_to, data.waypoints.data.first_point_id);
#endif
	return false;
}

bool Plan::request_zone_planner() {
	if (data.zone_planner_id == UINT32_MAX) {
		if (!data.requested_zone_planner_issue) {
			data.requested_zone_planner_issue = ZonePlanner::_pool_planners.make_request();

			//log(String("zone_planner_issue : ") + data.requested_zone_planner_issue);
		}

		ZonePlanner *planner = ZonePlanner::_pool_planners.queued_request(data.zone_planner_id, data.requested_zone_planner_issue);

		if (planner) {
			// Super important!
			// Note that the request is no longer valid,
			// so the counts in the pool are kept in sync.
			data.requested_zone_planner_issue = 0;

			planner->create();
			return true;
		} else {
			NP_DEV_ASSERT(data.zone_planner_id == UINT32_MAX);
		}
	}
	return false;
}

bool Plan::request_waypoint_planner() {
	if (data.waypoint_planner_id == UINT32_MAX) {
		if (!data.requested_waypoint_planner_issue) {
			data.requested_waypoint_planner_issue = Planner::_pool_planners.make_request();

			//log(String("waypoint_planner_issue : ") + data.requested_waypoint_planner_issue);
		}

		Planner *planner = Planner::_pool_planners.queued_request(data.waypoint_planner_id, data.requested_waypoint_planner_issue);

		if (planner) {
			// Super important!
			// Note that the request is no longer valid,
			// so the counts in the pool are kept in sync.
			data.requested_waypoint_planner_issue = 0;

			planner->create();
			return true;
		} else {
			NP_DEV_ASSERT(data.waypoint_planner_id == UINT32_MAX);
		}
	}
	return false;
}

void Plan::free_zone_planner() {
	if (data.zone_planner_id != UINT32_MAX) {
		ZonePlanner::_pool_planners[data.zone_planner_id].destroy();
		ZonePlanner::_pool_planners.free(data.zone_planner_id);
		data.zone_planner_id = UINT32_MAX;

		//log("freeing zone_planner");
	}

	data.free_zone_planner_request();
}

void Plan::free_waypoint_planner() {
	if (data.waypoint_planner_id != UINT32_MAX) {
		Planner::_pool_planners[data.waypoint_planner_id].destroy();
		Planner::_pool_planners.free(data.waypoint_planner_id);
		data.waypoint_planner_id = UINT32_MAX;

		//log("freeing waypoint_planner");
	}

	data.free_waypoint_planner_request();
}

void Plan::Data::free_zone_planner_request() {
	if (requested_zone_planner_issue) {
		//ZonePlanner::_pool_planners.cancel_request();
		requested_zone_planner_issue = 0;
	}
}

void Plan::Data::free_waypoint_planner_request() {
	if (requested_waypoint_planner_issue) {
		//Planner::_pool_planners.cancel_request();
		requested_waypoint_planner_issue = 0;
	}
}

void Plan::create() {
}

void Plan::destroy() {
	free_waypoint_planner();
	free_zone_planner();
	reset();
}

void Plan::reset(bool p_repath) {
	//	free_waypoint_planner();
	//	free_zone_planner();
	data.reset(p_repath);
}

void Plan::iterate_pathfind() {
	FullPath &curr = data.fullpath;
	FullPath &next = data.fullpath_next;

#if 0
	if (data.waiting_for_repath) {
		if (data.zone_planner_id == UINT32_MAX) {
			request_zone_planner();
		}
		if (data.waypoint_planner_id == UINT32_MAX) {
			request_waypoint_planner();
		}

		if ((data.zone_planner_id != UINT32_MAX) && (data.waypoint_planner_id != UINT32_MAX)) {
			//if (data.zone_planner_id != UINT32_MAX) {
			plan_repath();
			data.waiting_for_repath = false;
		} else {
			// No more pathfinding while waiting, just follow any waypoints left.
			return;
		}
	}
#endif

	// Zones
	if (next.status == PATH_PENDING) {
		switch (next.plan_status) {
			case PLAN_STATUS_ZONEPOINTS: {
				if (data.zone_planner_id == UINT32_MAX) {
					// Try and allocate a planner.
					if (request_zone_planner()) {
						_plan_zone_path();
						//_plan_path_agent_agent(instruction.agent_from, instruction.agent_to);
					}
				} else {
					if (!data.planned_zone_path) {
						_plan_zone_path();
					}
				}
				if (data.zone_planner_id != UINT32_MAX) {
					ZonePlanner &planner = ZonePlanner::_pool_planners[data.zone_planner_id];
					PathStatus res = planner.iterate(next.zonepoints.data.curr_point_id);
					//data.status = res;

					// If the Zone path was found successfully,
					// move on to plotting the first waypoint path
					// between the first two zone points.
					if (res == PATH_OK) {
						next.plan_status = PLAN_STATUS_WAYPOINTS;
						next.status = PATH_PENDING;

						// Only copy across straight away if we are not waiting for repath.
						if (!data.waiting_for_repath) {
							// Copy the zone path across to curr.
							if (!_shift_pending_zone_path()) {
								log("Shift pending zone path error.");
							}

							// We now are awaiting waypoints.
							curr.status = PATH_PENDING;
							//data.set_both_status(PATH_PENDING);
						}
					}

					// If we have finished with the zone planner,
					// release it so other agents can use it.
					if (res != PATH_PENDING) {
						free_zone_planner();
					}

					if (res == PATH_FAILED) {
						// Could delete the paths here? ToDo
						data.set_both_status(PATH_FAILED);
					}
				}
			} break;

			default: {
			} break;
		}
	}

	// Waypoints
	if (next.status == PATH_PENDING) {
		switch (next.plan_status) {
			case PLAN_STATUS_WAYPOINTS: {
				// Have we reached the end of the zone path?
				// The source zonepoints depend on whether we are on the first segment or not.

				FullPath &source = data.waiting_for_repath ? next : curr;

				bool on_first_segment = source.waypoints.is_empty();
				const ZonePoint *zp_next = on_first_segment ? source.zonepoints.get_next_point() : source.zonepoints.get_next_next_point();
				if (!zp_next) {
					next.status = PATH_FINISHED;
					return;
				}

				if (data.waypoint_planner_id == UINT32_MAX) {
					// Try and allocate a planner.
					if (request_waypoint_planner()) {
						if (!_plan_waypoint_path(source)) {
							free_waypoint_planner();
							return;
						}
					}
				} else {
					if (!data.planned_waypoint_path) {
						if (!_plan_waypoint_path(source)) {
							free_waypoint_planner();
							return;
						}
					}
				}

				if (data.waypoint_planner_id != UINT32_MAX) {
					Planner &planner = Planner::_pool_planners[data.waypoint_planner_id];
					PathStatus res = planner.iterate(next.waypoints.data.curr_point_id);
					next.status = res;

					if (res != PATH_PENDING) {
						free_waypoint_planner();

						// What we do here depends on whether we are on a repath.
						if (data.waiting_for_repath) {
							// Copy both zone path and waypoints to curr.
							_shift_pending_zone_path();
							_shift_pending_waypoint_path();
							data.waiting_for_repath = false;
						} else {
							// If the initial waypoint path is empty, copy there immediately
							// and clear to plan the next waypoint path segment.
							if (curr.waypoints.is_empty()) {
								_shift_pending_waypoint_path();
							} else {
								// If we can't find the next path segment, cancel the current path immediately.
								if (next.status == PATH_FAILED) {
									curr.status = PATH_FAILED;
								}
							}
						} // not a repath
					}
				}
			} break;
			default: {
			} break;
		}
	}
}

void PlanStore::iterate() {
	ZonePlanner::_pool_planners.iterate();
	Planner::_pool_planners.iterate();

	//u32 zone_free = ZonePlanner::_pool_planners.reserved_size() - ZonePlanner::_pool_planners.used_size();
	//log(String("Zone requests queued : ") + ZonePlanner::_pool_planners.get_requests_queued() + ", free " + zone_free);
	//u32 wp_free = Planner::_pool_planners.reserved_size() - Planner::_pool_planners.used_size();
	//log(String("Waypoint requests queued : ") + Planner::_pool_planners.get_requests_queued() + ", free " + wp_free);

	u32 num_active = plans.active_size();
	for (u32 n = 0; n < num_active; n++) {
		plans.get_active(n).iterate_pathfind();
	}
}

} //namespace NavPhysics
