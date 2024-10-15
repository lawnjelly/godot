#pragma once

#include "navphysics_pointf.h"
#include "navphysics_pointi.h"
#include "navphysics_rect.h"
#include "navphysics_structs.h"
#include "navphysics_transform.h"
#include "navphysics_vector.h"

namespace NavPhysics {

class Mesh {
	friend class Loader;
	friend class MeshInstance;
	friend class MeshFuncs;

	Vector<u32> _inds;
	Vector<u32> _inds_next;

	Vector<IPoint2> _verts;
	Vector<FPoint3> _fverts3;

	Vector<u32> _links;
	Vector<Wall> _walls;
	Vector<Poly> _polys;
	Vector<PolyExtra> _polys_extra;
	Vector<Narrowing> _narrowings;

	struct Data {
		Vector<u32> wall_connections;
	} data;

	FPoint2 _f32_to_fp_scale;
	FPoint2 _f32_to_fp_offset;

	FPoint2 _fp_to_f32_scale;
	FPoint2 _fp_to_f32_offset;

	AABB _aabb;

	u32 _mesh_id = UINT32_MAX;
	u32 _map_id = UINT32_MAX;
	//u32 _region_id = UINT32_MAX;

	u32 _mesh_id_map_slot = UINT32_MAX;
	//u32 _mesh_id_region_slot = UINT32_MAX;

	//	Transform _transform;
	//	Transform _transform_inverse;
	//	bool _transform_identity = true;

	// Pointer from the main navigation, used for unloading.
	//const NavMesh *_source_nav_mesh = nullptr;

	//bool _agent_enter_poly(u32 p_old_poly_id, u32 p_new_poly_id, bool p_force_allow = false);

public:
	void clear() {
		_inds.clear();
		_inds_next.clear();

		_verts.clear();
		_fverts3.clear();

		_links.clear();
		_walls.clear();
		_polys.clear();
		_polys_extra.clear();
		_narrowings.clear();

		data.wall_connections.clear();
	}

	u32 get_mesh_id() const { return _mesh_id; }

	void set_map_id(u32 p_id, u32 p_slot_id) {
		_map_id = p_id;
		_mesh_id_map_slot = p_slot_id;
	}
	u32 get_map_id(u32 &r_slot_id) const {
		r_slot_id = _mesh_id_map_slot;
		return _map_id;
	}

	//	void set_region_id(u32 p_id, u32 p_slot_id) {
	//		_region_id = p_id;
	//		_mesh_id_region_slot = p_slot_id;
	//	}
	//	u32 get_region_id(u32 &r_slot_id) const {
	//		r_slot_id = _mesh_id_region_slot;
	//		return _region_id;
	//	}

	//	void set_transform(const Transform &p_xform, const Transform &p_xform_inv, bool p_is_identity);
	//	const Transform &get_transform() const { return _transform; }
	//	const Transform &get_transform_inverse() const { return _transform_inverse; }
	//	bool is_transform_identity() const { return _transform_identity; }

	//PoolVector<Face3> mesh_get_faces() const;

protected:
	// accessors
	u32 get_ind(u32 p_idx) const { return _inds[p_idx]; }
	u32 get_ind_next(u32 p_idx) const { return _inds_next[p_idx]; }
	u32 get_num_inds() const { return _inds.size(); }

	const IPoint2 &get_vert(u32 p_idx) const { return _verts[p_idx]; }
	FPoint2 get_fvert(u32 p_idx) const { return FPoint2::make(_fverts3[p_idx].x, _fverts3[p_idx].z); }
	const FPoint3 &get_fvert3(u32 p_idx) const { return _fverts3[p_idx]; }
	u32 get_num_verts() const { return _verts.size(); }

	u32 get_link(u32 p_idx) const { return _links[p_idx]; }
	u32 get_num_links() const { return _links.size(); }
	bool is_hard_wall(u32 p_idx) const { return get_link(p_idx) == UINT32_MAX; }

	const Wall &get_wall(u32 p_idx) const { return _walls[p_idx]; }
	u32 get_num_walls() const { return _walls.size(); }

