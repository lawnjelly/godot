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

#include <stdint.h>

struct SIMDParam;

namespace GSimd {

class Simd_4f32_SSE {
public:
	static void add(const SIMDParam &p_source, const SIMDParam &p_source2, const SIMDParam &p_dest, unsigned int p_num_elements);
	static void sub(const SIMDParam &p_source, const SIMDParam &p_source2, const SIMDParam &p_dest, unsigned int p_num_elements);
	static void mul(const SIMDParam &p_source, const SIMDParam &p_source2, const SIMDParam &p_dest, unsigned int p_num_elements);
	static void div(const SIMDParam &p_source, const SIMDParam &p_source2, const SIMDParam &p_dest, unsigned int p_num_elements);

	static void value_add(float *p_value, const SIMDParam &p_source, const SIMDParam &p_dest, unsigned int p_num_elements);
	static void value_sub(float *p_value, const SIMDParam &p_source, const SIMDParam &p_dest, unsigned int p_num_elements);
	static void value_mul(float *p_value, const SIMDParam &p_source, const SIMDParam &p_dest, unsigned int p_num_elements);
	static void value_div(float *p_value, const SIMDParam &p_source, const SIMDParam &p_dest, unsigned int p_num_elements);

	static void sqrt(const SIMDParam &p_source, const SIMDParam &p_dest, unsigned int p_num_elements);
	static void rsqrt(const SIMDParam &p_source, const SIMDParam &p_dest, unsigned int p_num_elements);
	static void rcp(const SIMDParam &p_source, const SIMDParam &p_dest, unsigned int p_num_elements);
};

} //namespace GSimd
