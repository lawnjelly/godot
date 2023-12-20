#pragma once

#include "core/math/math_funcs.h"
#include <cstdint>

namespace GSimd {
struct i32_3 {
	union {
		struct {
			int32_t x, y, z;
		};
		int32_t v[3];
	};

	uint64_t length_squared() const {
		return ((int64_t)v[0] * v[0]) + ((int64_t)v[1] * v[1]) + ((int64_t)v[2] * v[2]);
	}

	//	uint64_t lengthi() const {
	//		uint64_t sl = length_squared();
	//		return (0.5 + Math::sqrt((double)sl)); // include some rounding
	//	}

	int64_t dot(const i32_3 &o) const {
		int64_t r = 0;
		for (int n = 0; n < 3; n++)
			r += v[n] * o.v[n];
		return r;
	}

	i32_3 cross(const i32_3 &o) {
		i32_3 r;
		r.x = (y * o.z) - (z * o.y);
		r.y = (z * o.x) - (x * o.z);
		r.z = (x * o.y) - (y * o.x);
		return r;
	}
};

} //namespace GSimd
