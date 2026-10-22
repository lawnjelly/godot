// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#pragma once

#include "navphysics_defines.h"
#include "navphysics_plane.h"
#include "navphysics_pointf.h"
#include "navphysics_pointi.h"

namespace NavPhysics {


class Map;

// If an agent exits a mesh instance
//struct ExitInfo
//{
//	u32 prev_mesh_instance_id = UINT32_MAX;
//	u32 new_mesh_instance_id = UINT32_MAX;
//	u32 map_id = UINT32_MAX;
//};

struct Poly {
	void init() {
		first_ind = 0;
		num_inds = 0;
		plane.zero();
		center.zero();
		center3.zero();
	}

	u32 first_ind = 0;
	u32 num_inds = 0;
	Plane plane;
	IPoint2 center;
	FPoint3 center3;
};

// Extra data not needed for fast lookup.
struct PolyExtra {
private:
	// Bottlenecks.
	u32 area_narrowing_id = UINT32_MAX;
	//	u32 narrowing_width = UINT32_MAX;
public:
	u32 zone_id = UINT32_MAX;
	// TODO : Area is not currently calculated ..
	u64 area_absolute = 0;
	f32 area = 0;

	u32 first_ceiling_link = 0;
	u32 num_ceiling_links = 0;

	u16 island_id = UINT16_MAX;
	//	u16 flood_fill_counter = 0; // doubles as a flood fill counter for finding bottleneck areas

	void set_area_unset() { area_narrowing_id = UINT32_MAX; }
	bool is_area_unset() const { return area_narrowing_id == UINT32_MAX; }
	bool is_area() const { return area_narrowing_id & 0xF0000000; }
	bool is_narrowing() const { return !is_area(); }
	u32 get_area_id() const { return area_narrowing_id & (~0xF0000000); }
	u32 get_narrowing_id() const { return area_narrowing_id; }
	void set_area_id(u32 p_id) { area_narrowing_id = p_id | 0xF0000000; }
	void set_narrowing_id(u32 p_id) { area_narrowing_id = p_id; }
};

struct Wall {
	u32 get_swapped_vert_a() const { return verts_swapped ? vert_b : vert_a; }
	u32 get_swapped_vert_b() const { return verts_swapped ? vert_a : vert_b; }
	u32 vert_a = UINT32_MAX;
	u32 vert_b = UINT32_MAX;
	bool verts_swapped = false;
	u32 prev_wall = UINT32_MAX;
	u32 next_wall = UINT32_MAX;
	IPoint2 wall_vec;
	IPoint2 normal;
	u32 poly_id = UINT32_MAX;
	u32 jump_info_id = UINT32_MAX;
	bool has_vert(u32 p_vert_id) const {
		return (vert_a == p_vert_id) || (vert_b == p_vert_id);
	}
};

struct JumpFinderData {
	FPoint3 pt_from;
	FPoint3 pt_to;
	FPoint3 pt_far;
	IPoint2 pt_from_local;
	IPoint2 pt_to_local;
	IPoint2 pt_far_local;

//#define NP_VERIFY_JUMP_FINDER_VELOCITY
#ifdef NP_VERIFY_JUMP_FINDER_VELOCITY
	FPoint3 pt_verify_curr_plus_vel;
	IPoint2 pt_verify_to_plus_vel_local;
#endif

	FPoint3 vel_world;
	IPoint2 vel_local2;
	freal vel_local_jump = 0;
};

enum ZoneType : u32 {
	ZONE_TYPE_UNDEFINED,
	ZONE_TYPE_NARROWING,
	ZONE_TYPE_AREA,
};

struct Zone {
	ZoneType type = ZONE_TYPE_UNDEFINED;
	u32 area_or_narrowing_id = UINT32_MAX;
	u32 first_link = 0;
	u32 num_links = 0;
	FPoint3 local_pos3;
	u32 max_agents = 0;
};

struct ZoneLink {
#ifdef NP_DEV_ENABLED
	u32 zone_from_id = UINT32_MAX;
#endif
	u32 zone_to_id = UINT32_MAX;
	u32 zone_from_poly_id = UINT32_MAX;
	u32 zone_to_poly_id = UINT32_MAX;

	// Calculated in advance in the loader,
	// based on the centre of the edge line.
	IPoint2 pt_crossing;

	// In mesh space.
	FPoint3 pt_crossing3;
};

struct AreaBase {
	//	u32 max_agents = 0;
	Vector<u32> area_links;
};

struct Narrowing : public AreaBase {
	Vector<u32> linked_areas;
};

struct Area : public AreaBase {
	Vector<u32> linked_narrowings;
};

struct AreaLink {
	u32 area_id = UINT32_MAX;
	u32 area_poly_id = UINT32_MAX;
	u32 narrowing_id = UINT32_MAX;
	u32 narrowing_poly_id = UINT32_MAX;

	// Calculated in advance in the loader,
	// based on the centre of the edge line.
	//IPoint2 pt_crossing;

	// The pt_crossing will either fall
	// on the area poly or the narrowing poly
	// (by random chance).
	//bool crossing_on_area_poly = false;
};

struct ZoneInstance {
	u32 used = 0;

#ifdef NP_DEV_ENABLED
	TVector<u32> used_agent_ids;
#endif
};

} // namespace NavPhysics
