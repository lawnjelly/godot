#include "np_mesh.h"
#include "core/math/geometry.h"

namespace NavPhysics {

void Mesh::set_transform(const Transform &p_xform, const Transform &p_xform_inv, bool p_is_identity) {
	_transform = p_xform;
	_transform_inverse = p_xform_inv;
	_transform_identity = p_is_identity;
}

void Mesh::debug_poly(uint32_t p_poly_id) const {
}

void Mesh::_log(const String &p_string, int p_depth) const {
	//print_line(p_string);
}

real_t Mesh::find_agent_fit(Agent &r_agent) const {
	// multiple meshs NYI
	return 1.0;
}

void Mesh::teleport_agent(Agent &r_agent) const {
	r_agent.pos = float_to_vec2(r_agent.fpos);

	//r_agent.pos.set(7906, 41409);

	r_agent.poly_id = find_poly_within(r_agent.pos);
	if (r_agent.poly_id == UINT32_MAX) {
		WARN_PRINT("not within poly");

		// just guess a poly
		if (get_num_polys()) {
			r_agent.poly_id = 0;
			const Poly &poly = get_poly(r_agent.poly_id);
			r_agent.pos = poly.center;
		}

		return;
	} else {
		print_line("c++ agent within poly " + itos(r_agent.poly_id));
	}
}

void Mesh::iterate_agent(Agent &r_agent) const {
	//print_line("c++ agent at pos " + String(Variant(r_agent.fpos)));

	// has the float position moved significantly? if not, retain
	// the fixed point position as this is the gold standard
	//vec2 new_pos_fp = float_to_vec2(r_agent.fpos);
	//vec2 offset = new_pos_fp - r_agent.pos;
	//if ((Math::abs(offset.x) > 1) || (Math::abs(offset.y) > 1)) {
	//r_agent.pos = new_pos_fp;
	//}

	vec2 vel_add = vel_to_fp_vel(r_agent.fvel);
	r_agent.vel += vel_add;

	_iterate_agent(r_agent);

	// apply friction
	r_agent.vel *= r_agent.friction;

	if (r_agent.poly_id != UINT32_MAX) {
		r_agent.height = find_height_on_poly(r_agent.poly_id, r_agent.pos);
	}

	// only update the floating point position if significantly different
	Vector2 new_fpos = vec2_to_float(r_agent.pos);
	if (!new_fpos.is_equal_approx(r_agent.fpos)) {
		r_agent.fpos = new_fpos;
	}

	r_agent.fvel = fp_vel_to_vel(r_agent.vel);
}

void Mesh::_iterate_agent(Agent &r_agent) const {
	Agent &ag = r_agent;

	if (ag.poly_id == UINT32_MAX) {
		ag.poly_id = find_poly_within(ag.pos);
		if (ag.poly_id == UINT32_MAX) {
			WARN_PRINT("not within poly");
			return;
		} else {
			print_line("c++ agent within poly " + itos(ag.poly_id));
		}
	}

	real_t mag = ag.vel.length();
	if (mag < 0.0001f) {
		//_log("no significant velocity");
		return;
	}

	vec2 old_pos = ag.pos;

	vec2 dir = ag.vel;
	//var dir = agent._vel
	dir.normalize();
	_log("iterate pos " + ag.pos.sz() + ", vel " + ag.vel.sz() + ", poly " + itos(ag.poly_id));

	if (ag.wall_id == UINT32_MAX) {
		DEV_ASSERT(poly_contains_point(ag.poly_id, ag.pos));
	}
	MoveInfo minfo;
	recursive_move(0, ag.pos, ag.vel, ag.poly_id, -2, ag.wall_id, minfo);
	// poly, intersect, dir
	//if res == null:
	//	return

	//real_t dist_moved = (minfo.pos_reached - ag.pos).length();
	//_log("\t\tdistance moved overall : " + String(Variant(dist_moved)));

	ag.poly_id = minfo.poly_id;
	ag.pos = minfo.pos_reached;
	ag.wall_id = minfo.wall_id;

	ag.vel = ag.pos - old_pos;

	_log("\tagent FINAL pos " + ag.pos.sz() + ", vel " + ag.vel.sz() + ", poly " + itos(ag.poly_id));

	// limit the magnitude to the ingoing magnitude
	//	real_t new_mag = minfo.vel_mag_final;
	//	new_mag = MIN(new_mag, mag);

	//	ag.vel = minfo.vel_dir;
	//	ag.vel.normalize_to_scale(new_mag);

	//if (!poly_contains_point(agent._poly_id, agent._pos, true)):
	//	assert (poly_contains_point(agent._poly_id, agent._pos))

	//		var test_poly_id = find_poly_within(agent._pos)
	//		if (agent._poly_id != test_poly_id):
	//			print("incorrect poly")
}

uint32_t Mesh::find_poly_within(const vec2 &p_pt) const {
	// slow only use when teleporting
	for (uint32_t p = 0; p < get_num_polys(); p++) {
		if (poly_contains_point(p, p_pt)) {
			// print("within poly " + str(p))
			return p;
		}
	}

	return UINT32_MAX;
}

Mesh::MoveResult Mesh::recursive_move(int32_t p_depth, vec2 p_from, vec2 p_vel, uint32_t p_poly_id, uint32_t p_poly_from_id, uint32_t p_hug_wall_id, MoveInfo &r_info) const {
	if (p_depth >= 8) {
		_log("\t\trecursive_move depth limit reached");
		r_info.poly_id = p_poly_id;
		r_info.pos_reached = p_from;
		r_info.wall_id = p_hug_wall_id;
		return MR_LIMIT;
	}

	real_t vel_length = p_vel.length();

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
		real_t dot = wall.normal.dot(p_vel);
		if (dot < 0.001f) {
			// hugging wall
			real_t wall_length = wall.wall_vec.length();
			const vec2 &wall_start = get_vert(wall.vert_a);
			const vec2 &wall_end = get_vert(wall.vert_b);
			real_t dist_along_wall = p_from.distance_to(wall_start);
			// real_t dist_remaining = wall_length - dist_along_wall;

			real_t wall_dot = wall.wall_vec.dot(p_vel);

			// change move length according to angle with wall
			vel_length *= wall_dot;

			// directly calculate new position
			real_t dist = dist_along_wall + vel_length;
			real_t fract = dist / wall_length;
			vec2 to = wall_start + (wall.wall_vec * fract);
			_log("\t\twall dot: " + String(Variant(wall_dot)));

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
					const Wall &next_wall = get_wall(p_hug_wall_id);
					return recursive_move(p_depth + 1, wall_end, p_vel, next_wall.poly_id, p_poly_from_id, p_hug_wall_id, r_info);
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
					const Wall &next_wall = get_wall(p_hug_wall_id);
					return recursive_move(p_depth + 1, wall_start, p_vel, next_wall.poly_id, p_poly_from_id, p_hug_wall_id, r_info);
				}
			}
		} else {
			_log("\t\tleaving hug wall " + itos(p_hug_wall_id));
			p_hug_wall_id = UINT32_MAX;
		}
	}

	// new destination
	vec2 to = p_from + p_vel;

	_log("\trecursive_move [" + itos(p_depth) + "] poly " + itos(p_poly_id) + " to " + to.sz() + " ... vel " + p_vel.sz(), p_depth);

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
		DEV_ASSERT(poly_contains_point(trace_info.poly_id, to));
		r_info.poly_id = trace_info.poly_id;
		r_info.pos_reached = to;
		r_info.wall_id = p_hug_wall_id;
		return MR_OK;
	}

	// slide
	DEV_ASSERT(res == TR_SLIDE);
	p_poly_id = trace_info.poly_id;
	uint32_t wall_id = trace_info.slide_wall;
	vec2 pt_intersect = trace_info.hit_point;

	// reduce the velocity by how far travelled to the wall
	p_vel -= (pt_intersect - p_from);
	p_hug_wall_id = wall_id;
	// const Wall &wall = get_wall(p_hug_wall_id);
	return recursive_move(p_depth + 1, pt_intersect, p_vel, p_poly_id, p_poly_from_id, p_hug_wall_id, r_info);
}

