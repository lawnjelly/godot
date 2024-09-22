#include "navphysics_mesh.h"
#include "navphysics_map.h"

namespace NavPhysics {

const Mesh &MeshInstance::get_mesh() const {
	NP_DEV_ASSERT(_mesh_id != UINT32_MAX);
	return g_world.get_mesh(_mesh_id);
}

void MeshInstance::set_transform(const Transform &p_xform, const Transform &p_xform_inv, bool p_is_identity) {
	_transform = p_xform;
	_transform_inverse = p_xform_inv;
	_transform_identity = p_is_identity;
}

void Mesh::set_transform(const Transform &p_xform, const Transform &p_xform_inv, bool p_is_identity) {
	_transform = p_xform;
	_transform_inverse = p_xform_inv;
	_transform_identity = p_is_identity;
}

void Mesh::debug_poly(u32 p_poly_id) const {
}

void Mesh::_log(const String &p_string, int p_depth) const {
	//print_line(p_string);
}

freal MeshInstance::find_agent_fit(Agent &r_agent) const {
	// multiple meshs NYI
	return 1.0;
}

void MeshInstance::teleport_agent(Agent &r_agent) {
	const Mesh &mesh = get_mesh();

	r_agent.pos = mesh.float_to_fixed_point_2(r_agent.fpos);
	//r_agent.pos.set(7906, 41409);

	u32 new_poly_id = mesh.find_poly_within(r_agent.pos);
	_agent_enter_poly(r_agent, new_poly_id, true);

	if (r_agent.poly_id == UINT32_MAX) {
		NP_WARN_PRINT("not within poly");

		// just guess a poly
		if (mesh.get_num_polys()) {
			const Poly &poly = mesh.get_poly(0);
			r_agent.pos = poly.center;
			_agent_enter_poly(r_agent, 0, true);
		}

		return;
	} else {
		print_line(String("c++ agent within poly ") + itos(r_agent.poly_id));
	}
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

void MeshInstance::agent_get_info(const Agent &p_agent, BodyInfo &r_body_info) const {
	r_body_info.poly_id = p_agent.poly_id;
	r_body_info.blocking_narrowing_id = p_agent.blocking_narrowing_id;

	const Mesh &mesh = get_mesh();

	if (p_agent.poly_id != UINT32_MAX) {
		r_body_info.narrowing_id = mesh.get_poly(p_agent.poly_id).narrowing_id;

		if (r_body_info.narrowing_id != UINT32_MAX) {
			const NavPhysics::Narrowing &nar = mesh.get_narrowing(r_body_info.narrowing_id);
			r_body_info.narrowing_available = nar.available;

			const NarrowingInstance &nari = get_narrowing_instance(r_body_info.narrowing_id);
			r_body_info.narrowing_used = nari.used;
		}
	}

	if (r_body_info.blocking_narrowing_id != UINT32_MAX) {
		const NavPhysics::Narrowing &nar = mesh.get_narrowing(r_body_info.blocking_narrowing_id);
		r_body_info.blocking_narrowing_available = nar.available;

		const NarrowingInstance &nari = get_narrowing_instance(r_body_info.blocking_narrowing_id);
		r_body_info.blocking_narrowing_used = nari.used;
	}
}

void MeshInstance::iterate_agent(Agent &r_agent) {
	//print_line("c++ agent at pos " + String(Variant(r_agent.fpos)));

	// has the f32 position moved significantly? if not, retain
	// the fixed point position as this is the gold standard
	//IPoint2 new_pos_fp = float_to_fixed_point_2(r_agent.fpos);
	//IPoint2 offset = new_pos_fp - r_agent.pos;
	//if ((Math::abs(offset.x) > 1) || (Math::abs(offset.y) > 1)) {
	//r_agent.pos = new_pos_fp;
	//}

	const Mesh &mesh = get_mesh();

	IPoint2 vel_add = mesh.float_to_fixed_point_vel(r_agent.fvel);
	r_agent.vel += vel_add;

	_iterate_agent(r_agent);

	// apply friction
	r_agent.vel *= r_agent.friction;

	if (r_agent.poly_id != UINT32_MAX) {
		r_agent.height = mesh.find_height_on_poly(r_agent.poly_id, r_agent.pos);
	}

	// only update the f32ing point position if significantly different
	FPoint2 new_fpos = mesh.fixed_point_to_float_2(r_agent.pos);
	if (!new_fpos.is_equal_approx(r_agent.fpos)) {
		r_agent.fpos = new_fpos;
	}

	r_agent.fvel = mesh.fixed_point_vel_to_float(r_agent.vel);
}

bool MeshInstance::_agent_enter_poly(Agent &r_agent, u32 p_new_poly_id, bool p_force_allow) {
	if (r_agent.ignore_narrowings) {
		r_agent.poly_id = p_new_poly_id;
		return true;
	}

	u32 old_poly_id = r_agent.poly_id;

	if (old_poly_id == p_new_poly_id) {
		return true;
	}

	/*
		const Mesh &mesh = get_mesh();

		// get old narrowing
		Poly *old_poly = nullptr;
		//Narrowing *old_narrowing = nullptr;
		u32 old_available = 0;

		if (old_poly_id != UINT32_MAX) {
			old_poly = &mesh.get_poly(old_poly_id);

			if (old_poly->narrowing_id != UINT32_MAX) {
				//old_narrowing = &get_narrowing_instance_narrowings[old_poly->narrowing_id];
				old_available = mesh.get_narrowing(old_poly->narrowing_id).available;
			}
		}

		// get new narrowing
		Poly *new_poly = nullptr;
		Narrowing *new_narrowing = nullptr;
		NarrowingInstance *new_narrowing_instance = nullptr;

		if (p_new_poly_id != UINT32_MAX) {
			new_poly = &get_poly(p_new_poly_id);

			if (new_poly->narrowing_id != UINT32_MAX) {
				new_narrowing = &mesh.get_narrowing(new_poly->narrowing_id);
				new_narrowing_instance = &get_narrowing_instance(new_poly->narrowing_id);
			}
		}
		////////////////////////////////////////////////////////////

		// try to enter the new poly BEFORE leaving the old
		if (new_poly) {
			// special case, both polys are in the same narrowing
			if (old_poly && (old_poly->narrowing_id == new_poly->narrowing_id)) {
				r_agent.poly_id = p_new_poly_id;
				return true;
			}

			if (new_narrowing) {
				u32 available = new_narrowing->available;

				// Special modification... when moving from a larger narrowing to a tighter,
				// allow 1 less agent to enter. This makes it more likely agents can exit from tight
				// narrowings.
				if (old_available > available) {
					available--;
					// always allow at least one
					if (!available) {
						available = 1;
					}
				}

				if ((new_narrowing->used >= available) && !p_force_allow) {
					return false;
				}

				new_narrowing->used += 1;
	#ifdef NP_DEV_ENABLED

				i64 found = new_narrowing->used_agent_ids.find(r_agent.agent_id);
				if (found != -1) {
					NP_WARN_PRINT("New narrowing already contains agent id.");
				}
				new_narrowing->used_agent_ids.push_back(r_agent.agent_id);
	#endif
			}
		}
	*/
	// save the new poly id
	r_agent.poly_id = p_new_poly_id;

	/*
	if (old_narrowing) {
		if (old_narrowing->used) {
			old_narrowing->used -= 1;
		} else {
			NP_WARN_PRINT("Old narrowing has no agents.");
		}

#ifdef NP_DEV_ENABLED
		i64 found = old_narrowing->used_agent_ids.find(r_agent.agent_id);
		if (found != -1) {
			old_narrowing->used_agent_ids.remove_unordered(found);
		} else {
			NP_WARN_PRINT("Old narrowing does not contain agent id.");
		}
#endif
	}
*/
	return true;
}

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

void MeshInstance::link_mesh(u32 p_mesh_id) {
	_narrowing_instances.clear();
	_mesh_id = p_mesh_id;

	if (_mesh_id != UINT32_MAX) {
		const Mesh &mesh = get_mesh();
		_narrowing_instances.resize(mesh.get_num_narrowings());
	}
}

void MeshInstance::_iterate_agent(Agent &r_agent) {
	Agent &ag = r_agent;

	const Mesh &mesh = get_mesh();

	if (ag.poly_id == UINT32_MAX) {
		u32 new_poly_id = mesh.find_poly_within(ag.pos);
		//ag.poly_id = find_poly_within(ag.pos);
		if (new_poly_id == UINT32_MAX) {
			NP_WARN_PRINT("not within poly");
			return;
		} else {
			print_line(String("c++ agent within poly ") + itos(ag.poly_id));
			_agent_enter_poly(r_agent, new_poly_id, true);
		}
	}

	freal mag = ag.vel.lengthf();
	if (mag < 0.0001f) {
		//_log("no significant velocity");
		return;
	}

	IPoint2 old_pos = ag.pos;

	IPoint2 dir = ag.vel;
	//var dir = agent._vel
	dir.normalize();
	log(String("iterate pos ") + str(ag.pos) + ", vel " + str(ag.vel) + ", poly " + itos(ag.poly_id));

	if (ag.wall_id == UINT32_MAX) {
		NP_DEV_ASSERT(poly_contains_point(ag.poly_id, ag.pos));
	}
	Mesh::MoveInfo minfo;
	mesh.recursive_move(0, ag.pos, ag.vel, ag.poly_id, -2, ag.wall_id, minfo);
	// poly, intersect, dir
	//if res == null:
	//	return

	//freal dist_moved = (minfo.pos_reached - ag.pos).length();
	//_log("\t\tdistance moved overall : " + String(Variant(dist_moved)));

	// if the move allowed by poly agent limits
	if (_agent_enter_poly(r_agent, minfo.poly_id)) {
		ag.pos = minfo.pos_reached;
		ag.wall_id = minfo.wall_id;

		ag.vel = ag.pos - old_pos;
		ag.blocking_narrowing_id = UINT32_MAX;
	} else {
		// debugging, record the blocking narrowing
		if (minfo.poly_id != UINT32_MAX) {
			ag.blocking_narrowing_id = mesh.get_poly(minfo.poly_id).narrowing_id;
		}
		ag.vel.zero();
		ag.state = AGENT_STATE_BLOCKED_BY_NARROWING;

		// print_line("narrowing blocked agent at poly " + itos(minfo.poly_id));
	}

	log(String("\tagent FINAL pos ") + str(ag.pos) + ", vel " + str(ag.vel) + ", poly " + itos(ag.poly_id));

	// limit the magnitude to the ingoing magnitude
	//	freal new_mag = minfo.vel_mag_final;
	//	new_mag = MIN(new_mag, mag);

	//	ag.vel = minfo.vel_dir;
	//	ag.vel.normalize_to_scale(new_mag);

	//if (!poly_contains_point(agent._poly_id, agent._pos, true)):
	//	assert (poly_contains_point(agent._poly_id, agent._pos))

	//		var test_poly_id = find_poly_within(agent._pos)
	//		if (agent._poly_id != test_poly_id):
	//			print("incorrect poly")
}

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
			log(String("\t\twall dot: ") + String(wall_dot));

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
			_log(String("\t\tleaving hug wall ") + itos(p_hug_wall_id));
			p_hug_wall_id = UINT32_MAX;
		}
	}

	// new destination
	IPoint2 to = p_from + p_vel;

	log(String("\trecursive_move [") + itos(p_depth) + "] poly " + itos(p_poly_id) + " to " + str(to) + " ... vel " + str(p_vel), p_depth);

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

	// reduce the velocity by how far travelled to the wall
	p_vel -= (pt_intersect - p_from);
	p_hug_wall_id = wall_id;
	// const Wall &wall = get_wall(p_hug_wall_id);
	return recursive_move(p_depth + 1, pt_intersect, p_vel, p_poly_id, p_poly_from_id, p_hug_wall_id, r_info);
}

