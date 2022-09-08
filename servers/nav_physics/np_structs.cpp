#include "np_structs.h"
#include "core/variant.h"

namespace NavPhysics {

String vec2::sz() const {
	return String(Variant(Vector2(x, y)));
}

void Agent::set_mesh_id(uint32_t p_mesh_id) {
	print_line("setting mesh ID of agent at " + itos((int64_t)this) + " to " + itos(p_mesh_id));
	mesh_id = p_mesh_id;
}

} //namespace NavPhysics