	u32 get_num_polys() const { return _polys.size(); }
	const Poly &get_poly(u32 p_idx) const { return _polys[p_idx]; }
	Poly &get_poly(u32 p_idx) { return _polys[p_idx]; }
	const PolyExtra &get_poly_extra(u32 p_idx) const { return _polys_extra[p_idx]; }

	void debug_poly(u32 p_poly_id) const;

	const Narrowing &get_narrowing(u32 p_idx) const { return _narrowings[p_idx]; }
	u32 get_num_narrowings() const { return _narrowings.size(); }

	IPoint2 float_to_fixed_point_vel(const FPoint2 &p_vel) const {
		return IPoint2::make(p_vel * _f32_to_fp_scale);
	}

	FPoint2 fixed_point_vel_to_float(const IPoint2 &p_vel) const {
		return FPoint2::make(p_vel.x, p_vel.y) * _fp_to_f32_scale;
	}

	IPoint2 float_to_fixed_point_2(const FPoint2 p_pt) const {
		IPoint2 res;
		res.x = (p_pt.x + _f32_to_fp_offset.x) * _f32_to_fp_scale.x;
		res.y = (p_pt.y + _f32_to_fp_offset.y) * _f32_to_fp_scale.y;
		return res;
	}

	FPoint2 fixed_point_to_float_2(const IPoint2 p_pt) const {
		FPoint2 res;
		res.x = (p_pt.x * _fp_to_f32_scale.x) + _fp_to_f32_offset.x;
		res.y = (p_pt.y * _fp_to_f32_scale.y) + _fp_to_f32_offset.y;
		return res;
	}

private:
	/////////////////////////////////
	bool poly_contains_point(u32 p_poly_id, const IPoint2 &p_pt) const;
	bool wall_in_front_cross(u32 p_wall_id, const IPoint2 &p_pt) const;
	i64 wall_cross(u32 p_wall_id, const IPoint2 &p_pt) const;
	freal find_height_on_poly(u32 p_poly_id, const IPoint2 &p_pt) const;

	enum TraceResult {
		TR_CLEAR,
		TR_SLIDE,
		TR_LIMIT,
	};
	struct TraceInfo {
		u32 poly_id = UINT32_MAX;
		u32 slide_wall = UINT32_MAX;
		//IPoint2 hit_point{ 0, 0 };
		IPoint2 hit_point;
	};
	enum MoveResult {
		MR_OK,
		MR_LIMIT,
	};
	struct MoveInfo {
		u32 poly_id = UINT32_MAX;
		u32 wall_id = UINT32_MAX;
		//IPoint2 pos_reached{ 0, 0 };
		IPoint2 pos_reached;
	};

	MoveResult recursive_move(i32 p_depth, IPoint2 p_from, IPoint2 p_vel, u32 p_poly_id, u32 p_poly_from_id, u32 p_hug_wall_id, MoveInfo &r_info) const;
	TraceResult recursive_trace(i32 p_depth, IPoint2 p_from, IPoint2 p_to, u32 p_poly_id, TraceInfo &r_info) const;

	// utility funcs
	bool wall_find_intersect(u32 p_wall_id, const IPoint2 &p_from, const IPoint2 &p_to, IPoint2 &r_hit) const;
	bool find_lines_intersect_integer(const IPoint2 &p_from_a, const IPoint2 &p_to_a, const IPoint2 &p_from_b, const IPoint2 &p_to_b, IPoint2 &r_hit) const;

	// helpers
	void get_wall_verts(u32 p_wall_id, IPoint2 &r_a, IPoint2 &r_b) const {
		u32 ind_a = get_ind(p_wall_id);
		u32 ind_b = get_ind_next(p_wall_id);
		NP_DEV_ASSERT(ind_a != ind_b);
		r_a = get_vert(ind_a);
		r_b = get_vert(ind_b);
	}

	void _log(const String &p_string, int p_depth = 0) const;

	// Check signs of two signed numbers
	bool same_signs(i32 p_a, i32 p_b) const {
		// check this for bugs, there was an error in the precedence in the gdscript
		return (p_a ^ p_b) >= 0;
	}

public:
	u32 find_poly_within(const IPoint2 &p_pt) const;
	void init() {
	}
};

} // namespace NavPhysics
