#include "np_ticker.h"

#include "core/error_macros.h"

#include "thirdparty/navphysics/navphysics_defines.h"
#include "thirdparty/navphysics/navphysics_map.h"

uint32_t NPTicker::_tick = UINT32_MAX;
int32_t NPTicker::_ref_count = 0;
bool NPTicker::_initialized = false;

NPTicker::TPSData NPTicker::tps_data = NPTicker::TPSData();

void NPTicker::TPSData::initialize(uint32_t p_tps) {
	double delta = 1.0 / p_tps;
	multiplier_jump = delta;

	struct Fudge {
		uint32_t tps = 0;
		real_t fudge = 1;
		Fudge(uint32_t p_tps, real_t p_fudge) {
			tps = p_tps;
			fudge = p_fudge;
		}
		Fudge() {}
	};
	Vector<Fudge> fudges;
	fudges.push_back(Fudge(1, 2.0f));
	fudges.push_back(Fudge(30, 1.4f));
	fudges.push_back(Fudge(60, 1.0f));
	fudges.push_back(Fudge(120, 0.5f)); // 0.8
	fudges.push_back(Fudge(240, 0.25f));
	fudges.push_back(Fudge(1024, 0.1f));

	// User is on their own after 1024 tps.
	for (uint32_t n = 1; n < fudges.size(); n++) {
		const Fudge &a = fudges[n - 1];
		const Fudge &b = fudges[n];

		if (p_tps <= b.tps) {
			uint32_t diff = b.tps - a.tps;
			freal offset = p_tps - a.tps;
			freal fraction = offset / diff;
			multiplier_impulse = a.fudge + ((b.fudge - a.fudge) * fraction);
			print_line(String("NPAgent setting TPS fudge factor to ") + rtos(multiplier_impulse));
			multiplier_impulse *= delta;
			break;
		}
	}
}

void NPTicker::initialize(uint32_t p_physics_ticks_per_second) {
	_initialized = true;
	// One off initialization, set the callback.
	NavPhysics::World::set_ticks_per_second(p_physics_ticks_per_second);
	NavPhysics::World::set_agent_callback(&_agent_callback);
	tps_data.initialize(p_physics_ticks_per_second);
}

void NPTicker::tick(uint32_t p_physics_tick, real_t p_physics_delta) {
	if (_ref_count > 0) {
		if (p_physics_tick != _tick) {
			_tick = p_physics_tick;
			DEV_CHECK_ONCE(_initialized);
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
	agent->_nav_physics_update_transform(tr);
}
