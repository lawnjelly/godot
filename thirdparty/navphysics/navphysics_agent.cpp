// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#include "navphysics_agent.h"
#include "navphysics_mesh_instance.h"

//#define NP_AGENT_LOG(a) log(String((i64)this) + ":\t" + a)

//#define NP_AGENT_LOG(a) log(String("\t: ") + a)
#define NP_AGENT_LOG(a)

namespace NavPhysics {

u32 AgentStatus::jump_wall_id = UINT32_MAX;
IPoint2 AgentStatus::jump_target_wall_pos = IPoint2();
IPoint3 AgentStatus::jump_target_cross_pos = IPoint3();
IPoint3 AgentStatus::jump_target_vel = IPoint3();
//freal AgentStatus::jump_target_jump_vel = 0;
//FPoint3 AgentStatus::jump_target_world_space_vel = FPoint3();

//freal AgentStatus::jump_target_local_height = 0;
bool AgentStatus::jump_cross_only = false;
u32 AgentStatus::suggested_poly_id = UINT32_MAX;
u32 AgentStatus::jump_mesh_instance_id = UINT32_MAX;

void AgentStatus::debug_print(String p_sz) {
	log(p_sz + String(" ... AgentStatus : wall_id ") + jump_wall_id + ", wall_pos " + jump_target_wall_pos + ", cross_pos " + jump_target_cross_pos + ", poly " + suggested_poly_id + ", meshi_id " + jump_mesh_instance_id + ", target_set " + jump_cross_only);
}

void Agent::set_height(freal p_height) {
	if (p_height != pos.y) {
		NP_AGENT_LOG("set_height " + p_height);
		pos.y = p_height;
	}
}

void Agent::set_jump_velocity(i32 p_vel) {
	if (p_vel != vel.y) {
		NP_AGENT_LOG("set_jump_velocity " + (p_vel / 1000));
		vel.y = p_vel;
	}
}

void Agent::set_velocity(const IPoint3 &p_vel) {
	if (p_vel != vel) {
		NP_AGENT_LOG("set_vel " + (p_vel / 1000));
		vel = p_vel;
	}
}

void Agent::set_pos(const IPoint3 &p_pos) {
	if (p_pos != pos) {
		NP_AGENT_LOG("set_pos " + (p_pos / 1000));
		pos = p_pos;
		pos.clamp_to_fp_range();
	}
}

void Agent::set_poly_id(u32 p_poly_id) {
	if (p_poly_id != poly_id) {
		NP_AGENT_LOG("set_poly_id " + p_poly_id);
		poly_id = p_poly_id;
	}
}

void Agent::set_wall_id(u32 p_wall_id) {
	if (p_wall_id != wall_id) {
		NP_AGENT_LOG("set_wall_id " + p_wall_id);
		wall_id = p_wall_id;
	}
}

void Agent::set_mesh_instance_id(uint32_t p_mesh_id) {
	if (p_mesh_id != mesh_instance_id) {
		NP_AGENT_LOG("set_mesh_instance_id " + p_mesh_id);
		mesh_instance_id = p_mesh_id;
	}
}

void Agent::set_friction(f32 p_friction, bool p_air) {
	if (!p_air) {
		friction = p_friction;
		log(String("agent_adjusted_friction is ") + friction);
	} else {
		air_friction = p_friction;
	}
}

void Agent::set_yaw_movement_restriction(freal p_value, bool p_air) {
	yaw_movement_restriction[p_air ? 1 : 0] = CLAMP(p_value, 0.0f, 1.0f);
}

void Agent::apply_friction() {
	// Apply friction.
	// The friction multipliers should be adjusted according to the physics TPS
	// in the client code to give consistent friction at different TPS.
	// e.g. friction = pow(friction, 60 * physics_delta).
	freal f = is_on_floor() ? friction : air_friction;
	//log(String("vel ") + vel);
	set_velocity(get_velocity() * f);
}

void Agent::apply_restricted_velocity(const IPoint2 &p_new_velocity_before_restriction) {
	if (p_new_velocity_before_restriction.is_zero()) {
		set_2d_velocity(IPoint2());
		return;
	}

	f32 restriction = yaw_movement_restriction[is_on_floor() ? 0 : 1];

	IPoint3 old_vel = get_velocity();

	// Simplest case, there is no restriction in progress, pass through.
	if (restriction <= 0) {
		set_2d_velocity(p_new_velocity_before_restriction);
	}

	// Apply the restriction to the CHANGE, not to the existing velocity.
	IPoint2 diff = p_new_velocity_before_restriction - old_vel.xz();

	Math::restrict_velocity_to_angle(diff, yaw, restriction);
	add_2d_velocity(diff);
}

void Agent::apply_impulse_world(FPoint3 p_impulse) {
	fimpulse3 += p_impulse;
}

void Agent::apply_jump_world(FPoint3 p_jump) {
	data.pending_jump_world += p_jump;
}

void Agent::iterate_jump_before(const MeshInstance &p_mesh_instance) {
	// Give a change to do jumps BEFORE the iteration.
	// The floor height should be up to date from the end of the previous iteration.
	if (AgentStatus::is_in_jump_link()) {
		return;
	}

	// Add any pending jumps.
	// Note that the jump should be mostly in the opposite direction to gravity, and not defined by the straight up direction.
	if (!data.pending_jump_world.is_zero()) {
		IPoint3 j = p_mesh_instance.velocity_world_space_to_local_fixed_point(p_mesh_instance.get_mesh(), data.pending_jump_world);
		add_velocity(j);
		//FPoint3 g = p_mesh_instance.get_gravity_local_space();
		//g *= -data.pending_jump_world;
		//add_velocity(IPoint3(g));

		//add_jump_velocity(Math::round32(p_mesh_instance.get_scale_world_space_to_local_fixed_point() * data.pending_jump_world));
		data.pending_jump_world.zero();
		on_floor = false;
	}

	// Initial jump off the floor.
	if (on_floor && (get_jump_velocity() > 0)) {
		//set_height(floor_height + get_jump_velocity());
		on_floor = false;
		return;
	}
}

void Agent::iterate_jump_after(const MeshInstance &p_mesh_instance, const freal *p_ceiling_height) {
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

	if (on_floor) {
		// If we are on the floor, and no upward velocity,
		// tightly match the floor surface.
		set_height(floor_height);
	} else {
		// Move height up or down...
		if (get_jump_velocity() >= 0) {
			set_height(get_height() + get_jump_velocity());
		} else {
			set_height(get_height() + get_jump_velocity() * 2);
		}

		// If we hit the floor?
		if (get_height() <= floor_height) {
			set_height(floor_height);
			set_jump_velocity(0);
			on_floor = true;
			grounded = true;
		} else {
			// Apply gravity for the next tick.
			FPoint3 g = p_mesh_instance.get_gravity_local_space();

			// Agent gravity in world space.
			// MeshInstance gravity determines direction and scales from world space to local integer space.
			g *= gravity;

			add_velocity(IPoint3(g));

#if 0
			if (world_space_gravity) {
				FPoint3 g(0, -gravity, 0);
				add_velocity(IPoint3(g));

				const Mesh &mesh = p_mesh_instance.get_mesh();
				FPoint3 g_local = p_mesh_instance.world_space_velocity_to_local3(g);
				IPoint2 v = mesh.float_to_fixed_point_vel(g_local.xz());
				//IPoint2 v = p_mesh_instance.world_space_velocity_to_local(mesh, g);
				//g = p_mesh_instance.get_transform().basis.xform(g);

				// Hack for upside down / 90 degree meshes.
				// We want to force agents to always falls back to the mesh at the bottom, otherwise
				// the behaviour is nonsense if it can't leave the mesh.
				g_local.y = MIN(g_local.y, -0.1f);
				add_jump_velocity(g_local.y);
				add_velocity(v);
			} else {
				add_jump_velocity(-gravity);
			}
#endif

			// Have we hit the ceiling?
			if (p_ceiling_height && (get_height() > *p_ceiling_height)) {
				// Ceiling should always be above the floor, but just in case...
				// Could be an assert?
				set_height(MAX(*p_ceiling_height, floor_height));

				// Eliminate any upward jump velocity so bounce off ceiling.
				set_jump_velocity(MIN(get_jump_velocity(), 0));
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
