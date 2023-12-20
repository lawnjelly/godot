#pragma once

#include "fast_access_f.h"
#include "type_i32_3.h"
#include "type_i32_4.h"

class FastAccessI : public FastAccessF {
public:
	FACCESS_DECL_VAL_TWO_TO_ONE(i4_value_add, GSimd::i32_4)
	FACCESS_DECL_VAL_TWO_TO_ONE(i4_value_sub, GSimd::i32_4)
	FACCESS_DECL_VAL_TWO_TO_ONE(i4_value_mul, GSimd::i32_4)
	FACCESS_DECL_VAL_TWO_TO_ONE(i4_value_div, GSimd::i32_4)

	GSimd::i32_4 i4_read(uint32_t p_offset) const;
	void i4_write(uint32_t p_offset, const GSimd::i32_4 &p_val);

	float i_read(uint32_t p_offset) const;
	void i_write(uint32_t p_offset, int32_t p_val);
};
