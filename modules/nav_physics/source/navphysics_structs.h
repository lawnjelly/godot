#pragma once

#include "navphysics_defines.h"
#include "navphysics_plane.h"
#include "navphysics_pointf.h"
#include "navphysics_pointi.h"

namespace NavPhysics {

// Single threaded way of passing around data

struct AgentStatus {
	// When in a jump link state between valid polys
	static u32 jump_wall_id;

	static IPoint2 jump_target_wall_pos;
	static IPoint2 jump_target_cross_pos;

	static IPoint2 jump_target_vel;
	//static FPoint3 jump_target_world_space_vel;

	static freal jump_target_local_height;

	static bool jump_cross_only;

	static bool is_in_jump_link() { return jump_wall_id != UINT32_MAX; }
	static bool changing_mesh() { return jump_mesh_instance_id != UINT32_MAX; }

	// When moving between meshes, we can
	// pass a suggested poly id to make teleporting
	// more efficient, as the calculation will have been done
	// already in find_best_fit().
	static u32 suggested_poly_id;
	static u32 jump_mesh_instance_id;

	static void debug_print(String p_sz);

	static void reset() {
		jump_wall_id = UINT32_MAX;
		jump_cross_only = false;
		suggested_poly_id = UINT32_MAX;
		jump_mesh_instance_id = UINT32_MAX;

#ifdef NP_DEV_ENABLED
		// Just bug detection
		jump_target_local_height = FLT_MAX;
		jump_target_wall_pos = IPoint2(INT32_MAX, INT32_MAX);
		jump_target_cross_pos = IPoint2(INT32_MAX, INT32_MAX);
		jump_target_vel = IPoint2(INT32_MAX, INT32_MAX);
#endif
	}
};

// Must be POD.
struct Agent {
private:
	u32 mesh_instance_id = UINT32_MAX;

public:
	struct CallbackData {
		// Used by client code to identify the agent,
		// could be an ID or pointer or handle.
		u64 user_data = 0;
		// Object *receiver = nullptr;
	} callback;

	u32 get_mesh_instance_id() const { return mesh_instance_id; }
	void set_mesh_instance_id(u32 p_mesh_id);

	np_handle map = 0;
	u32 revision = 0;
	u32 agent_id = 0;

#ifdef NP_DEV_ENABLED
	// Used for communicating back to the client.
	FPoint3 debug_pos[3];
#endif

	IPoint2 pos;
	IPoint2 vel;
	FPoint2 fpos;
	FPoint2 fvel;
	float yaw = 0;
	float worldspace_yaw = 0;

	// The agent may be jumping *above* the floor
	freal floor_height = 0;
	freal agent_height = 0;
	freal jump_velocity = 0;

	u32 poly_id = 0;
	u32 wall_id = 0;
	u32 ceiling_poly_id = UINT32_MAX;
	u32 zone_id = UINT32_MAX;
	u32 blocking_zone_id = UINT32_MAX;

	// When in a jump link state between valid polys
	u32 jump_link_wall_id = UINT32_MAX;
	IPoint2 jump_link_target_pos;

	bool is_in_jump_link() const { return jump_link_wall_id != UINT32_MAX; }

	AgentState state : 4;
	bool is_npc : 1;
	bool on_floor : 1;

	// When crossing jump links, we aren't grounded until
	// we hit the floor after the link, so we can prevent ceiling collisions
	// during the transition.
	bool grounded : 1;

	// Depending on whether the agent is a Player or NPC
	// you may want to disallow falling off edges etc.
	// NPCs can STILL traverse jumps with pathfinding,
	// but won't accidentally traverse links.
	bool guard_internal_jump_links : 1;
	bool pathfind_internal_jump_links : 1;
	bool guard_external_jump_links : 1;
	bool pathfind_external_jump_links : 1;

	u16 priority = 0;

	// each obstacle has an effect here, this is reported back to the client
	// for avoidance
	FPoint3 avoidance_fvel3;

	freal friction = 0;

	// Modifiers
	freal air_friction_modifier = 0;
	freal uphill_modifier = 0;
	freal downhill_modifier = 0;

	freal gravity = 0;
	freal radius = 1.0;

	// Final input and output,
	// these may be transformed by the mesh,
	// and may not be in mesh space except during iteration.
	FPoint3 fpos3;
	FPoint3 fvel3;

	FPoint3 fpos3_teleport;

	bool is_on_floor() const { return on_floor; }
	void apply_jump(float p_vel) {
		jump_velocity += p_vel;
	}
	void force_off_floor() {
		on_floor = false;
		grounded = false;
	}
	void fall() {
		on_floor = false;
		jump_velocity = 0;
	}
	void iterate_jump(const freal *p_ceiling_height = nullptr) {
		if (on_floor && (jump_velocity > 0)) {
			agent_height = floor_height + jump_velocity;
			on_floor = false;
		}

		if (on_floor) {
			agent_height = floor_height;
		} else {
			agent_height += jump_velocity;
			if (agent_height <= floor_height) {
				agent_height = floor_height;
				jump_velocity = 0;
				on_floor = true;
				grounded = true;
			} else {
				jump_velocity -= gravity;

				if (p_ceiling_height && (agent_height > *p_ceiling_height)) {
					// Ceiling should always be above the floor, but just in case...
					// Could be an assert?
					agent_height = MAX(*p_ceiling_height, floor_height);

					// Eliminate any upward jump velocity so bounce off ceiling.
					jump_velocity = MIN(jump_velocity, 0);
				}
			}
		}
	}
	//void seek_yaw(float p_yaw);

	// Call to blank relevant data when jumping between meshes.
	void changing_mesh() {
		poly_id = UINT32_MAX;
		ceiling_poly_id = UINT32_MAX;
		wall_id = UINT32_MAX;
		grounded = false;
	}

	void blank() {
		// DO NOT CHANGE REVISION,
		// this should be preserved for error checking handles.
#ifdef NP_DEV_ENABLED
		agent_id = 0;
		debug_pos[0] = FPoint3();
		debug_pos[1] = FPoint3();
		debug_pos[2] = FPoint3();
#endif
		map = 0;
		mesh_instance_id = UINT32_MAX;
		pos.zero();
		vel.zero();
		floor_height = 0;
		agent_height = 0;
		jump_velocity = 0;
		poly_id = UINT32_MAX;
		wall_id = UINT32_MAX;
		state = AGENT_STATE_CLEAR;
		is_npc = true;
		on_floor = true;
		grounded = true;
		zone_id = UINT32_MAX;
		blocking_zone_id = UINT32_MAX;
		friction = 0.4;

		air_friction_modifier = 0;
		uphill_modifier = 0;
		downhill_modifier = 0;

		gravity = 0.1;
		radius = 1;
		fpos3.zero();
		fvel3.zero();
		fpos3_teleport.zero();
		// callback.receiver = nullptr;
		priority = 0;
		avoidance_fvel3.zero();

		guard_internal_jump_links = true;
		guard_external_jump_links = true;
		pathfind_internal_jump_links = true;
		pathfind_external_jump_links = true;
	}
};

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
	FPoint3 pt_curr;
	FPoint3 pt_far;
	IPoint2 pt_from_local;
	IPoint2 pt_curr_local;
	IPoint2 pt_far_local;

//#define NP_VERIFY_JUMP_FINDER_VELOCITY
#ifdef NP_VERIFY_JUMP_FINDER_VELOCITY
	FPoint3 pt_verify_curr_plus_vel;
	IPoint2 pt_verify_curr_plus_vel_local;
#endif

	FPoint3 vel;
	IPoint2 vel_local;
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
