// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#pragma once

#include "navphysics_typedefs.h"

namespace NavPhysics {

struct IPoint2;

class Math {
	static freal _fabs(f32 p_v);
	static freal _fabs(f64 p_v);

	static f32 _sin(f32 p_v);
	static f64 _sin(f64 p_v);
	static f32 _cos(f32 p_v);
	static f64 _cos(f64 p_v);

public:
	static constexpr f32 NP_CMP_EPSILON = 0.00001;

	static constexpr f64 NP_TAU = 6.2831853071795864769252867666;
	static constexpr f64 NP_PI = 3.1415926535897932384626433833;

	static f32 sqrt32(f32 p_v);
	static f64 sqrt64(f64 p_v);
	static freal sqrt_real(freal p_v);
	static freal atan2_real(freal p_a, freal p_b);

	static freal sin_real(freal p_v);
	static freal cos_real(freal p_v);

	static freal fabs_real(freal p_v);
	static f32 fabs32(f32 p_v);
	static u32 abs(i32 p_v);
	static u64 abs(i64 p_v);

	static i32 round32(f32 p_v);

	static double pow(double p_v, double p_factor);
	static float pow(float p_v, float p_factor);

	static u32 rand();
	static f32 randf();
	static f32 rand_range(f32 from, f32 to);

	static bool is_equal_approx(f32 a, f32 b, f32 tolerance = NP_CMP_EPSILON);
	static bool is_zero_approx(f32 s, f32 tolerance = NP_CMP_EPSILON);

	static f32 fmod(f32 p_x, f32 p_y);
	static f32 lerp_angle(f32 p_from, f32 p_to, f32 p_weight);
	static f32 shift_angle(f32 p_from, f32 p_to, f32 p_max_change);
	static f32 normalize_angle(f32 p_angle);

	// Restriction is from 0 to 1.
	static void constrain_velocity_to_angle(IPoint2 &r_velocity, f32 p_angle, f32 p_restriction);

	static f32 get_adjusted_friction(f32 p_friction, u32 p_tps);
};

} // namespace NavPhysics
