#include "navphysics_mesh_instance.h"
#include "navphysics_map.h"

namespace NavPhysics {
const Mesh &MeshInstance::get_mesh() const {
	NP_DEV_ASSERT(_mesh_id != UINT32_MAX);
	return g_world.get_mesh(_mesh_id);
}

void MeshInstance::set_transform(const Transform &p_xform)
{
	_set_transform(p_xform, p_xform.affine_inverse(), p_xform.is_identity());
}

void MeshInstance::_set_transform(const Transform &p_xform, const Transform &p_xform_inv, bool p_is_identity) {
	_transform = p_xform;
	_transform_inverse = p_xform_inv;
	_transform_identity = p_is_identity;
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

} //namespace NavPhysics
