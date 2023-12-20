#include "fast_access_f.h"
#include "fast_array_macros.h"
#include "simd_cpu.h"
#include "simd_f32_sse.h"
#include "simd_f32_sse3.h"
#include "simd_f32_sse4_1.h"

using namespace GSimd;

/////////////////////////////////////////////////////////////////////////////

FA_IMPL_VAL_TWO_TO_ONE_A(f4_value_add, f32_4, sizeof(f32_4), sizeof(f32_4), 4)
#ifdef GSIMD_USE_SSE
FA_SIMD_IF(Godot_CPU::F_SSE)
Simd_4f32_SSE::value_add((float *)&p_val, p_source.get_param(), p_dest.get_param(), p_num_elements);
FA_SIMD_IF_END_SINGLE_SOURCE
#endif
FA_IMPL_VAL_TWO_TO_ONE_B(add, f32_4, f32_4)

FA_IMPL_VAL_TWO_TO_ONE_A(f4_value_sub, f32_4, sizeof(f32_4), sizeof(f32_4), 4)
#ifdef GSIMD_USE_SSE
FA_SIMD_IF(Godot_CPU::F_SSE)
Simd_4f32_SSE::value_sub((float *)&p_val, p_source.get_param(), p_dest.get_param(), p_num_elements);
FA_SIMD_IF_END_SINGLE_SOURCE
#endif
FA_IMPL_VAL_TWO_TO_ONE_B(subtract, f32_4, f32_4)

FA_IMPL_VAL_TWO_TO_ONE_A(f4_value_mul, f32_4, sizeof(f32_4), sizeof(f32_4), 4)
#ifdef GSIMD_USE_SSE
FA_SIMD_IF(Godot_CPU::F_SSE)
Simd_4f32_SSE::value_mul((float *)&p_val, p_source.get_param(), p_dest.get_param(), p_num_elements);
FA_SIMD_IF_END_SINGLE_SOURCE
#endif
FA_IMPL_VAL_TWO_TO_ONE_B(multiply, f32_4, f32_4)

FA_IMPL_VAL_TWO_TO_ONE_A(f4_value_div, f32_4, sizeof(f32_4), sizeof(f32_4), 4)
#ifdef GSIMD_USE_SSE
FA_SIMD_IF(Godot_CPU::F_SSE)
Simd_4f32_SSE::value_div((float *)&p_val, p_source.get_param(), p_dest.get_param(), p_num_elements);
FA_SIMD_IF_END_SINGLE_SOURCE
#endif
FA_IMPL_VAL_TWO_TO_ONE_B(divide, f32_4, f32_4)

FA_IMPL_VAL_TWO_TO_ONE_A(_f3_xform, f32_transform, sizeof(f32_4), sizeof(f32_4), 4)
FA_IMPL_VAL_TWO_TO_ONE_B(xform, f32_3, f32_3)

FA_IMPL_VAL_TWO_TO_ONE_A(_f3_inv_xform, f32_transform, sizeof(f32_4), sizeof(f32_4), 4)
FA_IMPL_VAL_TWO_TO_ONE_B(inv_xform, f32_3, f32_3)

/////////////////////////////////////////////////////////////////////////////

// Add
FA_IMPL_TWO_TO_ONE_A(f4_add, sizeof(f32_4), sizeof(f32_4), sizeof(f32_4), 4)
#ifdef GSIMD_USE_SSE
FA_SIMD_IF(Godot_CPU::F_SSE)
Simd_4f32_SSE::add(p_source.get_param(), p_source2.get_param(), p_dest.get_param(), p_num_elements);
FA_SIMD_IF_END_DOUBLE_SOURCE
#endif
FA_IMPL_TWO_TO_ONE_B(add, f32_4, f32_4, f32_4)

