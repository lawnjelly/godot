#pragma once

#include "navphysics_pointf.h"
#include "navphysics_pointi.h"
#include "navphysics_structs.h"
#include "navphysics_transform.h"
#include "navphysics_vector.h"

namespace NavPhysics {
class MeshInstance {
	void _iterate_agent(Agent &r_agent, Mesh::MoveInfo &r_move_info);
	bool _agent_enter_poly(Agent &r_agent, u32 p_new_poly_id, bool p_force_allow = false);

	Transform _transform;
	Transform _transform_inverse;
	bool _transform_identity = true;

	u32 _mesh_id = UINT32_MAX;
	u32 _map_slot = UINT32_MAX;

	const Mesh &get_mesh() const;

	Vector<NarrowingInstance> _narrowing_instances;

	const NarrowingInstance &get_narrowing_instance(u32 p_idx) const { return _narrowing_instances[p_idx]; }

	void _set_transform(const Transform &p_xform, const Transform &p_xform_inv, bool p_is_identity);

	void llog(String p_sz);

public:
	// less is better fit
	freal find_agent_fit(Agent &r_agent) const;
	void iterate_agent(Agent &r_agent, Mesh::MoveInfo &r_move_info);
	void teleport_agent(Agent &r_agent);
	void agent_get_info(const Agent &p_agent, BodyInfo &r_body_info) const;

	void body_trace(const Agent &p_agent, NavPhysics::TraceResult &r_result) const;
	void body_dual_trace(const Agent &p_agent, FPoint3 p_intermediate_destination, NavPhysics::TraceResult &r_result) const;

	FPoint3 choose_random_location() const;

	void set_transform(const Transform &p_xform);

	const Transform &get_transform() const { return _transform; }
	const Transform &get_transform_inverse() const { return _transform_inverse; }
	bool is_transform_identity() const { return _transform_identity; }

	void link_mesh(u32 p_mesh_id);

	void link_map(u32 p_map_slot) { _map_slot = p_map_slot; }
	u32 get_map_slot() const { return _map_slot; }

	void init() {
		_transform.init();
		_transform_inverse.init();
	}
};

} //namespace NavPhysics
