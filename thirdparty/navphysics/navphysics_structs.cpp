#include "navphysics_structs.h"

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

//void Agent::seek_yaw(float p_yaw) {
//	yaw = Math::shift_angle(yaw, p_yaw, 0.1f);
//}

} //namespace NavPhysics
