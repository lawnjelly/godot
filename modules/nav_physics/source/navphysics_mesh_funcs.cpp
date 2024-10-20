#include "navphysics_mesh_funcs.h"
#include "navphysics_mesh.h"
#include "navphysics_structs.h"

namespace NavPhysics {

void MeshFuncs::editor_toggle_wall_connection(Mesh &r_mesh, const FPoint3 &p_from, const FPoint3 &p_to) {
	u32 wall_id = find_nearest_wall(r_mesh, p_from, p_to);

	if (wall_id == UINT32_MAX) {
		return;
	}

	// Is it a side wall?
	if (!r_mesh.is_hard_wall(wall_id)) {
		return;
	}

	i64 found = r_mesh.data.wall_connections.find(wall_id);

	if (found == -1) {
		r_mesh.data.wall_connections.push_back(wall_id);
		log(String("Adding wall connection ") + wall_id);
	} else {
		r_mesh.data.wall_connections.remove_unordered(found);
		log(String("Removing wall connection ") + wall_id);
	}

	for (u32 n = 0; n < r_mesh.data.wall_connections.size(); n++) {
		log(String("\tconn ") + r_mesh.data.wall_connections[n]);
	}
}

u32 MeshFuncs::find_nearest_wall(const Mesh &p_mesh, const FPoint3 &p_from, const FPoint3 &p_to) {
	u32 nearest_wall = UINT32_MAX;
	freal closest = FLT_MAX;

	for (u32 n = 0; n < p_mesh.get_num_walls(); n++) {
		const Wall &wall = p_mesh.get_wall(n);

		const FPoint3 &a = p_mesh.get_fvert3(wall.vert_a);
		const FPoint3 &b = p_mesh.get_fvert3(wall.vert_b);

		freal dist = get_closest_distance_between_segments(p_from, p_to, a, b);
		if (dist < closest) {
			closest = dist;
			nearest_wall = n;
		}
	}

	return nearest_wall;
}

void MeshFuncs::get_closest_points_between_segments(const FPoint3 &p_p0, const FPoint3 &p_p1, const FPoint3 &p_q0, const FPoint3 &p_q1, FPoint3 &r_ps, FPoint3 &r_qt) {
	// Based on David Eberly's Computation of Distance Between Line Segments algorithm.

	FPoint3 p = p_p1 - p_p0;
	FPoint3 q = p_q1 - p_q0;
	FPoint3 r = p_p0 - p_q0;

	freal a = p.dot(p);
	freal b = p.dot(q);
	freal c = q.dot(q);
	freal d = p.dot(r);
	freal e = q.dot(r);

	freal s = 0.0f;
	freal t = 0.0f;

	freal det = a * c - b * b;
	if (det > Math::NP_CMP_EPSILON) {
		// Non-parallel segments
		freal bte = b * e;
		freal ctd = c * d;

		if (bte <= ctd) {
			// s <= 0.0f
			if (e <= 0.0f) {
				// t <= 0.0f
				s = (-d >= a ? 1 : (-d > 0.0f ? -d / a : 0.0f));
				t = 0.0f;
			} else if (e < c) {
				// 0.0f < t < 1
				s = 0.0f;
				t = e / c;
			} else {
				// t >= 1
				s = (b - d >= a ? 1 : (b - d > 0.0f ? (b - d) / a : 0.0f));
				t = 1;
			}
		} else {
			// s > 0.0f
			s = bte - ctd;
			if (s >= det) {
				// s >= 1
				if (b + e <= 0.0f) {
					// t <= 0.0f
					s = (-d <= 0.0f ? 0.0f : (-d < a ? -d / a : 1));
					t = 0.0f;
				} else if (b + e < c) {
					// 0.0f < t < 1
					s = 1;
					t = (b + e) / c;
				} else {
					// t >= 1
					s = (b - d <= 0.0f ? 0.0f : (b - d < a ? (b - d) / a : 1));
					t = 1;
				}
			} else {
				// 0.0f < s < 1
				freal ate = a * e;
				freal btd = b * d;

				if (ate <= btd) {
					// t <= 0.0f
					s = (-d <= 0.0f ? 0.0f : (-d >= a ? 1 : -d / a));
					t = 0.0f;
				} else {
					// t > 0.0f
					t = ate - btd;
					if (t >= det) {
						// t >= 1
						s = (b - d <= 0.0f ? 0.0f : (b - d >= a ? 1 : (b - d) / a));
						t = 1;
					} else {
						// 0.0f < t < 1
						s /= det;
						t /= det;
					}
				}
			}
		}
	} else {
		// Parallel segments
		if (e <= 0.0f) {
			s = (-d <= 0.0f ? 0.0f : (-d >= a ? 1 : -d / a));
			t = 0.0f;
		} else if (e >= c) {
			s = (b - d <= 0.0f ? 0.0f : (b - d >= a ? 1 : (b - d) / a));
			t = 1;
		} else {
			s = 0.0f;
			t = e / c;
		}
	}

	r_ps = (1 - s) * p_p0 + s * p_p1;
	r_qt = (1 - t) * p_q0 + t * p_q1;
}

freal MeshFuncs::get_closest_distance_between_segments(const FPoint3 &p_p0, const FPoint3 &p_p1, const FPoint3 &p_q0, const FPoint3 &p_q1) {
	FPoint3 ps;
	FPoint3 qt;
	get_closest_points_between_segments(p_p0, p_p1, p_q0, p_q1, ps, qt);
	FPoint3 st = qt - ps;
	return st.length();
}

} //namespace NavPhysics
