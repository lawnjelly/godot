// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#include "navphysics_zone_planner.h"
#include "navphysics_map.h"
#include "navphysics_mesh.h"
#include "navphysics_mesh_instance.h"
#include "navphysics_string.h"

namespace NavPhysics {

PooledList<ZonePlanner::PlanPoint> ZonePlanner::_pool_plan_points;
QueuedPooledList<ZonePlanner, ZonePlanner::NUM_PLANNERS> ZonePlanner::_pool_planners;

void ZonePlanner::create() {
	NP_DEV_ASSERT(data.mesh_instance == UINT32_MAX);
	NP_DEV_ASSERT(data.open_list.is_empty());
	data.closed_list.create();
}

void ZonePlanner::destroy() {
	data.open_list.destroy();
	data.closed_list.destroy();
	data.mesh_instance = UINT32_MAX;

	data.poly_start = UINT32_MAX;
	data.poly_end = UINT32_MAX;
}

void ZonePlanner::OpenList::sort_last() {
	SortItem si_new = *sort_list.get_last();
	f32 cost_new = _pool_plan_points[si_new.point_id].total_cost;

	// Find where to insert
	u32 insert_pos = UINT32_MAX;

	for (u32 n = 0; n < sort_list.size() - 1; n++) {
		f32 cost = _pool_plan_points[sort_list[n].point_id].total_cost;

		if (cost_new > cost) {
			insert_pos = n;
			break;
		}
	}

	if (insert_pos == UINT32_MAX) {
		// No change.
		return;
	}

	// Move the rest.
	u32 elements_to_move = sort_list.size() - insert_pos - 1;
	memmove(&sort_list[insert_pos + 1], &sort_list[insert_pos], sizeof(SortItem) * elements_to_move);

	// Insert
	sort_list[insert_pos] = si_new;
}

f32 ZonePlanner::cost(const FPoint3 &p_a, const FPoint3 &p_b) const {
	return (p_b - p_a).length();
}

f32 ZonePlanner::heuristic(const Mesh &p_mesh, const PlanPoint &p) const {
	// const Zone &zone = p_mesh.get_zone(p.info.zone_id);
	// return (data.goal_pos - zone.local_pos3).length();
	return (data.goal_pos - p.pos).length();
}

void ZonePlanner::calculate_waypoint_pos3(const Mesh &p_mesh, ZonePoint &r_wp) const {
	r_wp.local_pos3 = p_mesh.local_point_to_point3(r_wp.local_pos, r_wp.poly_id);
}

void ZonePlanner::fill_zonepoint(const Mesh &p_mesh, ZonePoint &r_zone_point, const ZonePoint &p_zone_point_prev) const {
	NP_DEV_ASSERT(r_zone_point.wp.zone_link_id != UINT32_MAX);
#if 0
	const Zone &zone = p_mesh.get_zone(r_zone_point.wp.zone_id);
	const Zone &zone_prev = p_mesh.get_zone(p_zone_point_prev.wp.zone_id);

	// Zone link child is the zone child ID for the previous node.
	u32 zone_link_child = r_zone_point.wp.zone_link_id;

	u32 area_link_id = UINT32_MAX;
	switch (zone_prev.type) {
		default: {
			NP_DEV_ASSERT(0);
		} break;
		case ZoneType::ZONE_TYPE_AREA: {
			const Area &area = p_mesh._areas[zone_prev.area_or_narrowing_id];
			area_link_id = area.area_links[zone_link_child];

			// const AreaLink &area_link = p_mesh._area_links[area_link_id];
			// r_zone_point.poly_id = area_link.narrowing_poly_id;
		} break;
		case ZoneType::ZONE_TYPE_NARROWING: {
			const Narrowing &narr = p_mesh._narrowings[zone.area_or_narrowing_id];
			area_link_id = narr.area_links[zone_link_child];

			// const AreaLink &area_link = p_mesh._area_links[area_link_id];
			// r_zone_point.poly_id = area_link.area_poly_id;
		} break;
	}

	const AreaLink &area_link = p_mesh._area_links[area_link_id];
	r_zone_point.poly_id = area_link.crossing_on_area_poly ? area_link.area_poly_id : area_link.narrowing_poly_id;
	r_zone_point.local_pos = area_link.pt_crossing;
	r_zone_point.local_pos3 = p_mesh.local_point_to_point3(area_link.pt_crossing, r_zone_point.poly_id);
#endif

	// The link from the previous zone to the current zone.
	const ZoneLink &zl = p_mesh._zone_links[r_zone_point.wp.zone_link_id];

	r_zone_point.poly_id = zl.zone_to_poly_id;
	r_zone_point.local_pos = zl.pt_crossing;
	r_zone_point.local_pos3 = p_mesh.local_point_to_point3(r_zone_point.local_pos, r_zone_point.poly_id);

	//r_zone_point.local_pos = p_mesh.get_poly(r_zone_point.poly_id).center;
	//r_zone_point.local_pos3 = p_mesh.get_poly(r_zone_point.poly_id).center3;
}

void ZonePlanner::finalize_path(const Mesh &p_mesh, StackVector<ZonePoint> &r_waypoints) const {
	if (r_waypoints.size() <= 1) {
		return;
	}

#if 0
	// Debug print path
	log("Zonepath:");
	for (u32 n = 0; n < r_waypoints.size(); n++) {
		const ZonePoint &zp = r_waypoints[n];
		log(String("\t") + n + " :\tzone_id " + zp.wp.zone_id + ", poly_id " + zp.poly_id + ", pos " + zp.local_pos);
	}
#endif

	// Find the crossing points between areas and narrowings.
	// Only the first and last point are known on entering this routine.
	for (u32 n = 1; n < r_waypoints.size() - 1; n++) {
		fill_zonepoint(p_mesh, r_waypoints[n], r_waypoints[n - 1]);
	}
}

PathStatus ZonePlanner::iterate(u32 &r_first_waypoint, u32 p_iterations_limit) {
	NP_ERR_FAIL_COND_V(data.mesh_instance == UINT32_MAX, PATH_FAILED);

	MeshInstance *mi = NPWORLD.safe_get_mesh_instance(data.mesh_instance);
	if (!mi) {
		return PATH_FAILED;
	}
	const Mesh &mesh = mi->get_mesh();

	u32 iterations_counter = p_iterations_limit;

	while (!data.open_list.is_empty()) {
		// Limit how much processing on each frame.
		if (iterations_counter-- == 0) {
			return PATH_PENDING; // More to do on next frame.
		}

		u32 popped_id = 0;
		data.open_list.pop_back_and_keep(popped_id);

		const PlanPoint *pp = &_pool_plan_points[popped_id];
		// log(String("popping zone ") + p.info.zone_id + " from open list");

		// Add all popped to the closed list.
		data.closed_list.plan_points.push_back(popped_id);
		data.closed_list.point_infos.push_back(pp->info);

		if (pp->info == data.end_info) {
			// Finished path.
			StackVector<ZonePoint> waypoints;
			waypoints.setup_external((ZonePoint *)alloca(sizeof(ZonePoint) * MAX_PATH_POINTS), MAX_PATH_POINTS);

			// Add in reverse...

			{
				// Last point.
				ZonePoint wp;
				wp.create();
				wp.wp = data.end_info;

				wp.poly_id = data.poly_end;
				wp.local_pos = data.dest_pos;
				calculate_waypoint_pos3(mesh, wp);
				waypoints.push_back(wp);
			}

			// Skip first and the last here can be removed from the list, because they
			// are specified exactly.
			pp = pp->parent_closed_id == UINT32_MAX ? nullptr : &_pool_plan_points[data.closed_list.plan_points[pp->parent_closed_id]];

			while (pp) {
				// log(String("intermediate pp ") + pp->info.zone_id + ", start cost: " + pp->start_cost + ", end_cost: " + pp->end_cost + ", total_cost: " + pp->total_cost);

				ZonePoint wp;
				wp.create();
				wp.wp = pp->info;

				waypoints.push_back(wp);

				pp = pp->parent_closed_id == UINT32_MAX ? nullptr : &_pool_plan_points[data.closed_list.plan_points[pp->parent_closed_id]];

				if (!pp) {
					// Remove the last (actually the first because we are in reverse)
					// because we have more info to add.
					waypoints.resize(waypoints.size() - 1);
				}
			}

			{
				// First point.
				ZonePoint wp;
				wp.create();
				wp.wp.zone_id = data.zone_start;

				wp.poly_id = data.poly_start;
				wp.local_pos = data.start_pos;
				calculate_waypoint_pos3(mesh, wp);
				waypoints.push_back(wp);
			}

			waypoints.invert();

#if 0
			String sz = "Path : ";
			for (u32 n = 0; n < r_points.size(); n++) {
				sz += String(r_points[n].poly_id) + ", ";
			}

			log(sz);
#endif

			// Translate the stack vector into linked list of waypoints.
			if (!waypoints.size()) {
				return PATH_FAILED;
			}

			finalize_path(mesh, waypoints);

			// WayPoint::_pool_waypoints._debug = true;

			ZonePoint *wp = ZonePoint::_pool_zonepoints.request(r_first_waypoint);
			*wp = waypoints[0];
			for (u32 n = 1; n < waypoints.size(); n++) {
				ZonePoint *next = ZonePoint::_pool_zonepoints.request(wp->next_id);
				*next = waypoints[n];
				wp = next;
			}

			//log(String("Zonepath found of length ") + waypoints.size() + ".");
			return PATH_OK;
		}

		// Get the neighbours.
		const Zone &zone = mesh.get_zone(pp->info.zone_id);

		for (u32 w = 0; w < zone.num_links; w++) {
			u32 zone_link_id = zone.first_link + w;

			const ZoneLink &zone_link = mesh._zone_links[zone_link_id];
			u32 zone_to_id = zone_link.zone_to_id;

			// Already on closed list?
			if (data.closed_list.contains(zone_to_id)) {
				continue;
			}

			float tentative_start_cost = pp->start_cost + cost(pp->pos, zone_link.pt_crossing3);

			// Does open list contain this zone already?
			PlanPoint *found = data.open_list.find(zone_to_id, zone_link_id);

			if (!found) {
				// POTENTIAL BUG!!!
				// ToDo: Check this in the waypoints also.
				// Watch out this may invalidate the data in pp
				// so it contains garbage.
				found = &data.open_list.request();

				// Reget the popped ID because the pool might have grown
				// and invalidated pp.
				pp = &_pool_plan_points[popped_id];
			} else {
				if (tentative_start_cost >= found->start_cost) {
					found = nullptr;
				}
			}

			if (found) {
				// Note that the zone link child is for the PREVIOUS node!
				// This can be kind of confusing. This ZonePoint looks up the area link
				// from the ZonePoint previous.
				found->info.zone_link_id = zone_link_id;

				found->info.zone_id = zone_to_id;
				found->parent_closed_id = data.closed_list.plan_points.size() - 1;
				found->start_cost = tentative_start_cost;

				found->pos = zone_link.pt_crossing3;

				// Must set up found->pos (the crossing point) prior to finding the heuristic.
				found->end_cost = heuristic(mesh, *found);
				found->total_cost = found->start_cost + found->end_cost;

				data.open_list.sort_last();
			}
		}
	}

	// No path found.
	log("Zonepath not found.");
	return PATH_FAILED;
}

PathStatus ZonePlanner::pathfind_pos(np_handle p_mesh_instance, const IPoint2 &p_start, const IPoint2 &p_end, u32 &r_first_waypoint, u32 p_poly_start, u32 p_poly_end) {
	data.mesh_instance = p_mesh_instance;
	r_first_waypoint = UINT32_MAX;

	MeshInstance *mi = NPWORLD.safe_get_mesh_instance(data.mesh_instance);
	if (!mi) {
		return PATH_FAILED;
	}
	const Mesh &mesh = mi->get_mesh();

	if (p_poly_start == UINT32_MAX) {
		p_poly_start = mesh.find_poly_within(p_start);
		if (p_poly_start == UINT32_MAX) {
			return PATH_FAILED;
		}
	}
	if (p_poly_end == UINT32_MAX) {
		p_poly_end = mesh.find_poly_within(p_end);
		if (p_poly_end == UINT32_MAX) {
			return PATH_FAILED;
		}
	}

	// Store for iteration
	data.poly_start = p_poly_start;
	data.poly_end = p_poly_end;
	data.start_pos = p_start;
	data.dest_pos = p_end;

	data.start_pos3 = mesh.local_point_to_point3(p_start, p_poly_start);

	//const Poly &poly_end = mesh.get_poly(p_poly_end);
	const PolyExtra &ex_start = mesh.get_poly_extra(p_poly_start);
	const PolyExtra &ex_end = mesh.get_poly_extra(p_poly_end);

	data.zone_start = ex_start.zone_id;
	data.end_info.zone_id = ex_end.zone_id;

	// Goal is the actual end position, NOT the centre of the end zone.
	data.goal_pos = mesh.local_point_to_point3(p_end, p_poly_end);

	// Seed with first point.
	PlanPoint p;
	p.info.zone_id = data.zone_start;
	p.pos = data.start_pos3;

	p.end_cost = heuristic(mesh, p);
	p.total_cost = p.end_cost;

	//log(String("path start to goal cost :") + p.total_cost);

	data.open_list.add(p);

	return PATH_PENDING;

	//	PathStatus result = iterate(r_first_waypoint);
	//	return result;
}

} //namespace NavPhysics