Mesh::TraceResult Mesh::recursive_trace(int32_t p_depth, vec2 p_from, vec2 p_to, uint32_t p_poly_id, TraceInfo &r_info) const {
	//	if (p_depth >= 8) {
	//		_log("\t\ttrace recursion depth limit reached");
	//		DEV_ASSERT(poly_contains_point(p_poly_id, p_from));
	//		//return [poly_id, from, vel_dir, vel_mag_global]
	//		return TR_LIMIT;
	//	}

	const Poly &poly = get_poly(p_poly_id);
	real_t smallest_dist = FLT_MAX;
	uint32_t best_wall_id = UINT32_MAX;
	vec2 best_intersect{ 0, 0 };

	for (uint32_t w = 0; w < poly.num_inds; w++) {
		uint32_t wall_id = poly.first_ind + w;
		int64_t cross_to = -wall_cross(wall_id, p_to);

		// crossed wall
		if (cross_to > 0) {
			vec2 intersect;
			if (wall_find_intersect(wall_id, p_from, p_to, intersect)) {
				real_t dist = p_from.distance_to(intersect);
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

	uint32_t linked_poly_id = get_link(best_wall_id);

	if (linked_poly_id == UINT32_MAX) {
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
		//DEV_ASSERT(poly_contains_point(p_poly_id, best_intersect));
		//return [poly_id, from, vel_dir, vel_mag_global]
		return TR_LIMIT;
	}

	// recurse into neighbouring cell
	return recursive_trace(p_depth + 1, p_from, p_to, linked_poly_id, r_info);
}

bool Mesh::poly_contains_point(uint32_t p_poly_id, const vec2 &p_pt) const {
	const Poly &poly = get_poly(p_poly_id);
	for (uint32_t n = 0; n < poly.num_inds; n++) {
		uint32_t wall_id = poly.first_ind + n;

		if (wall_in_front_cross(wall_id, p_pt)) {
			return false;
		}
	}

	return true;
}

bool Mesh::wall_in_front_cross(uint32_t p_wall_id, const vec2 &p_pt) const {
	return wall_cross(p_wall_id, p_pt) < 0;
}

int64_t Mesh::wall_cross(uint32_t p_wall_id, const vec2 &p_pt) const {
	vec2 wa, wb;
	get_wall_verts(p_wall_id, wa, wb);

	vec2 wall_vec = wb - wa;
	vec2 point_vec = p_pt - wa;
	return wall_vec.cross(point_vec);
}

real_t Mesh::find_height_on_poly(uint32_t p_poly_id, const vec2 &p_pt) const {
	Vector2 pt = vec2_to_float(p_pt);

	DEV_ASSERT(p_poly_id != UINT32_MAX);
	const Poly &poly = get_poly(p_poly_id);

	Vector3 intersection;
	bool hit = poly.plane.intersects_ray(Vector3(pt.x, 1000, pt.y), Vector3(0, -1, 0), &intersection);
	if (hit) {
		return intersection.y;
	}
	WARN_PRINT("find_height_on_poly failed");
	return 0.0;
}

bool Mesh::wall_find_intersect(uint32_t p_wall_id, const vec2 &p_from, const vec2 &p_to, vec2 &r_hit) const {
	vec2 wa, wb;
	get_wall_verts(p_wall_id, wa, wb);

	return find_lines_intersect_integer(wa, wb, p_from, p_to, r_hit);
}

// https://stackoverflow.com/questions/21224361/calculate-intersection-of-two-lines-using-integers-only
// intersect 2 lines using integer math
bool Mesh::find_lines_intersect_integer(const vec2 &p_from_a, const vec2 &p_to_a, const vec2 &p_from_b, const vec2 &p_to_b, vec2 &r_hit) const {
	int32_t x1 = p_from_a.x;
	int32_t y1 = p_from_a.y;
	int32_t x2 = p_to_a.x;
	int32_t y2 = p_to_a.y;
	int32_t x3 = p_from_b.x;
	int32_t y3 = p_from_b.y;
	int32_t x4 = p_to_b.x;
	int32_t y4 = p_to_b.y;

	// First line coefficients where "a1 x  +  b1 y  +  c1  =  0"
	int32_t a1 = y2 - y1;
	int32_t b1 = x1 - x2;
	int64_t c1 = x2 * y1 - x1 * y2;

	// Second line coefficients
	int32_t a2 = y4 - y3;
	int32_t b2 = x3 - x4;
	int64_t c2 = x4 * y3 - x3 * y4;

	int64_t denom = a1 * b2 - a2 * b1;

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
	int64_t offset = denom / 2;
	if (denom < 0) {
		offset = -offset;
	}

	int64_t x;
	int64_t y;

	{
		int64_t num = b1 * c2 - b2 * c1;
		if (num < 0) {
			x = num - offset;
		} else {
			x = num + offset;
		}
		x /= denom;
	}

	{
		int64_t num = a2 * c1 - a1 * c2;
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

} //namespace NavPhysics
