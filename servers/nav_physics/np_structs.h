#pragma once

#include "core/math/plane.h"
#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include "np_defines.h"
#include <stdint.h>

namespace NavPhysics {

struct _NO_DISCARD_CLASS_ vec3 {
	int32_t x;
	int32_t y;
	int32_t z;
};

// Keep as POD unless required otherwise
struct _NO_DISCARD_CLASS_ vec2 {
	int32_t x;
	int32_t y;

	enum {
		FP_RANGE = 65535,
	};

	void zero() {
		x = 0;
		y = 0;
	}
	void set(int32_t p_x, int32_t p_y) {
		x = p_x;
		y = p_y;
	}
	Vector2 to_Vector2() const {
		return Vector2(x, y);
	}
	void from_Vector2(const Vector2 &p_pt) {
		set(p_pt.x, p_pt.y);
	}
	static vec2 make(const Vector2 &p_pt) {
		vec2 res;
		res.from_Vector2(p_pt);
		return res;
	}

#define DEC_OPERATOR_RET(OP)                  \
	vec2 operator OP(const vec2 &p_v) const { \
		vec2 res;                             \
		res.set(x OP p_v.x, y OP p_v.y);      \
		return res;                           \
	}

#define DEC_OPERATOR_IN_PLACE(OP)       \
	void operator OP(const vec2 &p_v) { \
		x OP p_v.x;                     \
		y OP p_v.y;                     \
	}

	vec2 operator-() const {
		vec2 res;
		res.set(-x, -y);
		return res;
	}

	DEC_OPERATOR_RET(+)
	DEC_OPERATOR_RET(-)
	DEC_OPERATOR_RET(*)
	DEC_OPERATOR_RET(/)

	DEC_OPERATOR_IN_PLACE(+=)
	DEC_OPERATOR_IN_PLACE(-=)
	DEC_OPERATOR_IN_PLACE(*=)
	DEC_OPERATOR_IN_PLACE(/=)

#undef DEC_OPERATOR_RET
#undef DEC_OPERATOR_IN_PLACE

	bool operator==(const vec2 &p_v) const { return (x == p_v.x) && (y == p_v.y); }
	bool operator!=(const vec2 &p_v) const { return !(*this == p_v); }

	vec2 operator/(int32_t p_val) const {
		vec2 res;
		res.set(x / p_val, y / p_val);
		return res;
	}
	void operator/=(int32_t p_val) {
		x /= p_val;
		y /= p_val;
	}

	vec2 operator*(real_t p_val) const {
		vec2 res;
		res.set(x * p_val, y * p_val);
		return res;
	}

	vec2 operator*(int32_t p_val) const {
		vec2 res;
		res.set(x * p_val, y * p_val);
		return res;
	}
	void operator*=(int32_t p_val) {
		x *= p_val;
		y *= p_val;
	}
	void operator*=(real_t p_val) {
		x *= p_val;
		y *= p_val;
	}

	int64_t cross(const vec2 &p_v) const {
		return (int64_t)(x * p_v.y) - (y * p_v.x);
	}

	void normalize() {
		normalize_to_scale(FP_RANGE);
	}

	void normalize_to_scale(real_t p_scale) {
		Vector2 v = to_Vector2();
		v = v.normalized() * p_scale;
		x = v.x;
		y = v.y;
	}

	real_t dot(const vec2 &p_o) const {
		Vector2 v0 = to_Vector2();
		Vector2 v1 = p_o.to_Vector2();
		v0.normalize();
		v1.normalize();
		return v0.dot(v1);
	}

	real_t length() const {
		return to_Vector2().length();
	}

	real_t distance_to(const vec2 &p_pt) const {
		vec2 offset;
		offset = p_pt - *this;
		return offset.length();
	}

	String sz() const;
};

// Must be POD, don't include complex types
// like StringName
struct Agent {
private:
	uint32_t mesh_id = UINT32_MAX;

public:
	struct CallbackData {
		Object *receiver = nullptr;
	} callback;

	uint32_t get_mesh_id() const { return mesh_id; }
	void set_mesh_id(uint32_t p_mesh_id);

	np_handle map = 0;
	uint32_t revision = 0;
#ifdef DEV_ENABLED
	uint32_t agent_id = 0;
#endif

	vec2 pos = { 0, 0 };
	vec2 vel = { 0, 0 };
	Vector2 fpos;
	Vector2 fvel;
	real_t height = 0;
	uint32_t poly_id = 0;
	uint32_t wall_id = 0;

	AgentState state : 4;
	bool ignore_narrowings : 1;
	uint32_t blocking_narrowing_id = UINT32_MAX;

	real_t friction = 0;
	real_t radius = 1.0;

	// Final input and output,
	// these may be transformed by the mesh,
	// and may not be in mesh space except during iteration.
	Vector3 fpos3;
	Vector3 fvel3;

	Vector3 fpos3_teleport;

	void blank() {
		// DO NOT CHANGE REVISION,
		// this should be preserved for error checking handles.
#ifdef DEV_ENABLED
		agent_id = 0;
#endif
		map = 0;
		mesh_id = UINT32_MAX;
		pos.zero();
		vel.zero();
		height = 0.0;
		poly_id = UINT32_MAX;
		wall_id = UINT32_MAX;
		state = AGENT_STATE_CLEAR;
		ignore_narrowings = false;
		blocking_narrowing_id = UINT32_MAX;
		friction = 0.6;
		radius = 1.0;
		fpos3 = Vector3();
		fvel3 = Vector3();
		fpos3_teleport = Vector3();
		callback.receiver = nullptr;
	}
};

struct Poly {
	uint32_t first_ind = 0;
	uint32_t num_inds = 0;
	Plane plane;
	vec2 center = { 0, 0 };
	Vector3 center3;

	// bottlenecks
	uint32_t narrowing_id = UINT32_MAX;

	uint16_t narrowing_width = 0;
	uint16_t flood_fill_counter = 0; // doubles as a flood fill counter for finding bottleneck areas
};

struct Wall {
	uint32_t vert_a = UINT32_MAX;
	uint32_t vert_b = UINT32_MAX;
	uint32_t prev_wall = UINT32_MAX;
	uint32_t next_wall = UINT32_MAX;
	vec2 wall_vec = { 0, 0 };
	vec2 normal = { 0, 0 };
	uint32_t poly_id = UINT32_MAX;
	bool has_vert(uint32_t p_vert_id) const {
		return (vert_a == p_vert_id) || (vert_b == p_vert_id);
	}
};

struct Narrowing {
	uint32_t available = 0;
	uint32_t used = 0;

#ifdef DEV_ENABLED
	LocalVector<uint32_t> used_agent_ids;
#endif
};

}; //namespace NavPhysics
