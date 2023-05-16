#pragma once

#include "navphysics_defines.h"
#include "navphysics_plane.h"
#include "navphysics_pointf.h"
#include "navphysics_pointi.h"

namespace NavPhysics {
// Must be POD.
struct Agent {
private:
	u32 mesh_id = UINT32_MAX;

public:
	struct CallbackData {
		// Object *receiver = nullptr;
	} callback;

	u32 get_mesh_id() const { return mesh_id; }
	void set_mesh_id(u32 p_mesh_id);

	np_handle map = 0;
	u32 revision = 0;
#ifdef NP_DEV_ENABLED
	u32 agent_id = 0;
#endif

	IPoint2 pos = { 0, 0 };
	IPoint2 vel = { 0, 0 };
	FPoint2 fpos;
	FPoint2 fvel;
	freal height = 0;
	u32 poly_id = 0;
	u32 wall_id = 0;

	AgentState state : 4;
	bool ignore_narrowings : 1;
	u32 blocking_narrowing_id = UINT32_MAX;

	u16 priority = 0;

	// each obstacle has an effect here, this is reported back to the client
	// for avoidance
	FPoint3 avoidance_fvel3;

	freal friction = 0;
	freal radius = 1.0;

	// Final input and output,
	// these may be transformed by the mesh,
	// and may not be in mesh space except during iteration.
	FPoint3 fpos3;
	FPoint3 fvel3;

	FPoint3 fpos3_teleport;

	void blank() {
		// DO NOT CHANGE REVISION,
		// this should be preserved for error checking handles.
#ifdef NP_DEV_ENABLED
		agent_id = 0;
#endif
		map = 0;
		mesh_id = UINT32_MAX;
		pos.zero();
		vel.zero();
		height = 0;
		poly_id = UINT32_MAX;
		wall_id = UINT32_MAX;
		state = AGENT_STATE_CLEAR;
		ignore_narrowings = false;
		blocking_narrowing_id = UINT32_MAX;
		friction = 0.6;
		radius = 1;
		fpos3.zero();
		fvel3.zero();
		fpos3_teleport.zero();
		// callback.receiver = nullptr;
		priority = 0;
		avoidance_fvel3.zero();
	}
};

struct Poly {
	u32 first_ind = 0;
	u32 num_inds = 0;
	Plane plane;
	IPoint2 center = { 0, 0 };
	FPoint3 center3;

	// bottlenecks
	u32 narrowing_id = UINT32_MAX;

	uint16_t narrowing_width = 0;
	uint16_t flood_fill_counter = 0; // f64s as a flood fill counter for finding bottleneck areas
};

struct PolyExtra {
	// extra data not needed for fast lookup
	f32 area = 0.0f;
};

struct Wall {
	u32 vert_a = UINT32_MAX;
	u32 vert_b = UINT32_MAX;
	u32 prev_wall = UINT32_MAX;
	u32 next_wall = UINT32_MAX;
	IPoint2 wall_vec = { 0, 0 };
	IPoint2 normal = { 0, 0 };
	u32 poly_id = UINT32_MAX;
	bool has_vert(u32 p_vert_id) const {
		return (vert_a == p_vert_id) || (vert_b == p_vert_id);
	}
};

struct Narrowing {
	u32 available = 0;
	u32 used = 0;

#ifdef NP_DEV_ENABLED
	Vector<u32> used_agent_ids;
#endif
};

} // namespace NavPhysics
