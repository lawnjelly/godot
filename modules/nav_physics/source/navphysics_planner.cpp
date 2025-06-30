#include "navphysics_planner.h"
#include "navphysics_map.h"
#include "navphysics_mesh.h"
#include "navphysics_mesh_instance.h"
#include "navphysics_string.h"

// #define NAVPHYSICS_WAYPOINT_PLANNER_VERBOSE

namespace NavPhysics {

PooledList<Planner::PlanPoint> Planner::_pool_plan_points;
QueuedPooledList<Planner, Planner::NUM_PLANNERS> Planner::_pool_planners;

void Planner::create() {
	NP_DEV_ASSERT(data.mesh_instance == UINT32_MAX);
	NP_DEV_ASSERT(data.open_list.is_empty());
	data.closed_list.create();
}

void Planner::destroy() {
	data.open_list.destroy();
	data.closed_list.destroy();
	data.mesh_instance = UINT32_MAX;

	data.poly_start = UINT32_MAX;
	data.poly_end = UINT32_MAX;
}

void Planner::OpenList::sort_last() {
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

#ifdef NAVPHYSICS_WAYPOINT_PLANNER_VERBOSE
	log("sorted openlist:");
	for (u32 n = 0; n < sort_list.size(); n++) {
		const PlanPoint &pp = _pool_plan_points[sort_list[n].point_id];
		log(String("\t") + n + " : point_id " + sort_list[n].point_id + ", total_cost: " + pp.total_cost + ", poly_id: " + pp.poly_id);
	}
#endif
}

f32 Planner::cost(const Poly &p_a, const Poly &p_b) const {
	return (p_b.center - p_a.center).lengthf();
}

f32 Planner::heuristic(const Mesh &p_mesh, const PlanPoint &p, u32 p_poly_end) const {
	const Poly &poly = p_mesh.get_poly(p.poly_id);
	return (data.goal_pos - poly.center).lengthf();
}

i64 Planner::tri_area(const FunnelPoint *a, const FunnelPoint *b, const FunnelPoint *c) const {
	return -tri_area2(a->local_pos, b->local_pos, c->local_pos);
}

i64 Planner::tri_area2(const IPoint2 &p_a, const IPoint2 &p_b, const IPoint2 &p_c) const {
	IPoint2 edge_a = p_b - p_a;
	IPoint2 edge_b = p_c - p_a;
	return edge_b.cross(edge_a);
}

bool Planner::construct_portal_path(const Mesh &p_mesh, const StackVector<FunnelPoint> &p_source, StackVector<FunnelPoint> &r_dest) {
	const StackVector<FunnelPoint> &s = p_source;
	StackVector<FunnelPoint> &d = r_dest;

	if (s.size() <= 2) {
		return false;
	}

	// Copy orig path.
	// And expand to edges of portals.
	u32 num_portals = s.size() - 1;

	d.resize(4 + (num_portals * 2));

	// Start and last position are considered within a poly.
	d[0] = s[0];
	d[1] = s[0];
	//d[1] = s[1];
	d.last() = s.last();
	d.last(1) = s.last();
	//d.last(1) = s.last(1);

	// Add each portal.
	for (u32 n = 0; n < num_portals; n++) {
		FunnelPoint &dl = d[2 + n * 2];
		FunnelPoint &dr = d[3 + n * 2];

		//const WayPoint &sw_before = s[n];
		const FunnelPoint &sw = s[1 + n];

		// Copy poly id and entry wall.
		dl = sw;
		dr = sw;

		// Get FP coords.
		NP_DEV_ASSERT(sw.entry_wall_id != UINT32_MAX);
		p_mesh.get_wall_verts(sw.entry_wall_id, dl.local_pos, dr.local_pos);

		// Get the two verts of the portal on the correct side
		// relative to the path that is being travelled...
		i64 cross_l = IPoint2::which_side(s[n].local_pos, s[n + 1].local_pos, dl.local_pos);
		i64 cross_r = IPoint2::which_side(s[n].local_pos, s[n + 1].local_pos, dr.local_pos);

		if (cross_l > cross_r) {
			SWAP(dl.local_pos, dr.local_pos);
		}

		// Calc 3D coords.
		dl.local_pos3 = p_mesh.fixed_point_to_float_3(dl.local_pos);
		dr.local_pos3 = p_mesh.fixed_point_to_float_3(dr.local_pos);

		dl.local_pos3.y = p_mesh.find_height_on_poly_plane(sw.poly_id, dl.local_pos);
		dr.local_pos3.y = p_mesh.find_height_on_poly_plane(sw.poly_id, dr.local_pos);
	}

	// Need the entry point for the last point, for funnel algorithm.
	// d.last()->entry_wall_id = d[d.size()-2].entry_wall_id;

	// debug print
#if 0
	for (u32 n = 0; n < d.size(); n++) {
		const FunnelPoint &wp = d[n];
		log(String("\tportal funnelpoint ") + n + " : " + (wp.local_pos / 1000));
	}
#endif

	return true;
}

// Some hints for general technique:
// https://digestingduck.blogspot.com/2010/03/simple-stupid-funnel-algorithm.html
void Planner::funnel_path(const Mesh &p_mesh, StackVector<FunnelPoint> &r_points) {
//#define NP_PLANNER_EXPORT_SVG_ORIG
#ifdef NP_PLANNER_EXPORT_SVG_ORIG
	{
		Vector<Mesh::SVGPoint> svg_points;
		Vector<u32> svg_polys;
		Vector<u32> svg_walls;

		for (u32 n = 0; n < r_points.size(); n++) {
			const WayPoint &wp = r_points[n];
			if (wp.poly_id != UINT32_MAX) {
				svg_polys.check_and_push_back(wp.poly_id);
			}

			Mesh::SVGPoint pt;
			pt.pos = wp.local_pos;
			svg_points.push_back(pt);
		}

		p_mesh.svg_export_custom("../plan_orig.svg", svg_walls, svg_polys, svg_points);
	}
#endif

//#define NP_PLANNER_EXPORT_SVG
#ifdef NP_PLANNER_EXPORT_SVG
	Vector<Mesh::SVGPoint> svg_points;
	Vector<u32> svg_polys;
	Vector<u32> svg_walls;

	if (r_points.size() >= 2) {
		svg_polys.check_and_push_back(r_points[0].poly_id);
		svg_polys.check_and_push_back(r_points[1].poly_id);
	}
#endif

	StackVector<FunnelPoint> path;
	path.setup_external((FunnelPoint *)alloca(sizeof(FunnelPoint) * MAX_PATH_POINTS), MAX_PATH_POINTS);

	//if (!construct_portal_path(p_mesh, r_points, path)) {
	if (!construct_portal_path(p_mesh, r_points, path)) {
		return;
	}

//#define NP_PLANNER_EXPORT_SVG_PORTAL
#ifdef NP_PLANNER_EXPORT_SVG_PORTAL
	{
		Vector<Mesh::SVGPoint> svg_points;
		Vector<u32> svg_polys;
		Vector<u32> svg_walls;

		for (u32 n = 0; n < path.size(); n++) {
			const WayPoint &wp = path[n];
			if (wp.poly_id != UINT32_MAX) {
				svg_polys.check_and_push_back(wp.poly_id);
			}

			Mesh::SVGPoint pt;
			pt.pos = wp.local_pos;
			svg_points.push_back(pt);
		}

		p_mesh.svg_export_custom("../plan_portal.svg", svg_walls, svg_polys, svg_points);
	}
#endif

	u32 num_portals = path.size() / 2;
	r_points.clear();

	i32 apex_index = 0;
	i32 left_index = 0;
	i32 right_index = 0;

	const FunnelPoint *portal_apex = &path[0];
	const FunnelPoint *portal_left = &path[0];
	const FunnelPoint *portal_right = &path[1];

	// Add first points.
	r_points.push_back(path[0]);

	for (u32 i = 1; i < num_portals; i++) {
		const FunnelPoint *left = &path[(i * 2) + 0];
		const FunnelPoint *right = &path[(i * 2) + 1];

#ifdef NP_PLANNER_EXPORT_SVG
		if (left->entry_wall_id != UINT32_MAX) {
			svg_walls.check_and_push_back(left->entry_wall_id);
		}

		svg_polys.check_and_push_back(left->poly_id);
#endif

		// Update the right vertex
		i64 tri_area_right = tri_area(portal_apex, portal_right, right);
		if (tri_area_right <= 0) {
			i64 tri_area_right = tri_area(portal_apex, portal_left, right);
			if ((*portal_apex == *portal_right) || (tri_area_right > 0)) {
				// Tighten the funnel.
				portal_right = right;
				right_index = i;
			} else {
				// Right over left, insert left to path and
				// restart scan from portal left point.
				r_points.push_back(*portal_left);

				// Make current left the new apex.
				portal_apex = portal_left;
				apex_index = left_index;

				// Reset portal.
				portal_left = portal_apex;
				portal_right = portal_apex;
				left_index = apex_index;
				right_index = apex_index;

				// Restart scan
				i = apex_index;
				continue;
			}
		}

		// Update left vertex.
		i64 tri_area_left = tri_area(portal_apex, portal_left, left);
		if (tri_area_left >= 0) {
			i64 tri_area_left2 = tri_area(portal_apex, portal_right, left);
			if ((*portal_apex == *portal_left) || (tri_area_left2 < 0)) {
				// Tighten the funnel.
				portal_left = left;
				left_index = i;
			} else {
				// Left over right, insert right to path and
				// restart scan from portal right point.
				r_points.push_back(*portal_right);

				// Make current right the new apex.
				portal_apex = portal_right;
				apex_index = right_index;

				// Reset portal.
				portal_left = portal_apex;
				portal_right = portal_apex;
				left_index = apex_index;
				right_index = apex_index;

				// Restart scan
				i = apex_index;
				continue;
			}
		}
	}

	// Append last point to path.
	r_points.push_back(path[path.size() - 1]);

#ifdef NP_PLANNER_EXPORT_SVG
	for (u32 n = 0; n < r_points.size(); n++) {
		Mesh::SVGPoint pt;
		pt.pos = r_points[n].local_pos;
		svg_points.push_back(pt);
	}

	p_mesh.svg_export_custom("../plan.svg", svg_walls, svg_polys, svg_points);
#endif
}

void Planner::calculate_waypoint_pos3(const Mesh &p_mesh, FunnelPoint &r_wp) const {
	r_wp.local_pos3 = p_mesh.local_point_to_point3(r_wp.local_pos, r_wp.poly_id);
}

PathStatus Planner::iterate(u32 &r_first_waypoint, u32 p_iterations_limit) {
	NP_ERR_FAIL_COND_V(data.mesh_instance == UINT32_MAX, PATH_FAILED);

	MeshInstance *mi = NPWORLD.safe_get_mesh_instance(data.mesh_instance);
	if (!mi) {
		return PATH_FAILED;
	}
	const Mesh &mesh = mi->get_mesh();

//#define NAVPHYSICS_SHORT_WAYPOINT_PLANNER
#ifdef NAVPHYSICS_SHORT_WAYPOINT_PLANNER
	WayPoint *wp = WayPoint::_pool_waypoints.request(r_first_waypoint);
	wp->create();
	wp->poly_id = data.poly_start;
	wp->local_pos3 = mesh.local_point_to_point3(data.start_pos, wp->poly_id);

	WayPoint *next = WayPoint::_pool_waypoints.request(wp->next_id);
	wp = next;
	wp->create();
	wp->poly_id = data.poly_end;
	wp->local_pos3 = mesh.local_point_to_point3(data.dest_pos, wp->poly_id);

	return PATH_OK;

#else

	u32 iterations_counter = p_iterations_limit;

	while (!data.open_list.is_empty()) {
		// Limit how much processing on each frame.
		if (iterations_counter-- == 0) {
			return PATH_PENDING; // More to do on next frame.
		}

		u32 popped_id;
		data.open_list.pop_back_and_keep(popped_id);

		PlanPoint &p = _pool_plan_points[popped_id];

#ifdef NAVPHYSICS_WAYPOINT_PLANNER_VERBOSE
		log(String("popping ") + p.poly_id + " from open list");
#endif

		// Add all popped to the closed list.
		//PlanPoint *pp = &data.closed_plan_points.request();
		//*pp = p;
		data.closed_list.plan_points.push_back(popped_id);
		data.closed_list.poly_ids.push_back(p.poly_id);

		PlanPoint *pp = &p;

		if (pp->poly_id == data.poly_end) {
			// Finished path.
			// Create a temporary stack vector for the waypoints,
			// this makes it easier to do funneling.
			StackVector<FunnelPoint> waypoints;
			waypoints.setup_external((FunnelPoint *)alloca(sizeof(FunnelPoint) * MAX_PATH_POINTS), MAX_PATH_POINTS);

			// Add in reverse...

			{
				// Last point.
				FunnelPoint wp;
				wp.poly_id = data.poly_end;
				wp.local_pos = data.dest_pos;
				wp.entry_wall_id = pp->entry_wall_id;
				calculate_waypoint_pos3(mesh, wp);
				//r_points.push_back(wp);
				waypoints.push_back(wp);
			}

			// Skip first and the last here can be removed from the list, because they
			// are specified exactly.
			pp = pp->parent_closed_id == UINT32_MAX ? nullptr : &_pool_plan_points[data.closed_list.plan_points[pp->parent_closed_id]];

			while (pp) {
				FunnelPoint wp;
				wp.poly_id = pp->poly_id;
				wp.entry_wall_id = pp->entry_wall_id;
				wp.local_pos = mesh.get_poly(pp->poly_id).center;
				wp.local_pos3 = mesh.get_poly(pp->poly_id).center3;

				waypoints.push_back(wp);
				//r_points.push_back(wp);

				//pp = pp->parent_closed_id == UINT32_MAX ? nullptr : &data.closed_plan_points[pp->parent_closed_id];
				//pp = pp->parent_closed_id == UINT32_MAX ? nullptr : &_pool_plan_points[data.closed_plan_points[pp->parent_closed_id]];
				pp = pp->parent_closed_id == UINT32_MAX ? nullptr : &_pool_plan_points[data.closed_list.plan_points[pp->parent_closed_id]];

				if (!pp) {
					// Remove the last
					//r_points.resize(r_points.size() - 1);
					waypoints.resize(waypoints.size() - 1);
				}
			}

			{
				// First point.
				FunnelPoint wp;
				wp.poly_id = data.poly_start;
				wp.local_pos = data.start_pos;
				//wp.entry_wall_id = pp->entry_wall_id;
				calculate_waypoint_pos3(mesh, wp);
				//r_points.push_back(wp);
				waypoints.push_back(wp);
			}

			//r_points.invert();
			waypoints.invert();

			//funnel_path(mesh, r_points);
			funnel_path(mesh, waypoints);

#ifdef NAVPHYSICS_WAYPOINT_PLANNER_VERBOSE
			String sz = "Path : ";
			for (u32 n = 0; n < waypoints.size(); n++) {
				sz += String(waypoints[n].poly_id) + ", ";
			}

			log(sz);
#endif

			// Translate the stack vector into linked list of waypoints.
			if (!waypoints.size()) {
				return PATH_FAILED;
			}

			// WayPoint::_pool_waypoints._debug = true;

			WayPoint *wp = WayPoint::_pool_waypoints.request(r_first_waypoint);
			waypoints[0].to_waypoint(*wp);
			for (u32 n = 1; n < waypoints.size(); n++) {
				WayPoint *next = WayPoint::_pool_waypoints.request(wp->next_id);
				waypoints[n].to_waypoint(*next);
				wp = next;
			}

			//log("Waypoint path found.");
			return PATH_OK;
		}

		// Get the neighbours.
		const Poly &poly = mesh.get_poly(p.poly_id);
		for (u32 w = 0; w < poly.num_inds; w++) {
			u32 wall_id = poly.first_ind + w;

			if (!mesh.is_link_regular(wall_id)) {
				continue;
			}
			u32 npoly_id = mesh.get_link(wall_id);

			// Already on closed list?
			if (data.closed_list.poly_ids.contains(npoly_id)) {
				continue;
			}

			const Poly &npoly = mesh.get_poly(npoly_id);

			float tentative_start_cost = p.start_cost + cost(poly, npoly);

			// Does open list contain this poly already?
			PlanPoint *found = data.open_list.find(npoly_id);

			if (!found) {
				// POTENTIAL BUG!!!
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
				found->poly_id = npoly_id;
				found->entry_wall_id = wall_id;
				found->parent_closed_id = data.closed_list.plan_points.size() - 1;
				found->start_cost = tentative_start_cost;
				found->end_cost = heuristic(mesh, *found, data.poly_end);
				found->total_cost = found->start_cost + found->end_cost;

#ifdef NAVPHYSICS_WAYPOINT_PLANNER_VERBOSE
				log(String("\topen list found .. poly_id ") + found->poly_id + ", start_cost: " + found->start_cost + ", end_cost: " + found->end_cost + ", total_cost: " + found->total_cost);
#endif
				data.open_list.sort_last();
			}
		}
	}

	// No path found.
	log("Waypoint path not found.");
	return PATH_FAILED;
#endif
}

PathStatus Planner::pathfind_pos(np_handle p_mesh_instance, const IPoint2 &p_start, const IPoint2 &p_end, u32 &r_first_waypoint, u32 p_poly_start, u32 p_poly_end) {
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

	data.goal_pos = mesh.get_poly(p_poly_end).center;

	// Seed with first point.
	PlanPoint p;
	p.poly_id = p_poly_start;
	p.end_cost = heuristic(mesh, p, p_poly_end);
	p.total_cost = p.end_cost;
	data.open_list.add(p);

	return PATH_PENDING;

	PathStatus result = iterate(r_first_waypoint);

	return result;
	//return  result == PATH_OK;

	//	return true;

	// return pathfind_poly(p_mesh_instance, p_poly_start, p_poly_end, r_points);
}

} //namespace NavPhysics

