#include "navphysics_mesh.h"
#include "navphysics_map.h"

namespace NavPhysics {

//void Mesh::set_transform(const Transform &p_xform, const Transform &p_xform_inv, bool p_is_identity) {
//	_transform = p_xform;
//	_transform_inverse = p_xform_inv;
//	_transform_identity = p_is_identity;
//}

void Mesh::debug_poly(u32 p_poly_id) const {
}

void Mesh::_log(const String &p_string, int p_depth) const {
	//print_line(p_string);
}

/*
PoolVector<Face3> Mesh::mesh_get_faces() const {
	PoolVector<Face3> faces;

	for (u32 n = 0; n < get_num_polys(); n++) {
		const Poly &poly = get_poly(n);
		if ((poly.narrowing_id != UINT32_MAX) && (poly.num_inds >= 3)) {
			// add the poly to the debug faces
			Face3 face;
			for (u32 c = 0; c < 3; c++) {
				u32 ind = get_ind(poly.first_ind + c);
				face.vertex[c] = _fverts3[ind];
			}
			faces.push_back(face);
		}
	}

	return faces;
	// return PoolVector<Face3>();
}
*/

/*
// return whether allowed
bool Mesh::_agent_enter_poly(u32 p_old_poly_id, u32 p_new_poly_id, bool p_force_allow) {
	if (p_old_poly_id == p_new_poly_id) {
		return true;
	}

 // try to enter the new poly BEFORE leaving the old
 if (p_new_poly_id != UINT32_MAX) {
	 Poly &new_poly = get_poly(p_new_poly_id);

	 if (new_poly.narrowing_id != UINT32_MAX) {
		 Narrowing &narrowing = _narrowings[new_poly.narrowing_id];
		 if ((narrowing.used >= narrowing.available) && !p_force_allow) {
			 return false;
		 }

		 narrowing.used += 1;
	 }
 }

 if (p_old_poly_id != UINT32_MAX) {
	 Poly &old_poly = get_poly(p_old_poly_id);
	 if (old_poly.narrowing_id != UINT32_MAX) {
		 Narrowing &narrowing = _narrowings[old_poly.narrowing_id];
		 if (narrowing.used) {
			 narrowing.used -= 1;
		 } else {
			 NP_WARN_PRINT("Old narrowing has no agents.");
		 }
	 }
 }

 return true;
}
*/

u32 Mesh::find_poly_within(const IPoint2 &p_pt) const {
	// slow only use when teleporting
	for (u32 p = 0; p < get_num_polys(); p++) {
		if (poly_contains_point(p, p_pt)) {
			// print("within poly " + str(p))
			return p;
		}
	}

	return UINT32_MAX;
}

Mesh::MoveResult Mesh::recursive_move(i32 p_depth, IPoint2 p_from, IPoint2 p_vel, u32 p_poly_id, u32 p_poly_from_id, u32 p_hug_wall_id, MoveInfo &r_info) const {
	if (p_depth >= 8) {
		_log("\t\trecursive_move depth limit reached");
		r_info.poly_id = p_poly_id;
		r_info.pos_reached = p_from;
		r_info.wall_id = p_hug_wall_id;
		return MR_LIMIT;
	}

	freal vel_length = p_vel.lengthf();

	if (vel_length < 0.001f) {
		_log("\t\trecursion ending, vel_mag is zero");
		r_info.poly_id = p_poly_id;
		r_info.pos_reached = p_from;
		r_info.wall_id = p_hug_wall_id;
		return MR_OK;
	}

	// are we moving along a wall?
	if (p_hug_wall_id != UINT32_MAX) {
		// are we still heading along the wall?
		const Wall &wall = get_wall(p_hug_wall_id);
		freal dot = wall.normal.dot(p_vel);
		if (dot < 0.001f) {
			// hugging wall
			freal wall_length = wall.wall_vec.lengthf();
			const IPoint2 &wall_start = get_vert(wall.vert_a);
			const IPoint2 &wall_end = get_vert(wall.vert_b);
			freal dist_along_wall = p_from.distancef_to(wall_start);
			// freal dist_remaining = wall_length - dist_along_wall;

			freal wall_dot = wall.wall_vec.dot(p_vel);

			// change move length according to angle with wall
			vel_length *= wall_dot;

			// directly calculate new position
			freal dist = dist_along_wall + vel_length;
			freal fract = dist / wall_length;
			IPoint2 to = wall_start + (wall.wall_vec * fract);
			_log(String("\t\twall dot: ") + String(wall_dot));

			// goes along wall forwards or backwards?
			if (wall_dot >= 0.0f) {
				// forwards
				if (fract <= 1.0f) {
					r_info.poly_id = p_poly_id;
					r_info.pos_reached = to;
					r_info.wall_id = p_hug_wall_id;
					return MR_OK;
				} else {
					// we have left the wall
					p_hug_wall_id = wall.next_wall;
					// remaining velocity
					p_vel = to - wall_end;

					// special case, we are moving onto a connection
					if (is_connecting_wall(p_hug_wall_id)) {
						_log(String("\t\tleaving hug wall ") + itos(p_hug_wall_id));
						p_hug_wall_id = UINT32_MAX;
					} else {
						const Wall &next_wall = get_wall(p_hug_wall_id);
						return recursive_move(p_depth + 1, wall_end, p_vel, next_wall.poly_id, p_poly_from_id, p_hug_wall_id, r_info);
					}
				}

			} else {
				// backwards
				if (fract >= 0.0f) {
					r_info.poly_id = p_poly_id;
					r_info.pos_reached = to;
					r_info.wall_id = p_hug_wall_id;
					return MR_OK;
				} else {
					// we have left the wall
					p_hug_wall_id = wall.prev_wall;
					// remaining velocity
					p_vel = to - wall_start;
					// special case, we are moving onto a connection
					if (is_connecting_wall(p_hug_wall_id)) {
						_log(String("\t\tleaving hug wall ") + itos(p_hug_wall_id));
						p_hug_wall_id = UINT32_MAX;
					} else {
						const Wall &next_wall = get_wall(p_hug_wall_id);
						return recursive_move(p_depth + 1, wall_start, p_vel, next_wall.poly_id, p_poly_from_id, p_hug_wall_id, r_info);
					}
				}
			}
		} else {
			_log(String("\t\tleaving hug wall ") + itos(p_hug_wall_id));
			p_hug_wall_id = UINT32_MAX;
		}
	}

	// new destination
	IPoint2 to = p_from + p_vel;

	_log(String("\trecursive_move [") + itos(p_depth) + "] poly " + itos(p_poly_id) + " to " + str(to) + " ... vel " + str(p_vel), p_depth);

	TraceInfo trace_info;
	TraceResult res = recursive_trace(0, p_from, to, p_poly_id, trace_info);
	if (res == TR_LIMIT) {
		r_info.poly_id = trace_info.poly_id;
		r_info.pos_reached = trace_info.hit_point;
		r_info.wall_id = UINT32_MAX;
		return MR_LIMIT;
	}

	if (res == TR_CLEAR) {
		_log("\t\tmove ok", p_depth);
		NP_DEV_ASSERT(poly_contains_point(trace_info.poly_id, to));
		r_info.poly_id = trace_info.poly_id;
		r_info.pos_reached = to;
		r_info.wall_id = p_hug_wall_id;
		return MR_OK;
	}

	// slide
	NP_DEV_ASSERT(res == TR_SLIDE);
	p_poly_id = trace_info.poly_id;
	u32 wall_id = trace_info.slide_wall;
	IPoint2 pt_intersect = trace_info.hit_point;

	// account for possibility we are going through a connection to a new mesh instance
	if (is_connecting_wall(wall_id)) {
		IPoint2 pt_potential = p_from + p_vel;
		if (move_to_new_mesh(pt_potential, r_info)) {
			r_info.poly_id = UINT32_MAX;
			r_info.pos_reached = pt_potential;
			r_info.wall_id = UINT32_MAX;
			return MR_OK;
		}
	}

	// reduce the velocity by how far travelled to the wall
	p_vel -= (pt_intersect - p_from);
	p_hug_wall_id = wall_id;
	// const Wall &wall = get_wall(p_hug_wall_id);
	return recursive_move(p_depth + 1, pt_intersect, p_vel, p_poly_id, p_poly_from_id, p_hug_wall_id, r_info);
}

bool Mesh::move_to_new_mesh(const IPoint2 &p_pt, MoveInfo &r_info) const {
	Map &map = NavPhysics::g_world.get_map(r_info.map_id);
	Agent &agent = NavPhysics::g_world.get_body(r_info.agent_id);

	// Get the coords into world space.
	u32 old_mesh_instance_id = agent.get_mesh_instance_id();
	NP_DEV_ASSERT(old_mesh_instance_id != UINT32_MAX);

	MeshInstance &old_mesh_instance = NavPhysics::g_world.get_mesh_instance(old_mesh_instance_id);

	// only update the f32ing point position if significantly different
	agent.fpos = fixed_point_to_float_2(agent.pos);

	//r_agent.fvel = mesh.fixed_point_vel_to_float(r_agent.vel);

	agent.fpos3 = FPoint3::make(agent.fpos.x, agent.height, agent.fpos.y);
	agent.fpos3 = old_mesh_instance.get_transform().xform(agent.fpos3);

	u32 mesh_instance_id = map.find_best_fit_agent_mesh(agent, old_mesh_instance_id);

	if (mesh_instance_id != UINT32_MAX) {
		r_info.new_mesh_instance_id = mesh_instance_id;
		return true;
	}

	return false;
}

Mesh::TraceResult Mesh::recursive_trace(i32 p_depth, IPoint2 p_from, IPoint2 p_to, u32 p_poly_id, TraceInfo &r_info) const {
	//	if (p_depth >= 8) {
	//		_log("\t\ttrace recursion depth limit reached");
	//		NP_DEV_ASSERT(poly_contains_point(p_poly_id, p_from));
	//		//return [poly_id, from, vel_dir, vel_mag_global]
	//		return TR_LIMIT;
	//	}

	const Poly &poly = get_poly(p_poly_id);
	freal smallest_dist = FLT_MAX;
	u32 best_wall_id = UINT32_MAX;
	//IPoint2 best_intersect{ 0, 0 };
	IPoint2 best_intersect;

	for (u32 w = 0; w < poly.num_inds; w++) {
		u32 wall_id = poly.first_ind + w;
		i64 cross_to = -wall_cross(wall_id, p_to);

		// crossed wall
		if (cross_to > 0) {
			IPoint2 intersect;
			if (wall_find_intersect(wall_id, p_from, p_to, intersect)) {
				freal dist = p_from.distancef_to(intersect);
				if (dist < smallest_dist) {
					smallest_dist = dist;
					best_wall_id = wall_id;
					best_intersect = intersect;
				}
			}
		}
	}

	if (best_wall_id == UINT32_MAX) {
		// move okay
		r_info.poly_id = p_poly_id;
		return TR_CLEAR;
		//return [0, poly_id]
	}

	u32 linked_poly_id = get_link(best_wall_id);

	if (is_hard_wall(best_wall_id)) {
		// indicates slide and which wall
		r_info.poly_id = p_poly_id;
		r_info.slide_wall = best_wall_id;
		r_info.hit_point = best_intersect;
		return TR_SLIDE;
	}

	if (p_depth >= 8) {
		_log("\t\ttrace recursion depth limit reached");
		r_info.hit_point = best_intersect;

		if (poly_contains_point(p_poly_id, best_intersect)) {
			r_info.poly_id = p_poly_id;
		} else if (poly_contains_point(linked_poly_id, best_intersect)) {
			r_info.poly_id = linked_poly_id;
		} else {
			// last ditch attempt
			r_info.poly_id = p_poly_id;
			r_info.hit_point = get_poly(p_poly_id).center;
		}
		//NP_DEV_ASSERT(poly_contains_point(p_poly_id, best_intersect));
		//return [poly_id, from, vel_dir, vel_mag_global]
		return TR_LIMIT;
	}

	// recurse into neighbouring cell
	return recursive_trace(p_depth + 1, p_from, p_to, linked_poly_id, r_info);
}

bool Mesh::poly_contains_point(u32 p_poly_id, const IPoint2 &p_pt) const {
	const Poly &poly = get_poly(p_poly_id);
	for (u32 n = 0; n < poly.num_inds; n++) {
		u32 wall_id = poly.first_ind + n;

		if (wall_in_front_cross(wall_id, p_pt)) {
			return false;
		}
	}

	return true;
}

bool Mesh::wall_in_front_cross(u32 p_wall_id, const IPoint2 &p_pt) const {
	return wall_cross(p_wall_id, p_pt) < 0;
}

i64 Mesh::wall_cross(u32 p_wall_id, const IPoint2 &p_pt) const {
	IPoint2 wa, wb;
	get_wall_verts(p_wall_id, wa, wb);

	IPoint2 wall_vec = wb - wa;
	IPoint2 point_vec = p_pt - wa;
	return wall_vec.cross(point_vec);
}

freal Mesh::find_height_on_poly(u32 p_poly_id, const IPoint2 &p_pt) const {
	FPoint2 pt = fixed_point_to_float_2(p_pt);

	NP_DEV_ASSERT(p_poly_id != UINT32_MAX);
	const Poly &poly = get_poly(p_poly_id);

	FPoint3 intersection;
	bool hit = poly.plane.intersects_ray(FPoint3::make(pt.x, 1000, pt.y), FPoint3::make(0, -1, 0), &intersection);
	if (hit) {
		return intersection.y;
	}
	NP_WARN_PRINT("find_height_on_poly failed");
	return 0.0;
}

bool Mesh::wall_find_intersect(u32 p_wall_id, const IPoint2 &p_from, const IPoint2 &p_to, IPoint2 &r_hit) const {
	IPoint2 wa, wb;
	get_wall_verts(p_wall_id, wa, wb);

	return find_lines_intersect_integer(wa, wb, p_from, p_to, r_hit);
}

// https://stackoverflow.com/questions/21224361/calculate-intersection-of-two-lines-using-integers-only
// intersect 2 lines using integer math
bool Mesh::find_lines_intersect_integer(const IPoint2 &p_from_a, const IPoint2 &p_to_a, const IPoint2 &p_from_b, const IPoint2 &p_to_b, IPoint2 &r_hit) const {
	i32 x1 = p_from_a.x;
	i32 y1 = p_from_a.y;
	i32 x2 = p_to_a.x;
	i32 y2 = p_to_a.y;
	i32 x3 = p_from_b.x;
	i32 y3 = p_from_b.y;
	i32 x4 = p_to_b.x;
	i32 y4 = p_to_b.y;

	// First line coefficients where "a1 x  +  b1 y  +  c1  =  0"
	i32 a1 = y2 - y1;
	i32 b1 = x1 - x2;
	// These calcs need to be 64 bit to prevent overflow crossing 65535.
	i64 c1 = (i64)x2 * y1 - (i64)x1 * y2;

	// Second line coefficients
	i32 a2 = y4 - y3;
	i32 b2 = x3 - x4;
	i64 c2 = (i64)x4 * y3 - (i64)x3 * y4;

	i64 denom = (i64)a1 * b2 - (i64)a2 * b1;

	// Lines are colinear
	if (denom == 0) {
		return false;
	}

	// Compute sign values
	/*
	if false:
		var r3 = a1 * x3 + b1 * y3 + c1
		var r4 = a1 * x4 + b1 * y4 + c1

		 # Sign values for second line
		 var r1 = a2 * x1 + b2 * y1 + c2
		 var r2 = a2 * x2 + b2 * y2 + c2

		  # Flag denoting whether intersection point is on passed line segments. If this is false,
		  # the intersection occurs somewhere along the two mathematical, infinite lines instead.
		  # Check signs of r3 and r4.  If both point 3 and point 4 lie on same side of line 1, the
		  # line segments do not intersect.
		  # Check signs of r1 and r2.  If both point 1 and point 2 lie on same side of second line
		  # segment, the line segments do not intersect.
		  var is_on_segments = (r3 != 0 && r4 != 0 && same_signs(r3, r4)) || (r1 != 0 && r2 != 0 && same_signs(r1, r2))

		 if is_on_segments == false:
			 return null
			 */

	// If we got here, line segments intersect. Compute intersection point using method similar
	// to that described here: http://paulbourke.net/geometry/pointlineplane/#i2l

	// The denom/2 is to get rounding instead of truncating. It is added or subtracted to the
	// numerator, depending upon the sign of the numerator.
	i64 offset = denom / 2;
	if (denom < 0) {
		offset = -offset;
	}

	i64 x;
	i64 y;

	{
		i64 num = (i64)b1 * c2 - (i64)b2 * c1;
		if (num < 0) {
			x = num - offset;
		} else {
			x = num + offset;
		}
		x /= denom;
	}

	{
		i64 num = (i64)a2 * c1 - (i64)a1 * c2;
		if (num < 0) {
			y = num - offset;
		} else {
			y = num + offset;
		}
		y /= denom;
	}

	r_hit.set(x, y);
	return true;
}

} // namespace NavPhysics
