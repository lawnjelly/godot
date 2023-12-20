#include "fast_array.h"

//#include "core/color.h"
#include "core/method_bind_ext.gen.inc"

#define GODOT_STRINGIFY(x) #x
#define GODOT_TOSTRING(x) GODOT_STRINGIFY(x)

using namespace GSimd;

///////////////////////////

void FastArray::_bind_methods() {
	ClassDB::bind_method(D_METHOD("resize", "size_bytes", "blank_byte"), &FastArray::resize, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("size_b"), &FastArray::size_b);
	ClassDB::bind_method(D_METHOD("size_f"), &FastArray::size_f);
	ClassDB::bind_method(D_METHOD("size_f4"), &FastArray::size_f4);
	ClassDB::bind_method(D_METHOD("size_f3"), &FastArray::size_f3);

#define BIND_COPY_FROM_TO(FUNC_NAME)                                                                                                                                    \
	ClassDB::bind_method(D_METHOD("copy_from_" GODOT_TOSTRING(FUNC_NAME), "array", "source_offset", "dest_offset", "num_elements"), &FastArray::copy_from_##FUNC_NAME); \
	ClassDB::bind_method(D_METHOD("copy_to_" GODOT_TOSTRING(FUNC_NAME), "array", "source_offset", "dest_offset", "num_elements"), &FastArray::copy_to_##FUNC_NAME);

	BIND_COPY_FROM_TO(pool_byte_array)
	BIND_COPY_FROM_TO(pool_color_array)
	BIND_COPY_FROM_TO(pool_int_array)
	BIND_COPY_FROM_TO(pool_real_array)
	BIND_COPY_FROM_TO(pool_vector2_array)
	BIND_COPY_FROM_TO(pool_vector3_array)

	//	ClassDB::bind_method(D_METHOD("bind_pool_byte_array", "array", "read_only"), &FastArray::bind_pool_byte_array, DEFVAL(false));
	//	ClassDB::bind_method(D_METHOD("bind_pool_color_array", "array", "read_only"), &FastArray::bind_pool_color_array, DEFVAL(false));
	//	ClassDB::bind_method(D_METHOD("bind_pool_int_array", "array", "read_only"), &FastArray::bind_pool_int_array, DEFVAL(false));
	//	ClassDB::bind_method(D_METHOD("bind_pool_real_array", "array", "read_only"), &FastArray::bind_pool_real_array, DEFVAL(false));
	//	ClassDB::bind_method(D_METHOD("bind_pool_string_array", "array", "read_only"), &FastArray::bind_pool_string_array, DEFVAL(false));
	//	ClassDB::bind_method(D_METHOD("bind_pool_vector2_array", "array", "read_only"), &FastArray::bind_pool_vector2_array, DEFVAL(false));
	//	ClassDB::bind_method(D_METHOD("bind_pool_vector3_array", "array", "read_only"), &FastArray::bind_pool_vector3_array, DEFVAL(false));

#define BIND_VARIATIONS_FF_TO_F(FUNC, DEFAULT_STRIDE)                                                                                                                                                                                                                                                                         \
	ClassDB::bind_method(D_METHOD("f4_value_" GODOT_TOSTRING(FUNC), "value", "source_fast_array", "source_offset", "dest_offset", "num_elements", "source_stride", "dest_stride"), &FastArray::f4_value_##FUNC, DEFVAL(DEFAULT_STRIDE), DEFVAL(DEFAULT_STRIDE));                                                              \
	ClassDB::bind_method(D_METHOD("f4_" GODOT_TOSTRING(FUNC), "source_fast_array", "source_offset", "source_fast_array2", "source_offset2", "dest_offset", "num_elements", "source_stride", "source_stride2", "dest_stride"), &FastArray::f4_##FUNC, DEFVAL(DEFAULT_STRIDE), DEFVAL(DEFAULT_STRIDE), DEFVAL(DEFAULT_STRIDE)); \
	ClassDB::bind_method(D_METHOD("f_" GODOT_TOSTRING(FUNC), "source_fast_array", "source_offset", "source_fast_array2", "source_offset2", "dest_offset", "num_elements", "source_stride", "source_stride2", "dest_stride"), &FastArray::f_##FUNC, DEFVAL(1), DEFVAL(1), DEFVAL(1));

	BIND_VARIATIONS_FF_TO_F(add, 4)
	BIND_VARIATIONS_FF_TO_F(sub, 4)
	BIND_VARIATIONS_FF_TO_F(mul, 4)
	BIND_VARIATIONS_FF_TO_F(div, 4)

	//	BIND_VARIATIONS_F_TO_F(sqrt)
	//	BIND_VARIATIONS_SIMPLE(inv_sqrt)
	//	BIND_VARIATIONS_SIMPLE(reciprocal)

#define BIND_VARIATIONS_ONE_TO_ONE(FUNC, DEFAULT_SOURCE_STRIDE, DEFAULT_DEST_STRIDE) \
	ClassDB::bind_method(D_METHOD(GODOT_TOSTRING(FUNC), "source_fast_array", "source_offset", "dest_offset", "num_elements", "source_stride", "dest_stride"), &FastArray::FUNC, DEFVAL(DEFAULT_SOURCE_STRIDE), DEFVAL(DEFAULT_DEST_STRIDE));

	BIND_VARIATIONS_ONE_TO_ONE(f4_sqrt, 4, 4)
	BIND_VARIATIONS_ONE_TO_ONE(f_sqrt, 1, 1)
	BIND_VARIATIONS_ONE_TO_ONE(f4_inv_sqrt, 4, 4)
	BIND_VARIATIONS_ONE_TO_ONE(f_inv_sqrt, 1, 1)
	BIND_VARIATIONS_ONE_TO_ONE(f4_reciprocal, 4, 4)
	BIND_VARIATIONS_ONE_TO_ONE(f_reciprocal, 1, 1)
	BIND_VARIATIONS_ONE_TO_ONE(f3_length, 3, 1)
	BIND_VARIATIONS_ONE_TO_ONE(f3_length_squared, 3, 1)
	BIND_VARIATIONS_ONE_TO_ONE(f3_normalize, 3, 3)

#define BIND_VARIATIONS_VAL_TWO_TO_ONE(FUNC, DEFAULT_STRIDE) \
	ClassDB::bind_method(D_METHOD(GODOT_TOSTRING(FUNC), "value", "source_fast_array", "source_offset", "dest_offset", "num_elements", "source_stride", "dest_stride"), &FastArray::FUNC, DEFVAL(DEFAULT_STRIDE), DEFVAL(DEFAULT_STRIDE));

	BIND_VARIATIONS_VAL_TWO_TO_ONE(f3_xform, 3)
	BIND_VARIATIONS_VAL_TWO_TO_ONE(f3_inv_xform, 3)

#define BIND_VARIATIONS_TWO_TO_ONE(FUNC, DEFAULT_SOURCE_STRIDE, DEFAULT_DEST_STRIDE) \
	ClassDB::bind_method(D_METHOD(GODOT_TOSTRING(FUNC), "source_fast_array", "source_offset", "source_fast_array2", "source_offset2", "dest_offset", "num_elements", "source_stride", "source_stride2", "dest_stride"), &FastArray::FUNC, DEFVAL(DEFAULT_SOURCE_STRIDE), DEFVAL(DEFAULT_SOURCE_STRIDE), DEFVAL(DEFAULT_DEST_STRIDE));

	BIND_VARIATIONS_TWO_TO_ONE(f3_dot, 3, 1)

	ClassDB::bind_method(D_METHOD("f4_read", "offset"), &FastArray::f4_read);
	ClassDB::bind_method(D_METHOD("f4_write", "offset", "value"), &FastArray::f4_write);
	ClassDB::bind_method(D_METHOD("f_read", "offset"), &FastArray::f_read);
	ClassDB::bind_method(D_METHOD("f_write", "offset", "value"), &FastArray::f_write);

	ClassDB::bind_method(D_METHOD("i_read", "offset"), &FastArray::i_read);
	ClassDB::bind_method(D_METHOD("i_write", "offset", "value"), &FastArray::i_write);
}

void FastArray::resize(uint32_t p_bytes, int32_t p_blank) {
	_data.resize(p_bytes);
	_access.set(_data.ptr(), p_bytes);
	_external_data = false;

	if (p_blank >= 0) {
		_data.fill(p_blank);
	}
}

void FastArray::bind_external_data(uint8_t *p_data, uint32_t p_data_size_bytes) {
	_data.resize(0);
	_access.set(p_data, p_data_size_bytes);
	_external_data = true;
}
