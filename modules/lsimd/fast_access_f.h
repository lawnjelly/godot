#pragma once

#include "fast_access_base.h"

class FastAccessF : public FastAccessBase {
public:
	// Wrappers
	void f3_xform(const Transform &p_val, FAParam &p_source, FAParam &p_dest, uint32_t p_num_elements) {
		GSimd::f32_transform tr;
		tr.from_transform(p_val);
		_f3_xform(tr, p_source, p_dest, p_num_elements);
	}

	void f3_inv_xform(const Transform &p_val, FAParam &p_source, FAParam &p_dest, uint32_t p_num_elements) {
		GSimd::f32_transform tr;
		tr.from_transform_inv(p_val);
		_f3_inv_xform(tr, p_source, p_dest, p_num_elements);
	}

	FACCESS_DECL_VAL_TWO_TO_ONE(f4_value_add, GSimd::f32_4)
	FACCESS_DECL_VAL_TWO_TO_ONE(f4_value_sub, GSimd::f32_4)
	FACCESS_DECL_VAL_TWO_TO_ONE(f4_value_mul, GSimd::f32_4)
	FACCESS_DECL_VAL_TWO_TO_ONE(f4_value_div, GSimd::f32_4)

	FACCESS_DECL_VAL_TWO_TO_ONE(_f3_xform, GSimd::f32_transform)
	FACCESS_DECL_VAL_TWO_TO_ONE(_f3_inv_xform, GSimd::f32_transform)

	FACCESS_DECL_ONE_TO_ONE(f3_length)
	FACCESS_DECL_ONE_TO_ONE(f3_length_squared)
	FACCESS_DECL_ONE_TO_ONE(f3_normalize)
	FACCESS_DECL_ONE_TO_ONE(f4_sqrt)
	FACCESS_DECL_ONE_TO_ONE(f_sqrt)
	FACCESS_DECL_ONE_TO_ONE(f4_inv_sqrt)
	FACCESS_DECL_ONE_TO_ONE(f_inv_sqrt)
	FACCESS_DECL_ONE_TO_ONE(f4_reciprocal)
	FACCESS_DECL_ONE_TO_ONE(f_reciprocal)

	FACCESS_DECL_TWO_TO_ONE(f3_dot)
	FACCESS_DECL_TWO_TO_ONE(f3_cross)
	FACCESS_DECL_TWO_TO_ONE(f3_unit_cross)
	FACCESS_DECL_TWO_TO_ONE(f4_add)
	FACCESS_DECL_TWO_TO_ONE(f4_sub)
	FACCESS_DECL_TWO_TO_ONE(f4_mul)
	FACCESS_DECL_TWO_TO_ONE(f4_div)
	FACCESS_DECL_TWO_TO_ONE(f_add)
	FACCESS_DECL_TWO_TO_ONE(f_sub)
	FACCESS_DECL_TWO_TO_ONE(f_mul)
	FACCESS_DECL_TWO_TO_ONE(f_div)

	GSimd::f32_4 f4_read(uint32_t p_offset) const;
	void f4_write(uint32_t p_offset, const GSimd::f32_4 &p_val);

	float f_read(uint32_t p_offset) const;
	void f_write(uint32_t p_offset, float p_val);
};
