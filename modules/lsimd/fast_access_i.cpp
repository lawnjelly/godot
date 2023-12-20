#include "fast_access_i.h"
#include "fast_array_macros.h"

FA_IMPL_VAL_TWO_TO_ONE_A(i4_value_add, i32_4, sizeof(i32_4), sizeof(i32_4), 4)
//#ifdef GSIMD_USE_SSE
//FA_SIMD_IF(Godot_CPU::F_SSE)
//Simd_4f32_SSE::value_add((float *)&p_val, p_source.get_param(), p_dest.get_param(), p_num_elements);
//FA_SIMD_IF_END_SINGLE_SOURCE
//#endif
FA_IMPL_VAL_TWO_TO_ONE_B(add, i32_4, i32_4)

/////////////////////////////////////
GSimd::i32_4 FastAccessI::i4_read(uint32_t p_offset) const {
	// 16 bytes per i4.
	p_offset *= 16;
	ERR_FAIL_COND_V(!bounds_check(p_offset, sizeof(i32_4)), i32_4::zero());
	return *((i32_4 *)&_data[p_offset]);
}

void FastAccessI::i4_write(uint32_t p_offset, const GSimd::i32_4 &p_val) {
	// 16 bytes per f4.
	p_offset *= 16;
	ERR_FAIL_COND(!bounds_check(p_offset, sizeof(i32_4)));
	*(i32_4 *)&_data[p_offset] = p_val;
}

float FastAccessI::i_read(uint32_t p_offset) const {
	p_offset *= 4;
	ERR_FAIL_COND_V(!bounds_check(p_offset, sizeof(int32_t)), 0);
	return *((int32_t *)&_data[p_offset]);
}

void FastAccessI::i_write(uint32_t p_offset, int32_t p_val) {
	p_offset *= 4;
	ERR_FAIL_COND(!bounds_check(p_offset, sizeof(int32_t)));
	*(int32_t *)&_data[p_offset] = p_val;
}
