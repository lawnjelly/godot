#include "navphysics_structs.h"

namespace NavPhysics {

void Agent::set_mesh_instance_id(uint32_t p_mesh_id) {
	print_line(String("setting mesh ID of agent at ") + itos((int64_t)this) + " to " + itos(p_mesh_id));
	mesh_instance_id = p_mesh_id;
}

} //namespace NavPhysics