#if 0
Let's break down how to implement A* pathfinding in C++.  This explanation will cover the core concepts and provide a structured approach to help you build your own implementation.

1. Data Structures:

Node: Represents a location in your grid or graph.  Crucially, it needs to store:

x, y coordinates (or equivalent representation of location).
gCost: Cost from the start node to this node.
hCost: Estimated cost from this node to the end node (heuristic).
fCost: gCost + hCost (the total estimated cost).
parent: Pointer to the previous node in the path.
Open List:  A container (e.g., std::priority_queue or std::set) of nodes that are candidates for expansion.  It should be ordered by fCost (lowest fCost at the top/front).

Closed List: A container (e.g., std::unordered_set or std::set) of nodes that have already been evaluated.  This prevents revisiting nodes unnecessarily.

Grid/Graph:  How you represent your environment.  A 2D vector (std::vector<std::vector<int>>) for grids is common.  int values could represent:

0: Walkable
1: Obstacle
Other values: Different terrain types with associated costs.
2. Core Algorithm:

C++

std::vector<Node*> aStar(Grid& grid, Node* startNode, Node* endNode) {
    std::priority_queue<Node*, std::vector<Node*>, CompareNode> openList; // Min-heap based on fCost
    std::unordered_set<Node*> closedList;

    startNode->gCost = 0;
    startNode->hCost = heuristic(startNode, endNode); // Implement your heuristic
    startNode->fCost = startNode->gCost + startNode->hCost;
    openList.push(startNode);

    while (!openList.empty()) {
        Node* current = openList.top();
        openList.pop();

        if (current == endNode) {
            return reconstructPath(current); // Build the path by following parent pointers
        }

        closedList.insert(current);

        std::vector<Node*> neighbors = getNeighbors(grid, current); // Get valid neighbors

        for (Node* neighbor : neighbors) {
            if (closedList.count(neighbor)) {
                continue; // Already evaluated
            }

            int tentativeGCost = current->gCost + cost(current, neighbor); // Cost to move

            if (!contains(openList, neighbor) || tentativeGCost < neighbor->gCost) {
                neighbor->parent = current;
                neighbor->gCost = tentativeGCost;
                neighbor->hCost = heuristic(neighbor, endNode);
                neighbor->fCost = neighbor->gCost + neighbor->hCost;

                if (!contains(openList, neighbor)) {
                    openList.push(neighbor);
                } else {
                    // If you are using a std::set, you need to remove and re-insert
                    // to update the ordering.  std::priority_queue does not support this.
                    // It is usually more efficient to leave it and let it be discovered again.
                }
            }
        }
    }

    return {}; // No path found
}
3. Key Functions:

