#pragma once

#include "core/local_vector.h"
#include "np_structs.h"

//class NavMesh;

namespace NavPhysics {

class Mesh {
	friend class Loader;
	LocalVector<uint32_t> _inds;
	LocalVector<uint32_t> _inds_next;

	LocalVector<vec2> _verts;
	LocalVector<Vector3> _fverts3;
	LocalVector<Vector2> _fverts;

	LocalVector<uint32_t> _links;
	LocalVector<Wall> _walls;
	LocalVector<Poly> _polys;
	LocalVector<PolyExtra> _polys_extra;
	LocalVector<Narrowing> _narrowings;

	Vector2 _float_to_fp_scale;
	Vector2 _float_to_fp_offset;

	Vector2 _fp_to_float_scale;
	Vector2 _fp_to_float_offset;

	uint32_t _mesh_id = UINT32_MAX;
	uint32_t _map_id = UINT32_MAX;
	uint32_t _region_id = UINT32_MAX;

	uint32_t _mesh_id_map_slot = UINT32_MAX;
	uint32_t _mesh_id_region_slot = UINT32_MAX;

	Transform _transform;
	Transform _transform_inverse;
	bool _transform_identity = true;

	void clear() {
		_inds.clear();
		_inds_next.clear();

		_verts.clear();
		_fverts3.clear();
		_fverts.clear();

		_links.clear();
		_walls.clear();
		_polys.clear();
		_polys_extra.clear();
		_narrowings.clear();
	}

	// Pointer from the main navigation, used for unloading.
	//const NavMesh *_source_nav_mesh = nullptr;

	void _iterate_agent(Agent &r_agent);
	//bool _agent_enter_poly(uint32_t p_old_poly_id, uint32_t p_new_poly_id, bool p_force_allow = false);
	bool _agent_enter_poly(Agent &r_agent, uint32_t p_new_poly_id, bool p_force_allow = false);

public:
	// less is better fit
	real_t find_agent_fit(Agent &r_agent) const;

	uint32_t get_mesh_id() const { return _mesh_id; }

	void set_map_id(uint32_t p_id, uint32_t p_slot_id) {
		_map_id = p_id;
		_mesh_id_map_slot = p_slot_id;
	}
	uint32_t get_map_id(uint32_t &r_slot_id) const {
		r_slot_id = _mesh_id_map_slot;
		return _map_id;
	}

	void set_region_id(uint32_t p_id, uint32_t p_slot_id) {
		_region_id = p_id;
		_mesh_id_region_slot = p_slot_id;
	}
	uint32_t get_region_id(uint32_t &r_slot_id) const {
		r_slot_id = _mesh_id_region_slot;
		return _region_id;
	}

	void iterate_agent(Agent &r_agent);
	void teleport_agent(Agent &r_agent);
	void set_transform(const Transform &p_xform, const Transform &p_xform_inv, bool p_is_identity);
	const Transform &get_transform() const { return _transform; }
	const Transform &get_transform_inverse() const { return _transform_inverse; }
	bool is_transform_identity() const { return _transform_identity; }

	void agent_get_info(const Agent &p_agent, BodyInfo &r_body_info) const;

	void body_trace(const Agent &p_agent, NavPhysics::TraceResult &r_result) const;
	void body_dual_trace(const Agent &p_agent, Vector3 p_intermediate_destination, NavPhysics::TraceResult &r_result) const;

	Vector3 choose_random_location() const;

	PoolVector<Face3> mesh_get_faces() const;

protected:
	// accessors
	uint32_t get_ind(uint32_t p_idx) const { return _inds[p_idx]; }
	uint32_t get_ind_next(uint32_t p_idx) const { return _inds_next[p_idx]; }
	uint32_t get_num_inds() const { return _inds.size(); }

	const vec2 &get_vert(uint32_t p_idx) const { return _verts[p_idx]; }
	const Vector2 &get_fvert(uint32_t p_idx) const { return _fverts[p_idx]; }
	const Vector3 &get_fvert3(uint32_t p_idx) const { return _fverts3[p_idx]; }
	uint32_t get_num_verts() const { return _verts.size(); }

	uint32_t get_link(uint32_t p_idx) const { return _links[p_idx]; }
	uint32_t get_num_links() const { return _links.size(); }
	bool is_hard_wall(uint32_t p_idx) const { return get_link(p_idx) == UINT32_MAX; }

