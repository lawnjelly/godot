#pragma once

#include "navphysics_pointf.h"
#include "navphysics_pointi.h"
#include "navphysics_poly_finder.h"
#include "navphysics_rect.h"
#include "navphysics_structs.h"
#include "navphysics_transform.h"
#include "navphysics_vector.h"

namespace NavPhysics {

class Mesh {
	friend class Loader;
	friend class MeshInstance;
	friend class MeshFuncs;
	friend class MeshExtender;
	friend class JumpFinder;
	friend class BSP;
	friend class PolyFinder;
	friend class Planner;
	friend class ZonePlanner;

	TVector<u32> _inds_next;

	struct SubMesh {
		TVector<IPoint2> verts;
		TVector<FPoint3> fverts3;

		TVector<u32> inds;
		TVector<Poly> polys;
		TVector<IRect2> poly_bounds;

		u32 get_num_polys() const { return polys.size(); }
		u32 get_num_verts() const { return verts.size(); }
		u32 get_num_fverts() const { return fverts3.size(); }

		PolyFinder poly_finder;

		void clear() {
			verts.clear();
			fverts3.clear();
			inds.clear();
			polys.clear();
			poly_bounds.clear();
		}
	};

	SubMesh ceiling;
	SubMesh floor;

	TVector<u32> _links;
	TVector<Wall> _walls;
	Vector<PolyExtra> _polys_extra;

	Vector<Narrowing> _narrowings;
	Vector<Area> _areas;
	Vector<AreaLink> _area_links;

	Vector<Zone> _zones;
	Vector<ZoneLink> _zone_links;

	// Each ceiling link is a ceiling poly ID,
	// a number of these are referenced from PolyExtra
	// and enable quickly finding height from the floor poly.
	Vector<u32> _ceiling_links;

	struct WallPair {
		u32 vert_ids[2] = {};
		//		bool contains_vert(u32 p_vert_id) const {
		//			return (vert_ids[0] == p_vert_id) || (vert_ids[1] == p_vert_id);
		//		}
	};

	struct Data {
		// These are prior to extending.
		TVector<u32> external_wall_ids;

		// For finding external walls after extending.
		TVector<WallPair> external_wall_pairs;

		// These are used at runtime to detect possible jumps
		// between meshes.
		TVector<u32> external_wall_ids_final;

		// These are prior to extending.
		TVector<u32> internal_wall_ids;
		// For finding internal walls after extending.
		TVector<WallPair> internal_wall_pairs;

		TVector<u32> lipped_wall_ids;
	} data;

public:
	struct MeshParams {
		f32 agent_radius = 0.5;
		f32 agent_height = 1.5;
		f32 exit_lip = 0.1;
		f32 exit_max_step_up = 0.5;
		f32 exit_max_drop = 1;
	};

	enum LinkFlags : u32 {
		LINK_FLAG_HARD = (u32)(1 << 31), // Hard wall, no jump links or external.
		LINK_FLAG_EXTERNAL = 1 << 30, // External link (to other meshes)
		LINK_FLAG_INTERNAL = 1 << 29, // Jump links within the mesh.
	};

private:
	MeshParams mesh_params;

	struct ExtensionData {
		u32 orig_num_inds = 0;
		u32 orig_num_verts = 0;
		u32 orig_num_polys = 0;

		u32 agent_radius = 10000;
		u32 agent_lip = 1000;

	} extension_data;

	struct JumpInfo {
		u32 first_wall_jump = 0;
		u32 num_wall_jumps = 0;
		u32 first_poly_jump = 0;
		u32 num_poly_jumps = 0;
	};

	struct JumpData {
		TVector<u32> jump_wall_ids;
		TVector<u32> jump_poly_ids;
		TVector<JumpInfo> jump_info;
		void clear() {
			jump_wall_ids.clear();
			jump_poly_ids.clear();
			jump_info.clear();
		}
	} jump_data;

	freal _f32_to_fp_scale;
	FPoint2 _f32_to_fp_offset;

	freal _fp_to_f32_scale;
	FPoint2 _fp_to_f32_offset;

	AABB _aabb;
	AABB _extended_aabb;

	u32 _mesh_id = UINT32_MAX;
	u32 _map_id = UINT32_MAX;

	u32 _mesh_id_map_slot = UINT32_MAX;

public:
	static freal _inverse_timestep;
	static freal _timestep;
	static u64 _tick;
	static u32 _ticks_per_sec;