heuristic(Node* a, Node* b):  Estimates the cost from a to b.  Common heuristics:

Manhattan distance (for grid-based movement with no diagonals).
Euclidean distance (for grid-based movement with diagonals).
Diagonal distance (for grid-based movement with diagonals, often more accurate).
getNeighbors(Grid& grid, Node* node):  Returns a list of valid neighbor nodes.  Important to check bounds of the grid and if a neighbor is an obstacle.

cost(Node* a, Node* b): Returns the cost to move from a to b.  Could be 1 for uniform cost or different values for different terrain.

reconstructPath(Node* endNode):  Backtracks from the endNode using the parent pointers to build the path.

contains(Container& container, Node* node): Checks if a node is present in the container.  Implement efficiently based on the container type.

CompareNode:  A comparator struct for the priority queue:

C++

struct CompareNode {
    bool operator()(Node* a, Node* b) {
        return a->fCost > b->fCost; // Min-heap
    }
};
4. Implementation Details:

Memory Management: Be very careful with memory management, especially if you're dynamically allocating Node objects. Use smart pointers (std::unique_ptr, std::shared_ptr) to avoid memory leaks.
Optimization: A* can be computationally intensive. Consider optimizations like jump point search (JPS) for uniform-cost grids.
Heuristic Choice: The choice of heuristic significantly impacts performance. An admissible heuristic (never overestimates the cost) is essential for finding the optimal path.
Grid Representation: Consider how you'll represent your grid. A 2D vector of custom structs (containing cost information, etc.) might be more flexible than just integers.
Example Heuristic (Manhattan):

C++

int manhattanDistance(Node* a, Node* b) {
    return std::abs(a->x - b->x) + std::abs(a->y - b->y);
}
This detailed explanation should give you a solid foundation for implementing A* pathfinding in C++.  Remember to adapt the code to your specific game or application.  If you have further questions about specific parts of the implementation, feel free to ask!
#endif
