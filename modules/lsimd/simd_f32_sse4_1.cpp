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

#include "simd_f32_sse4_1.h"
//#include "simd_f32.h"
#include "simd_cpu.h"
#include "simd_macros.h"

#ifdef GSIMD_USE_SSE4_1
#include <smmintrin.h>
#endif

#ifdef GSIMD_USE_SSE4_1
namespace GSimd {

void Simd_4f32_SSE4_1::vec3_normalize_to_4(const SIMDParam &p_source, const SIMDParam &p_dest, unsigned int p_num_elements) {
	const uint8_t *source = (uint8_t *)p_source.data;
	uint8_t *dest = (uint8_t *)p_dest.data;
	for (int n = 0; n < p_num_elements; n++) {
		__m128 m = _mm_loadu_ps((float *)source);
		__m128 res = _mm_mul_ps(m, _mm_rsqrt_ps(_mm_dp_ps(m, m, 0x7F)));
		_mm_storeu_ps((float *)dest, res);

		source += p_source.stride_units;
		dest += p_dest.stride_units;
	}
}

// Safe version, preserves the final float when writing.
void Simd_4f32_SSE4_1::vec3_normalize(const SIMDParam &p_source, const SIMDParam &p_dest, unsigned int p_num_elements) {
	const uint8_t *source = (uint8_t *)p_source.data;
	uint8_t *dest = (uint8_t *)p_dest.data;
	for (int n = 0; n < p_num_elements; n++) {
		__m128 m = _mm_loadu_ps((float *)source);
		__m128 res = _mm_mul_ps(m, _mm_rsqrt_ps(_mm_dp_ps(m, m, 0x7F)));

		// DON'T DO LOAD PRESERVE / STORE, it is slower
		// than just storing 3 floats separately.

		// https://github.com/microsoft/DirectXMath/issues/93
		// Store XY values
		_mm_store_sd((double *)dest, res);
		// Store Z value
		// SSE 2
		//res = _mm_movehl_ps(res, res);
		//_mm_store_ss((float *)(dest + 8), res);
		// SSE4.1
		*((float *)(dest + 8)) = _mm_extract_ps(res, 2);

		source += p_source.stride_units;
		dest += p_dest.stride_units;
	}
}

void Simd_4f32_SSE4_1::vec3_dot(const SIMDParam &p_source, const SIMDParam &p_source2, const SIMDParam &p_dest, unsigned int p_num_elements) {
	const float *source = (float *)p_source.data;
	const float *source2 = (float *)p_source.data;
	float *dest = (float *)p_dest.data;
	for (int n = 0; n < p_num_elements; n++) {
		__m128 m = _mm_loadu_ps(source);
		__m128 m2 = _mm_loadu_ps(source2);

		// https://www.felixcloutier.com/x86/dpps
		// try and use xyz only
		__m128 res = _mm_dp_ps(m, m2, 0x7F); // 0xff
		*dest = _mm_cvtss_f32(res);

		source += p_source.stride_units;
		source2 += p_source2.stride_units;
		dest += p_dest.stride_units;
	}
}

} //namespace GSimd
#endif // GSIMD_USE_SSE4_1

#include "simd_macros_undef.h"
