#include "np_agent.h"
//#include "../source/navphysics_vector.h"
//#include "../source/navphysics_map.h"
#include "../source/navphysics_loader.h"
#include "../source/navphysics_log.h"
#include "../source/navphysics_map.h"
#include "../source/navphysics_pointf.h"
#include "../source/navphysics_pointi.h"
#include "../source/navphysics_vector.h"

//#define GODOT_DEBUG_NP_AGENT

void NPAgent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("nav_teleport", "position"), &NPAgent::nav_teleport);
	ClassDB::bind_method(D_METHOD("apply_impulse"), &NPAgent::apply_impulse);

	ClassDB::bind_method(D_METHOD("set_friction", "friction"), &NPAgent::set_friction);
	ClassDB::bind_method(D_METHOD("get_friction"), &NPAgent::get_friction);

	ADD_PROPERTY(PropertyInfo(Variant::REAL, "friction", PROPERTY_HINT_RANGE, "0,1"), "set_friction", "get_friction");
}

void NPAgent::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			_nav_update();
		} break;
		case NOTIFICATION_ENTER_TREE: {
			_update_process_mode();
		} break;
	}
}

void NPAgent::_update_params() {
	float friction_multiplier = 1.0 - (data.friction * data.friction);
	print_line("Setting friction_multiplier to " + rtos(friction_multiplier));

	NavPhysics::Agent *agent = NPWORLD.safe_get_body(data.h_agent);
	ERR_FAIL_NULL(agent);

	agent->friction = friction_multiplier;
}

void NPAgent::_update_process_mode() {
	if (!Engine::get_singleton()->is_editor_hint()) {
		set_physics_process_internal(true);
	}
}

void NPAgent::_nav_update() {
	u32 agent_id;
	NavPhysics::Agent *agent = NPWORLD.safe_get_body(data.h_agent, &agent_id);
	ERR_FAIL_NULL(agent);

	agent->fvel3 = *(NavPhysics::FPoint3 *)&data.vel;

	NPWORLD.safe_get_default_map()->iterate_agent(agent_id);

	Transform tr = get_transform();
	tr.origin = *(Vector3 *)&agent->fpos3;

#ifdef GODOT_DEBUG_NP_AGENT
	print_line(String(Variant(tr.origin)));
#endif
	set_transform(tr);

	data.vel = *(Vector3 *)&agent->fvel3;

	//data.vel *= data.friction_multiplier;
}

void NPAgent::nav_teleport(const Vector3 &p_pos) {
	u32 agent_id;
	NavPhysics::Agent *agent = NPWORLD.safe_get_body(data.h_agent, &agent_id);
	ERR_FAIL_NULL(agent);

	NPWORLD.safe_get_default_map()->body_teleport(*agent, agent_id, *(const NavPhysics::FPoint3 *)&p_pos);
}

void NPAgent::apply_impulse(const Vector3 &p_impulse) {
	data.vel += p_impulse;
}

NPAgent::NPAgent() {
	data.h_agent = NPWORLD.safe_body_create();

	NPWORLD.safe_link_body(data.h_agent, NPWORLD.get_handle_default_map());

	_update_params();
}

void NPAgent::set_friction(float p_friction) {
	data.friction = p_friction;
	_update_params();
}

NPAgent::~NPAgent() {
	if (data.h_agent) {
		NPWORLD.safe_unlink_body(data.h_agent, NavPhysics::g_world.get_handle_default_map());

		NPWORLD.safe_body_free(data.h_agent);
		data.h_agent = 0;
	}
}
