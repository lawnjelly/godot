#pragma once

#include "navphysics_pointf.h"

typedef u32 np_handle;

namespace NavPhysics {
enum AgentState : u32 {
	AGENT_STATE_CLEAR,
	AGENT_STATE_COLLIDING,
	AGENT_STATE_PENDING_COLLIDING,
	AGENT_STATE_BLOCKED_BY_NARROWING,
};

struct TraceResult {
	struct MeshTraceResult {
		bool hit = false;
		FPoint3 hit_point;
		FPoint3 hit_normal;

		u32 hit_poly_id = UINT32_MAX;

		// Set if something blocked the first intermediate trace in a dual trace.
		// Unused in normal body trace.
		bool first_trace_hit = false;
	} mesh;

	struct ObstacleTraceResult {
		enum {
			MAX_OBSTACLES = 256,
		};

		u32 num_hit = 0;
		np_handle bodies_hit[MAX_OBSTACLES];
		// Object *objects_hit[MAX_OBSTACLES];
	} obstacles;

	void blank() {
		mesh.hit = false;
		obstacles.num_hit = 0;
	}
};

struct BodyInfo {
	u32 poly_id;

	u32 narrowing_id;
	u32 narrowing_available;
	u32 narrowing_used;

	u32 blocking_narrowing_id;
	u32 blocking_narrowing_available;
	u32 blocking_narrowing_used;

	BodyInfo() { blank(); }
	void blank() {
		poly_id = UINT32_MAX;

		narrowing_id = UINT32_MAX;
		narrowing_available = 0;
		narrowing_used = 0;

		blocking_narrowing_id = UINT32_MAX;
		blocking_narrowing_available = 0;
		blocking_narrowing_used = 0;
	}
	u32 checksum() const {
		return poly_id + narrowing_id + narrowing_available - narrowing_used + blocking_narrowing_id + blocking_narrowing_available - narrowing_used;
	}
};

} // namespace NavPhysics
