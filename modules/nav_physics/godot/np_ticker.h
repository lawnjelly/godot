#pragma once

#include <stdint.h>

namespace NavPhysics {
struct FPoint3;
}

// This class is responsible for updating ticking NavPhysics on each physics tick,
// and also holds the agent callback where the NavPhysics can move agents
// without requiring them to "pull" the data.
class NPTicker {
	// Allow NPMeshInstance to bump refcount,
	// No need to tick the system if there are no NPMeshInstances.
	friend class NPMeshInstance;

	static uint32_t _tick;
	static int32_t _ref_count;
	static bool _initialized;

	static void _agent_callback(uint64_t p_user_data, const NavPhysics::FPoint3 &p_position, const NavPhysics::FPoint3 &p_velocity);

public:
	static void tick(uint32_t p_physics_tick, real_t p_physics_delta);
	static void initialize(uint32_t p_physics_ticks_per_second);

	// Data that needs to be adjusted according to TPS,
	// to give vaguely consistent results as we change TPS.
	static struct TPSData {
		float multiplier_impulse = 1;
		float multiplier_jump = 1;
		void initialize(uint32_t p_tps);
	} tps_data;
};
