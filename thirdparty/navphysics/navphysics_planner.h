#pragma once

#include "navphysics_path.h"
#include "navphysics_pooled_list.h"
#include "navphysics_structs.h"

namespace NavPhysics {

class Planner {
	static const u32 MAX_PATH_POINTS = 1024;

	struct SortItem {
		u32 point_id = 0;
		//f32 total_cost = 0;
	};

	// Pooled point for A star pathfinds.
	struct PlanPoint {
		u32 poly_id = UINT32_MAX;
		f32 start_cost = 0;
		f32 end_cost = 0;
		f32 total_cost = 0;
		u32 parent_closed_id = UINT32_MAX;
		u32 entry_wall_id = UINT32_MAX;

		bool operator<(const PlanPoint &p_o) const {
			return total_cost < p_o.total_cost;
		}
	};
	static PooledList<PlanPoint> _pool_plan_points;

	// Point for funneling, needs entry wall.
	struct FunnelPoint {
		IPoint2 local_pos;
		FPoint3 local_pos3;
		u32 poly_id = UINT32_MAX;
		u32 entry_wall_id = UINT32_MAX;

		bool operator==(const FunnelPoint &p_o) const { return local_pos == p_o.local_pos; }
		void to_waypoint(WayPoint &r_wp) const {
			r_wp.create();
			r_wp.local_pos3 = local_pos3;
			r_wp.poly_id = poly_id;
		}
	};

	class OpenList {
		Vector<SortItem> sort_list;

	public:
		PlanPoint *find(u32 p_poly_id) {
			for (u32 n = 0; n < sort_list.size(); n++) {
				PlanPoint *pt = &_pool_plan_points[sort_list[n].point_id];
				if (pt->poly_id == p_poly_id) {
					return pt;
				}
			}

			return nullptr;
		}
		void add(const PlanPoint &p) {
			request() = p;
		}
		bool is_empty() const { return sort_list.is_empty(); }

		bool pop_back_and_keep(u32 &r_point_id) {
			if (sort_list.is_empty()) {
				return false;
			}
			SortItem si;
			if (!sort_list.pop_back(si)) {
				return false;
			}

			r_point_id = si.point_id;
			return true;
		}

		bool pop_back(PlanPoint &r_pt) {
			if (sort_list.is_empty()) {
				return false;
			}
			SortItem si;
			if (!sort_list.pop_back(si)) {
				return false;
			}
			r_pt = _pool_plan_points[si.point_id];
			_pool_plan_points.free(si.point_id);
			return true;
		}
		PlanPoint &request() {
			SortItem si;
			PlanPoint *pt = _pool_plan_points.request(si.point_id);
			sort_list.push_back(si);
			return *pt;
		}
		void sort_last();

		void destroy() {
			// Allow abandoning the pathfind,
			// we must return all PlanPoints to the pool.
			PlanPoint dummy;
			while (pop_back(dummy)) {
				;
			}
		}
	};

	struct ClosedList {
		Vector<u32> plan_points;
		Vector<u32> poly_ids;

		void create() {
			NP_DEV_ASSERT(plan_points.is_empty());
			NP_DEV_ASSERT(poly_ids.is_empty());
		}
		void destroy() {
			for (u32 n = 0; n < plan_points.size(); n++) {
				_pool_plan_points.free(plan_points[n]);
			}

			plan_points.clear();
			poly_ids.clear();
		}
	};

	struct Data {
		np_handle mesh_instance = UINT32_MAX;
		OpenList open_list;
		ClosedList closed_list;

		IPoint2 goal_pos; // Centre of poly_end.

		// User specified start and destination.
		IPoint2 start_pos;
		IPoint2 dest_pos;

		u32 poly_start = UINT32_MAX;
		u32 poly_end = UINT32_MAX;
	} data;

	f32 heuristic(const Mesh &p_mesh, const PlanPoint &p, u32 p_poly_end) const;
	f32 cost(const Poly &p_a, const Poly &p_b) const;

	i64 tri_area2(const IPoint2 &p_a, const IPoint2 &p_b, const IPoint2 &p_c) const;
	i64 tri_area(const FunnelPoint *a, const FunnelPoint *b, const FunnelPoint *c) const;
	void calculate_waypoint_pos3(const Mesh &p_mesh, FunnelPoint &r_wp) const;
	void funnel_path(const Mesh &p_mesh, StackVector<FunnelPoint> &r_points);
	bool construct_portal_path(const Mesh &p_mesh, const StackVector<FunnelPoint> &p_source, StackVector<FunnelPoint> &r_dest);

public:
	void create();
	void destroy();

	PathStatus pathfind_pos(np_handle p_mesh_instance, const IPoint2 &p_start, const IPoint2 &p_end, u32 &r_first_waypoint, u32 p_poly_start = UINT32_MAX, u32 p_poly_end = UINT32_MAX);

	PathStatus iterate(u32 &r_first_waypoint, u32 p_iterations_limit = 8);

	// Limited number of planners.
	static const uint32_t NUM_PLANNERS = 2;
	static QueuedPooledList<Planner, NUM_PLANNERS> _pool_planners;
};

} //namespace NavPhysics
