#include "np_ticker.h"

#include "thirdparty/navphysics/navphysics_defines.h"
#include "thirdparty/navphysics/navphysics_map.h"

uint32_t NPTicker::_tick = UINT32_MAX;
int32_t NPTicker::_ref_count = 0;
bool NPTicker::_initialized = false;

void NPTicker::tick(uint32_t p_physics_tick, real_t p_physics_delta) {
	if (_ref_count > 0) {
		if (p_physics_tick != _tick) {
			_tick = p_physics_tick;
			if (!_initialized) {
				// One off initialization, set the callback.
				_initialized = true;
				NavPhysics::World::set_agent_callback(&_agent_callback);
			}

			NPWORLD.tick_update(_tick, p_physics_delta);
		}
	}
}

void NPTicker::_agent_callback(uint64_t p_user_data, const NavPhysics::FPoint3 &p_position, const NavPhysics::FPoint3 &p_velocity) {
	NPAgent *agent = (NPAgent *)p_user_data;
	ERR_FAIL_NULL(agent);
	Transform tr = agent->get_transform();
	//Transform tr;
	tr.origin = *(Vector3 *)&p_position;

	agent->data.vel = *(Vector3 *)&p_velocity;

	//print_line("vel " + String(Variant(agent->data.vel)));

	// Calculate yaw
	agent->update_yaw();

	//agent->data.vel.zero();

	//tr.basis = Basis(Vector3(0, Math::randf(), 0));
	tr.basis = Basis(Vector3(0, (Math_PI / 2) - agent->data.yaw, 0));
	agent->set_transform(tr);
}
