#pragma once

#include "scene/3d/spatial.h"

class NPAgentClass {
public:
};

class NPAgent : public Spatial {
	GDCLASS(NPAgent, Spatial);

protected:
	static void _bind_methods();

public:
	void nav_teleport(const Vector3 &p_pos);
	void nav_apply_force(const Vector3 &p_force);
};
