#pragma once

#include "navphysics_typedefs.h"

namespace NavPhysics {

struct Math {
	static constexpr f32 NP_CMP_EPSILON = 0.00001;

	static f32 sqrt32(f32 p_v);
	static f64 sqrt64(f64 p_v);
	static freal sqrt_real(freal p_v);
	static freal atan2_real(freal p_a, freal p_b);

	static u32 rand();
	static f32 randf();
	static f32 rand_range(f32 from, f32 to);

	static bool is_equal_approx(f32 a, f32 b, f32 tolerance = NP_CMP_EPSILON);
	static bool is_zero_approx(f32 s, f32 tolerance = NP_CMP_EPSILON);
};

} // namespace NavPhysics