FPoint3 MeshInstance::choose_random_location() const {
	const Mesh &mesh = get_mesh();
	NP_NP_ERR_FAIL_COND_V(!mesh.get_num_polys(), FPoint3());

	f32 total = 0;
	for (u32 n = 0; n < mesh.get_num_polys(); n++) {
		total += mesh.get_poly_extra(n).area;
	}

	f32 val = Math::randf() * total;

	total = 0;
	u32 poly_id = 0;

	for (u32 n = 0; n < mesh.get_num_polys(); n++) {
		total += mesh.get_poly_extra(n).area;
		if (total >= val) {
			poly_id = n;
			break;
		}
	}

	//u32 poly_id = Math::rand() % get_num_polys();
	return get_transform().xform(mesh.get_poly(poly_id).center3);
}

void MeshInstance::body_dual_trace(const Agent &p_agent, FPoint3 p_intermediate_destination, NavPhysics::TraceResult &r_result) const {
	Mesh::TraceInfo trace_info;
	const Mesh &mesh = get_mesh();

	FPoint3 final_dest = r_result.mesh.hit_point;

	// transform destination to mesh space
	p_intermediate_destination = get_transform_inverse().xform(p_intermediate_destination);
	IPoint2 intermediate_to = mesh.float_to_fixed_point_2(FPoint2::make(p_intermediate_destination.x, p_intermediate_destination.z));

	Mesh::TraceResult res = mesh.recursive_trace(0, p_agent.pos, intermediate_to, p_agent.poly_id, trace_info);
	r_result.mesh.first_trace_hit = (res != Mesh::TR_CLEAR);

	if (r_result.mesh.first_trace_hit) {
		return;
	}

	// transform destination to mesh space
	final_dest = get_transform_inverse().xform(final_dest);
	IPoint2 final_to = mesh.float_to_fixed_point_2(FPoint2::make(final_dest.x, final_dest.z));

	res = mesh.recursive_trace(0, intermediate_to, final_to, trace_info.poly_id, trace_info);
	r_result.mesh.hit = (res != Mesh::TR_CLEAR);

	if (r_result.mesh.hit) {
		FPoint2 hp = mesh.fixed_point_to_float_2(trace_info.hit_point);
		freal height = mesh.find_height_on_poly(trace_info.poly_id, trace_info.hit_point);
		r_result.mesh.hit_point = FPoint3::make(hp.x, height, hp.y);

		// back transform
		r_result.mesh.hit_point = get_transform().xform(r_result.mesh.hit_point);

		// normal
		NP_NP_ERR_FAIL_COND(trace_info.slide_wall == UINT32_MAX);
		const IPoint2 &norm = mesh.get_wall(trace_info.slide_wall).normal;
		FPoint2 norm2 = mesh.fixed_point_to_float_2(norm);

		r_result.mesh.hit_normal = FPoint3::make(norm2.x, 0, norm2.y);
		r_result.mesh.hit_normal = get_transform().basis.xform_normal(r_result.mesh.hit_normal);
	}
}

