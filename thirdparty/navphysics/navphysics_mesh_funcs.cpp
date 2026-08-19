#include "navphysics_mesh_funcs.h"
#include "navphysics_loader.h"
#include "navphysics_mesh.h"
#include "navphysics_structs.h"

namespace NavPhysics {

void MeshFuncs::editor_toggle_wall_connection(Mesh &r_mesh, const FPoint3 &p_from, const FPoint3 &p_to, bool p_external_or_internal) {
	struct Extender {
		Loader loader;
		Mesh &mesh;
		Extender(Mesh &p_mesh) :
				mesh(p_mesh) {
			loader.unextend_mesh(mesh);
		}
		~Extender() {
			loader.extend_mesh(mesh);
		}
	};

	Extender guard(r_mesh);

	u32 wall_id = find_nearest_wall(r_mesh, p_from, p_to);
	NP_LOG(String("Wall clicked : ") + wall_id);

	if (wall_id == UINT32_MAX) {
		return;
	}

	// Is it a side wall?
	if (!r_mesh.is_link_hard(wall_id)) {
		log(String("Not a side wall, link id ") + r_mesh.get_link(wall_id));
		return;
	}

	TVector<u32> *wall_ids = p_external_or_internal ? &r_mesh.data.external_wall_ids : &r_mesh.data.internal_wall_ids;

	i64 found = wall_ids->find(wall_id);

	if (found == -1) {
		wall_ids->push_back(wall_id);
		NP_LOG(String("Adding wall connection ") + wall_id);
	} else {
		wall_ids->remove_unordered(found);
		NP_LOG(String("Removing wall connection ") + wall_id);
	}

	for (u32 n = 0; n < wall_ids->size(); n++) {
		NP_LOG(String("\tconn ") + (*(wall_ids))[n]);
	}

	// Whether to lip the wall id.
	// If external, but not internal, lip.
	// If external and internal, lip.
	// If internal only, don't lip.
	// If not on either, don't lip.
	bool on_external = r_mesh.data.external_wall_ids.contains(wall_id);
	bool on_lipped = r_mesh.data.lipped_wall_ids.contains(wall_id);
	bool should_lip = on_external;

	if (should_lip) {
		if (!on_lipped) {
			r_mesh.data.lipped_wall_ids.push_back(wall_id);
		}
	} else {
		if (on_lipped) {
			r_mesh.data.lipped_wall_ids.erase(wall_id);
		}
	}
}

u32 MeshFuncs::find_nearest_wall(const Mesh &p_mesh, const FPoint3 &p_from, const FPoint3 &p_to) {
	u32 nearest_wall = UINT32_MAX;
	freal closest = FLT_MAX;
	freal closest_cam = FLT_MAX;

	FPoint3 ps;
	FPoint3 qt;
	FPoint3 st;

	for (u32 n = 0; n < p_mesh.get_num_walls(); n++) {
		// Only side walls at the moment.
		if (!p_mesh.is_link_hard(n)) {
			continue;
		}

		const Wall &wall = p_mesh.get_wall(n);

		const FPoint3 &a = p_mesh.get_fvert3(wall.vert_a);
		const FPoint3 &b = p_mesh.get_fvert3(wall.vert_b);

		// freal dist_to_ray = get_closest_distance_between_segments(p_from, p_to, a, b);

		get_closest_points_between_segments(p_from, p_to, a, b, ps, qt);
		st = qt - ps;
		freal dist_to_ray = st.length();
		if (dist_to_ray > 0.5f) {
			continue;
		}

		freal dist_to_cam = (ps - p_from).length();

		freal metric = (dist_to_ray + (dist_to_cam * 0.1));

		if ((metric < closest) || (dist_to_cam < (closest_cam - 3))) {
			closest = metric;
			closest_cam = dist_to_cam;
			nearest_wall = n;

			//log(String("nearest metric ") + metric + ", dist_ray " + dist_to_ray + ", dist_cam " + dist_to_cam);
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

freal MeshFuncs::get_closest_distance_between_segments(const IPoint2 &p_p0, const IPoint2 &p_p1, const IPoint2 &p_q0, const IPoint2 &p_q1) {
	FPoint3 pts[4];
	pts[0] = FPoint3::make(p_p0.x, 0, p_p0.y);
	pts[1] = FPoint3::make(p_p1.x, 0, p_p1.y);
	pts[2] = FPoint3::make(p_q0.x, 0, p_q0.y);
	pts[3] = FPoint3::make(p_q1.x, 0, p_q1.y);

	return get_closest_distance_between_segments(pts[0], pts[1], pts[2], pts[3]);
}

freal MeshFuncs::get_closest_distance_between_segments(const FPoint3 &p_p0, const FPoint3 &p_p1, const FPoint3 &p_q0, const FPoint3 &p_q1) {
	FPoint3 ps;
	FPoint3 qt;
	get_closest_points_between_segments(p_p0, p_p1, p_q0, p_q1, ps, qt);
	FPoint3 st = qt - ps;
	return st.length();
}

FPoint3 MeshFuncs::get_closest_point_to_segment(const FPoint3 &p_point, const FPoint3 *p_segment) {
	FPoint3 p = p_point - p_segment[0];
	FPoint3 n = p_segment[1] - p_segment[0];
	freal l2 = n.length_squared();
	if (l2 < 1e-20f) {
		return p_segment[0]; // Both points are the same, just give any.
	}

	freal d = n.dot(p) / l2;

	if (d <= 0) {
		return p_segment[0]; // Before first point.
	} else if (d >= 1) {
		return p_segment[1]; // After first point.
	} else {
		return p_segment[0] + n * d; // Inside.
	}
}

} //namespace NavPhysics
