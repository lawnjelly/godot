#pragma once

#include "../source/navphysics_defines.h"
#include "scene/3d/spatial.h"

class NPAgentClass {
public:
};

class NPAgent : public Spatial {
	GDCLASS(NPAgent, Spatial);

	struct Data {
		np_handle h_agent = 0;
		Vector3 vel;
		float friction = 0.4;
	} data;

	void _update_process_mode();
	void _nav_update();
	void _update_params();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void nav_teleport(const Vector3 &p_pos);
	void apply_impulse(const Vector3 &p_impulse);

	void set_friction(float p_friction);
	float get_friction() const { return data.friction; }

	NPAgent();
	~NPAgent();
};