void MeshInstance::body_trace(const Agent &p_agent, NavPhysics::TraceResult &r_result) const {
	Mesh::TraceInfo trace_info;
	const Mesh &mesh = get_mesh();

	// transform destination to mesh space
	FPoint3 dest = r_result.mesh.hit_point;
	dest = get_transform_inverse().xform(dest);

	IPoint2 to = mesh.float_to_fixed_point_2(FPoint2::make(dest.x, dest.z));

	Mesh::TraceResult res = mesh.recursive_trace(0, p_agent.pos, to, p_agent.poly_id, trace_info);
	r_result.mesh.hit = (res != Mesh::TR_CLEAR);

	if (r_result.mesh.hit) {
		FPoint2 hp = mesh.fixed_point_to_float_2(trace_info.hit_point);
		freal height = mesh.find_height_on_poly(trace_info.poly_id, trace_info.hit_point);
		r_result.mesh.hit_point = FPoint3::make(hp.x, height, hp.y);

		// back transform
		r_result.mesh.hit_point = get_transform().xform(r_result.mesh.hit_point);

		// normal
		NP_NP_ERR_FAIL_COND(trace_info.slide_wall == UINT32_MAX);
		const IPoint2 &norm = mesh.get_wall(trace_info.slide_wall).normal;
		FPoint2 norm2 = mesh.fixed_point_to_float_2(norm);

		r_result.mesh.hit_normal = FPoint3::make(norm2.x, 0, norm2.y);
		r_result.mesh.hit_normal = get_transform().basis.xform_normal(r_result.mesh.hit_normal);
	}
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
	IPoint2 best_intersect{ 0, 0 };

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
	i64 c1 = x2 * y1 - x1 * y2;

	// Second line coefficients
	i32 a2 = y4 - y3;
	i32 b2 = x3 - x4;
	i64 c2 = x4 * y3 - x3 * y4;

	i64 denom = a1 * b2 - a2 * b1;

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
		i64 num = b1 * c2 - b2 * c1;
		if (num < 0) {
			x = num - offset;
		} else {
			x = num + offset;
		}
		x /= denom;
	}

	{
		i64 num = a2 * c1 - a1 * c2;
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
