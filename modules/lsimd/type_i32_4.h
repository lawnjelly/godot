#pragma once

//	Copyright (c) 2019 Lawnjelly

//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:

//	The above copyright notice and this permission notice shall be included in all
//	copies or substantial portions of the Software.

//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//	SOFTWARE.

/**
	@author lawnjelly <lawnjelly@gmail.com>
*/

#include "core/math/math_funcs.h"
#include <cstdint>

namespace GSimd {
struct i32_4 {
	union {
		struct {
			int32_t x, y, z, w;
		};
		int32_t v[4];
	};

	i32_4 add(const i32_4 &o) const {
		i32_4 t;
		for (int i = 0; i < 4; i++)
			t.v[i] = v[i] + o.v[i];
		return t;
	}

	i32_4 subtract(const i32_4 &o) const {
		i32_4 t;
		for (int i = 0; i < 4; i++)
			t.v[i] = v[i] - o.v[i];
		return t;
	}

	i32_4 multiply(const i32_4 &o) const {
		i32_4 t;
		for (int i = 0; i < 4; i++)
			t.v[i] = v[i] * o.v[i];
		return t;
	}

	i32_4 divide(const i32_4 &o) const {
		i32_4 t;
		for (int i = 0; i < 4; i++)
			t.v[i] = v[i] / o.v[i];
		return t;
	}

	int64_t dot(const i32_4 &o) const {
		int64_t r = 0;
		for (int n = 0; n < 4; n++)
			r += v[n] * o.v[n];
		return r;
	}

	static i32_4 zero() {
		i32_4 v;
		v.v[0] = 0;
		v.v[1] = 0;
		v.v[2] = 0;
		v.v[3] = 0;
		return v;
	}
};

} //namespace GSimd
