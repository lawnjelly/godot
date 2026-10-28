// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#include "navphysics_math.h"
#include "navphysics_mesh.h"
#include <math.h>

namespace NavPhysics {
f32 Math::sqrt32(f32 p_v) {
	return ::sqrt(p_v);
}
f64 Math::sqrt64(f64 p_v) {
	return ::sqrt(p_v);
}
freal Math::sqrt_real(freal p_v) {
	return ::sqrt(p_v);
}

i32 Math::round32(f32 p_v) {
	return ::lround(p_v);
}

freal Math::sin_real(freal p_v) {
	return Math::_sin(p_v);
}

f32 Math::_sin(f32 p_v) {
	return ::sinf(p_v);
}

f64 Math::_sin(f64 p_v) {
	return ::sin(p_v);
}

freal Math::cos_real(freal p_v) {
	return Math::_cos(p_v);
}

f32 Math::_cos(f32 p_v) {
	return ::cosf(p_v);
}

f64 Math::_cos(f64 p_v) {
	return ::cos(p_v);
}

f32 Math::fmod(f32 p_x, f32 p_y) {
	return ::fmodf(p_x, p_y);
}

freal Math::atan2_real(freal p_a, freal p_b) {
	return ::atan2(p_a, p_b);
}

u32 Math::abs(i32 p_v) {
	return ::abs(p_v);
}

u64 Math::abs(i64 p_v) {
	return ::abs(p_v);
}

double Math::pow(double p_v, double p_factor) {
	return ::pow(p_v, p_factor);
}

float Math::pow(float p_v, float p_factor) {
	return ::powf(p_v, p_factor);
}

freal Math::fabs_real(freal p_v) {
	return Math::_fabs(p_v);
}

f32 Math::fabs32(f32 p_v) {
	return ::fabsf(p_v);
}

freal Math::_fabs(f32 p_v) {
	return ::fabsf(p_v);
}

freal Math::_fabs(f64 p_v) {
	return ::fabs(p_v);
}

u32 Math::rand() {
	return ::rand();
}

f32 Math::randf() {
	return (f32)rand() / (f32)RAND_MAX;
}

f32 Math::rand_range(f32 from, f32 to) {
	f32 range = to - from;
	f32 x = randf() * range;
	return from + x;
}

bool Math::is_equal_approx(f32 a, f32 b, f32 tolerance) {
	if (a == b) {
		return true;
	}
	return ::fabsf(a - b) <= tolerance;
}

bool Math::is_zero_approx(f32 s, f32 tolerance) {
	return ::fabsf(s) <= tolerance;
}

f32 Math::shift_angle(f32 p_from, f32 p_to, f32 p_max_change) {
	f32 difference = fmod(p_to - p_from, (f32)NP_TAU);
	f32 distance = fmod(2.0f * difference, (f32)NP_TAU) - difference;

	if (distance >= 0) {
		distance = MIN(distance, p_max_change);
	} else {
		distance = MAX(distance, -p_max_change);
	}

	return p_from + distance;
}

f32 Math::lerp_angle(f32 p_from, f32 p_to, f32 p_weight) {
	f32 difference = fmod(p_to - p_from, (f32)NP_TAU);
	f32 distance = fmod(2.0f * difference, (f32)NP_TAU) - difference;
	return p_from + distance * p_weight;
}

f32 Math::normalize_angle(f32 p_angle) {
	f32 a = fmod(p_angle + (f32)NP_PI, (f32)NP_TAU);
	if (a < 0) {
		a += (f32)NP_TAU;
	}
	return a - (f32)NP_PI;
}

void Math::constrain_velocity_to_angle(IPoint2 &r_velocity, f32 p_angle, f32 p_restriction) {
	NP_DEV_ASSERT(p_restriction >= 0);
	if (p_restriction <= 0.001f) {
		return;
	}
	NP_DEV_ASSERT(p_restriction <= 1);

	FPoint2 vel = r_velocity.to_f32();

	FPoint2 restrict_dir;
	restrict_dir.from_angle(p_angle);

	f32 dot = restrict_dir.dot(vel);
	f32 abs_dot = Math::fabs32(dot);

	// Special case for 90 degrees to the
	if (abs_dot <= 0.01f) {
		r_velocity.zero();
		return;
	}

	// Special case, velocity is aligned with the angle.
	// Shortcut to revent overshooting due to float error.
	if (abs_dot >= (0.99f)) {
		// No change.
		return;
	}

	// The dot is the magnitude of the new direction.
	restrict_dir *= dot;

	u64 sq_before = r_velocity.length_squared();
	IPoint2 new_vel;

	new_vel.from_f32(restrict_dir);
	u64 sq_after = new_vel.length_squared();

	// Note it is still possible the output could be larger than the input, due to rounding.
	// May or may not be worth checking this.
	// This extra check may not be necessary for high performance
	// (it may not be possible, but just in case, due to rounding).
	if (sq_after < sq_before) {
		r_velocity = new_vel;
	}
}

f32 Math::get_adjusted_friction(f32 p_friction, u32 p_tps) {
	double delta = 1.0 / p_tps;

	double f = 1.0 - p_friction;
	f = Math::pow(f, delta);

	// Fudge.
	//f = Math::pow(f, 0.9);
	return f;

	/*
	// Clamp inputs to safe limits.
	if (p_friction_at_60_tps <= 0.0f || p_friction_at_60_tps >= 1.0f) {
		return p_friction_at_60_tps; // 0 and 1 don't scale
	}

	p_target_tps = MAX(1, p_target_tps);

	// Compute time scale ratio relative to 60 TPS baseline
	f32 time_ratio = static_cast<f32>(p_target_tps) / 60;

	// Apply the analytical exponential scaling formula
	return std::pow(p_friction_at_60_tps, time_ratio);
*/
}

} // namespace NavPhysics
