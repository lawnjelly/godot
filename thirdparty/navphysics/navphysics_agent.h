// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#pragma once

#include "navphysics_defines.h"
#include "navphysics_plane.h"
#include "navphysics_pointf.h"
#include "navphysics_pointi.h"

namespace NavPhysics {

class MeshInstance;

// Single threaded way of passing around data

struct AgentStatus {
	// When in a jump link state between valid polys
	static u32 jump_wall_id;

	static IPoint2 jump_target_wall_pos;
	static IPoint2 jump_target_cross_pos;

	static IPoint2 jump_target_vel;
	static freal jump_target_jump_vel;
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

	// The client code is responsible for scaling these to physics TPS.
	// They are straight velocity multipliers.
	freal friction = 0;
	freal air_friction = 0;

public:
	struct CallbackData {
		// Used by client code to identify the agent,
		// could be an ID or pointer or handle.
		u64 user_data = 0;
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
#if 0
	float yaw = 0;
	float worldspace_yaw = 0;
#endif

	// The agent may be jumping *above* the floor
	freal floor_height = 0;
	
	// Agent height is in local MeshInstance space,
	// NOT in fixed point space.
	freal agent_height = 0;
	
	// Jump velocity is also in local MeshInstance space.
	freal jump_velocity = 0;
	
	//FPoint3 world_space_air_velocity;

	u32 poly_id = UINT32_MAX;
	u32 wall_id = UINT32_MAX;
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
	bool world_space_gravity : 1;

	// We need two flags to maintain the on_agent flag, as we want a single tick delay
	// to clear it, because of order of operations.
	bool on_agent : 1;
	bool on_agent_prev_tick : 1;

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

	// Keep this up to date as we move between meshes,
	// use it for collision response.
	u32 radius_mesh_units = 0;

	// each obstacle has an effect here, this is reported back to the client
	// for avoidance
	FPoint3 avoidance_fvel3;

	// Modifiers
	freal uphill_modifier = 0;
	freal downhill_modifier = 0;
	
	//  This is in local mesh space, or world space if the up_align is not mesh.
	freal gravity = 0;

	// Height is the physical height of the agent,
	// whereas agent_height is the height above the ground.
	freal radius = 1.0;
	freal height = 2.0;

	// Final input and output,
	// these may be transformed by the mesh,
	// and may not be in mesh space except during iteration.
	FPoint3 fpos3;
	FPoint3 fimpulse3;

	// This is a separation bodge for agent collision.
	// It is added to the velocity on the next frame only,
	// then re-zeroed, so that there is no bouncing.
	IPoint2 instant_impulse;

	FPoint3 fpos3_teleport;

	bool is_on_floor() const { return on_floor; }
	bool is_on_agent() const { return on_agent; }
	void set_on_agent() {
		on_agent = true;
		on_agent_prev_tick = true;
	}
	void apply_impulse(FPoint3 p_impulse);
	void apply_jump(float p_vel);
	void apply_friction();

	// E.g. 0.95 is gamey type friction.
	// As if multiplied by the velocity each tick
	// (in reality this is fudged for tick rate independence).
	void set_friction(f32 p_friction);
	void set_air_friction(f32 p_friction);

	void force_off_floor() {
		on_floor = false;
		grounded = false;
	}
	void fall() {
		on_floor = false;
		jump_velocity = 0;
	}
	void iterate_jump(const MeshInstance &p_mesh_instance, const freal *p_ceiling_height = nullptr);
	// void seek_yaw(float p_yaw);

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
		instant_impulse.zero();
		floor_height = 0;
		agent_height = 0;
		jump_velocity = 0;
		poly_id = UINT32_MAX;
		wall_id = UINT32_MAX;
		state = AGENT_STATE_CLEAR;
		is_npc = true;
		on_floor = true;
		world_space_gravity = false;
		on_agent = false;
		on_agent_prev_tick = false;
		grounded = true;
		zone_id = UINT32_MAX;
		blocking_zone_id = UINT32_MAX;

		friction = 0.99;
		air_friction = 0.99;

		uphill_modifier = 0;
		downhill_modifier = 0;

		gravity = 0.1;
		radius = 1;
		height = 2;
		fpos3.zero();
		fimpulse3.zero();
		fpos3_teleport.zero();
		priority = 0;
		avoidance_fvel3.zero();

		guard_internal_jump_links = true;
		guard_external_jump_links = true;
		pathfind_internal_jump_links = true;
		pathfind_external_jump_links = true;
	}
};

} // namespace NavPhysics
