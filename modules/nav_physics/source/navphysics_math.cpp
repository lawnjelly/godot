#include "navphysics_math.h"
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

f32 Math::fmod(f32 p_x, f32 p_y) {
	return ::fmodf(p_x, p_y);
}

freal Math::atan2_real(freal p_a, freal p_b) {
	return ::atan2(p_a, p_b);
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
	return abs(a - b) < tolerance;
}

bool Math::is_zero_approx(f32 s, f32 tolerance) {
	return abs(s) < NP_CMP_EPSILON;
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
	float difference = fmod(p_to - p_from, (f32)NP_TAU);
	float distance = fmod(2.0f * difference, (f32)NP_TAU) - difference;
	return p_from + distance * p_weight;
}

} // namespace NavPhysics
