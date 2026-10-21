// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#include "navphysics_agent.h"
#include "navphysics_mesh_instance.h"

namespace NavPhysics {

u32 AgentStatus::jump_wall_id = UINT32_MAX;
IPoint2 AgentStatus::jump_target_wall_pos = IPoint2();
IPoint2 AgentStatus::jump_target_cross_pos = IPoint2();
IPoint2 AgentStatus::jump_target_vel = IPoint2();
//FPoint3 AgentStatus::jump_target_world_space_vel = FPoint3();

freal AgentStatus::jump_target_local_height = 0;
bool AgentStatus::jump_cross_only = false;
u32 AgentStatus::suggested_poly_id = UINT32_MAX;
u32 AgentStatus::jump_mesh_instance_id = UINT32_MAX;

void AgentStatus::debug_print(String p_sz) {
	log(p_sz + String(" ... AgentStatus : wall_id ") + jump_wall_id + ", wall_pos " + jump_target_wall_pos + ", cross_pos " + jump_target_cross_pos + ", poly " + suggested_poly_id + ", meshi_id " + jump_mesh_instance_id + ", target_set " + jump_cross_only + ", height " + jump_target_local_height);
}

void Agent::set_mesh_instance_id(uint32_t p_mesh_id) {
	NP_LOG(String("setting mesh ID of agent at ") + (i64)this + " to " + p_mesh_id);
	mesh_instance_id = p_mesh_id;
}

void Agent::set_friction(f32 p_friction) {
	friction = p_friction;
	log(String("agent_adjusted_friction is ") + friction);	
}

void Agent::set_air_friction(f32 p_friction) {
	air_friction = p_friction;
}

void Agent::apply_friction() {
	// Apply friction.
	// The friction multipliers should be adjusted according to the physics TPS
	// in the client code to give consistent friction at different TPS.
	// e.g. friction = pow(friction, 60 * physics_delta).
	freal f = is_on_floor() ? friction : air_friction;
	log(String("vel ") + vel);
	vel *= f;
}

void Agent::apply_impulse(FPoint3 p_impulse) {
	fimpulse3 += p_impulse;
}

void Agent::apply_jump(float p_vel) {
	jump_velocity += p_vel;
}

void Agent::iterate_jump(const MeshInstance &p_mesh_instance, const freal *p_ceiling_height) {
	// Remove on_agent flag, it must be continuously set if we are
	// walking on another agent.
	if (on_agent) {
		if (on_agent_prev_tick) {
			on_agent_prev_tick = false;
		} else {
			on_agent = false;
			// print_line("clearing on_agent");
		}
	}
	
		   // Initial jump off the floor.
	if (on_floor && (jump_velocity > 0)) {
		agent_height = floor_height + jump_velocity;
		on_floor = false;
		return;
	}
	
	if (on_floor) {
		// If we are on the floor, and no upward velocity,
		// tightly match the floor surface.
		agent_height = floor_height;
	} else {
		// Move height up or down...
		agent_height += jump_velocity; // - gravity_half_step;
		
			   // If we hit the floor?
		if (agent_height <= floor_height) {
			agent_height = floor_height;
			jump_velocity = 0;
			on_floor = true;
			grounded = true;
		} else {
			// Apply gravity for the next tick.
			if (world_space_gravity)
			{
				FPoint3 g(0, -gravity, 0);
				
				const Mesh &mesh = p_mesh_instance.get_mesh();
				FPoint3 g_local = p_mesh_instance.world_space_velocity_to_local3(mesh, g);
				IPoint2 v = mesh.float_to_fixed_point_vel(g_local.xz());
				//IPoint2 v = p_mesh_instance.world_space_velocity_to_local(mesh, g);
				//g = p_mesh_instance.get_transform().basis.xform(g);
				
				// Hack for upside down / 90 degree meshes.
				// We want to force agents to always falls back to the mesh at the bottom, otherwise
				// the behaviour is nonsense if it can't leave the mesh.
				g_local.y = MIN(g_local.y, -0.1f);
				jump_velocity += g_local.y;
				vel += v;
			}
			else
			{
				jump_velocity -= gravity;
			}
			
				   // Have we hit the ceiling?
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

#if 0
void Agent::seek_yaw(float p_yaw) {
	yaw = Math::shift_angle(yaw, p_yaw, 0.1f);
}
#endif


} // namespace NavPhysics