	void clear(bool p_preserve_extended_data = false) {
		if (!p_preserve_extended_data) {
			floor.clear();
			ceiling.clear();

			_polys_extra.clear();

			data.external_wall_ids.clear();
			data.external_wall_pairs.clear();

			data.internal_wall_ids.clear();
			data.internal_wall_pairs.clear();

			data.lipped_wall_ids.clear();
		}

		_inds_next.clear();
		_links.clear();
		_walls.clear();
		_narrowings.clear();
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

	const MeshParams &get_mesh_params() const { return mesh_params; }
	const PolyExtra &get_poly_extra(u32 p_idx) const { return _polys_extra[p_idx]; }

protected:
	// accessors
	u32 get_ind(u32 p_idx, bool p_ceiling = false) const { return p_ceiling ? ceiling.inds[p_idx] : floor.inds[p_idx]; }
	u32 get_num_inds(bool p_ceiling = false) const { return p_ceiling ? ceiling.inds.size() : floor.inds.size(); }

	u32 get_ind_next(u32 p_idx) const { return _inds_next[p_idx]; }

	const IPoint2 &get_vert(u32 p_idx, bool p_ceiling = false) const { return p_ceiling ? ceiling.verts[p_idx] : floor.verts[p_idx]; }
	u32 get_num_verts(bool p_ceiling = false) const { return p_ceiling ? ceiling.get_num_verts() : floor.get_num_verts(); }

	FPoint2 get_fvert(u32 p_idx, bool p_ceiling = false) const { return get_fvert3(p_idx, p_ceiling).xz(); }
	const FPoint3 &get_fvert3(u32 p_idx, bool p_ceiling = false) const { return p_ceiling ? ceiling.fverts3[p_idx] : floor.fverts3[p_idx]; }

	u32 get_link(u32 p_idx) const { return _links[p_idx]; }
	u32 get_num_links() const { return _links.size(); }

	bool is_link_hard(u32 p_idx) const { return get_link(p_idx) & LINK_FLAG_HARD; }
	bool is_link_external(u32 p_idx) const { return get_link(p_idx) & LINK_FLAG_EXTERNAL; }
	bool is_link_internal(u32 p_idx) const { return get_link(p_idx) & LINK_FLAG_INTERNAL; }
	bool is_link_regular(u32 p_idx) const { return (get_link(p_idx) & (LINK_FLAG_INTERNAL | LINK_FLAG_EXTERNAL | LINK_FLAG_HARD)) == 0; }

	const Wall &get_wall(u32 p_idx) const { return _walls[p_idx]; }
	u32 get_num_walls() const { return _walls.size(); }

	u32 get_num_polys(bool p_ceiling = false) const { return p_ceiling ? ceiling.get_num_polys() : floor.get_num_polys(); }
	const Poly &get_poly(u32 p_idx, bool p_ceiling = false) const { return p_ceiling ? ceiling.polys[p_idx] : floor.polys[p_idx]; }
	Poly &get_poly(u32 p_idx, bool p_ceiling = false) { return p_ceiling ? ceiling.polys[p_idx] : floor.polys[p_idx]; }
	PolyExtra &get_poly_extra(u32 p_idx) { return _polys_extra[p_idx]; }
	const IRect2 &get_poly_bound(u32 p_idx, bool p_ceiling = false) const { return p_ceiling ? ceiling.poly_bounds[p_idx] : floor.poly_bounds[p_idx]; }

	u32 get_poly_verts(u32 p_poly_id, IPoint2 *r_verts, u32 p_max_verts, bool p_ceiling = false) const {
		const Poly &poly = get_poly(p_poly_id, p_ceiling);
		u32 num_verts = MIN(poly.num_inds, p_max_verts);
		for (u32 n = 0; n < num_verts; n++) {
			u32 ind = get_ind(poly.first_ind + n);
			r_verts[n] = get_vert(ind);
		}
		return num_verts;
	}

	const Zone &get_poly_zone(u32 p_idx) const {
		NP_DEV_ASSERT(get_poly_extra(p_idx).zone_id != UINT32_MAX);
		return _zones[get_poly_extra(p_idx).zone_id];
	}

	const Zone &get_zone(u32 p_idx) const { return _zones[p_idx]; }
	u32 get_num_zones() const { return _zones.size(); }
	const ZoneLink &get_zone_link(u32 p_idx) const { return _zone_links[p_idx]; }

	void debug_poly(u32 p_poly_id) const;

	const Narrowing &get_narrowing(u32 p_idx) const { return _narrowings[p_idx]; }
	u32 get_num_narrowings() const { return _narrowings.size(); }
	u32 get_num_areas() const { return _areas.size(); }

	IPoint2 float_to_fixed_point_vel(const FPoint2 &p_vel) const {
		return IPoint2::make(p_vel * _f32_to_fp_scale);
	}

	FPoint2 fixed_point_vel_to_float(const IPoint2 &p_vel) const {
		return FPoint2::make(p_vel.x, p_vel.y) * _fp_to_f32_scale;
	}

	IPoint2 float_to_fixed_point_2(const FPoint2 p_pt) const {
		IPoint2 res;
		res.x = (p_pt.x + _f32_to_fp_offset.x) * _f32_to_fp_scale;
		res.y = (p_pt.y + _f32_to_fp_offset.y) * _f32_to_fp_scale;
		return res;
	}

	FPoint2 fixed_point_to_float_2(const IPoint2 p_pt) const {
		FPoint2 res;
		res.x = (p_pt.x * _fp_to_f32_scale) + _fp_to_f32_offset.x;
		res.y = (p_pt.y * _fp_to_f32_scale) + _fp_to_f32_offset.y;
		return res;
	}

	FPoint3 fixed_point_to_float_3(const IPoint2 p_pt) const {
		FPoint2 pt2 = fixed_point_to_float_2(p_pt);
		return FPoint3::make(pt2.x, 0, pt2.y);
	}

private:
	/////////////////////////////////
	bool poly_contains_point(u32 p_poly_id, const IPoint2 &p_pt, bool p_ceiling = false) const;
	bool poly_contains_point_debug(u32 p_poly_id, const IPoint2 &p_pt) const;

	bool poly_contains_vert(u32 p_poly_id, u32 p_vert_id) const;
	bool debug_poly_contains_point(u32 p_poly_id, const IPoint2 &p_pt) const;
	bool wall_in_front_cross(u32 p_wall_id, const IPoint2 &p_pt) const;
	i64 wall_cross(u32 p_wall_id, const IPoint2 &p_pt) const;
	freal find_height_on_poly_plane(u32 p_poly_id, const IPoint2 &p_pt, bool p_ceiling = false) const;
	freal get_distance_from_wall_edge(u32 p_wall_id, const IPoint2 &p_pt) const;

	void _check_for_duplicate_verts();

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

public:
	//	struct JumpLinkInfo {
	//		u32 wall_id = UINT32_MAX;
	//		IPoint2 target_pos;
	//	};

	struct MoveInfo {
		u32 poly_id = UINT32_MAX;
		u32 wall_id = UINT32_MAX;
		IPoint2 pos_reached;

		// If we cross to a different mesh instance,
		// there will be a remaining velocity,
		// and we should store and use this on the remaining
		// move instead of using the full velocity (which will
		// result in a double move).
		// IPoint2 remaining_velocity;

		// This isn't true momentum (as in physics)
		// but starts at the initial move velocity length,
		// then is reduced after hitting walls, but not simply
		// by changing polys on a clear path.
		float momentum = 0;

		Agent *agent = nullptr;

		// Exit info
		u32 prev_mesh_instance_id = UINT32_MAX;
		u32 new_mesh_instance_id = UINT32_MAX;

		u32 map_id = UINT32_MAX;
		u32 agent_id = UINT32_MAX;

		bool on_floor = true;

		// Jump links
		//JumpLinkInfo jump;

		//u32 jump_wall_id = UINT32_MAX;
		//IPoint2 jump_target_pos;

		freal remaining_velocity = 0; // For continuing a move after jumping a wall.
	};

	String fverts_to_string() const;
	String verts_to_string() const;

	struct SVGPoint {
		IPoint2 pos;
		u32 label;
		SVGPoint(const IPoint2 p_pos, u32 p_label) {
			pos = p_pos;
			p_label = label;
		}
		SVGPoint() = default;
	};

	bool svg_export(String p_filename, u32 p_start_poly = 0) const;
	bool svg_export_custom(String p_filename, const Vector<u32> &p_walls, const Vector<u32> &p_polys, const Vector<SVGPoint> p_points) const;
	void svg_scale_point(const IPoint2 &p_pt, i32 &r_x, i32 &r_y) const {
		r_x = p_pt.x / 65;
		r_y = p_pt.y / 65;
	}

private:
	String _svg_header(float p_scale = 1) const;
	void modify_velocity_for_poly_slope(const MoveInfo &p_info, IPoint2 &r_vel, u32 p_poly_id) const;
	void modify_velocity_for_poly_wall(const MoveInfo &p_info, float &r_vel, u32 p_poly_id, u32 p_wall_id) const;

	MoveResult recursive_move(i32 p_depth, IPoint2 p_from, IPoint2 p_vel, u32 p_poly_id, u32 p_poly_from_id, u32 p_hug_wall_id, MoveInfo &r_info) const;
	TraceResult recursive_trace(i32 p_depth, IPoint2 p_from, const IPoint2 &p_to, u32 p_poly_id, TraceInfo &r_info) const;

	bool cross_internal_or_external_link(u32 p_wall_id, const IPoint2 &p_from, const IPoint2 &p_vel, MoveInfo &r_info) const;
	freal _calculate_jump_range_from_momentum(freal p_momentum, bool p_apply_minimum) const;
	freal _estimate_height_after_ledge_jump(const Agent &p_agent, freal p_agent_height, freal p_forward_velocity, freal p_dist_to_ledge) const;

	bool jump_within_mesh(u32 p_source_wall_id, const IPoint2 &p_from, const IPoint2 &p_vel, MoveInfo &r_info) const;
	bool _can_jump_to_wall(u32 p_dest_wall_id, const IPoint2 &p_from, IPoint2 &p_to, freal p_agent_height, freal p_agent_jump_height, bool p_on_floor) const;
	bool move_to_new_mesh(const IPoint2 &p_from, const IPoint2 &p_vel, MoveInfo &r_info) const;
	bool bounce_on_wall(u32 p_wall_id, IPoint2 &r_vel, float &r_momentum) const;

	// utility funcs
	bool wall_find_intersect(u32 p_wall_id, const IPoint2 &p_from, const IPoint2 &p_to, IPoint2 &r_hit) const;
	bool find_lines_intersect_integer(const IPoint2 &p_from_a, const IPoint2 &p_to_a, const IPoint2 &p_from_b, const IPoint2 &p_to_b, IPoint2 &r_hit) const;
	bool wall_segments_find_intersect(u32 p_wall_id, const IPoint2 &p_from, const IPoint2 &p_to, IPoint2 &r_hit) const;
	bool find_line_segments_intersect_integer(const IPoint2 &p_from_a, const IPoint2 &p_to_a, const IPoint2 &p_from_b, const IPoint2 &p_to_b, IPoint2 &r_hit) const;

	void _unit_test_find_lines_intersect_integer();

	// helpers
	bool debug_check_agent_integrity(const IPoint2 &p_pos, u32 p_poly_id, u32 p_hug_wall_id) const;

	void get_wall_verts(u32 p_wall_id, IPoint2 &r_a, IPoint2 &r_b) const {
		u32 ind_a = get_ind(p_wall_id);
		u32 ind_b = get_ind_next(p_wall_id);
		NP_DEV_ASSERT(ind_a != ind_b);
		r_a = get_vert(ind_a);
		r_b = get_vert(ind_b);
	}
	void get_swapped_wall_verts(u32 p_wall_id, IPoint2 &r_a, IPoint2 &r_b) const {
		u32 ind_a = get_ind(p_wall_id);
		u32 ind_b = get_ind_next(p_wall_id);
		NP_DEV_ASSERT(ind_a != ind_b);

		if (get_wall(p_wall_id).verts_swapped) {
			SWAP(ind_a, ind_b);
		}
		r_a = get_vert(ind_a);
		r_b = get_vert(ind_b);
	}

	void _log(const String &p_string, int p_depth = 0) const;

	// Check signs of two signed numbers
	bool same_signs(i32 p_a, i32 p_b) const {
		// check this for bugs, there was an error in the precedence in the gdscript
		return (p_a ^ p_b) >= 0;
	}
	bool same_signs64(i64 p_a, i64 p_b) const {
		return (p_a ^ p_b) >= 0;
	}

public:
	FPoint3 local_point_to_point3(const IPoint2 &p_pt, u32 p_poly_id) const {
		NP_DEV_ASSERT(p_poly_id != UINT32_MAX);
		FPoint3 res = fixed_point_to_float_3(p_pt);
		res.y = find_height_on_poly_plane(p_poly_id, p_pt);
		return res;
	}

	u32 find_poly_within(const IPoint2 &p_pt, u32 p_poly_id_hint = UINT32_MAX) const;

	u32 find_best_poly_within(const IPoint2 &p_pt, freal p_height, freal p_max_drop, freal p_max_step_up, freal &r_goodness_of_fit) const;

	u32 find_best_jump_poly_within(const Agent &p_agent, const JumpFinderData &p_jd, freal p_max_drop, freal p_max_step_up, freal &r_goodness_of_fit) const;

	bool find_ceiling_height(u32 p_floor_poly_id, const IPoint2 &p_pt, freal &r_height, u32 &r_ceiling_poly_id_hint) const;

	void init() {
		_unit_test_find_lines_intersect_integer();
	}

	void refresh_local_agent_position_from_fixed_point(Agent &r_agent) const {
		FPoint2 new_fpos = fixed_point_to_float_2(r_agent.pos);
		if (!new_fpos.is_equal_approx(r_agent.fpos)) {
			r_agent.fpos = new_fpos;
		}
		r_agent.fpos3 = FPoint3::make(r_agent.fpos.x, r_agent.agent_height, r_agent.fpos.y);
	}
};

} // namespace NavPhysics
