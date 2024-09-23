#pragma once

#include "navphysics_pointf.h"
#include "navphysics_pointi.h"
#include "navphysics_structs.h"
#include "navphysics_transform.h"
#include "navphysics_vector.h"

namespace NavPhysics {
class MeshInstance {
	void _iterate_agent(Agent &r_agent);
	bool _agent_enter_poly(Agent &r_agent, u32 p_new_poly_id, bool p_force_allow = false);

	Transform _transform;
	Transform _transform_inverse;
	bool _transform_identity = true;

	u32 _mesh_id = UINT32_MAX;

	const Mesh &get_mesh() const;

	Vector<NarrowingInstance> _narrowing_instances;

	const NarrowingInstance &get_narrowing_instance(u32 p_idx) const { return _narrowing_instances[p_idx]; }

public:
	// less is better fit
	freal find_agent_fit(Agent &r_agent) const;
	void iterate_agent(Agent &r_agent);
	void teleport_agent(Agent &r_agent);
	void agent_get_info(const Agent &p_agent, BodyInfo &r_body_info) const;

	void body_trace(const Agent &p_agent, NavPhysics::TraceResult &r_result) const;
	void body_dual_trace(const Agent &p_agent, FPoint3 p_intermediate_destination, NavPhysics::TraceResult &r_result) const;

	FPoint3 choose_random_location() const;

	void set_transform(const Transform &p_xform, const Transform &p_xform_inv, bool p_is_identity);
	const Transform &get_transform() const { return _transform; }
	const Transform &get_transform_inverse() const { return _transform_inverse; }
	bool is_transform_identity() const { return _transform_identity; }

	void link_mesh(u32 p_mesh_id);
};

} //namespace NavPhysics
