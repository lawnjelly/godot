// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#include "navphysics_mesh_instance.h"
#include "navphysics_map.h"
#include "navphysics_mesh.h"
#include "navphysics_rect.h"

namespace NavPhysics {

const Mesh &MeshInstance::get_mesh() const {
	NP_DEV_ASSERT(_mesh_id != UINT32_MAX);
	return g_world.get_mesh(_mesh_id);
}

FPoint3 MeshInstance::local_fixed_point_to_world_space(const Mesh &p_mesh, const IPoint3 &p_pt_local) const {
	FPoint3 pt = p_mesh.fixed_point_to_float(p_pt_local);
	return local_to_world_space(pt);
}

IPoint3 MeshInstance::world_space_to_local_fixed_point(const Mesh &p_mesh, const FPoint3 &p_pt_world) const {
	FPoint3 pt = world_space_to_local(p_pt_world);
	return p_mesh.float_to_fixed_point(pt);
}

/*
FPoint3 MeshInstance::local_pos_to_world_space(const Mesh &p_mesh, const IPoint2 &p_pt_local, freal p_agent_height) const {
	FPoint2 pos2 = p_mesh.fixed_point2_to_float2(p_pt_local);
	FPoint3 pt_world = FPoint3::make(pos2.x, p_agent_height, pos2.y);
	return get_transform().xform(pt_world);
}
*/

FPoint3 MeshInstance::local_to_world_space(const FPoint3 &p_pt_local) const {
	return get_transform().xform(p_pt_local);
}

FPoint3 MeshInstance::world_space_to_local(const FPoint3 &p_pt_world) const {
	return get_transform_inverse().xform(p_pt_world);
}

FPoint3 MeshInstance::velocity_local_to_world_space(const FPoint3 &p_vel_local) const {
	return get_transform().basis.xform(p_vel_local);
}

FPoint3 MeshInstance::velocity_world_space_to_local(const FPoint3 &p_vel_world) const {
	return get_transform_inverse().basis.xform(p_vel_world);
}

/*
IPoint2 MeshInstance::world_space_pos_to_local(const Mesh &p_mesh, const FPoint3 &p_pt_world, freal &r_agent_height) const {
	FPoint3 pos_local = world_space_pos_to_local3(p_pt_world);
	r_agent_height = pos_local.y;
	return p_mesh.float2_to_fixed_point2(pos_local.xz());
}
*/


/*
FPoint3 MeshInstance::local_velocity_with_jump_velocity_to_world_space(const Mesh &p_mesh, const IPoint2 &p_vel_local, freal p_jump_velocity) const {
	FPoint2 vel2 = p_mesh.fixed_point_vel_to_float(p_vel_local);
	return velocity_local_to_world_space(FPoint3(vel2.x, p_jump_velocity, vel2.y));
}
*/

FPoint3 MeshInstance::velocity_local_fixed_point_to_world_space(const Mesh &p_mesh, const IPoint3 &p_vel_local) const
{
	FPoint3 vel = p_mesh.dist_fixed_point_to_float(p_vel_local);
	return velocity_local_to_world_space(vel);
}


/*
FPoint3 MeshInstance::local_velocity_to_world_space(const Mesh &p_mesh, const IPoint2 &p_vel_local) const {
	FPoint2 vel2 = p_mesh.fixed_point_vel_to_float(p_vel_local);
	return velocity_local_to_world_space(FPoint3(vel2));
	//return get_transform().basis.xform(FPoint3(vel2));
}
*/

IPoint3 MeshInstance::velocity_world_space_to_local_fixed_point(const Mesh &p_mesh, const FPoint3 &p_vel_world) const
{
	FPoint3 vel_local = velocity_world_space_to_local(p_vel_world);
	return p_mesh.dist_float_to_fixed_point(vel_local);
}


/*
IPoint2 MeshInstance::world_space_velocity_to_local(const Mesh &p_mesh, const FPoint3 &p_vel_world) const {
	//FPoint3 vel_local = get_transform_inverse().basis.xform(p_vel_world);
	FPoint3 vel_local = velocity_world_space_to_local(p_vel_world);
	return p_mesh.float_to_fixed_point_vel(vel_local.xz());
}
*/

void MeshInstance::poi_add(POI *p_poi) {
	NP_DEV_ASSERT(p_poi);
	_POIs.push_back(p_poi);

	if (p_poi->handle_mesh_instance != 0) {
		NP_WARN_PRINT_ONCE("poi_add() :  poi already is assigned to a mesh instance.");
	}
	p_poi->handle_mesh_instance = get_handle();
}

void MeshInstance::poi_set_position(POI *p_poi, const FPoint3 &p_pt_world) {
	NP_DEV_ASSERT(p_poi);

#ifdef NP_DEV_ENABLED
	uint32_t id = poi_find(p_poi);
	NP_ERR_FAIL_COND(id == UINT32_MAX);
#endif

	// Calculate the poi local space position relative to the mesh instance.
	//p_poi->local_pos3 = world_space_to_local(p_pt_world);

	NP_ERR_FAIL_COND(_mesh_id == UINT32_MAX);
	const Mesh &mesh = get_mesh();
	
	p_poi->local_point.pos = world_space_to_local_fixed_point(mesh, p_pt_world);

	// Find the polygon.
	freal fit = 0;
	
	p_poi->local_point.poly_id = mesh.find_best_poly_within(p_poi->local_point.pos, 10.0f * _scale_world_space_to_local_fixed_point, 0.5f * _scale_world_space_to_local_fixed_point, fit);
	print_line(String("Best polygon is ") + itos(p_poi->local_point.poly_id));
}

uint32_t MeshInstance::poi_find(POI *p_poi) const {
	for (uint32_t n = 0; n < _POIs.size(); n++) {
		if (_POIs[n] == p_poi) {
			return n;
		}
	}

	return UINT32_MAX;
}

bool MeshInstance::poi_remove(POI *p_poi) {
	uint32_t id = poi_find(p_poi);
	if (id != UINT32_MAX) {
		_POIs.remove_unordered(id);
		p_poi->handle_mesh_instance = 0;
		return true;
	}

	return false;
}

void MeshInstance::set_active(bool p_active) {
	_active = p_active;
}

void MeshInstance::set_gravity(const FPoint3 &p_direction, bool p_local_space) {
	// Direction NYI
	_is_gravity_local_space = p_local_space;
}

void MeshInstance::set_transform(const Transform &p_xform) {
	_set_transform(p_xform, p_xform.affine_inverse(), p_xform.is_identity());
	_refresh_gravity();
}

void MeshInstance::_refresh_gravity() {
	// Scale and direct a unit length world space down vector to the local mesh instance.
	_gravity_local_space = _transform_inverse.basis.xform(FPoint3(0, -1, 0));
	freal l = _gravity_local_space.length();
	
	const Mesh &mesh = get_mesh();
	
	// Also refresh general scaling while we are at it.
	//_scale_world_space_to_local = l;
	_scale_world_space_to_local_fixed_point = mesh.dist_float_to_fixed_point(l);
	
	
	if (_is_gravity_local_space) {
		_gravity_local_space = FPoint3(0, l, 0);
	}

	// Scale the down vector to integer space for the mesh.
	_gravity_local_space = mesh.dist_float_to_fixed_point(_gravity_local_space).to_f32();
}

void MeshInstance::_set_transform(const Transform &p_xform, const Transform &p_xform_inv, bool p_is_identity) {
	_transform = p_xform;
	_transform_inverse = p_xform_inv;
	_transform_identity = p_is_identity;
}

freal MeshInstance::find_agent_jump_fit(Agent &r_agent, const JumpFinderData &p_jump_data, u32 &r_best_poly_id, freal p_max_step_up, freal p_max_drop, freal p_best_fit_so_far) const {
	
	// Drop and step up should be in local units, rather than world space.	
	p_max_drop *=_scale_world_space_to_local_fixed_point;
	p_max_step_up *=_scale_world_space_to_local_fixed_point;
	
	JumpFinderDataLocal jl;

	r_best_poly_id = UINT32_MAX;

	// This is the mesh being JUMPED TO, not the mesh being jumped from.
	const Mesh &mesh = get_mesh();

	// xform agent world space to local space

#define NP_FIND_AGENT_JUMP_WORLD_TO_FP(PARAM)                                                  \
	{                                                                                          \
		jl.pt_##PARAM##_local = world_space_to_local(p_jump_data.pt_##PARAM##_world); \
		jl.PARAM##_local = mesh.float_to_fixed_point(jl.pt_##PARAM##_local);                   \
	}

	NP_FIND_AGENT_JUMP_WORLD_TO_FP(from);
	NP_FIND_AGENT_JUMP_WORLD_TO_FP(to);
	NP_FIND_AGENT_JUMP_WORLD_TO_FP(far);

	// TODO: Can these local velocities be calculated after the AABB check?

	// FROM IS TOO FAR FORWARD, WE NEED TO BACK TRACK
	jl.vel_local = velocity_world_space_to_local_fixed_point(mesh, p_jump_data.vel_world);

#ifdef NP_DEV_ENABLED
	// FPoint3 unit_test_world_vel = local_velocity_with_jump_velocity_to_world_space(mesh, jd.vel_local2, jd.vel_local_jump);
	//log(String("world vel after jump calculated as : ") + unit_test_world_vel);
#endif

#ifdef NP_VERIFY_JUMP_FINDER_VELOCITY
	NP_FIND_AGENT_JUMP_WORLD_TO_FP(pt_verify_curr_plus_vel, pt_verify_to_plus_vel_local);
	jd.pt_verify_to_plus_vel_local = jd.pt_verify_to_plus_vel_local - jd.pt_to_local;
#endif

#undef NP_FIND_AGENT_JUMP_WORLD_TO_FP

	// outside AABB?
	NP_LLOG(String("find_agent_jump_fit pos from ") + p_jump_data.pt_from_world + ", aabb " + mesh._extended_aabb);

	// Include step up.
	AABB aabb = mesh._extended_aabb;
	aabb.position.y -= p_max_step_up;
	aabb.size.y += p_max_step_up;

	AABB aabb_jump;
	aabb_jump.position = jl.pt_from_local;
	aabb_jump.expand_to(jl.pt_far_local);

	if (!aabb.overlaps_aabb_or_above(aabb_jump)) {
		NP_LLOG("\tnot in aabb");
		return 1000000;
	}
	
	NP_LLOG(String("\tfind_agent_jump_fit ipos ") + jl.pt_from_local + " to " + jl.pt_far_local);

	// Calculate a bonus ALLOWED drop of whatever jump height the agent currently has.
	freal jump_height = r_agent.get_height() - r_agent.floor_height;
	p_max_drop += jump_height;

	freal fit = p_best_fit_so_far;

	// Note that this routine is an optimized one that relies on the velocity crossing a poly wall,
	// and therefore that the up vectors of both meshes are somewhat aligned.
	// If the two meshes are at 90 degrees, and the velocity is _straight down_ into the new mesh,
	// no wall may be crossed, and this will return UINT32_MAX, so we need to try the slower backup path.
	r_best_poly_id = mesh.find_best_jump_poly_within(r_agent, jl, p_max_drop, p_max_step_up, fit);

	if (r_best_poly_id == UINT32_MAX) {
		
		IPoint3 to_local = ((jl.to_local - jl.from_local) / 2) + jl.from_local;
		FPoint3 to_world = ((p_jump_data.pt_to_world - p_jump_data.pt_from_world) * 0.5f) + p_jump_data.pt_from_world;
		
		// If the external links fail, try teleporting directly into the new mesh.
		r_best_poly_id = mesh.find_best_poly_within(to_local, p_max_drop, p_max_step_up, fit);
		
		// If the to position failed, try the from.
		if (r_best_poly_id == UINT32_MAX)
		{
			to_local = jl.from_local;
			to_world = p_jump_data.pt_from_world;
			r_best_poly_id = mesh.find_best_poly_within(to_local, p_max_drop, p_max_step_up, fit);
		}
		
		// If we teleport directly into the new mesh, we must be on a poly.
		// We use the from position, and we directly set the agent,
		// rather than using a jump target.
		if ((r_best_poly_id != UINT32_MAX) && (fit < p_best_fit_so_far)) {
			r_agent.set_poly_id(r_best_poly_id);
			r_agent.clear_wall_id();
			
			
			r_agent.set_pos(to_local);
			NP_DEV_ASSERT(local_fixed_point_to_world_space(mesh, to_local).is_equal_approx(to_world));
			NP_LLOG(String("\tJUMP best fit so far world pos is ") + to_world + " on mesh_instance ID : " + get_id());
			
			//r_agent.set_height(jl.pt_to_local.y);
			r_agent.clear_wall_id();

			r_agent.set_velocity(jl.vel_local);
			NP_DEV_ASSERT(velocity_local_fixed_point_to_world_space(mesh, jl.vel_local).is_equal_approx(p_jump_data.vel_world));
			
			
			
			//r_agent.set_jump_velocity(jl.vel_local.height);

			// Set local velocity and jump vel NYI

			/*
			AgentStatus::jump_target_cross_pos = jl.to_local;
			AgentStatus::jump_target_vel = jl.vel_local.xy;
			AgentStatus::jump_target_jump_vel = jl.vel_local.height;
			AgentStatus::jump_cross_only = true;
			AgentStatus::jump_target_local_height = jl.pt_to_local.y;
			*/

			//AgentStatus::jump_target_world_space_vel = local_velocity_to_world_space(get_mesh(), AgentStatus::jump_target_vel);
		}
	} else {
		NP_LOG("Found jump target position");
		//AgentStatus::jump_target_world_space_vel = local_velocity_to_world_space(get_mesh(), AgentStatus::jump_target_vel);
	}

	if (r_best_poly_id == UINT32_MAX) {
		NP_LLOG("\tnot in tri");
		return 10000;
	}

	// multiple meshs NYI
	NP_LLOG("\tinside navmesh");

	// If we are replacing the best fit, we can overwrite the jump data on the agent.
	if (fit < p_best_fit_so_far) {
		//AgentStatus::debug_print("find_agent_jump_fit best_poly is ");
	}

	return fit;
}

bool MeshInstance::find_last_ditch_poly(Agent &r_agent, const FPoint3 &p_world_pos) const {
	// xform agent world space to local space
	FPoint3 pos = get_transform_inverse().xform(p_world_pos);
	const Mesh &mesh = get_mesh();

	// Confine the test point to the AABB of the mesh.
	// There is no hope of a hit unless we are within the AABB.
	AABB aabb = mesh._extended_aabb;
	aabb.confine_point(pos);

	// Convert position to fixed point.
	IPoint2 ipos = mesh.float2_to_fixed_point2(pos.xz());

	return mesh.find_best_poly_within_last_ditch_FINAL(r_agent, pos, ipos);
}

freal MeshInstance::find_agent_fit(Agent &r_agent, const FPoint3 &p_world_pos, u32 &r_best_poly_id, freal p_max_step_up, freal p_max_drop) const {
	r_best_poly_id = UINT32_MAX;

	// xform agent world space to local space
	FPoint3 pos_local = world_space_to_local(p_world_pos);

	// outside AABB?
	const Mesh &mesh = get_mesh();

	NP_LLOG(String("find_agent_fit pos ") + pos_local + ", aabb " + mesh._extended_aabb);

	// Include step up.
	AABB aabb = mesh._extended_aabb;
	aabb.position.y -= p_max_step_up;
	aabb.size.y += p_max_step_up;

	if (!aabb.contains_point_or_above(pos_local)) {
		NP_LLOG("\tnot in aabb");

		// Calculate the distance to the AABB, so we can preferentially choose with ties.
		freal proximity = aabb.distance_to(pos_local);

		// Convert proximity to world space.
		FPoint3 proximity_vec = get_transform().basis.xform(FPoint3(proximity, 0, 0));
		proximity = proximity_vec.length();

		return 1000000 + proximity;
	}

	// Convert position to fixed point.
	IPoint3 ipos = mesh.float_to_fixed_point(pos_local);
	//IPoint2 ipos = mesh.float2_to_fixed_point2(pos_local.xz());
	NP_LLOG(String("\tfind_agent_fit ipos ") + ipos);

	// Check for inclusion inside tri.
	//r_best_poly_id = mesh.find_poly_within(ipos);
	freal fit = 10000;
	
	// Drop and step up should be in local units, rather than world space.	
	p_max_drop *=_scale_world_space_to_local_fixed_point;
	p_max_step_up *=_scale_world_space_to_local_fixed_point;
	
	r_best_poly_id = mesh.find_best_poly_within(ipos, p_max_drop, p_max_step_up, fit);

	if (r_best_poly_id == UINT32_MAX) {
		NP_LLOG("\tnot in tri");
		return 10000;
	}

	// multiple meshs NYI
	NP_LLOG("\tinside navmesh");
	return fit;
}

void MeshInstance::teleport_agent(Agent &r_agent) {
	// Make sure wall is reset.
	r_agent.clear_wall_id();

	const Mesh &mesh = get_mesh();

	// Get the fixed point velocity into the new local space.
	//r_agent.vel.zero();
	IPoint3 vel_local = velocity_world_space_to_local_fixed_point(mesh, r_agent.fimpulse3);
//	FPoint3 vel_local = get_transform_inverse().basis.xform(r_agent.fimpulse3);
//	r_agent.set_velocity(mesh.dist_float_to_fixed_point(vel_local));
	r_agent.set_velocity(vel_local);

	NP_LOG(String("teleport world vel: ") + r_agent.fimpulse3 + ", local vel: " + vel_local);

	r_agent.set_pos(mesh.float_to_fixed_point(r_agent.fpos3));

	NP_LOG(String("\tr_agent.pos is ") + r_agent.get_pos());
	//r_agent.pos.set(7906, 41409);

	//u32 new_poly_id = mesh.find_poly_within(r_agent.pos, r_agent.suggested_poly_id);

	// r_agent.fpos3_teleport.y is NOT in local space, and find_best_poly_within
	// needs local space height.

	// Don't attempt to find the best poly if we are in a jump link.
	if (AgentStatus::is_in_jump_link()) {
		return;
	}

	freal fit;
	u32 new_poly_id = mesh.find_best_poly_within(r_agent.get_pos(), mesh.mesh_params.exit_max_drop, mesh.mesh_params.exit_max_step_up, fit);

	// Reset the suggested poly id, as it can be used only once.
	AgentStatus::suggested_poly_id = UINT32_MAX;

	_agent_enter_poly(r_agent, new_poly_id, true);

	if (r_agent.is_poly_id_null()) {
		NP_WARN_PRINT("not within poly : Is MAX_DROP and MAX_STEP_UP correct? Guessing a poly centre.");

		// just guess a poly
		if (mesh.get_num_polys()) {
			const Poly &poly = mesh.get_poly(0);
			r_agent.set_pos(poly.center);
			_agent_enter_poly(r_agent, 0, true);
		}

	} else {
		NP_LOG(String("\tc++ agent within poly ") + r_agent.get_poly_id());
	}

	//	if (ag.wall_id == UINT32_MAX) {
#ifdef NP_DEV_ENABLED
	NP_DEV_ASSERT(mesh.poly_contains_point(r_agent.get_poly_id(), r_agent.get_pos().xz()));
	NP_LOG(String("Confirm ") + r_agent.get_pos() + " is within poly " + r_agent.get_poly_id());
#endif
	//	}

	// Rejig the height to the new poly.
	//	if (r_agent.poly_id != UINT32_MAX) {
	//		r_agent.floor_height = mesh.find_height_on_poly(r_agent.poly_id, r_agent.pos);
	//	}
}

void MeshInstance::agent_get_info(const Agent &p_agent, BodyInfo &r_body_info) const {
	r_body_info.poly_id = p_agent.get_poly_id();
	r_body_info.blocking_zone_id = p_agent.blocking_zone_id;

#if 0
	const Mesh &mesh = get_mesh();

// NYI
	if (p_agent.poly_id != UINT32_MAX) {
		r_body_info.narrowing_id = mesh.get_poly_extra(p_agent.poly_id).narrowing_id;

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
#endif
}

void MeshInstance::iterate_agent(Agent &r_agent, Mesh::MoveInfo &r_move_info) {
	//print_line("c++ agent at pos " + String(Variant(r_agent.fpos)));

	// has the f32 position moved significantly? if not, retain
	// the fixed point position as this is the gold standard
	//IPoint2 new_pos_fp = float_to_fixed_point_2(r_agent.fpos);
	//IPoint2 offset = new_pos_fp - r_agent.pos;
	//if ((Math::abs(offset.x) > 1) || (Math::abs(offset.y) > 1)) {
	//r_agent.pos = new_pos_fp;
	//}

	//const Mesh &mesh = get_mesh();

	/*
	if (!AgentStatus::is_in_jump_link()) {
		IPoint2 impulse = mesh.float_to_fixed_point_vel(r_agent.fimpulse3.xz());
		r_agent.vel += impulse;
		log(String("iterate_agent : adding to vel ") + r_agent.vel);
		//r_agent.apply_friction();
	}
*/

	// Add the instant impulse before the move, then remove it afterwards
	if (r_agent.instant_impulse != IPoint2()) {
		freal instant_length = r_agent.instant_impulse.lengthf();
		r_agent.add_2d_velocity(r_agent.instant_impulse);

		_iterate_agent(r_agent, r_move_info);

		// Remove the instant impulse from the remaining velocity.
		freal after_length = r_agent.get_velocity().lengthf();
		freal new_length = after_length - instant_length;

		if (new_length >= 1) {
			IPoint2 tmp = r_agent.get_2d_velocity();
			tmp.normalize_to_scale(new_length);
			r_agent.set_2d_velocity(tmp);
		} else {
			r_agent.set_2d_velocity(IPoint2());
		}
		r_agent.instant_impulse.zero();
	} else {
		_iterate_agent(r_agent, r_move_info);
	}

	bool changed_mesh_instance = r_move_info.new_mesh_instance_id != UINT32_MAX;

	if (!changed_mesh_instance) {
		iterate_agent_housekeeping_after(r_agent);

		// only update the f32ing point position if significantly different
		//mesh.refresh_local_agent_position_from_fixed_point(r_agent);
	} else {
		NP_LLOG("changed mesh instance");
	}

	// Return the fixed point space to float space after iteration,
	// to allow avoidance etc on the next tick.
	// r_agent.fvel = mesh.fixed_point_vel_to_float(r_agent.vel);
}

u32 MeshInstance::calculate_agent_radius_in_mesh_units(freal p_radius) const {
	NP_DEV_ASSERT(p_radius >= 0);

	// First transform the world space radius into mesh instance space.
	FPoint3 pt = _transform.basis.xform(FPoint3(p_radius, 0, 0));
	freal radius_mesh_space = pt.length();

	const Mesh &mesh = get_mesh();
	i32 fp_radius = mesh.dist_float_to_fixed_point(radius_mesh_space);
	return Math::abs(fp_radius);
}

void MeshInstance::refresh_agent_local_velocity_from_world_space(Agent &r_agent, const FPoint3 &p_vel_world) const {
	const Mesh &mesh = get_mesh();
	IPoint3 vel_local = velocity_world_space_to_local_fixed_point(mesh, p_vel_world);
	r_agent.set_velocity(vel_local);
}

void MeshInstance::refresh_agent_world_space_position(Agent &r_agent, bool p_remake_pos) const {
	// Translate the agent height back into final output pos3.
//	if (p_remake_pos) {
//		r_agent.fpos3 = FPoint3::make(r_agent.fpos.x, r_agent.get_height(), r_agent.fpos.y);
//	}

	// Transform the location back into world space.
	//r_agent.fpos3 = get_transform().xform(r_agent.fpos3);
	
	r_agent.fpos3 = local_fixed_point_to_world_space(get_mesh(), r_agent.get_pos());
	NP_LOG(String("\tcalculating fpos3 from fixed point : ") + r_agent.get_pos() + " on mesh_instance_id : " + get_id() + ", world space is : " + r_agent.fpos3);
	
	NP_DEV_ASSERT(r_agent.fpos3.isfinite());

#if 0
	if (r_agent.vel.length_squared() > 1024) {
		r_agent.seek_yaw(r_agent.vel.angle());
	}

	// Transform yaw back to world space.
	FPoint3 dir = FPoint3(0, r_agent.yaw, 0);
	//	dir = get_transform().basis.xform_normal(dir);
	r_agent.worldspace_yaw = dir.y;
#endif
}

void MeshInstance::iterate_agent_housekeeping_after(Agent &r_agent) {
	//log(String("iterate_agent_housekeeping on tick ") + Mesh::_tick);
	const Mesh &mesh = get_mesh();

	if (!AgentStatus::is_in_jump_link()) {
		r_agent.apply_friction();

		if (!r_agent.is_poly_id_null()) {
			r_agent.floor_height = mesh.find_height_on_poly_plane(r_agent.get_poly_id(), r_agent.get_pos().xz());
		}
	} else {
		// Calculate the new floor height based on the jump poly destination.
		const Wall &jump_wall = mesh.get_wall(AgentStatus::jump_wall_id);

		NP_DEV_ASSERT(jump_wall.poly_id != UINT32_MAX);
		r_agent.floor_height = mesh.find_height_on_poly_plane(jump_wall.poly_id, r_agent.get_pos().xz());
	}

	// Keep the agent height updated if we are jumping.
	if ((!r_agent.is_on_floor() || r_agent.get_jump_velocity() > 0) && !r_agent.is_poly_id_null() && r_agent.grounded) {
		freal height;
		if (mesh.find_ceiling_height(r_agent.get_poly_id(), r_agent.get_pos().xz(), height, r_agent.ceiling_poly_id)) {
#ifdef NP_DEV_ENABLED
			// Set debug position for client.
			FPoint3 debug_pos = mesh.fixed_point_to_float(r_agent.get_pos());
			debug_pos.y = height;
			r_agent.debug_pos[2] = debug_pos;
#endif

			// The ceiling height must be adjusted for both the agent height,
			// and the offset downward of the ceiling mesh from the ceiling (agent radius?).
			height -= mesh.mesh_params.agent_height;

			r_agent.iterate_jump_after(*this, &height);
			return;
		}
	}

	r_agent.iterate_jump_after(*this);
}

bool MeshInstance::_agent_enter_poly(Agent &r_agent, u32 p_new_poly_id, bool p_force_allow) {
	u32 old_poly_id = r_agent.get_poly_id();
	if (old_poly_id == p_new_poly_id) {
		return true;
	}

	if (!r_agent.is_npc) {
#ifdef NP_DEV_ENABLED
		const Mesh &mesh = get_mesh();
		u32 old_zone_id = r_agent.get_poly_id() == UINT32_MAX ? UINT32_MAX : mesh.get_poly_extra(r_agent.get_poly_id()).zone_id;
		u32 new_zone_id = p_new_poly_id == UINT32_MAX ? UINT32_MAX : mesh.get_poly_extra(p_new_poly_id).zone_id;
		if (old_zone_id != new_zone_id) {
			String sz = String("Player entering zone_id ") + new_zone_id;

			if (new_zone_id != UINT32_MAX) {
				const Zone &new_zone2 = mesh.get_zone(new_zone_id);
				const ZoneInstance &new_zone_instance2 = get_zone_instance(new_zone_id);

				sz += String(", max_agents: ") + new_zone2.max_agents + ", used: " + new_zone_instance2.used;
			}

			log(sz);
		}
#endif

		r_agent.set_poly_id(p_new_poly_id);
		return true;
	}

	// Nothing blocking by default.
	r_agent.blocking_zone_id = UINT32_MAX;

#if 1
	const Mesh &mesh = get_mesh();

	// get old narrowing
	const PolyExtra *old_poly = nullptr;
	//Narrowing *old_narrowing = nullptr;
	//u32 old_available = 0;
	ZoneInstance *old_zone_instance = nullptr;

	if (old_poly_id != UINT32_MAX) {
		old_poly = &mesh.get_poly_extra(old_poly_id);

		u32 old_zone_id = old_poly->zone_id;
		if (old_zone_id != UINT32_MAX) {
			old_zone_instance = &get_zone_instance(old_zone_id);
			//old_available = mesh.get_zone(old_zone_id).max_agents;
		}
	}

	// get new narrowing
	const PolyExtra *new_poly = nullptr;
	const Zone *new_zone = nullptr;
	ZoneInstance *new_zone_instance = nullptr;

	if (p_new_poly_id != UINT32_MAX) {
		new_poly = &mesh.get_poly_extra(p_new_poly_id);

		u32 new_zone_id = new_poly->zone_id;

		if (new_zone_id != UINT32_MAX) {
			new_zone = &mesh.get_zone(new_zone_id);
			new_zone_instance = &get_zone_instance(new_zone_id);
		}
	}
	////////////////////////////////////////////////////////////

	// try to enter the new poly BEFORE leaving the old
	if (new_poly) {
		// special case, both polys are in the same zone
		if (old_poly && (old_poly->zone_id == new_poly->zone_id)) {
			r_agent.set_poly_id(p_new_poly_id);
			return true;
		}

		if (new_zone) {
			u32 available = new_zone->max_agents;

			// Special modification... when moving from a larger narrowing to a tighter,
			// allow 1 less agent to enter. This makes it more likely agents can exit from tight
			// narrowings.
			//			if (old_available > available) {
			//				available--;
			//				// always allow at least one
			//				if (!available) {
			//					available = 1;
			//				}
			//			}

			if ((new_zone_instance->used >= available) && !p_force_allow) {
				r_agent.blocking_zone_id = new_poly->zone_id;
				//log(String("DISALLOW new zone available : ") + available + ", used : " + new_zone_instance->used);
				return false;
			}

			new_zone_instance->used += 1;
			//			if (!p_force_allow) {
			//				log(String("ALLOW new zone available : ") + available + ", used : " + new_zone_instance->used);
			//			}

#ifdef NP_DEV_ENABLED

			i64 found = new_zone_instance->used_agent_ids.find(r_agent.agent_id);
			if (found != -1) {
				NP_WARN_PRINT("New zone instance already contains agent id.");
			}
			new_zone_instance->used_agent_ids.push_back(r_agent.agent_id);
#endif
		}
	}
#endif

	// save the new poly id
	r_agent.zone_id = new_poly ? new_poly->zone_id : UINT32_MAX;
	r_agent.set_poly_id(p_new_poly_id);

#if 1
	if (old_zone_instance) {
		if (old_zone_instance->used) {
			old_zone_instance->used -= 1;
		} else {
			NP_WARN_PRINT("Old zone instance has no agents.");
		}

#ifdef NP_DEV_ENABLED
		i64 found = old_zone_instance->used_agent_ids.find(r_agent.agent_id);
		if (found != -1) {
			old_zone_instance->used_agent_ids.remove_unordered(found);
		} else {
			NP_WARN_PRINT("Old zone instance does not contain agent id.");
		}
#endif
	}
#endif
	return true;
}

FPoint3 MeshInstance::choose_random_location() const {
	const Mesh &mesh = get_mesh();
	NP_ERR_FAIL_COND_V(!mesh.get_num_polys(), FPoint3());

	f32 total = 0;
	for (u32 n = 0; n < mesh.get_num_polys(); n++) {
		total += mesh.get_poly_extra(n).area;
	}

	// Just for safety, shouldn't get hit.
	NP_ERR_FAIL_COND_V(total < 0.1f, FPoint3());

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
	IPoint2 intermediate_to = mesh.float2_to_fixed_point2(FPoint2::make(p_intermediate_destination.x, p_intermediate_destination.z));
	
	Mesh::TraceResult res = mesh.recursive_trace(0, p_agent.get_pos().xz(), intermediate_to, p_agent.get_poly_id(), trace_info);
	r_result.mesh.first_trace_hit = (res != Mesh::TR_CLEAR);

	if (r_result.mesh.first_trace_hit) {
		return;
	}

	// transform destination to mesh space
	final_dest = get_transform_inverse().xform(final_dest);
	IPoint2 final_to = mesh.float2_to_fixed_point2(FPoint2::make(final_dest.x, final_dest.z));

	res = mesh.recursive_trace(0, intermediate_to, final_to, trace_info.poly_id, trace_info);
	r_result.mesh.hit = (res != Mesh::TR_CLEAR);

	if (r_result.mesh.hit) {
		FPoint2 hp = mesh.fixed_point2_to_float2(trace_info.hit_point);
		freal height = mesh.find_float_height_on_poly_plane(trace_info.poly_id, trace_info.hit_point);
		r_result.mesh.hit_point = FPoint3::make(hp.x, height, hp.y);

		// back transform
		r_result.mesh.hit_point = get_transform().xform(r_result.mesh.hit_point);

		// normal
		NP_ERR_FAIL_COND(trace_info.slide_wall == UINT32_MAX);
		const IPoint2 &norm = mesh.get_wall(trace_info.slide_wall).normal;
		FPoint2 norm2 = mesh.fixed_point2_to_float2(norm);

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

	IPoint2 to = mesh.float2_to_fixed_point2(FPoint2::make(dest.x, dest.z));
	
	Mesh::TraceResult res = mesh.recursive_trace(0, p_agent.get_pos().xz(), to, p_agent.get_poly_id(), trace_info);
	r_result.mesh.hit = (res != Mesh::TR_CLEAR);

	if (r_result.mesh.hit) {
		FPoint2 hp = mesh.fixed_point2_to_float2(trace_info.hit_point);
		freal height = mesh.find_float_height_on_poly_plane(trace_info.poly_id, trace_info.hit_point);
		r_result.mesh.hit_point = FPoint3::make(hp.x, height, hp.y);

		// back transform
		r_result.mesh.hit_point = get_transform().xform(r_result.mesh.hit_point);

		// normal
		NP_ERR_FAIL_COND(trace_info.slide_wall == UINT32_MAX);
		const IPoint2 &norm = mesh.get_wall(trace_info.slide_wall).normal;
		FPoint2 norm2 = mesh.fixed_point2_to_float2(norm);

		r_result.mesh.hit_normal = FPoint3::make(norm2.x, 0, norm2.y);
		r_result.mesh.hit_normal = get_transform().basis.xform_normal(r_result.mesh.hit_normal);
	}
}

void MeshInstance::link_mesh(u32 p_mesh_id) {
	_zone_instances.clear();
	_mesh_id = p_mesh_id;

	if (_mesh_id != UINT32_MAX) {
		const Mesh &mesh = get_mesh();
		_zone_instances.resize(mesh.get_num_zones());

		//		for (u32 n = 0; n < _zone_instances.size(); n++) {
		//			_zone_instances[n].available = mesh.get_zone(n).max_agents;
		//		}
	}
}

void MeshInstance::llog(String p_sz) const {
	NP_LOG(p_sz);
}

bool MeshInstance::_iterate_agent_on_jump_link(Agent &r_agent, Mesh::MoveInfo &r_move_info, freal &r_distance) {
	Agent &ag = r_agent;
	const Mesh &mesh = get_mesh();

	const IPoint2 &dest = ag.jump_link_target_pos;
	u32 &wall_id = ag.jump_link_wall_id;
	
	IPoint2 offset = dest - ag.get_pos().xz();
	//freal mag = ag.vel.lengthf();

	// Let's add some minimum velocity when on a jump link, because we don't want to be traversing too slow...
	const freal min_speed = Mesh::_inverse_timestep * 2 * (FPoint2::FP_RANGE / 65535.0f);
	if (r_distance < min_speed) {
		//log(String("min speed from ") + r_distance + " to " + min_speed);
		r_distance = min_speed;
	}

	offset.normalize_to_scale(r_distance);
	
	IPoint2 orig_agent_pos = ag.get_pos().xz();
	ag.set_2d_pos(orig_agent_pos + offset);

	//i64 cross = mesh.wall_cross(ag.jump_link_wall_id, ag.pos);
	//NP_LOG(String("wall cross ") + cross);

	const Wall &wall = mesh.get_wall(wall_id);
	IPoint2 a, b;
	mesh.get_wall_verts(wall_id, a, b);
	NP_LOG(String("ag.pos is ") + ag.get_pos() + ", wall verts " + a + " to " + b);

	// End condition, we have crossed the wall
	if (!mesh.wall_in_front_cross(wall_id, ag.get_pos().xz()) || (offset.length_squared() == 0)) {
		NP_LOG(String("in front of jump link wall"));
		ag.set_poly_id(wall.poly_id);
		NP_LOG(String("jump link finished to poly ") + ag.get_poly_id());

		// Note that the agent position is approximate, due to rounding error
		// it may be outside the poly.
		// So to prevent poly inside errors, we immediately set the agent hug wall ID
		// to the jump wall id.
		ag.set_wall_id(wall_id);

		// Reset the jump link, as no longer active.
		ag.jump_link_wall_id = UINT32_MAX;

		// Calculate the remaining velocity magnitude.
		freal dist_to_targ = (dest - orig_agent_pos).lengthf();
		r_distance -= dist_to_targ;
		r_distance = MAX(r_distance, (freal)0);

		ag.set_2d_pos(dest);
		ag.force_off_floor();

		// Finished the jump link, return false.
		return false;
		//		u32 new_poly_id = mesh.find_poly_within(ag.pos, ag.poly_id);
		//		ag.poly_id = new_poly_id;
		//		NP_DEV_ASSERT(new_poly_id == ag.poly_id);
	}

	return true;
}

void MeshInstance::_agent_slide_on_ceiling(Agent &r_agent, Mesh::MoveInfo &r_move_info, IPoint2 &r_agent_velocity) {
	if ((r_agent.is_on_floor() && r_agent.get_jump_velocity() <= 0) || (r_agent.is_poly_id_null()) || !r_agent.grounded) {
		return;
	}

	const Mesh &mesh = get_mesh();
	
	IPoint2 dest_pos = r_agent.get_2d_pos() + r_agent_velocity;
	freal height;
	if (!mesh.find_ceiling_height(r_agent.get_poly_id(), dest_pos, height, r_agent.ceiling_poly_id)) {
		return;
	}

	// Start from a source point well back from the actual source, so we can detect the first crossing with the ceiling poly.
	IPoint2 backtrack = -r_agent_velocity;
	backtrack.normalize_to_scale(FPoint2::FP_RANGE / 8);
	IPoint2 source_pos = r_agent.get_2d_pos() + backtrack;

	// The ceiling height must be adjusted for both the agent height,
	// and the offset downward of the ceiling mesh from the ceiling (agent radius?).
	height -= mesh.mesh_params.agent_height;

	// If we are too high, then avoid the ceiling poly
	if ((r_agent.get_height() > height) && (r_agent.ceiling_poly_id != UINT32_MAX)) {
		// Find which wall we have crossed.
		const Poly &poly = mesh.get_poly(r_agent.ceiling_poly_id, true);

		const IPoint3 *vert_a = &mesh.get_vert(mesh.get_ind(poly.first_ind + poly.num_inds - 1, true), true);

		IPoint2 pt_hit;

		u64 best_dist = UINT64_MAX;
		u32 best_wall = UINT32_MAX;

		for (u32 w = 0; w < poly.num_inds; w++) {
			const IPoint3 *vert_b = &mesh.get_vert(mesh.get_ind(poly.first_ind + w, true), true);
			
			if (mesh.find_line_segments_intersect_integer(source_pos, dest_pos, vert_a->xz(), vert_b->xz(), pt_hit)) {
				u64 dist = (pt_hit - source_pos).length_squared();

				if (dist < best_dist) {
					best_dist = dist;
					best_wall = w;
				}
			}

			vert_a = vert_b;
		}

		// If we found a slide wall...
		if (best_wall != UINT32_MAX) {
			const IPoint2 a = mesh.get_vert2(mesh.get_ind(poly.first_ind + ((best_wall - 1) % poly.num_inds), true), true);
			const IPoint2 b = mesh.get_vert2(mesh.get_ind(poly.first_ind + best_wall, true), true);

			IPoint2 wall_vec = b - a;

			// Correct the polarity of the slide.
			freal dot = wall_vec.dot_normalized(r_agent_velocity);

			if (dot < 0) {
				wall_vec = -wall_vec;
				dot = -dot;
			}

			// Change the agent velocity to the direction of the slide.
			freal vel_mag = r_agent_velocity.lengthf();
			wall_vec.normalize_to_scale(vel_mag * dot);

			r_agent_velocity = wall_vec;
		}
	}
}

void MeshInstance::_iterate_agent(Agent &r_agent, Mesh::MoveInfo &r_move_info) {
	Agent &ag = r_agent;

	// The acting velocity also takes into account the instant impulse,
	// which is not proper physics, but allows separation bodges on a single tick.
	NP_LLLOG(String("\tpos : ") + ag.pos.x + String(", vel : ") + ag.vel.x + ", instant_vel : " + ag.instant_impulse.x);
	//IPoint2 acting_velocity = ag.vel + ag.instant_impulse;

	IPoint2 agent_remaining_velocity = ag.get_2d_velocity();
	freal agent_remaining_magnitude = agent_remaining_velocity.lengthf();

	// The old pos is used to calculate the velocity in the next iteration,
	// so should be saved BEFORE doing jump link movement.
	IPoint2 old_pos = ag.get_2d_pos();

	if (ag.is_in_jump_link()) {
		if (_iterate_agent_on_jump_link(r_agent, r_move_info, agent_remaining_magnitude)) {
			return;
		}
		agent_remaining_velocity.normalize_to_scale(agent_remaining_magnitude);
	}

	const Mesh &mesh = get_mesh();

	// This shouldn't happen normally?
	if (ag.is_poly_id_null()) {
		// Non-height sensitive routine.
		u32 new_poly_id = mesh.find_poly_within(ag.get_pos().xz());

		if (new_poly_id == UINT32_MAX) {
			NP_WARN_PRINT_ONCE("not within poly");
			return;
		} else {
			log(String("c++ agent within poly ") + itos(ag.get_poly_id()));
			_agent_enter_poly(r_agent, new_poly_id, true);
		}
	}

	if (agent_remaining_magnitude < 0.0001f) {
		//NP_LLOG("no significant velocity");
		return;
	}

	//IPoint2 dir = ag.vel;
	//dir.normalize();
#ifdef NP_DEV_EXCESSIVE_CHECKS
	NP_LLOG(String("iterate pos ") + str(ag.pos) + ", vel " + str(ag.vel) + ", poly " + itos(ag.poly_id));
#endif

	if (ag.is_wall_id_null()) {
		NP_DEV_ASSERT(mesh.poly_contains_point(ag.get_poly_id(), ag.get_2d_pos()));
	}

	// Is the move blocked by a ceiling?
	// Do we need to divert?
	_agent_slide_on_ceiling(r_agent, r_move_info, agent_remaining_velocity);

	Mesh::MoveInfo &minfo = r_move_info;
	minfo.on_floor = ag.is_on_floor();
	minfo.momentum = agent_remaining_magnitude;
	mesh.recursive_move(0, ag.get_2d_pos(), agent_remaining_velocity, ag.get_poly_id(), -2, ag.get_wall_id(), minfo);

	bool early_return = false;

	// If we entered a new mesh instance, don't do the rest of the stuff here.
	if (minfo.new_mesh_instance_id != UINT32_MAX) {
		// Wall ID is now unspecified on the new mesh instance.
		ag.changing_mesh();
		early_return = true;
	}

	if (AgentStatus::is_in_jump_link()) {
		ag.clear_wall_id();
		ag.jump_link_wall_id = AgentStatus::jump_wall_id;
		ag.jump_link_target_pos = AgentStatus::jump_target_wall_pos;
		ag.force_off_floor();

#ifdef NP_DEV_ENABLED
		// Set debug position for client.
		//		const Wall &wall = mesh.get_wall(ag.jump_link_wall_id);
		//		ag.debug_pos[0] = mesh.get_fvert3(wall.vert_a);
		//		ag.debug_pos[1] = mesh.get_fvert3(wall.vert_b);

		//		FPoint2 target_fpos = mesh.fixed_point_to_float_2(ag.jump_link_target_pos);
		//		r_agent.debug_pos[2] = FPoint3::make(target_fpos.x, r_agent.agent_height, target_fpos.y);

#endif
		if (!AgentStatus::changing_mesh()) {
			_iterate_agent_on_jump_link(r_agent, r_move_info, minfo.remaining_velocity);
		}
#ifdef NP_DEV_EXCESSIVE_CHECKS
		NP_LLOG(String("\tagent FINAL pos ") + str(ag.pos) + ", vel " + str(ag.vel) + ", on jump link ");
#endif
		return;
	}

	if (early_return) {
		return;
	}

	// if the move allowed by poly agent limits
	if (_agent_enter_poly(r_agent, minfo.poly_id)) {
		ag.set_2d_pos(minfo.pos_reached);
		ag.set_wall_id(minfo.wall_id);
		
		ag.set_2d_velocity(ag.get_pos().xz() - old_pos);
		//ag.vel -= ag.instant_impulse;
		//		ag.blocking_zone_id = UINT32_MAX;
	} else {
		// debugging, record the blocking narrowing
		//		if (minfo.poly_id != UINT32_MAX) {
		//			ag.blocking_zone_id = mesh.get_poly_extra(minfo.poly_id).zone_id;
		//		}
		ag.set_2d_velocity(IPoint2());
		ag.state = AGENT_STATE_BLOCKED_BY_FULL_ZONE;

		// print_line("narrowing blocked agent at poly " + itos(minfo.poly_id));
	}

#ifdef NP_DEV_EXCESSIVE_CHECKS
	NP_LLOG(String("\tagent FINAL pos ") + str(ag.pos) + ", vel " + str(ag.vel) + ", poly " + itos(ag.poly_id));
#endif

#ifdef NP_DEV_EXCESSIVE_CHECKS
	if (ag.wall_id == UINT32_MAX) {
		NP_DEV_ASSERT(mesh.poly_contains_point(ag.poly_id, ag.pos));
	}
#endif

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
