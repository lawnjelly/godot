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
	return abs(s) < CMP_EPSILON;
}

} // namespace NavPhysics
