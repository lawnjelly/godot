#include "np_agent.h"

void NPAgent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("nav_teleport", "position"), &NPAgent::nav_teleport);
	ClassDB::bind_method(D_METHOD("nav_apply_force"), &NPAgent::nav_apply_force);
}

void NPAgent::nav_teleport(const Vector3 &p_pos) {
}

void NPAgent::nav_apply_force(const Vector3 &p_force) {
}
