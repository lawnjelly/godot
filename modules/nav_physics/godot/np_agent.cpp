#include "np_agent.h"
#include "../source/navphysics_map.h"

void NPAgent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("nav_teleport", "position"), &NPAgent::nav_teleport);
	ClassDB::bind_method(D_METHOD("nav_apply_force"), &NPAgent::nav_apply_force);
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

	print_line(String(Variant(tr.origin)));
	set_transform(tr);

	data.vel *= 0.95f;
}

void NPAgent::nav_teleport(const Vector3 &p_pos) {
	u32 agent_id;
	NavPhysics::Agent *agent = NPWORLD.safe_get_body(data.h_agent, &agent_id);
	ERR_FAIL_NULL(agent);

	NPWORLD.safe_get_default_map()->body_teleport(*agent, agent_id, *(const NavPhysics::FPoint3 *)&p_pos);
}

void NPAgent::nav_apply_force(const Vector3 &p_force) {
	data.vel += p_force;
}

NPAgent::NPAgent() {
	data.h_agent = NPWORLD.safe_body_create();

	NPWORLD.safe_link_body(data.h_agent, NPWORLD.get_handle_default_map());
}

NPAgent::~NPAgent() {
	if (data.h_agent) {
		NPWORLD.safe_unlink_body(data.h_agent, NavPhysics::g_world.get_handle_default_map());

		NPWORLD.safe_body_free(data.h_agent);
		data.h_agent = 0;
	}
}