	const Wall &get_wall(uint32_t p_idx) const { return _walls[p_idx]; }
	uint32_t get_num_walls() const { return _walls.size(); }

	const Poly &get_poly(uint32_t p_idx) const { return _polys[p_idx]; }
	Poly &get_poly(uint32_t p_idx) { return _polys[p_idx]; }
	const PolyExtra &get_poly_extra(uint32_t p_idx) const { return _polys_extra[p_idx]; }
	uint32_t get_num_polys() const { return _polys.size(); }

	void debug_poly(uint32_t p_poly_id) const;

	const Narrowing &get_narrowing(uint32_t p_idx) const { return _narrowings[p_idx]; }

	vec2 vel_to_fp_vel(const Vector2 &p_vel) const {
		return vec2::make(p_vel * _float_to_fp_scale);
	}

	Vector2 fp_vel_to_vel(const vec2 &p_vel) const {
		return Vector2(p_vel.x, p_vel.y) * _fp_to_float_scale;
	}

	vec2 float_to_vec2(const Vector2 p_pt) const {
		vec2 res;
		res.x = (p_pt.x - _float_to_fp_offset.x) * _float_to_fp_scale.x;
		res.y = (p_pt.y - _float_to_fp_offset.y) * _float_to_fp_scale.y;
		return res;
	}

	Vector2 vec2_to_float(const vec2 p_pt) const {
		Vector2 res;
		res.x = (p_pt.x * _fp_to_float_scale.x) + _fp_to_float_offset.x;
		res.y = (p_pt.y * _fp_to_float_scale.y) + _fp_to_float_offset.y;
		return res;
	}

private:
	/////////////////////////////////
	bool poly_contains_point(uint32_t p_poly_id, const vec2 &p_pt) const;
	bool wall_in_front_cross(uint32_t p_wall_id, const vec2 &p_pt) const;
	int64_t wall_cross(uint32_t p_wall_id, const vec2 &p_pt) const;
	real_t find_height_on_poly(uint32_t p_poly_id, const vec2 &p_pt) const;

	enum TraceResult {
		TR_CLEAR,
		TR_SLIDE,
		TR_LIMIT,
	};
	struct TraceInfo {
		uint32_t poly_id = UINT32_MAX;
		uint32_t slide_wall = UINT32_MAX;
		vec2 hit_point{ 0, 0 };
	};
	enum MoveResult {
		MR_OK,
		MR_LIMIT,
	};
	struct MoveInfo {
		uint32_t poly_id = UINT32_MAX;
		uint32_t wall_id = UINT32_MAX;
		vec2 pos_reached{ 0, 0 };
	};

	MoveResult recursive_move(int32_t p_depth, vec2 p_from, vec2 p_vel, uint32_t p_poly_id, uint32_t p_poly_from_id, uint32_t p_hug_wall_id, MoveInfo &r_info) const;
	TraceResult recursive_trace(int32_t p_depth, vec2 p_from, vec2 p_to, uint32_t p_poly_id, TraceInfo &r_info) const;

	// utility funcs
	uint32_t find_poly_within(const vec2 &p_pt) const;
	bool wall_find_intersect(uint32_t p_wall_id, const vec2 &p_from, const vec2 &p_to, vec2 &r_hit) const;
	bool find_lines_intersect_integer(const vec2 &p_from_a, const vec2 &p_to_a, const vec2 &p_from_b, const vec2 &p_to_b, vec2 &r_hit) const;

	// helpers
	void get_wall_verts(uint32_t p_wall_id, vec2 &r_a, vec2 &r_b) const {
		uint32_t ind_a = get_ind(p_wall_id);
		uint32_t ind_b = get_ind_next(p_wall_id);
		DEV_ASSERT(ind_a != ind_b);
		r_a = get_vert(ind_a);
		r_b = get_vert(ind_b);
	}

	void _log(const String &p_string, int p_depth = 0) const;

	// Check signs of two signed numbers
	bool same_signs(int32_t p_a, int32_t p_b) const {
		// check this for bugs, there was an error in the precedence in the gdscript
		return (p_a ^ p_b) >= 0;
	}
};

} //namespace NavPhysics
