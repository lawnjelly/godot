// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#include "navphysics_mesh.h"
#include "navphysics_map.h"
#include "navphysics_mesh_instance.h"

namespace NavPhysics {

//void Mesh::set_transform(const Transform &p_xform, const Transform &p_xform_inv, bool p_is_identity) {
//	_transform = p_xform;
//	_transform_inverse = p_xform_inv;
//	_transform_identity = p_is_identity;
//}

freal Mesh::_inverse_timestep = 1.0 / 0.033;
freal Mesh::_timestep = 0.033;
u64 Mesh::_tick = 0;
u32 Mesh::_ticks_per_sec = 60;
bool Mesh::_timestep_initialized = false;
//freal Mesh::_tps_force_fudge = 1;

String Mesh::fverts_to_string() const {
	String sz = "fverts:\n";

	for (u32 n = 0; n < get_num_verts(); n++) {
		sz += String("\t") + n + String(" :\t") + get_fvert3(n) + "\n";
	}
	return sz;
}

String Mesh::verts_to_string() const {
	String sz = "verts:\n";

	for (u32 n = 0; n < get_num_verts(); n++) {
		sz += String("\t") + n + String(" :\t") + get_vert(n) + "\n";
	}

	return sz;
}

void Mesh::debug_poly(u32 p_poly_id) const {
	log(String("poly ") + p_poly_id);
	const Poly &poly = get_poly(p_poly_id);
	for (u32 n = 0; n < poly.num_inds; n++) {
		u32 ind = get_ind(poly.first_ind + n);
		log(String("\t") + n + " : " + get_vert(ind));
	}
}

void Mesh::_log(const String &p_string, int p_depth) const {
	NP_LOG(p_string);
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

u32 Mesh::find_best_jump_poly_within(const Agent &p_agent, const JumpFinderDataLocal &p_jl, i32 p_max_drop, i32 p_max_step_up, u32 &r_goodness_of_fit) const {
	// Go through the list of external connecting edges and see if
	// the jump crosses any.
	freal p_height = p_jl.pt_to_local.y;

	IPoint2 a, b;
	IPoint2 pt_hit;
	u32 best_poly_id = UINT32_MAX;

	i32 best_height = INT32_MIN;

	// Invariant.
	p_max_drop = -p_max_drop;

	for (u32 w = 0; w < data.external_wall_ids_final.size(); w++) {
		u32 wid = data.external_wall_ids_final[w];

		// If we are starting from the wrong side of the wall, ignore.
		i64 cross = wall_cross(wid, p_jl.from_local.xz());
		if (cross >= 0) {
			continue;
		}

		get_wall_verts(wid, a, b);

		// Collision detect the segments.
		if (!find_line_segments_intersect_integer(p_jl.from_local.xz(), p_jl.far_local.xz(), a, b, pt_hit)) {
			continue;
		}

		// Then check in more detail.
		//log(String("Mesh::find_best_jump_poly_within crossed to connecting wall ") + wid);
		u32 poly_id = get_wall(wid).poly_id;

		// Return information in the form of a jump,
		// but ONLY if the wall edge is IN FRONT of us
		// and not behind.
		IPoint2 pt_curr_perpendicular = p_jl.to_local.xz();
		IPoint2 vel = p_jl.to_local.xz() - p_jl.from_local.xz();
		pt_curr_perpendicular += IPoint2(vel.y, -vel.x);

		IPoint2 wall_vec = pt_curr_perpendicular - p_jl.to_local.xz();
		IPoint2 point_vec = pt_hit - p_jl.to_local.xz();
		cross = wall_vec.cross(point_vec);

		// We will allow the move EITHER
		// if the cross indicates the edge is AHEAD of the agent,
		// OR the agent is already on this poly, in which case,
		// we will use the current position for finding height.
		bool allow = false;

		bool ahead = cross > 0;

		if (ahead) {
			allow = true;
		} else {
			if (poly_contains_point(poly_id, p_jl.to_local.xz())) {
				pt_hit = p_jl.to_local.xz();
				allow = true;
			}
		}

		if (allow) {
			// Height of poly.
			i32 height = find_height_on_poly_plane(poly_id, pt_hit);

			// Calculate the height of the agent AFTER the jump to the ledge,
			// at the current velocity.
			// This is potentially slow calculation, or can use look up table.
			i32 agent_height_after_jump = _estimate_height_after_ledge_jump(p_agent, p_height, p_jl.vel_local.xz().lengthf(), (pt_hit - p_jl.to_local.xz()).lengthf());

			//log(String("\t\tpredicted jump to ledge is ") + (agent_height_after_jump - p_height) + " different than current agent height for poly ID " + poly_id + ".");

			// Get relative height of the step, relative to the agent.
			//			height -= p_height;
			height -= agent_height_after_jump;

#if 1
			if (height < p_max_drop) {
				continue;
			}
#endif

			// Relative to max step up
			height -= p_max_step_up;

			// TODO: This could be a blocker, if too high, but not above head height?
			if (height > 0) {
				//if (height > p_max_step_up) {
				//log("STEP UP TOO HIGH");
				continue;
			}

			// If stepping DOWN, higher the drop the better.
			if (height > best_height) {
				best_height = height;
				best_poly_id = poly_id;

				u32 fit = ABS(best_height);

				if (fit < r_goodness_of_fit) {
					AgentStatus::jump_target_vel = p_jl.vel_local;
					//AgentStatus::jump_target_jump_vel = p_jl.vel_local.height;
					AgentStatus::jump_target_cross_pos = p_jl.to_local;
					//AgentStatus::jump_target_local_height = p_jl.pt_to_local.y;

					if (ahead) {
						//log(String("Mesh::find_best_jump_poly_within JUMP to connecting wall ") + wid);
						AgentStatus::jump_wall_id = wid;
						AgentStatus::jump_target_wall_pos = pt_hit;
					} else {
						//log(String("Mesh::find_best_jump_poly_within CROSS to connecting wall ") + wid);
						AgentStatus::jump_wall_id = UINT32_MAX;
						AgentStatus::jump_cross_only = true;
					}
				}
			} else {
				//log(String("Mesh::find_best_jump_poly_within connecting wall ") + wid + " is not the best (height is " + height + ")");
			}
		}

		// NYI
	}

	if (best_poly_id != UINT32_MAX) {
		r_goodness_of_fit = ABS(best_height);
	}

	return best_poly_id;
}

void Mesh::_find_last_ditch_poly(Agent &r_agent, u32 p_poly_id, const FPoint3 &p_pt_local, const IPoint2 &p_pt_FP, freal &r_best_fit) const {
	const IRect2 bound = get_poly_bound(p_poly_id).get_rect2();
	u64 dist_from_bb_sq = bound.distance_squared(p_pt_FP);

	// Calculate in 64 bit.
	freal dist_from_bb = (double)_fp_to_f32_scale * dist_from_bb_sq;

	// Quick reject.
	if (dist_from_bb > r_best_fit) {
		return;
	}

	// Special case for inside.
	if (bound.contains_point(p_pt_FP) && poly_contains_point(p_poly_id, p_pt_FP)) {
		freal poly_height = find_float_height_on_poly_plane(p_poly_id, p_pt_FP);
		freal y_offset = (p_pt_local.y - poly_height);
		freal y_offset_squared = y_offset * y_offset;
		if (y_offset_squared < r_best_fit) {
			r_best_fit = y_offset_squared;
			r_agent.set_poly_id(p_poly_id);
			r_agent.set_wall_id(UINT32_MAX);
			r_agent.set_2d_pos(p_pt_FP);

			i32 fp_height = (poly_height + _f32_to_fp_offset.y) * _f32_to_fp_scale;
			r_agent.set_height(fp_height);
		}
		return;
	}

	const Poly &poly = get_poly(p_poly_id);

	u32 best_wall = UINT32_MAX;
	freal best_fraction = 0;

#if 0
	// Unit test
	{
		FPoint3 a(0, 0, 0);
		FPoint3 b(1, 0, 0);
		FPoint3 c(0.5f,  0, 1);
		freal f = 0;
		freal dist_sq = c.clamped_edge_projection(a, b, f);
		
		log(String("Unit test ") + p_pt_local);
		
	}
#endif

	// This is assuming we aren't inside the poly, it finds the min distance to the edges.
	log(String("\t_find_last_ditch_poly pt_local  ") + p_pt_local);
	for (u32 n = 0; n < poly.num_inds; n++) {
		u32 ind_a = get_ind(poly.first_ind + n);
		u32 ind_b = get_ind(poly.first_ind + ((n + 1) % poly.num_inds));

		const FPoint3 &a = get_fvert3(ind_a);
		const FPoint3 &b = get_fvert3(ind_b);

		freal f = 0;
		freal dist_sq = p_pt_local.clamped_edge_projection(a, b, f);

		log(String("\t\tedge ") + n + " [wall " + ind_a + "], (" + a.xz() + ") to (" + b.xz() + ") dist_sq " + dist_sq);

		if (dist_sq < r_best_fit) {
			r_best_fit = dist_sq;
			best_wall = ind_a;
			best_fraction = f;
		}
	}

	if (best_wall != UINT32_MAX) {
		log(String("\tbest_poly ") + p_poly_id + ", best_wall " + best_wall + ", fraction " + best_fraction);
		r_agent.set_poly_id(p_poly_id);
		r_agent.set_wall_id(best_wall);
		r_agent.set_2d_pos(_calculate_position_along_wall(best_wall, best_fraction));
	}
}

bool Mesh::find_best_poly_within_last_ditch_FINAL(Agent &r_agent, const FPoint3 &p_pt_local, const IPoint2 &p_pt2) const {
	//NP_DEV_ASSERT(r_agent.poly_id == UINT32_MAX);

	log(String("find_best_poly_within_last_ditch_FINAL : ") + p_pt2 + ", local space : " + p_pt_local);

	// First test the leaf.
	u32 num_test_polys = 0;
	const u32 *test_poly_ids = floor.poly_finder.find_leaf(p_pt2, num_test_polys);

	freal best_fit = FLT_MAX;

	log(String("\t") + num_test_polys + " test polys.");
	if (test_poly_ids) {
		for (u32 p = 0; p < num_test_polys; p++) {
			u32 poly_id = test_poly_ids[p];
			_find_last_ditch_poly(r_agent, poly_id, p_pt_local, p_pt2, best_fit);
		}
	}

	// Test ALL the polys, very slow, backup path.
	// (We could alternatively tile out from the first tile, but that's more complex.)

	// NOTE that this isn't correct completely.
	// The nearest poly could easily be outside the poly_finder leaf, but in most cases it should work.
	if (r_agent.is_poly_id_null()) {
		num_test_polys = get_num_polys();
		for (u32 p = 0; p < num_test_polys; p++) {
			_find_last_ditch_poly(r_agent, p, p_pt_local, p_pt2, best_fit);
		}
	}

	return !r_agent.is_poly_id_null();
}

bool Mesh::try_internal_jump_poly(Agent &r_agent, const IPoint2 &p_from, const IPoint2 &p_to, i32 p_ceiling_height) const {
	u32 current_poly_id = r_agent.get_poly_id();
	// i32 agent_height = r_agent.get_height();

	u32 jump_id = get_poly_extra(current_poly_id).jump_up_id;

	// Simplest case, no jumps available.
	if (jump_id == UINT32_MAX) {
		return UINT32_MAX;
	}

	const InternalJumpUp &j = jump_data.internal_jump_ups[jump_id];
	IPoint2 pt_hit;
	i32 poly_height = 0;

	// Best result.
	i32 best_poly = UINT32_MAX;
	u32 best_wall_id = UINT32_MAX;
	u64 best_dist = UINT64_MAX;
	IPoint2 best_hit;
	//i32 best_poly_height = 0;

	// Check each jump wall.
	for (u32 n = 0; n < j.num_jump_walls; n++) {
		u32 wall_id = jump_data.jump_wall_ids[j.first_jump_wall + n];
		const Wall &w = get_wall(wall_id);

		NP_DEV_ASSERT(w.poly_id != UINT32_MAX);

		bool on_poly = false;

		if (poly_contains_point(w.poly_id, p_from)) {
			poly_height = find_height_on_poly_plane(w.poly_id, p_to);
			on_poly = true;
		} else if (wall_find_intersect(wall_id, p_from, p_to, pt_hit)) {
			poly_height = find_height_on_poly_plane(w.poly_id, pt_hit);
		} else {
			continue;
		}

		if (poly_height < p_ceiling_height) {
			continue;
		}

		// Got a hit!
		log(String("can jump to poly ") + w.poly_id);

		if (on_poly) {
			best_poly = w.poly_id;
			best_dist = 0;
			//best_poly_height = poly_height;
			best_wall_id = UINT32_MAX;
			break;
		} else {
			u64 dist = (pt_hit - p_from).length_squared();
			if (dist < best_dist) {
				best_dist = dist;
				best_poly = w.poly_id;
				best_hit = pt_hit;
				best_wall_id = wall_id;
				//best_poly_height = poly_height;
			}
		}
	}

	if (best_poly != UINT32_MAX) {
		if (best_wall_id == UINT32_MAX) {
			r_agent.set_poly_id(best_poly);
			r_agent.clear_wall_id();
		} else {
			r_agent.clear_poly_id();
			r_agent.clear_wall_id();
			r_agent.jump_link_wall_id = best_wall_id;
			r_agent.jump_link_target_pos = best_hit;
			r_agent.force_off_floor();
		}

		return true;
	}

	return false;
}

u32 Mesh::find_best_poly_within(const IPoint3 &p_pt, i32 p_max_drop, i32 p_max_step_up, u32 *r_goodness_of_fit) const {
	IPoint2 pt2 = p_pt.xz();

	u32 num_test_polys = 0;
	const u32 *test_poly_ids = floor.poly_finder.find_leaf(pt2, num_test_polys);

	if (!test_poly_ids) {
		return UINT32_MAX;
	}

	i32 best_height = -INT32_MAX;
	u32 best_poly_id = UINT32_MAX;

	// Invariant.
	p_max_drop = -p_max_drop;

	//log(String("find_best_poly_within"));
	for (u32 p = 0; p < num_test_polys; p++) {
		u32 pid = test_poly_ids[p];
		//log(String("\t") + pid);

		if (poly_contains_point(pid, pt2)) {
			// Height of poly.
			i32 height = find_height_on_poly_plane(pid, pt2);

			// Get relative height.
			height -= p_pt.y;

#if 1
			if (height < p_max_drop) {
				continue;
			}
#endif
			// Relative to max step up
			height -= p_max_step_up;

			// Stepping higher than max step not allowed.
			if (height > 0) {
				continue;
			}

			// If stepping DOWN, higher the drop the better.
			if (height > best_height) {
				best_height = height;
				best_poly_id = pid;
			}
		}
	}

	if (best_poly_id != UINT32_MAX && r_goodness_of_fit) {
		*r_goodness_of_fit = ABS(best_height);
		//r_goodness_of_fit = dist_fixed_point_to_float(ABS(best_height));
	}

#if 0
	// slow only use when teleporting
	for (u32 p = 0; p < get_num_polys(); p++) {
		if (poly_contains_point(p, p_pt)) {
			freal height = find_height_on_poly(p, p_pt);
			height = comparison_height - height;

			if (height < 0) {
				// Too high for a step up.
				continue;
			}

			// Lower the "drop" the better...
			// height = -height;

			if (height < best_height) {
				log(String("find_best_poly_within"));
				for (u32 k = 0; k < num_test_polys; k++) {
					u32 pid = test_poly_ids[k];
					log(String("\t") + pid);
				}

				best_height = height;
				best_poly_id = p;
			}
		}
	}
#endif

	//log(String("find_best_poly_within returned ") + best_poly_id);
	return best_poly_id;
}

u32 Mesh::find_poly_within(const IPoint2 &p_pt, u32 p_poly_id_hint) const {
	u32 num_test_polys = 0;
	const u32 *test_poly_ids = floor.poly_finder.find_leaf(p_pt, num_test_polys);

	if (!test_poly_ids) {
		return UINT32_MAX;
	}

	if (p_poly_id_hint != UINT32_MAX) {
		NP_DEV_ASSERT(p_poly_id_hint < get_num_polys());
		if (poly_contains_point(p_poly_id_hint, p_pt)) {
			return p_poly_id_hint;
		}
	}

	for (u32 p = 0; p < num_test_polys; p++) {
		if (poly_contains_point(test_poly_ids[p], p_pt)) {
			// print("within poly " + str(p))
			return test_poly_ids[p];
		}
	}

	// slow only use when teleporting
	//	for (u32 p = 0; p < get_num_polys(); p++) {
	//		if (poly_contains_point(p, p_pt)) {
	//			// print("within poly " + str(p))
	//			return p;
	//		}
	//	}

	return UINT32_MAX;
}

void Mesh::modify_velocity_for_poly_slope(const MoveInfo &p_info, IPoint2 &r_vel, u32 p_poly_id) const {
	if (!p_info.agent) {
		return;
	}

	if (p_poly_id == UINT32_MAX) {
		return;
	}

	float length = r_vel.lengthf();
	if (length < 1) {
		return;
	}

	float inv_length = 1 / length;

	const Plane &plane = get_poly(p_poly_id).plane;
	FPoint3 vel = FPoint3::make(r_vel.x * inv_length, 0, r_vel.y * inv_length);

	// dot is now -1 to 1
	float dot = vel.dot(plane.normal);

	// exp effect
	if (dot >= 0) {
		dot *= dot;
		dot *= p_info.agent->downhill_modifier;
	} else {
		dot *= dot;
		dot *= p_info.agent->uphill_modifier;
	}

	//log(dot);
	dot += 1;
	r_vel.normalize_to_scale(dot * length);
}

void Mesh::modify_velocity_for_poly_wall(const MoveInfo &p_info, float &r_vel, u32 p_poly_id, u32 p_wall_id) const {
	if (!p_info.agent) {
		return;
	}

	if (p_poly_id == UINT32_MAX) {
		return;
	}

	// Polarity of velocity determines direction along wall.
	bool direction = r_vel >= 0;
	float abs_vel = direction ? r_vel : -r_vel;

	if (abs_vel < 1) {
		return;
	}

	const Wall &wall = get_wall(p_wall_id);
	FPoint3 wall_dir = FPoint3::make(wall.wall_vec.x, 0, wall.wall_vec.y);
	wall_dir.normalize();

	if (!direction) {
		wall_dir = -wall_dir;
	}

	const Plane &plane = get_poly(p_poly_id).plane;
	float dot = wall_dir.dot(plane.normal);

	// exp effect
	if (dot >= 0) {
		dot *= dot;
		dot *= p_info.agent->downhill_modifier;
	} else {
		dot *= dot;
		dot *= p_info.agent->uphill_modifier;
	}

	//log(dot);
	dot += 1;
	r_vel *= dot;
}

Mesh::CrossLinkResult Mesh::cross_internal_or_external_link(u32 p_wall_id, const IPoint2 &p_from, const IPoint2 &p_vel, MoveInfo &r_info) const {
	if (is_link_external(p_wall_id)) {
		if (move_to_new_mesh(p_from, p_vel, r_info)) {
			/*
			r_info.poly_id = UINT32_MAX;
			r_info.pos_reached = p_from + p_vel;
			r_info.wall_id = UINT32_MAX;
			r_info.remaining_velocity = p_vel.lengthf();
			*/
			r_info.poly_id = r_info.agent->get_poly_id();
			r_info.pos_reached = r_info.agent->get_pos().xz();
			r_info.wall_id = UINT32_MAX;
			r_info.remaining_velocity = 0;
			r_info.new_mesh_instance_id = r_info.agent->get_mesh_instance_id();
			return CrossLinkResult::CROSSED_TO_NEW_MESH;
		}
	}

	if (is_link_internal(p_wall_id)) {
		JumpWithinMeshResult res = jump_within_mesh(*r_info.agent, p_wall_id, p_from, p_vel, r_info);

		switch (res) {
			case JumpWithinMeshResult::CROSSED_TO_WALL: {
				r_info.poly_id = UINT32_MAX;
				r_info.pos_reached = p_from + p_vel;
				r_info.wall_id = UINT32_MAX;
				r_info.remaining_velocity = p_vel.lengthf();
				r_info.new_mesh_instance_id = r_info.agent->get_mesh_instance_id();
				NP_LOG(String("remaining velocity ") + r_info.remaining_velocity);
				return CrossLinkResult::CROSSED_TO_NEW_MESH;
			} break;
			case JumpWithinMeshResult::CROSSED_TO_POLY: {
				//				r_info.poly_id = r_info.agent->get_poly_id();
				//				r_info.pos_reached = p_from;
				//				r_info.wall_id = UINT32_MAX;
				//				r_info.remaining_velocity = p_vel.lengthf();
				//r_info.new_mesh_instance_id = r_info.agent->get_mesh_instance_id();
				return CrossLinkResult::CROSSED_INTERNAL;
			} break;
			default:
				break;
		}
	}

	return CrossLinkResult::FAILED;
}

IPoint2 Mesh::_calculate_position_along_wall(u32 p_wall_id, freal p_fraction) const {
	// NP_DEV_ASSERT(p_fraction >= 0);
	// NP_DEV_ASSERT(p_fraction <= 1);

	const Wall &wall = get_wall(p_wall_id);
	const IPoint2 &wall_start = get_vert2(wall.vert_a);
	return wall_start + (wall.wall_vec * p_fraction);
}

void Mesh::slide_move_on_blocks(u32 p_poly_from_id, u32 p_poly_id, const IPoint2 &p_from, IPoint2 &r_vel, const MoveInfo &p_info) const {
	const Vector<np_handle> *prev_list = nullptr;
	const Vector<np_handle> *curr_list = nullptr;

	u32 total_blocks = 0;

	if (p_poly_from_id < UINT32_MAX - 1) {
		prev_list = &p_info.mesh_instance->get_poly_block_lists()[p_poly_from_id];
		total_blocks += prev_list->size();
	}
	if (p_poly_id < UINT32_MAX - 1) {
		curr_list = &p_info.mesh_instance->get_poly_block_lists()[p_poly_id];
		total_blocks += curr_list->size();
	}

	// Cheapest option, no blocks.
	if (!total_blocks) {
		return;
	}

	// Create a unique list of all blocks affecting both polys.
	// This is kind of overly complex, but usually there should be few blocks to deal with on each move.

	// Allocate for worst case, there will likely be duplicates.
	// But still place a maximum, so as not to blow the stack.
	// If we hit more than max blocks, some of them just won't collide detect.
	if (total_blocks > 256) {
		total_blocks = 256;
		NP_WARN_PRINT_ONCE("Too many blocks during a move, some collisions may be ignored.");
	}
	StackVector<np_handle> blocks;
	blocks.setup_external((np_handle *)alloca(sizeof(np_handle) * total_blocks), total_blocks);

	if (prev_list) {
		for (u32 n = 0; n < prev_list->size(); n++) {
			if (blocks.is_full()) {
				break;
			}
			blocks.push_back((*prev_list)[n]);
		}
	}

	if (curr_list) {
		for (u32 n = 0; n < curr_list->size(); n++) {
			if (blocks.is_full()) {
				break;
			}
			// Only push if not already there from the previous poly.
			blocks.check_and_push_back((*curr_list)[n]);
		}
	}

	// Now collide detect.
	IPoint2 new_pos = p_from + r_vel;
	u64 old_vel_length_sq = r_vel.length_squared();

	for (u32 n = 0; n < blocks.size(); n++) {
		Block *block = NPWORLD.safe_get_block(blocks[n]);
		if (block && block->data.active) {
			if (block->data.bound.contains_point(new_pos)) {
				bool from_outside = false;
				bool to_outside = false;

				for (u32 w = 0; w < 4; w++) {
					const IPoint2 &a = block->data.corners[w];
					const IPoint2 &b = block->data.corners[(w + 1) % 4];

					if (IPoint2::which_side(a, b, p_from) > 0) {
						from_outside = true;
					}
					if (IPoint2::which_side(a, b, new_pos) > 0) {
						to_outside = true;
					}
				}

				if (to_outside == true) {
					continue;
				}

				u64 best_dist = UINT64_MAX;
				u32 best_wall = UINT32_MAX;
				IPoint2 best_hit;

				IPoint2 pt_hit;
				for (u32 w = 0; w < 4; w++) {
					const IPoint2 &a = block->data.corners[w];
					const IPoint2 &b = block->data.corners[(w + 1) % 4];

					if (find_lines_intersect_integer(a, b, p_from, new_pos, pt_hit)) {
						u64 dist = (pt_hit - p_from).length_squared();
						if (dist < best_dist) {
							best_dist = dist;
							best_wall = w;
							best_hit = pt_hit;
						}
					}
				}

				if (best_wall != UINT32_MAX) {
					const IPoint2 &a = block->data.corners[best_wall];
					const IPoint2 &b = block->data.corners[(best_wall + 1) % 4];

					r_vel.slide_velocity_on_wall(a, b);

					// Also check if we are inside completely. If so, push out to the wall.
					if (!from_outside && !to_outside) {
						r_vel += (best_hit - p_from);
					}
				}
			}
		}

#ifdef NP_DEV_ENABLED
		if (!block) {
			NP_WARN_PRINT_ONCE("Invalid block detected in poly list.");
		}
#endif
	}

	// Limit r_vel to the original velocity, to prevent adding momentum to the agent.
	// (It can get increased by the addition to move to the side of a block when penetrating.
	u64 new_vel_length_sq = r_vel.length_squared();

	if (new_vel_length_sq > old_vel_length_sq) {
		freal capped_vel = Math::sqrt_real(old_vel_length_sq);
		capped_vel = MAX(capped_vel, 1024);
		r_vel.normalize_to_scale(capped_vel);
	}
}

Mesh::MoveResult Mesh::recursive_move(i32 p_depth, IPoint2 p_from, IPoint2 p_vel, u32 p_poly_id, u32 p_poly_from_id, u32 p_hug_wall_id, MoveInfo &r_info) const {
	if (p_depth >= 8) {
		NP_LLOG("\t\trecursive_move depth limit reached");
		r_info.poly_id = p_poly_id;
		r_info.pos_reached = p_from;
		r_info.wall_id = p_hug_wall_id;
		return MR_LIMIT;
	}

	NP_DEV_ASSERT(debug_check_agent_integrity(p_from, p_poly_id, p_hug_wall_id));

	// Not all polys are flat.
	// If we are going uphill, slowdown,
	// if downhill, speedup.
	if (r_info.on_floor) {
		modify_velocity_for_poly_slope(r_info, p_vel, p_poly_id);
	}

	freal vel_length = p_vel.lengthf();

	if (vel_length < 0.001f) {
		NP_LLOG("\t\trecursion ending, vel_mag is zero");
		r_info.poly_id = p_poly_id;
		r_info.pos_reached = p_from;
		r_info.wall_id = p_hug_wall_id;
		return MR_OK;
	}

	NP_LLOG(String("\trecursive_move depth ") + p_depth + " pos " + p_from.readable() + " vel " + p_vel.readable());

	// Slide off doors etc.
	slide_move_on_blocks(p_poly_from_id, p_poly_id, p_from, p_vel, r_info);

	// are we moving along a wall?
	if (p_hug_wall_id != UINT32_MAX) {
		// are we still heading along the wall?
		const Wall &wall = get_wall(p_hug_wall_id);
		freal dot = wall.normal.dot_normalized(p_vel);
		if (dot < 0.001f) {
			// hugging wall

			// First try and cross the hug wall (if it's a link).
			CrossLinkResult cross_res = cross_internal_or_external_link(p_hug_wall_id, p_from, p_vel, r_info);
			switch (cross_res) {
				case CrossLinkResult::CROSSED_TO_NEW_MESH: {
					return MR_OK;
				} break;
				case CrossLinkResult::CROSSED_INTERNAL: {
					return recursive_move(p_depth + 1, p_from, p_vel, r_info.agent->get_poly_id(), p_poly_from_id, UINT32_MAX, r_info);
				} break;
				default:
					break;
			}

			freal wall_length = wall.wall_vec.lengthf();
			const IPoint2 &wall_start = get_vert2(wall.vert_a);
			const IPoint2 &wall_end = get_vert2(wall.vert_b);
			freal dist_along_wall = p_from.distancef_to(wall_start);
			// freal dist_remaining = wall_length - dist_along_wall;

			freal wall_dot = wall.wall_vec.dot_normalized(p_vel);

			// change move length according to angle with wall
			vel_length *= wall_dot;

			modify_velocity_for_poly_wall(r_info, vel_length, p_poly_id, p_hug_wall_id);

			// directly calculate new position
			freal dist = dist_along_wall + vel_length;

			freal fract = dist / wall_length;
			//IPoint2 to = wall_start + (wall.wall_vec * fract);
			IPoint2 to = _calculate_position_along_wall(p_hug_wall_id, fract);
			NP_LLOG(String("\t\twall dot: ") + String(wall_dot));

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
					if (is_link_external(p_hug_wall_id)) {
						NP_LLOG(String("\t\tleaving hug wall ") + itos(p_hug_wall_id));
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
					if (is_link_external(p_hug_wall_id)) {
						NP_LLOG(String("\t\tleaving hug wall ") + itos(p_hug_wall_id));
						p_hug_wall_id = UINT32_MAX;
					} else {
						const Wall &next_wall = get_wall(p_hug_wall_id);
						return recursive_move(p_depth + 1, wall_start, p_vel, next_wall.poly_id, p_poly_from_id, p_hug_wall_id, r_info);
					}
				}
			}
		} else {
			NP_LLOG(String("\t\tleaving hug wall ") + p_hug_wall_id);
			p_hug_wall_id = UINT32_MAX;
		}
	}

#ifdef NP_DEV_EXCESSIVE_CHECKS
	if (p_poly_from_id < UINT32_MAX - 1) {
		if ((!poly_contains_point(p_poly_from_id, p_from))) {
			debug_poly_contains_point(p_poly_from_id, p_from);
			NP_DEV_ASSERT(poly_contains_point(p_poly_from_id, p_from));
		}
	}
#endif

	// new destination
	IPoint2 to = p_from + p_vel;

#ifdef NP_DEV_EXCESSIVE_CHECKS
	NP_LLOG(String("\trecursive_move [") + itos(p_depth) + "] poly " + itos(p_poly_id) + " from " + p_from + " to " + str(to) + " ... vel " + str(p_vel), p_depth);
#endif

	TraceInfo trace_info;
	TraceResult res = recursive_trace(0, p_from, to, p_poly_id, trace_info);
	if (res == TR_LIMIT) {
		r_info.poly_id = trace_info.poly_id;
		r_info.pos_reached = trace_info.hit_point;
		r_info.wall_id = UINT32_MAX;
		return MR_LIMIT;
	}

	if (res == TR_CLEAR) {
		NP_LLOG2("\t\tmove ok", p_depth);
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

	NP_LLOG(String("\t\tpt_intersect ") + pt_intersect.readable());

	// account for possibility we are going through a connection to a new mesh instance
	CrossLinkResult cross_res = cross_internal_or_external_link(wall_id, p_from, p_vel, r_info);
	switch (cross_res) {
		case CrossLinkResult::CROSSED_TO_NEW_MESH: {
			return MR_OK;
		} break;
		case CrossLinkResult::CROSSED_INTERNAL: {
			return recursive_move(p_depth + 1, pt_intersect, p_vel, r_info.agent->get_poly_id(), p_poly_from_id, UINT32_MAX, r_info);
		} break;
		default:
			break;
	}

	// reduce the velocity by how far travelled to the wall
	NP_CHECK_32(pt_intersect.x, -, p_from.x);
	NP_CHECK_32(pt_intersect.y, -, p_from.y);
	p_vel -= (pt_intersect - p_from);

	p_hug_wall_id = wall_id;

	// If at high momentum, shift the velocity to bounce
	// on the next recursive move (shares the code path with
	// wall sliding, but will leave the wall slide on the next
	// recurse).
	bounce_on_wall(wall_id, p_vel, r_info.momentum);

	return recursive_move(p_depth + 1, pt_intersect, p_vel, p_poly_id, p_poly_from_id, p_hug_wall_id, r_info);
}

bool Mesh::bounce_on_wall(u32 p_wall_id, IPoint2 &r_vel, float &r_momentum) const {
	// Account for delta.
	f32 momentum = r_momentum * _inverse_timestep;

	// This threshold should depend on FPoint2::FP_RANGE
	constexpr f32 threshold = 50000 * ((f64)FPoint2::FP_RANGE / 65535);

	// Revert to sliding with small momentum
	if (momentum <= threshold) {
		return false;
	}

	// Revert to sliding with small velocity
	FPoint2 fvel = r_vel.to_f32();
	//	if (fvel.length_squared() < 100) {
	//		return false;
	//	}

	// Calculate a bounce factor dependent on momentum
	// (this should be user adjustable).
	float bounce = momentum - threshold;

	constexpr f32 bounce_divisor = 30000 * ((f64)FPoint2::FP_RANGE / 65535);

	bounce /= bounce_divisor;
	bounce = MIN(bounce, 1.0f);
	//log(String("\t\tbouncing on wall ") + p_wall_id + ", momentum " + momentum + ", bounce " + bounce);

	NP_DEV_ASSERT(p_wall_id != UINT32_MAX);
	const Wall &wall = get_wall(p_wall_id);

	FPoint2 fnorm = wall.normal.to_f32();
	fnorm.normalize();

	freal dot = fnorm.dot(fvel);

	bounce += 1.0f;
	//FPoint2 new_vec = (fvel) + (fnorm * -dot * 2);
	FPoint2 new_vec = (fvel) + (fnorm * (-dot * bounce));

#ifdef NP_DEBUG_BOUNCE
	String sz = String("bounce normal: ") + fnorm + ", before: " + r_vel + ", after: ";
#endif
	r_vel.round_from(new_vec);
#ifdef NP_DEBUG_BOUNCE
	sz += r_vel;
	log(sz);
#endif

	//r_vel.zero();
	//r_vel = -r_vel;
	return true;
}

freal Mesh::_calculate_jump_range_from_momentum(freal p_momentum, bool p_apply_minimum) const {
	// Momentum should decide the max allowable jump vector.
	freal r = Mesh::_inverse_timestep * p_momentum;
	//	freal scaled_momentum = Mesh::_inverse_timestep * p_momentum * 2;

	if (p_apply_minimum) {
		freal min_jump_distance = extension_data.agent_radius * 2;
		//scaled_momentum += min_jump_distance;
		r = MAX(r, min_jump_distance);
	}

	//log(String("Jump possible distance ") + scaled_momentum);
	return r;
}

Mesh::JumpWithinMeshResult Mesh::jump_within_mesh(Agent &r_agent, u32 p_source_wall_id, const IPoint2 &p_from, const IPoint2 &p_vel, MoveInfo &r_info) const {
	const Wall &source_wall = get_wall(p_source_wall_id);

	// There are no jump links, can't possibly jump.
	if (source_wall.jump_info_id == UINT32_MAX) {
		return JumpWithinMeshResult::FAILED;
	}

	// Is the agent allowed to use jump links?
	if (r_info.agent->guard_internal_jump_links) {
		return JumpWithinMeshResult::FAILED;
	}

	const InternalJumpInfo &info = jump_data.internal_jump_info[source_wall.jump_info_id];

	// Quick reject (maybe these can be removed as a pre-calculation?).
	if (!info.num_jump_polys && !info.num_jump_walls) {
		return JumpWithinMeshResult::FAILED;
	}

	// Agent height determines whether step up and step down to destinations is possible.
	i32 agent_height = r_info.agent->get_height();
	i32 agent_jump_height = agent_height - r_info.agent->floor_height;

	// Momentum should decide the max allowable jump vector.
	freal scaled_momentum = _calculate_jump_range_from_momentum(r_info.momentum, true);

	// Construct max allowable jump vector.
	IPoint2 jump_vec = p_vel;
	//jump_vec.normalize_to_scale(extension_data.agent_radius * 4);
	jump_vec.normalize_to_scale(scaled_momentum);

	// Hypothetical jump destination, to find crossing with the possible edges.
	IPoint2 jump_dest = p_from + jump_vec;

	// First try the easiest, have we landed directly on a jump poly?
	i32 highest_poly_height = INT32_MIN;
	u32 best_jump_poly = UINT32_MAX;

	for (u32 n = 0; n < info.num_jump_polys; n++) {
		u32 poly_id = jump_data.jump_poly_ids[info.first_jump_poly + n];
		if (poly_contains_point(poly_id, p_from)) {
			i32 poly_height = find_height_on_poly_plane(poly_id, p_from);

			if (poly_height > highest_poly_height) {
				highest_poly_height = poly_height;
				best_jump_poly = poly_id;
			}
		}
	}

	if (best_jump_poly != UINT32_MAX) {
		// We can transfer directly to this poly below.
		r_agent.set_poly_id(best_jump_poly);
		r_agent.clear_wall_id();
		r_agent.on_floor = false;

		return JumpWithinMeshResult::CROSSED_TO_POLY;
	}

	// Go through each jump link and find the best (if there is one).
	for (u32 n = 0; n < info.num_jump_walls; n++) {
		u32 dest_wall_id = jump_data.jump_wall_ids[info.first_jump_wall + n];

		IPoint2 local_jump_dest = jump_dest;

		// When we cross the jump wall we will be defined by a hug wall and a fraction.
		if (_can_jump_to_wall(dest_wall_id, p_from, local_jump_dest, agent_height, agent_jump_height, r_info.on_floor)) {
			// Do something
			//log("Jump allowed");
			AgentStatus::jump_target_wall_pos = local_jump_dest;
			AgentStatus::jump_wall_id = dest_wall_id;
			//			r_info.jump.target_pos = local_jump_dest;
			//			r_info.jump.wall_id = dest_wall_id;

			//log(String("Jump start velocity ") + p_vel);

			return JumpWithinMeshResult::CROSSED_TO_WALL;
		}
	}

	return JumpWithinMeshResult::FAILED;
}

i32 Mesh::_estimate_height_after_ledge_jump(const Agent &p_agent, i32 p_agent_height, freal p_forward_velocity, freal p_dist_to_ledge) const {
	// Approx ticks to ledge.
	u32 ticks_to_ledge = u32(p_dist_to_ledge / p_forward_velocity);

	// Predict height of agent after these ticks.
	i32 height = p_agent_height;
	i32 jump_vel = p_agent.get_jump_velocity();

	// TODO: This is not accounting for not straight down gravity!!
	i32 gravity = dist_float_to_fixed_point(p_agent.gravity);

	for (u32 n = 0; n < ticks_to_ledge; n++) {
		height += jump_vel;
		jump_vel -= gravity;
	}

	return height;

	//	// Momentum should decide the max allowable jump vector.
	//	freal r = Mesh::_inverse_timestep * p_momentum * 0.3f;
	//	//	freal scaled_momentum = Mesh::_inverse_timestep * p_momentum * 2;

	//	if (p_apply_minimum) {
	//		freal min_jump_distance = extension_data.agent_radius * 2;
	//		//scaled_momentum += min_jump_distance;
	//		r = MAX(r, min_jump_distance);
	//	}

	//log(String("Jump possible distance ") + scaled_momentum);
	//	return r;
}

// If we can jump to a wall, we calculate the wall fraction as a destination.
bool Mesh::_can_jump_to_wall(u32 p_dest_wall_id, const IPoint2 &p_from, IPoint2 &p_to, i32 p_agent_height, i32 p_agent_jump_height, bool p_on_floor) const {
	//IPoint2 to = p_to;
	IPoint2 pt_hit;
	if (wall_segments_find_intersect(p_dest_wall_id, p_from, p_to, pt_hit)) {
		p_to = pt_hit;

		// Check whether we pass the height requirement.
		const Wall &dest_wall = get_wall(p_dest_wall_id);
		i32 poly_height = find_height_on_poly_plane(dest_wall.poly_id, pt_hit);

		i32 height_change = poly_height - p_agent_height;

		if (height_change > 0) {
			// Needs to be jumping unless it is a drop.
			if (p_on_floor) {
				//log("\tjump wall needs jump");
				return false;
			}
			if (height_change > 0) {
				//if (height_change > mesh_params.exit_max_step_up) {
				// Too high
				//log("\tjump wall too high");
				return false;
			}
		} else {
			/*
			NP_DEV_ASSERT(mesh_params.fp_exit_max_drop != INT32_MAX);
			if (height_change < -(mesh_params.fp_exit_max_drop + p_agent_jump_height)) {
				// Too low
				//log("\tjump wall too low");
				//continue;
			}
			*/
		}

#if 0
		//wall_segments_find_intersect(p_dest_wall_id, p_from, p_to, pt_hit);
		IPoint2 a, b;
		get_wall_verts(p_dest_wall_id, a, b);
		String sz = String("_can_jump_to_wall from ") + p_from + " to " + p_to;
		sz += String(", wall ") + a + " -> " + b;
		sz += String(", intersect at  ") + pt_hit;
		log(sz);
#endif

		//		float dist_a_to_hit = (pt_hit - a).lengthf();
		//		float dist_a_to_b = (b - a).lengthf();

		//		if (dist_a_to_b) {
		//			r_wall_fraction = dist_a_to_hit / dist_a_to_b;
		//			r_wall_fraction = CLAMP(r_wall_fraction, 0.0f, 1.0f);
		//		} else {
		//			r_wall_fraction = 0;
		//		}

		return true;
	}
	//log("\tjump wall no hit");

	return false;
}

bool Mesh::move_to_new_mesh(const IPoint2 &p_from, const IPoint2 &p_vel, MoveInfo &r_info) const {
	if (r_info.agent->guard_external_jump_links) {
		return false;
	}

	// We are constructing 3 points:
	// Current (where we are now plus velocity (i.e. after the move)), IPoint2 and agent height
	// Near (where we are now but backtracked by the current velocity)
	// Far (where we are now plus the velocity times a distance related to the agent radius)

	// Ideally we would calculate agent height at each, but for now we are just using the current height for all.

	//IPoint2 to = p_from + p_vel;

	// Construct a far vector for jumps to ledges on the next mesh.
	// Momentum should decide the max allowable jump vector.
	freal scaled_momentum = _calculate_jump_range_from_momentum(r_info.momentum, false);

	IPoint2 far_vel = p_vel;
	far_vel.normalize_to_scale(scaled_momentum);

	// Ensure that far is always further than to.
	//IPoint2 far = to + far_vel;

	// Use a point near backtracking from the current direction,
	// so we can catch already crossed edges.
	IPoint2 near_vector = -p_vel;
	near_vector.normalize_to_scale(extension_data.agent_radius);
	//IPoint2 near = p_from + near_vector;
	//////////////////////////////////////////////////

	Map &map = NavPhysics::g_world.get_map(r_info.map_id);
	Agent &agent = NavPhysics::g_world.get_body(r_info.agent_id);

	// Get the coords into world space.
	u32 old_mesh_instance_id = agent.get_mesh_instance_id();
	NP_DEV_ASSERT(old_mesh_instance_id != UINT32_MAX);

	MeshInstance &old_mesh_instance = NavPhysics::g_world.get_mesh_instance(old_mesh_instance_id);

	// only update the f32ing point position if significantly different
	//agent.fpos = fixed_point2_to_float2(to);

	JumpFinderData jfd;
	/*
		// Use the jump height, rather than the floor height,
		// for assessing whether we can make the link.
	#define NP_MOVE_FP_TO_WORLD_SPACE(FP, WORLD_SPACE) \
		jfd.WORLD_SPACE = old_mesh_instance.local_fixed_point_to_world_space(*this, FP)

	#ifdef NP_VERIFY_JUMP_FINDER_VELOCITY
		NP_MOVE_FP_TO_WORLD_SPACE((to + p_vel), pt_verify_curr_plus_vel)
	#endif


		NP_MOVE_FP_TO_WORLD_SPACE(near, pt_from_world);
		NP_MOVE_FP_TO_WORLD_SPACE(far, pt_far_world);
		NP_MOVE_FP_TO_WORLD_SPACE(to, pt_to_world);

	#undef NP_MOVE_FP_TO_WORLD_SPACE
	*/
	// Get the velocity into world space from the old mesh coordinate space.
	//jfd.vel = old_mesh_instance.local_velocity_to_world_space(*this, p_vel);

	jfd.vel_world = old_mesh_instance.velocity_local_fixed_point_to_world_space(*this, agent.get_velocity());

	jfd.pt_from_world = old_mesh_instance.local_fixed_point_to_world_space(*this, agent.get_pos());
	jfd.pt_to_world = jfd.pt_from_world + jfd.vel_world;
	jfd.pt_far_world = jfd.pt_from_world + (jfd.vel_world * 2);

	//jfd.vel_world = old_mesh_instance.local_velocity_with_jump_velocity_to_world_space(*this, p_vel, agent.get_jump_velocity());
	// log(String("world vel before jump calculated as : ") + jfd.vel_world);

	//log(String("move_to_n ew_mesh pt_curr (to) in WORLD_SPACE ") + jfd.pt_curr);

	//u32 mesh_instance_id = map.find_best_fit_agent_mesh(agent, agent.fpos3, old_mesh_instance_id, &jfd);

	// Use new generatic jump mesh routine.
	u32 old_poly_id = agent.get_poly_id();
	u32 old_wall_id = agent.get_wall_id();
	agent.set_mesh_instance_id(UINT32_MAX);
	if (!map.jump_to_new_mesh(agent, agent.fpos3, old_mesh_instance_id, jfd)) {
		agent.set_poly_id(old_poly_id);
		agent.set_wall_id(old_wall_id);
		agent.set_mesh_instance_id(old_mesh_instance_id);

		return false;
	}

	return true;

#if 0
	// If we started a jump, hack to force later check to pass.
	// TODO - can be improved, maybe by passing r_info to find_best_fit_agent_mesh()?
	//	if (agent.is_in_jump_link())
	//	{
	//		r_info.jump.wall_id = agent.jump_link_wall_id;
	//		r_info.jump.target_pos = agent.jump_link_target_pos;
	//	}

	if (mesh_instance_id != UINT32_MAX) {
		r_info.new_mesh_instance_id = mesh_instance_id;

		// Rejig the height for the new mesh...
		return true;
	}

	return false;
#endif
}

bool Mesh::debug_check_agent_integrity(const IPoint2 &p_pos, u32 p_poly_id, u32 p_hug_wall_id) const {
#ifdef NP_DEV_ENABLED

	for (u32 n = 0; n < 2; n++) {
		if (ABS(p_pos.coord[n]) > NAVPHYSICS_MESH_FP_RANGE_WITH_LIP) {
			return false;
		}
	}
	//NP_CHECK_32(y2 - y1);

	// The location of an agent is either defined by position and poly id,
	// OR by position and hug_wall_id, in which case, the poly id isn't guaranteed
	// to be correct.
	if (p_poly_id != UINT32_MAX) {
		if (p_hug_wall_id == UINT32_MAX) {
			return poly_contains_point_debug(p_poly_id, p_pos);
		}
	}
#endif
	return true;
}

bool Mesh::ray_test_xz(const LocalPoint3 &p_from, const LocalPoint3 &p_to) const {
	u32 poly_id = p_from.poly_id;
	IPoint2 best_intersect;

	IPoint2 pt_from = p_from.pos.xz();
	IPoint2 pt_to = p_to.pos.xz();

	while (true) {
		if (poly_id == p_to.poly_id) {
			// If we hit the destination poly via a straight path, the trace is true.
			return true;
		}

		// Something has gone wrong...
		NP_ERR_FAIL_COND_V(poly_id == UINT32_MAX, false);

		const Poly &poly = get_poly(poly_id);

		// Find the best wall crossed.
		freal smallest_dist = FLT_MAX;
		u32 best_wall_id = UINT32_MAX;

		for (u32 w = 0; w < poly.num_inds; w++) {
			u32 wall_id = poly.first_ind + w;
			i64 cross_to = -wall_cross(wall_id, pt_to);

			// Crossed wall.
			if (cross_to > 0) {
				IPoint2 intersect;
				if (wall_find_intersect(wall_id, pt_from, pt_to, intersect)) {
					freal dist = pt_from.distancef_to(intersect);
					if (dist < smallest_dist) {
						smallest_dist = dist;
						best_wall_id = wall_id;
						best_intersect = intersect;
					}

					NP_DEV_ASSERT(dist < FLT_MAX);
				} else {
					// Have some fallback for robustness purposes.
					best_wall_id = wall_id;

					// Assuming we are moving right onto the wall.
					best_intersect = pt_to;

#ifdef NP_DEV_ENABLED
					// Why did the wall_find_intersect fail?
					//wall_find_intersect(wall_id, p_from, p_to, intersect);
#endif
				}
			}
		}

		// Shouldn't happen as we aren't in p_dest_poly_id,
		// but just in case...
		if (best_wall_id == UINT32_MAX) {
			return true;
		}

		if (is_link_hard(best_wall_id)) {
			return false;
		}

		// Transition into the next poly.
		poly_id = get_link(best_wall_id);
	}
	return false;
}

Mesh::TraceResult Mesh::recursive_trace(i32 p_depth, IPoint2 p_from, const IPoint2 &p_to, u32 p_poly_id, TraceInfo &r_info) const {
	//	if (p_depth >= 8) {
	//		NP_LLOG("\t\ttrace recursion depth limit reached");
	//		NP_DEV_ASSERT(poly_contains_point(p_poly_id, p_from));
	//		//return [poly_id, from, vel_dir, vel_mag_global]
	//		return TR_LIMIT;
	//	}

	const Poly &poly = get_poly(p_poly_id);
	u64 best_dist = UINT64_MAX;
	u32 best_wall_id = UINT32_MAX;
	IPoint2 best_intersect;

	for (u32 w = 0; w < poly.num_inds; w++) {
		u32 wall_id = poly.first_ind + w;
		i64 cross_to = -wall_cross(wall_id, p_to);

		// Crossed wall.
		if (cross_to > 0) {
			IPoint2 intersect;
			if (wall_find_intersect(wall_id, p_from, p_to, intersect)) {
				u64 dist = (intersect - p_from).length_squared();
				if (dist < best_dist) {
					best_dist = dist;
					best_wall_id = wall_id;
					best_intersect = intersect;
				}

				NP_DEV_ASSERT(dist < FLT_MAX);
			} else {
				// Have some fallback for robustness purposes.
				best_wall_id = wall_id;

				// Assuming we are moving right onto the wall.
				best_intersect = p_to;

#ifdef NP_DEV_ENABLED
				// Why did the wall_find_intersect fail?
				//wall_find_intersect(wall_id, p_from, p_to, intersect);
#endif
			}
		}
	}

	if (best_wall_id == UINT32_MAX) {
		// Move okay.
		r_info.poly_id = p_poly_id;

		NP_DEV_ASSERT(debug_check_agent_integrity(p_to, p_poly_id, best_wall_id));

#ifdef NP_DEV_ENABLED
//		if (!poly_contains_point(p_poly_id, p_to)) {
//			bool test = poly_contains_point(p_poly_id, p_to);
//		}
//		NP_DEV_ASSERT(poly_contains_point(p_poly_id, p_to));
#endif
		return TR_CLEAR;
	}

	u32 linked_poly_id = get_link(best_wall_id);

	if (is_link_hard(best_wall_id)) {
		// Indicates slide and which wall.
		r_info.poly_id = p_poly_id;
		r_info.slide_wall = best_wall_id;
		r_info.hit_point = best_intersect;

#if 0
		// The intersect is subject to error, so
		// for accuracy we will calculate the distance along the wall.
		const Wall &wall = get_wall(best_wall_id);
		freal wall_length = wall.wall_vec.lengthf();
		const IPoint2 &wall_start = get_vert(wall.vert_a);
		const IPoint2 &wall_end = get_vert(wall.vert_b);
		freal dist_along_wall = best_intersect.distancef_to(wall_start);
		// freal dist_remaining = wall_length - dist_along_wall;

		IPoint2 new_best_intersect = wall.wall_vec;
		new_best_intersect.normalize_to_scale(dist_along_wall);
		new_best_intersect += wall_start;

		// debug run again
		if (!poly_contains_point_debug(p_poly_id, best_intersect)) {
			// Let's output an SVG to debug this.
 			Vector<Mesh::SVGPoint> svg_points;
			Vector<u32> svg_polys;
			Vector<u32> svg_walls;

			svg_polys.push_back(p_poly_id);

			svg_points.push_back(Mesh::SVGPoint(p_from, 0));
			//svg_points.push_back(Mesh::SVGPoint(p_to, 1));
			svg_points.push_back(Mesh::SVGPoint(best_intersect, 2));

			svg_export_custom("../failed_poly.svg", svg_walls, svg_polys, svg_points);

			//recursive_trace(p_depth, p_from, p_to, p_poly_id, r_info);

			best_intersect = new_best_intersect;
		}
#endif

		NP_DEV_ASSERT(debug_check_agent_integrity(best_intersect, p_poly_id, best_wall_id));

		return TR_SLIDE;
	}

	if (p_depth >= 8) {
		NP_LLOG("\t\ttrace recursion depth limit reached");
		r_info.hit_point = best_intersect;

		if (poly_contains_point(p_poly_id, best_intersect)) {
			r_info.poly_id = p_poly_id;
		} else if (poly_contains_point(linked_poly_id, best_intersect)) {
			r_info.poly_id = linked_poly_id;
		} else {
			// Last ditch attempt.
			r_info.poly_id = p_poly_id;
			r_info.hit_point = get_poly(p_poly_id).center.xz();
		}
		//NP_DEV_ASSERT(poly_contains_point(p_poly_id, best_intersect));
		//return [poly_id, from, vel_dir, vel_mag_global]
		return TR_LIMIT;
	}

	// Recurse into neighbouring cell.
	return recursive_trace(p_depth + 1, p_from, p_to, linked_poly_id, r_info);
}

String Mesh::_svg_header(float p_scale) const {
	String sz;
	sz = "\t<style type=\"text/css\" >\n";
	sz += "\t\t<![CDATA[\n";
	sz += "\t\t\ttext {\n";
	sz += "\t\t\t\tfill: blue;\n";
	sz += "\t\t\t\tstroke: gray;\n";

	i32 font_size = 15.0f * p_scale;
	sz += String("\t\t\t\tfont-size: ") + font_size + "em;\n";
	sz += "\t\t\t\tdominant-baseline: middle;\n";
	sz += "\t\t\t\ttext-anchor: middle;\n";
	sz += "\t\t\t}\n";
	sz += "\t\t\tpolyline {\n";
	//	sz += "\t\t\t\tfill: green;\n";
	sz += "\t\t\t\tstroke: pink;\n";
	i32 stroke_width = 25.0f * p_scale;
	sz += String("\t\t\t\tstroke-width:") + stroke_width + ";\n";
	sz += "\t\t\t}\n";
	sz += "\t\t\tcircle {\n";
	//sz += "\t\t\t\tfill: gray;\n";
	sz += "\t\t\t\tstroke: white;\n";
	sz += String("\t\t\t\tstroke-width:") + stroke_width + ";\n";
	sz += "\t\t\t}\n";
	sz += "\t\t]]>\n";
	sz += "\t</style>\n";
	return sz;
}

bool Mesh::svg_export_custom(String p_filename, const Vector<u32> &p_walls, const Vector<u32> &p_polys, const Vector<SVGPoint> p_points) const {
	String sz = _svg_header();

	IRect2 rect;
	bool started = false;

	Vector<u32> vert_ids_done;
	Vector<IPoint2> points_done;

	String sz_labels;

	u32 radius = 20;

	for (u32 pass = 0; pass < 2; pass++) {
		if (pass == 1) {
			radius = rect.size.lengthf() / 60;
		}

		for (u32 w = 0; w < p_walls.size(); w++) {
			const Wall &wall = get_wall(p_walls[w]);

			IPoint2 a = get_vert2(wall.get_swapped_vert_a());
			IPoint2 b = get_vert2(wall.get_swapped_vert_b());

			a /= 65;
			b /= 65;

			if (pass == 1) {
				sz_labels += String("\t<circle cx=\"") + a.x + "\" cy=\"" + a.y + "\" r=\"" + radius + "\" style=\"fill:blue\" />\n";
				sz_labels += String("\t<circle cx=\"") + b.x + "\" cy=\"" + b.y + "\" r=\"" + radius + "\" style=\"fill:blue\" />\n";

				sz += String("\t<polyline points=\"") + a.x + "," + a.y + " " + b.x + "," + b.y + "\" style=\"stroke:blue\" />\n";
			}

			if (started) {
				rect.expand_to_fast(a);
				rect.expand_to_fast(b);
			} else {
				rect.position = a;
				rect.expand_to_fast(b);
				started = true;
			}
		}

		for (u32 p = 0; p < p_polys.size(); p++) {
			const Poly &poly = get_poly(p_polys[p]);

			if (pass == 1) {
				sz += "\t<polyline points=\"";
			}

			for (u32 i = 0; i < poly.num_inds; i++) {
				if (pass == 1) {
					if (i != 0) {
						sz += " ";
					}
				}
				u32 vert_id = get_ind(poly.first_ind + i);
				const IPoint2 &vert = get_vert2(vert_id);

				i32 x = vert.x / 65;
				i32 y = vert.y / 65;

				if (started) {
					rect.expand_to_fast(IPoint2(x, y));
				} else {
					rect.position = IPoint2(x, y);
					started = true;
				}

				if (pass == 1) {
					if (vert_ids_done.find(vert_id) == -1) {
						i32 xx = x;
						i32 yy = y;

						if (points_done.find(IPoint2(x, y)) == -1) {
							points_done.push_back(IPoint2(x, y));
						} else {
							xx += 32;
							yy += 32;
						}

						sz_labels += String("\t<circle cx=\"") + xx + "\" cy=\"" + yy + "\" r=\"" + radius + "\" />\n";
						//sz_labels += String("\t<text x=\"") + xx + "\" y=\"" + (yy + 16) + "\">" + vert_id + "</text>\n";
						vert_ids_done.push_back(vert_id);
					}

					sz += String(x) + "," + y;
				} // if pass
			}

			if (pass == 1) {
				sz += "\" />\n";
			} // pass
		}

		for (u32 p = 1; p < p_points.size(); p++) {
			IPoint2 a = p_points[p - 1].pos;
			IPoint2 b = p_points[p].pos;
			a /= 65;
			b /= 65;

			if (pass == 1) {
				sz_labels += String("\t<circle cx=\"") + a.x + "\" cy=\"" + a.y + "\" r=\"" + radius + "\" style=\"fill:blue\" />\n";

				// Last point?
				if (p == p_points.size() - 1) {
					sz_labels += String("\t<circle cx=\"") + b.x + "\" cy=\"" + b.y + "\" r=\"" + radius + "\" style=\"fill:blue\" />\n";
				}
				sz += String("\t<polyline points=\"") + a.x + "," + a.y + " " + b.x + "," + b.y + "\" style=\"stroke:blue\" />\n";
			}

			if (started) {
				rect.expand_to_fast(a);
				rect.expand_to_fast(b);
			} else {
				rect.position = a;
				rect.expand_to_fast(b);
				started = true;
			}
		}

	} // pass

	sz += sz_labels;

	sz += "</svg>";

	rect.increment_size();

	// Expand a bit.
	rect.position -= IPoint2(100, 100);
	rect.size += IPoint2(200, 200);

	float aspect = rect.size.x / (float)rect.size.y;

	i32 svg_width = 1024;
	i32 svg_height = 1024;

	if (aspect >= 1) {
		svg_height /= aspect;
	} else {
		svg_width *= aspect;
	}

	String rect_sz = String(rect.position.x) + " " + rect.position.y + " " + rect.size.x + " " + rect.size.y;
	sz = String("<svg width=\"") + svg_width + "\" height=\"" + svg_height + "\" viewBox=\"" + rect_sz + "\">\n" + sz;
	//log(sz);

	return sz.write_as_text_file(p_filename);
}

bool Mesh::svg_export(String p_filename, u32 p_start_poly) const {
	float scale = 0.5;
	i32 circle_radius = 100 * scale;
	i32 circle_offset = 160 * scale;

	String sz = _svg_header(scale);

	IRect2 rect;
	bool started = false;

	Vector<u32> vert_ids_done;
	Vector<IPoint2> points_done;

	String sz_labels;

	String sz_poly_labels;

	for (u32 p = p_start_poly; p < get_num_polys(); p++) {
		const Poly &poly = get_poly(p);
		const PolyExtra &ex = get_poly_extra(p);

		sz += "\t<polyline points=\"";
		for (u32 i = 0; i < poly.num_inds; i++) {
			if (i != 0) {
				sz += " ";
			}
			u32 vert_id = get_ind(poly.first_ind + i);
			const IPoint2 &vert = get_vert2(vert_id);

			i32 x, y;
			svg_scale_point(vert, x, y);

			if (vert_ids_done.find(vert_id) == -1) {
				i32 xx = x;
				i32 yy = y;

				if (points_done.find(IPoint2(x, y)) == -1) {
					points_done.push_back(IPoint2(x, y));
				} else {
					xx += 32;
					yy += 32;
				}

				if (false) {
					sz_labels += String("\t<circle cx=\"") + xx + "\" cy=\"" + yy + "\" r=\"" + circle_radius + "\" fill=\"yellow\"/>\n";
					sz_labels += String("\t<text x=\"") + xx + "\" y=\"" + (yy + circle_offset) + "\">" + vert_id + "</text>\n";
				}

				vert_ids_done.push_back(vert_id);
			}

			if (started) {
				rect.expand_to_fast(IPoint2(x, y));
			} else {
				rect.position = IPoint2(x, y);
				started = true;
			}

			sz += String(x) + "," + y;
		}
		sz += "\" ";
		sz += ex.is_narrowing() ? "fill=\"brown\"" : "fill=\"green\"";
		sz += " />\n";

#if 0
		{
			i32 x, y;
			svg_scale_point(poly.center, x, y);

			//if ((ex.narrowing_id != UINT32_MAX) || (ex.narrowing_width != UINT32_MAX)) {
			String sz_poly_id = String(" (") + p + ")";
			u32 zone_circle_radius = 250 * scale; // 150
			u32 zone_circle_offset = 80 * scale;
			if (ex.is_narrowing()) {
				sz_poly_labels += String("\t<circle cx=\"") + x + "\" cy=\"" + y + "\" r=\"" + zone_circle_radius + "\" fill=\"red\" />\n";
				sz_poly_labels += String("\t<text x=\"") + x + "\" y=\"" + (y + zone_circle_offset) + "\">" + ex.zone_id + sz_poly_id + "</text>\n";
			} else {
				sz_poly_labels += String("\t<circle cx=\"") + x + "\" cy=\"" + y + "\" r=\"" + zone_circle_radius + "\" fill=\"gray\"/>\n";
				sz_poly_labels += String("\t<text x=\"") + x + "\" y=\"" + (y + zone_circle_offset) + "\">" + ex.zone_id + sz_poly_id + "</text>\n";
			}
		}
#endif

		if ((p % 8) == 0) {
			sz += sz_poly_labels;
			sz_poly_labels.clear();
		}
	}
	sz += sz_poly_labels;
	sz += sz_labels;

	// Zone crossings.
	String sz_crossings;
	for (u32 n = 0; n < get_num_zones(); n++) {
		const Zone &zone = get_zone(n);

		for (u32 l = 0; l < zone.num_links; l++) {
			const ZoneLink &zl = get_zone_link(zone.first_link + l);

			// Only show one way.
			if (zl.zone_to_id < n)
				continue;

			i32 x, y;
			svg_scale_point(zl.pt_crossing, x, y);

			i32 crossing_radius = 100 * scale;
			sz_crossings += String("\t<circle cx=\"") + x + "\" cy=\"" + y + "\" r=\"" + crossing_radius + "\" fill=\"purple\" />\n";
		}
	}

	sz += sz_crossings;

	sz += "</svg>";

	rect.increment_size();

	// Expand a bit.
	rect.position -= IPoint2(1000, 1000);
	rect.size += IPoint2(2000, 2000);

	float aspect = rect.size.x / (float)rect.size.y;

	i32 svg_width = 1024;
	i32 svg_height = 1024;

	if (aspect >= 1) {
		svg_height /= aspect;
	} else {
		svg_width *= aspect;
	}

	String rect_sz = String(rect.position.x) + " " + rect.position.y + " " + rect.size.x + " " + rect.size.y;
	sz = String("<svg width=\"") + svg_width + "\" height=\"" + svg_height + "\" viewBox=\"" + rect_sz + "\">\n" + sz;
	//log(sz);

	return sz.write_as_text_file(p_filename);
}

void Mesh::_check_for_duplicate_verts() {
	return;
#ifdef NP_DEV_ENABLED
	NP_WARN_PRINT_ONCE("WARNING : checking for duplicate verts is active, this may drop performance");
	for (u32 i = 0; i < get_num_verts(); i++) {
		const IPoint3 &a = get_vert(i);

		for (u32 j = i + 1; j < get_num_verts(); j++) {
			const IPoint3 &b = get_vert(j);
			if (a == b) {
				log(String("Duplicate vert detected ") + i + " is the same as " + j);
				NP_DEV_ASSERT(a != b);
			}
		}
	}
#endif
}

bool Mesh::poly_contains_vert(u32 p_poly_id, u32 p_vert_id) const {
	const Poly &poly = get_poly(p_poly_id);
	for (u32 n = 0; n < poly.num_inds; n++) {
		u32 vert_id = get_ind(poly.first_ind + n);
		if (p_vert_id == vert_id) {
			return true;
		}
	}
	return false;
}

bool Mesh::poly_contains_point_debug(u32 p_poly_id, const IPoint2 &p_pt) const {
	const Poly &poly = get_poly(p_poly_id);
	//log(String("debug_poly_contains_point for poly ") + p_poly_id);

	for (u32 n = 0; n < poly.num_inds; n++) {
		u32 wall_id = poly.first_ind + n;

		// IPoint2 wa, wb;
		// get_wall_verts(wall_id, wa, wb);

		i64 cross = wall_cross(wall_id, p_pt);
		//log(String("\twall ") + wall_id + ", cross " + cross);
		if (cross < 0) {
			log(String("Mesh::poly_contains_point_debug failed cross was ") + cross);
			return false;
		}
	}

	return true;
}

bool Mesh::poly_contains_point(u32 p_poly_id, const IPoint2 &p_pt, bool p_ceiling) const {
	const Poly &poly = get_poly(p_poly_id, p_ceiling);

	// Different routine for ceiling, as no walls.
	if (p_ceiling) {
		u32 ind_a = get_ind(poly.first_ind, true);
		const IPoint3 *a = &get_vert(ind_a, true);

		for (u32 w = 0; w < poly.num_inds; w++) {
			u32 ind_b = get_ind(poly.first_ind + ((w + 1) % poly.num_inds), true);
			const IPoint3 *b = &get_vert(ind_b, true);

			IPoint2 wall_vec = b->xz() - a->xz();
			IPoint2 point_vec = p_pt - a->xz();
			i64 cross = wall_vec.cross(point_vec);

			// Note this condition <= has to match up with the condition for TR_CLEAR,
			// otherwise it will move to locations on the exact border if we use < here.
			// if (cross <= 0) {
			if (cross < 0) {
				return false;
			}

			ind_a = ind_b;
			a = b;
		}

		return true;
	}

//#define NP_DEBUG_POLY_CONTAINS_POINT
#ifdef NP_DEBUG_POLY_CONTAINS_POINT
	IRect2 rect;

	for (u32 n = 0; n < poly.num_inds; n++) {
		u32 wall_id = poly.first_ind + n;

		log(String("\twall id ") + wall_id);

		const Wall &wall = get_wall(wall_id);

		const IPoint2 &va = get_vert(wall.vert_a);
		const IPoint2 &vb = get_vert(wall.vert_b);
		if (n == 0) {
			rect.position = va;
		} else {
			rect.expand_to(va);
		}
		rect.expand_to(vb);
	}

	log(String("poly ") + p_poly_id + " rect " + rect);

	if (rect.contains_point(p_pt)) {
		log(String("poly ") + p_poly_id + " rect contains point " + p_pt);
	}
#endif

	for (u32 n = 0; n < poly.num_inds; n++) {
		u32 wall_id = poly.first_ind + n;

		//		i64 cross = wall_cross(wall_id, p_pt);
		//		if (cross < 0) {
		//			return false;
		//		}

		if (wall_in_front_cross(wall_id, p_pt)) {
			return false;
		}
	}

	return true;
}

freal Mesh::get_distance_from_wall_edge(u32 p_wall_id, const IPoint2 &p_pt) const {
	const Wall &wall = get_wall(p_wall_id);
	u64 line_sl = wall.wall_vec.length_squared();

	NP_ERR_FAIL_COND_V(!line_sl, 0);

	i64 cross = wall_cross(p_wall_id, p_pt);
	freal line_length = Math::sqrt_real(line_sl);

	// Distance is the absolute value of cross product divided by the length of the line vector
	freal dist = cross / line_length;
	//return wall.verts_swapped ? -dist : dist;
	return dist;
}

bool Mesh::wall_in_front_cross(u32 p_wall_id, const IPoint2 &p_pt) const {
	i64 cross = wall_cross(p_wall_id, p_pt);
	return cross < 0;
}

i64 Mesh::wall_cross(u32 p_wall_id, const IPoint2 &p_pt) const {
	IPoint2 wa, wb;
	get_wall_verts(p_wall_id, wa, wb);

	IPoint2 wall_vec = wb - wa;
	IPoint2 point_vec = p_pt - wa;
	return wall_vec.cross(point_vec);
}

bool Mesh::find_ceiling_height(u32 p_floor_poly_id, const IPoint2 &p_pt, i32 &r_height, u32 &r_ceiling_poly_id_hint) const {
	u32 found_ceiling_poly_id = UINT32_MAX;

	// Optimization, 99% of time we will be using the same ceiling poly for height,
	// so we don't need to check the others.
	if (r_ceiling_poly_id_hint != UINT32_MAX) {
		if (poly_contains_point(r_ceiling_poly_id_hint, p_pt, true)) {
			found_ceiling_poly_id = r_ceiling_poly_id_hint;
		}
	}

	if (found_ceiling_poly_id == UINT32_MAX) {
		const PolyExtra &ex = get_poly_extra(p_floor_poly_id);

		// Polys should be pre-ordered in order of height.
		// If we hit one, we shouldn't need to check further up.
		// (because of the nature of navmeshes, we shouldn't get overlap
		// on the same floor of a building)

		// TODO: There are some rare circumstances at the moment that are not dealt with:
		// If the room ABOVE our room has a poly that reaches below our ceiling, it may register
		// first. We can probably solve this by proper (advanced) height pre-sorting in the Loader.
		// e.g. Use the lowest point of the ceiling poly AT THE CORNERS of the floor poly.

		for (u32 n = 0; n < ex.num_ceiling_links; n++) {
			u32 ceil_poly_id = _ceiling_links[ex.first_ceiling_link + n];

			if (!poly_contains_point(ceil_poly_id, p_pt, true)) {
				continue;
			}

			found_ceiling_poly_id = ceil_poly_id;
			break;
		}
	}

	if (found_ceiling_poly_id != UINT32_MAX) {
		r_height = find_height_on_poly_plane(found_ceiling_poly_id, p_pt, true);

		// Store this on the agent, for the hint next time.
		r_ceiling_poly_id_hint = found_ceiling_poly_id;
		return true;
	}

	// We are not presently on a ceiling poly, so forget the last.
	r_ceiling_poly_id_hint = UINT32_MAX;
	return false;
}

i32 Mesh::find_height_on_poly_plane(u32 p_poly_id, const IPoint2 &p_pt, bool p_ceiling) const {
	freal height = find_float_height_on_poly_plane(p_poly_id, p_pt, p_ceiling);

	// Convert local float height to fixed point.
	height = (height + _f32_to_fp_offset.y) * _f32_to_fp_scale;
	return Math::round32(height);
}

freal Mesh::find_float_height_on_poly_plane(u32 p_poly_id, const IPoint2 &p_pt, bool p_ceiling) const {
	FPoint2 pt = fixed_point2_to_float2(p_pt);

	NP_DEV_ASSERT(p_poly_id != UINT32_MAX);

	const Poly &poly = p_ceiling ? ceiling.polys[p_poly_id] : get_poly(p_poly_id);
	NP_DEV_ASSERT(poly.plane.normal.isfinite());

	FPoint3 intersection;
	bool hit = poly.plane.intersects_ray(FPoint3::make(pt.x, 1000, pt.y), FPoint3::make(0, -1, 0), &intersection);
	if (hit) {
		NP_DEV_ASSERT(std::isfinite(intersection.y));
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

bool Mesh::wall_segments_find_intersect(u32 p_wall_id, const IPoint2 &p_from, const IPoint2 &p_to, IPoint2 &r_hit) const {
	IPoint2 wa, wb;
	get_wall_verts(p_wall_id, wa, wb);

	// Some unit testing.
	//NP_DEV_ASSERT(find_line_segments_intersect_integer(IPoint2(0, 5), IPoint2 (10, 5), IPoint2(5, 10), IPoint2(5, 0), r_hit));
	//NP_DEV_ASSERT(!find_line_segments_intersect_integer(IPoint2(0, 5), IPoint2 (4, 5), IPoint2(5, 10), IPoint2(5, 0), r_hit));

	return find_line_segments_intersect_integer(wa, wb, p_from, p_to, r_hit);
}

void Mesh::_unit_test_find_lines_intersect_integer() {
	// A priori calculations suggest 20 bit, but it's quite tricky as we have both
	// * expansion due to planks.
	// * velocity towards positions out of bounds.

	// Note: I previously thought this was over 20 bit, but analysing the function reveals
	// the exact threshold below.
	// const i32 BM = ((1 << 20) + (1 << 18));

	// By calculation, it turns out the exact cut off for overflow is (1 << 20) - 1.
	// We must adjust the mesh range (including lip) to be within this.
	const i32 BM = (1 << 20) - 1;

	IPoint2 p1(-BM, BM);
	IPoint2 p2(BM, BM);
	IPoint2 p3(BM, BM);
	IPoint2 p4(-BM, -BM);
	IPoint2 hit;
	find_lines_intersect_integer(p1, p2, p3, p4, hit);
}

bool Mesh::find_lines_intersect_integer(const IPoint2 &p_from_a, const IPoint2 &p_to_a, const IPoint2 &p_from_b, const IPoint2 &p_to_b, IPoint2 &r_hit) const {
	// Check incoming positions.
#ifdef NP_DEV_ENABLED
	if (!p_from_a.is_within_fp_range() || !p_to_a.is_within_fp_range() || !p_from_b.is_within_fp_range() || !p_to_b.is_within_fp_range()) {
		NP_DEV_ASSERT(0 && "find_lines_intersect_integer() : Input is out of range.");
		return false;
	}
#endif

	i32 x1 = p_from_a.x;
	i32 y1 = p_from_a.y;

	// Shift origin to p_from_a = (x1, y1).
	// Storing deltas as i64 avoids i32 subtraction overflow and extra sign-extensions.
	i64 dx1 = (i64)p_to_a.x - x1;
	i64 dy1 = (i64)p_to_a.y - y1;
	i64 dx2 = (i64)p_to_b.x - p_from_b.x;
	i64 dy2 = (i64)p_to_b.y - p_from_b.y;

	i64 tmp1, tmp2;
	NP_CHECKED_OP_64(tmp1, dx1, *, dy2);
	NP_CHECKED_OP_64(tmp2, dy1, *, dx2);
	i64 denom;
	NP_CHECKED_OP_64(denom, tmp1, -, tmp2);

	// Lines are parallel, collinear, or degenerate (zero-length)
	if (denom == 0) {
		return false;
	}

	i64 dx31 = (i64)p_from_b.x - x1;
	i64 dy31 = (i64)p_from_b.y - y1;

	NP_CHECKED_OP_64(tmp1, dx31, *, dy2);
	NP_CHECKED_OP_64(tmp2, dy31, *, dx2);
	i64 t_num;
	NP_CHECKED_OP_64(t_num, tmp1, -, tmp2);

	// Normalize denom to be positive to simplify rounding.
	// Note: if deltas can exceed i32 range (e.g. from -2^31 to +2^31-1),
	// denom could theoretically hit INT64_MIN only if it already overflowed i64 above.
	if (denom < 0) {
		denom = -denom;
		t_num = -t_num;
	}

	i64 offset = denom / 2;

	i64 x;
	{
		i64 num;
		NP_CHECKED_OP_64(num, t_num, *, dx1);
		if (num < 0) {
			NP_CHECKED_OP_64(x, num, -, offset);
			NP_CHECKED_OP_64(x, x, /, denom);
		} else {
			NP_CHECKED_OP_64(x, num, +, offset);
			NP_CHECKED_OP_64(x, x, /, denom);
		}
		NP_CHECKED_OP_64(x, x, +, x1);
	}

	i64 y;
	{
		i64 num;
		NP_CHECKED_OP_64(num, t_num, *, dy1);
		if (num < 0) {
			NP_CHECKED_OP_64(y, num, -, offset);
			NP_CHECKED_OP_64(y, y, /, denom);
		} else {
			NP_CHECKED_OP_64(y, num, +, offset);
			NP_CHECKED_OP_64(y, y, /, denom);
		}
		NP_CHECKED_OP_64(y, y, +, y1);
	}

	// At this point, near parallel lines can still meet outside the int32_t range.
	// We detect this, clamp to the NavMesh max size, and return false,
	// as the result is not reliable (and usually makes no sense to process in the calling code).
	bool inside = true;

	if (x > NAVPHYSICS_MESH_FP_RANGE_WITH_LIP) {
		x = NAVPHYSICS_MESH_FP_RANGE_WITH_LIP;
		inside = false;
	} else if (x < -NAVPHYSICS_MESH_FP_RANGE_WITH_LIP) {
		x = -NAVPHYSICS_MESH_FP_RANGE_WITH_LIP;
		inside = false;
	}
	if (y > NAVPHYSICS_MESH_FP_RANGE_WITH_LIP) {
		y = NAVPHYSICS_MESH_FP_RANGE_WITH_LIP;
		inside = false;
	} else if (y < -NAVPHYSICS_MESH_FP_RANGE_WITH_LIP) {
		y = -NAVPHYSICS_MESH_FP_RANGE_WITH_LIP;
		inside = false;
	}

	NP_CHECK_32(x, +, 0);
	NP_CHECK_32(y, +, 0);

	r_hit.set(x, y);
	return inside;
}

/*
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
	// 1 BIT GROWTH
	NP_CHECK_32(y2, -, y1);
	i32 a1 = y2 - y1;
	NP_CHECK_32(x1, -, x2);
	i32 b1 = x1 - x2;

	// These calcs need to be 64 bit to prevent overflow crossing 65535.
	// 1 EXP GROWTH
	NP_CHECK_64(x2, *, y1);
	NP_CHECK_64(x1, *, y2);
#ifdef NP_OVERFLOW_CHECKS
	{
		i64 temp_a = (i64)x2 * y1;
		i64 temp_b = (i64)x1 * y2;
		NP_CHECK_64(temp_a, -, temp_b);
	}
#endif

	i64 c1 = (i64)x2 * y1 - (i64)x1 * y2;

	// Second line coefficients
	// 1 BIT GROWTH
	NP_CHECK_32(y4, -, y3);
	i32 a2 = y4 - y3;
	NP_CHECK_32(x3, -, x4);
	i32 b2 = x3 - x4;

	// 1 EXP GROWTH
	NP_CHECK_64(x4, *, y3);
	NP_CHECK_64(x3, *, y4);
#ifdef NP_OVERFLOW_CHECKS
	{
		i64 temp_a = (i64)x4 * y3;
		i64 temp_b = (i64)x3 * y4;
		NP_CHECK_64(temp_a, -, temp_b);
	}
#endif
	i64 c2 = (i64)x4 * y3 - (i64)x3 * y4;

	// (1 BIT)	 EXP
	NP_CHECK_64(a1, *, b2);
	NP_CHECK_64(a2, *, b1);
#ifdef NP_OVERFLOW_CHECKS
	{
		i64 temp_a = (i64)a1 * b2;
		i64 temp_b = (i64)a2 * b1;
		NP_CHECK_64(temp_a, -, temp_b);
	}
#endif
	i64 denom = (i64)a1 * b2 - (i64)a2 * b1;

	// Lines are colinear
	if (denom == 0) {
		return false;
	}

	// If we got here, lines intersect. Compute intersection point using method similar
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
		// 1 BIT * 1 EXP GROWTH
		NP_CHECK_64(b1, *, c2);
		NP_CHECK_64(b2, *, c1);
#ifdef NP_OVERFLOW_CHECKS
		{
			i64 temp_a = (i64)b1 * c2;
			i64 temp_b = (i64)b2 * c1;
			NP_CHECK_64(temp_a, -, temp_b);
		}
#endif
		i64 num = (i64)b1 * c2 - (i64)b2 * c1;
		if (num < 0) {
			x = num - offset;
		} else {
			x = num + offset;
		}
		x /= denom;
	}

	{
		// 1 BIT * 1 EXP GROWTH
		NP_CHECK_64(a2, *, c1);
		NP_CHECK_64(a1, *, c2);
#ifdef NP_OVERFLOW_CHECKS
		{
			i64 temp_a = (i64)a2 * c1;
			i64 temp_b = (i64)a1 * c2;
			NP_CHECK_64(temp_a, -, temp_b);
		}
#endif
		i64 num = (i64)a2 * c1 - (i64)a1 * c2;
		if (num < 0) {
			y = num - offset;
		} else {
			y = num + offset;
		}
		y /= denom;
	}

	FIX THIS TOMMORROW
	THIS BY DEFINITION CAN OVERFLOW WITH NEAR PARALLEL LINES
	NP_CHECK_32(x, +, 0);
	NP_CHECK_32(y, +, 0);

	r_hit.set(x, y);
	return true;
}
*/

bool Mesh::find_line_segments_intersect_integer(const IPoint2 &p_from_a, const IPoint2 &p_to_a, const IPoint2 &p_from_b, const IPoint2 &p_to_b, IPoint2 &r_hit) const {
	// Check incoming positions.
#ifdef NP_DEV_ENABLED
	if (!p_from_a.is_within_fp_range() || !p_to_a.is_within_fp_range() || !p_from_b.is_within_fp_range() || !p_to_b.is_within_fp_range()) {
		NP_DEV_ASSERT(0 && "find_line_segments_intersect_integer() : Input is out of range.");
		return false;
	}
#endif

	i32 x1 = p_from_a.x;
	i32 y1 = p_from_a.y;

	// Shift origin to p_from_a = (x1, y1).
	// Storing deltas as i64 avoids i32 subtraction overflow and extra sign-extensions.
	i64 dx1 = (i64)p_to_a.x - x1;
	i64 dy1 = (i64)p_to_a.y - y1;
	i64 dx2 = (i64)p_to_b.x - p_from_b.x;
	i64 dy2 = (i64)p_to_b.y - p_from_b.y;

	i64 tmp1, tmp2;
	NP_CHECKED_OP_64(tmp1, dx1, *, dy2);
	NP_CHECKED_OP_64(tmp2, dy1, *, dx2);
	i64 denom;
	NP_CHECKED_OP_64(denom, tmp1, -, tmp2);

	// Lines are parallel, collinear, or degenerate (zero-length)
	if (denom == 0) {
		return false;
	}

	i64 dx31 = (i64)p_from_b.x - x1;
	i64 dy31 = (i64)p_from_b.y - y1;

	NP_CHECKED_OP_64(tmp1, dx31, *, dy2);
	NP_CHECKED_OP_64(tmp2, dy31, *, dx2);
	i64 t_num;
	NP_CHECKED_OP_64(t_num, tmp1, -, tmp2);

	bool neg_denom = (denom < 0);
	if (neg_denom) {
		denom = -denom;
		t_num = -t_num;
	}

	// Check if intersection lies on segment A (0 <= t <= 1)
	if (t_num < 0 || t_num > denom) {
		return false;
	}

	NP_CHECKED_OP_64(tmp1, dx31, *, dy1);
	NP_CHECKED_OP_64(tmp2, dy31, *, dx1);
	i64 u_num;
	NP_CHECKED_OP_64(u_num, tmp1, -, tmp2);
	if (neg_denom) {
		u_num = -u_num;
	}

	// Check if intersection lies on segment B (0 <= u <= 1)
	if (u_num < 0 || u_num > denom) {
		return false;
	}

	// If we got here, line segments intersect.
	// The denom/2 is to get rounding instead of truncating.
	i64 offset = denom / 2;

	i64 x;
	{
		i64 num;
		NP_CHECKED_OP_64(num, t_num, *, dx1);
		if (num < 0) {
			NP_CHECKED_OP_64(x, num, -, offset);
			NP_CHECKED_OP_64(x, x, /, denom);
		} else {
			NP_CHECKED_OP_64(x, num, +, offset);
			NP_CHECKED_OP_64(x, x, /, denom);
		}
		NP_CHECKED_OP_64(x, x, +, x1);
	}

	i64 y;
	{
		i64 num;
		NP_CHECKED_OP_64(num, t_num, *, dy1);
		if (num < 0) {
			NP_CHECKED_OP_64(y, num, -, offset);
			NP_CHECKED_OP_64(y, y, /, denom);
		} else {
			NP_CHECKED_OP_64(y, num, +, offset);
			NP_CHECKED_OP_64(y, y, /, denom);
		}
		NP_CHECKED_OP_64(y, y, +, y1);
	}

	NP_CHECK_32(x, +, 0);
	NP_CHECK_32(y, +, 0);

	r_hit.set((i32)x, (i32)y);
	return true;
}

/*
bool Mesh::find_line_segments_intersect_integer(const IPoint2 &p_from_a, const IPoint2 &p_to_a, const IPoint2 &p_from_b, const IPoint2 &p_to_b, IPoint2 &r_hit) const {
	// For numerical accuracy, do everything relative

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
	i64 r3 = (i64)a1 * x3 + (i64)b1 * y3 + (i64)c1;
	i64 r4 = (i64)a1 * x4 + (i64)b1 * y4 + (i64)c1;

	//  Sign values for second line
	i64 r1 = (i64)a2 * x1 + (i64)b2 * y1 + (i64)c2;
	i64 r2 = (i64)a2 * x2 + (i64)b2 * y2 + (i64)c2;

	// Flag denoting whether intersection point is on passed line segments. If this is false,
	// the intersection occurs somewhere along the two mathematical, infinite lines instead.
	// Check signs of r3 and r4.  If both point 3 and point 4 lie on same side of line 1, the
	// line segments do not intersect.
	// Check signs of r1 and r2.  If both point 1 and point 2 lie on same side of second line
	// segment, the line segments do not intersect.
	bool is_not_on_segments = (r3 != 0 && r4 != 0 && same_signs64(r3, r4)) || (r1 != 0 && r2 != 0 && same_signs64(r1, r2));

	if (is_not_on_segments) {
		return false;
	}

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
*/

} // namespace NavPhysics