// Subtract
FA_IMPL_TWO_TO_ONE_A(f4_sub, sizeof(f32_4), sizeof(f32_4), sizeof(f32_4), 4)
#ifdef GSIMD_USE_SSE
FA_SIMD_IF(Godot_CPU::F_SSE)
Simd_4f32_SSE::sub(p_source.get_param(), p_source2.get_param(), p_dest.get_param(), p_num_elements);
FA_SIMD_IF_END_DOUBLE_SOURCE
#endif
FA_IMPL_TWO_TO_ONE_B(subtract, f32_4, f32_4, f32_4)

// Multiply
FA_IMPL_TWO_TO_ONE_A(f4_mul, sizeof(f32_4), sizeof(f32_4), sizeof(f32_4), 4)
#ifdef GSIMD_USE_SSE
FA_SIMD_IF(Godot_CPU::F_SSE)
Simd_4f32_SSE::mul(p_source.get_param(), p_source2.get_param(), p_dest.get_param(), p_num_elements);
FA_SIMD_IF_END_DOUBLE_SOURCE
#endif
FA_IMPL_TWO_TO_ONE_B(multiply, f32_4, f32_4, f32_4)

// Divide
FA_IMPL_TWO_TO_ONE_A(f4_div, sizeof(f32_4), sizeof(f32_4), sizeof(f32_4), 4)
#ifdef GSIMD_USE_SSE
FA_SIMD_IF(Godot_CPU::F_SSE)
Simd_4f32_SSE::div(p_source.get_param(), p_source2.get_param(), p_dest.get_param(), p_num_elements);
FA_SIMD_IF_END_DOUBLE_SOURCE
#endif
FA_IMPL_TWO_TO_ONE_B(divide, f32_4, f32_4, f32_4)

// Cross
FA_IMPL_TWO_TO_ONE_A(f3_cross, sizeof(f32_4), sizeof(f32_4), sizeof(f32_4), 4)
FA_IMPL_TWO_TO_ONE_B(cross, f32_3, f32_3, f32_3)

// Unit Cross
FA_IMPL_TWO_TO_ONE_A(f3_unit_cross, sizeof(f32_4), sizeof(f32_4), sizeof(f32_4), 4)
FA_IMPL_TWO_TO_ONE_B(unit_cross, f32_3, f32_3, f32_3)

// Dot
FA_IMPL_TWO_TO_ONE_A(f3_dot, sizeof(f32_4), sizeof(f32_4), sizeof(float), 4)
#ifdef GSIMD_USE_SSE4_1
FA_SIMD_IF(Godot_CPU::F_SSE4_1)
Simd_4f32_SSE4_1::vec3_dot(p_source.get_param(), p_source2.get_param(), p_dest.get_param(), p_num_elements);
FA_SIMD_IF_END_DOUBLE_SOURCE
#endif
FA_IMPL_TWO_TO_ONE_B(dot, f32_3, f32_3, float)

/////////////////////////////////////////////////////////////////////////////

// Normalized
FA_IMPL_ONE_TO_ONE_A(f3_normalize, sizeof(f32_4), sizeof(f32_4), 4)
#ifdef GSIMD_USE_SSE4_1
FA_SIMD_IF(Godot_CPU::F_SSE4_1)
Simd_4f32_SSE4_1::vec3_normalize(p_source.get_param(), p_dest.get_param(), p_num_elements);
FA_SIMD_IF_END_SINGLE_SOURCE
#endif
FA_IMPL_ONE_TO_ONE_B(normalize, f32_3, f32_3)

// Sqrt
FA_IMPL_ONE_TO_ONE_A(f4_sqrt, sizeof(f32_4), sizeof(f32_4), 4)
#ifdef GSIMD_USE_SSE
FA_SIMD_IF(Godot_CPU::F_SSE)
Simd_4f32_SSE::sqrt(p_source.get_param(), p_dest.get_param(), p_num_elements);
FA_SIMD_IF_END_SINGLE_SOURCE
#endif
FA_IMPL_ONE_TO_ONE_B(sqrt, f32_4, f32_4)

