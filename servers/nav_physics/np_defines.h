#pragma once

#include "core/math/vector3.h"
#include <stdint.h>

typedef uint32_t np_handle;

namespace NavPhysics {

enum AgentState : unsigned int {
	AGENT_STATE_CLEAR,
	AGENT_STATE_COLLIDING,
	AGENT_STATE_PENDING_COLLIDING,
	AGENT_STATE_BLOCKED_BY_NARROWING,
};

struct TraceResult {
	struct MeshTraceResult {
		bool hit = false;
		Vector3 hit_point;
		Vector3 hit_normal;

		uint32_t hit_poly_id = UINT32_MAX;

		// Set if something blocked the first intermediate trace in a dual trace.
		// Unused in normal body trace.
		bool first_trace_hit = false;
	} mesh;

	struct ObstacleTraceResult {
		enum {
			MAX_OBSTACLES = 256,
		};

		uint32_t num_hit = 0;
		np_handle bodies_hit[MAX_OBSTACLES];
		Object *objects_hit[MAX_OBSTACLES];
	} obstacles;

	void blank() {
		mesh.hit = false;
		obstacles.num_hit = 0;
	}
};

struct BodyInfo {
	uint32_t p_poly_id;

	uint32_t p_narrowing_id;
	uint32_t p_narrowing_available;
	uint32_t p_narrowing_used;

	uint32_t p_blocking_narrowing_id;
	uint32_t p_blocking_narrowing_available;
	uint32_t p_blocking_narrowing_used;

	BodyInfo() { blank(); }
	void blank() {
		p_poly_id = UINT32_MAX;

		p_narrowing_id = UINT32_MAX;
		p_narrowing_available = 0;
		p_narrowing_used = 0;

		p_blocking_narrowing_id = UINT32_MAX;
		p_blocking_narrowing_available = 0;
		p_blocking_narrowing_used = 0;
	}
	uint32_t checksum() const {
		return p_poly_id + p_narrowing_id + p_narrowing_available - p_narrowing_used + p_blocking_narrowing_id + p_blocking_narrowing_available - p_narrowing_used;
	}
};

} //namespace NavPhysics
