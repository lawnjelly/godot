// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#pragma once

#include "navphysics_pointf.h"
#include "navphysics_pointi.h"
#include "navphysics_structs.h"
#include "navphysics_transform.h"
#include "navphysics_vector.h"

namespace NavPhysics {

class Mesh;

class MeshInstance {
	void _iterate_agent(Agent &r_agent, Mesh::MoveInfo &r_move_info);
	bool _agent_enter_poly(Agent &r_agent, u32 p_new_poly_id, bool p_force_allow = false);
	void _agent_slide_on_ceiling(Agent &r_agent, Mesh::MoveInfo &r_move_info, IPoint2 &r_agent_velocity);

	Transform _transform;
	Transform _transform_inverse;
	bool _transform_identity = true;
	bool _active = true;

	np_handle _handle = UINT32_MAX;
	u32 _mesh_id = UINT32_MAX;
	u32 _map_slot = UINT32_MAX;

	Vector<ZoneInstance> _zone_instances;

	const ZoneInstance &get_zone_instance(u32 p_idx) const { return _zone_instances[p_idx]; }
	ZoneInstance &get_zone_instance(u32 p_idx) { return _zone_instances[p_idx]; }

	void _set_transform(const Transform &p_xform, const Transform &p_xform_inv, bool p_is_identity);

	void llog(String p_sz) const;

public:
	const Mesh &get_mesh() const;

	bool _iterate_agent_on_jump_link(Agent &r_agent, Mesh::MoveInfo &r_move_info, freal &r_distance);

	void refresh_world_space_agent_position(Agent &r_agent, bool p_remake_pos) const;

	// less is better fit
	freal find_agent_fit(Agent &r_agent, const FPoint3 &p_world_pos, u32 &r_best_poly_id, freal p_max_step_up, freal p_max_drop) const;
	freal find_agent_jump_fit(Agent &r_agent, JumpFinderData p_jump_data, u32 &r_best_poly_id, freal p_max_step_up, freal p_max_drop, freal p_best_fit_so_far) const;

	void iterate_agent(Agent &r_agent, Mesh::MoveInfo &r_move_info);
	void iterate_agent_housekeeping(Agent &r_agent);

	void teleport_agent(Agent &r_agent);
	void agent_get_info(const Agent &p_agent, BodyInfo &r_body_info) const;

	void body_trace(const Agent &p_agent, NavPhysics::TraceResult &r_result) const;
	void body_dual_trace(const Agent &p_agent, FPoint3 p_intermediate_destination, NavPhysics::TraceResult &r_result) const;

	FPoint3 choose_random_location() const;

	void set_transform(const Transform &p_xform);
	void set_active(bool p_active);
	bool is_active() const { return _active; }

	const Transform &get_transform() const { return _transform; }
	const Transform &get_transform_inverse() const { return _transform_inverse; }
	bool is_transform_identity() const { return _transform_identity; }

	////////////////////////////////////////////
	// Reference routines for xforming points and velocities.
	FPoint3 local_pos_to_world_space(const Mesh &p_mesh, const IPoint2 &p_pt_local, freal p_agent_height) const;
	IPoint2 world_space_pos_to_local(const Mesh &p_mesh, const FPoint3 &p_pt_world, freal &r_agent_height) const;

	FPoint3 local_velocity_to_world_space(const Mesh &p_mesh, const IPoint2 &p_vel_local) const;
	IPoint2 world_space_velocity_to_local(const Mesh &p_mesh, const FPoint3 &p_vel_world) const;
	////////////////////////////////////////////

	void link_mesh(u32 p_mesh_id);

	void link_map(u32 p_map_slot) { _map_slot = p_map_slot; }
	u32 get_map_slot() const { return _map_slot; }

	np_handle get_handle() const { return _handle; }

	void init(np_handle p_handle) {
		_transform.init();
		_transform_inverse.init();
		_handle = p_handle;
	}
};

} //namespace NavPhysics