// Inv Sqrt
FA_IMPL_ONE_TO_ONE_A(f4_inv_sqrt, sizeof(f32_4), sizeof(f32_4), 4)
#ifdef GSIMD_USE_SSE
FA_SIMD_IF(Godot_CPU::F_SSE)
Simd_4f32_SSE::rsqrt(p_source.get_param(), p_dest.get_param(), p_num_elements);
FA_SIMD_IF_END_SINGLE_SOURCE
#endif
FA_IMPL_ONE_TO_ONE_B(inv_sqrt, f32_4, f32_4)

// Reciprocal
FA_IMPL_ONE_TO_ONE_A(f4_reciprocal, sizeof(f32_4), sizeof(f32_4), 4)
#ifdef GSIMD_USE_SSE
FA_SIMD_IF(Godot_CPU::F_SSE)
Simd_4f32_SSE::rcp(p_source.get_param(), p_dest.get_param(), p_num_elements);
FA_SIMD_IF_END_SINGLE_SOURCE
#endif
FA_IMPL_ONE_TO_ONE_B(reciprocal, f32_4, f32_4)

// Length
FA_IMPL_ONE_TO_ONE_A(f3_length, sizeof(f32_4), sizeof(float), 4)
#ifdef GSIMD_USE_SSE3
FA_SIMD_IF(Godot_CPU::F_SSE3)
Simd_4f32_SSE3::vec3_length(p_source.get_param(), p_dest.get_param(), p_num_elements);
FA_SIMD_IF_END_SINGLE_SOURCE
#endif
FA_IMPL_ONE_TO_ONE_B(length, f32_3, float)

// Length squared
FA_IMPL_ONE_TO_ONE_A(f3_length_squared, sizeof(f32_4), sizeof(float), 4)
#ifdef GSIMD_USE_SSE3
FA_SIMD_IF(Godot_CPU::F_SSE3)
Simd_4f32_SSE3::vec3_length_squared(p_source.get_param(), p_dest.get_param(), p_num_elements);
FA_SIMD_IF_END_SINGLE_SOURCE
#endif
FA_IMPL_ONE_TO_ONE_B(length_squared, f32_3, float)

/////////////////////////////////////////////////////////////////////////////

FA_IMPL_TWO_TO_ONE_QUADS(f_add, f4_add, +=, 4)
FA_IMPL_TWO_TO_ONE_QUADS(f_sub, f4_sub, -=, 4)
FA_IMPL_TWO_TO_ONE_QUADS(f_mul, f4_mul, *=, 4)
FA_IMPL_TWO_TO_ONE_QUADS(f_div, f4_div, /=, 4)

/////////////////////////////////////////////////////////////////////////////

FA_IMPL_ONE_TO_ONE_QUADS(f_sqrt, f4_sqrt, Math::sqrt(d), 4)
FA_IMPL_ONE_TO_ONE_QUADS(f_inv_sqrt, f4_inv_sqrt, (1.0f / Math::sqrt(d)), 4)
FA_IMPL_ONE_TO_ONE_QUADS(f_reciprocal, f4_reciprocal, (1.0f / d), 4)

/////////////////////////////////////////////////////////////////////////////

float FastAccessF::f_read(uint32_t p_offset) const {
	p_offset *= 4;
	ERR_FAIL_COND_V(!bounds_check(p_offset, sizeof(float)), 0.0f);
	return *((float *)&_data[p_offset]);
}

void FastAccessF::f_write(uint32_t p_offset, float p_val) {
	p_offset *= 4;
	ERR_FAIL_COND(!bounds_check(p_offset, sizeof(float)));
	*(float *)&_data[p_offset] = p_val;
}

f32_4 FastAccessF::f4_read(uint32_t p_offset) const {
	// 16 bytes per f4.
	p_offset *= 16;
	ERR_FAIL_COND_V(!bounds_check(p_offset, sizeof(f32_4)), f32_4::zero());
	return *((f32_4 *)&_data[p_offset]);
}

void FastAccessF::f4_write(uint32_t p_offset, const f32_4 &p_val) {
	// 16 bytes per f4.
	p_offset *= 16;
	ERR_FAIL_COND(!bounds_check(p_offset, sizeof(f32_4)));
	*(f32_4 *)&_data[p_offset] = p_val;
}
