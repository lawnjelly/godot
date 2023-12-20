#pragma once

#include "core/reference.h"
#include "fast_access.h"

class FastArray : public Reference {
	GDCLASS(FastArray, Reference);

	FastAccess _access;
	LocalVector<uint8_t> _data; // If using internal data.

	//	enum BoundDataType {
	//		BD_NONE,
	//		BD_POOL_BYTE_ARRAY,
	//		BD_POOL_COLOR_ARRAY,
	//		BD_POOL_INT_ARRAY,
	//		BD_POOL_REAL_ARRAY,
	//		BD_POOL_STRING_ARRAY,
	//		BD_POOL_VECTOR2_ARRAY,
	//		BD_POOL_VECTOR3_ARRAY,
	//	};
	//	BoundDataType _bound_data_type = BD_NONE;
	//	void *_bound_data = nullptr;
	bool _external_data = false;
	bool _external_data_read_only = false;

protected:
	static void _bind_methods();

public:
	void resize(uint32_t p_bytes, int32_t p_blank = 0); // blank -1 is no fill.

	uint32_t size_b() const { return _access.size(); }
	uint32_t size_f() const { return _access.size_f(); }
	uint32_t size_f4() const { return _access.size_f4(); }
	uint32_t size_f3() const { return _access.size_f3(); }

	FastAccess &get_access() { return _access; }

#define FARRAY_DECL_VAL_TWO_TO_ONE(FUNC, VAL_TYPE, VAL_CONVERSION)                                                                                                                    \
	void FUNC(const VAL_TYPE &p_val, Object *p_source, uint32_t p_source_offset, uint32_t p_dest_offset, uint32_t p_num_elements, uint32_t p_source_stride, uint32_t p_dest_stride) { \
		FastArray *source_array = Object::cast_to<FastArray>(p_source);                                                                                                               \
		ERR_FAIL_NULL(source_array);                                                                                                                                                  \
		FAParam source(source_array->get_access());                                                                                                                                   \
		source.set(p_source_offset, p_source_stride);                                                                                                                                 \
		FAParam dest(get_access());                                                                                                                                                   \
		dest.set(p_dest_offset, p_dest_stride);                                                                                                                                       \
		_access.FUNC(VAL_CONVERSION, source, dest, p_num_elements);                                                                                                                   \
	}

	FARRAY_DECL_VAL_TWO_TO_ONE(f4_value_add, Plane, GSimd::f32_4::from_plane(p_val))
	FARRAY_DECL_VAL_TWO_TO_ONE(f4_value_sub, Plane, GSimd::f32_4::from_plane(p_val))
	FARRAY_DECL_VAL_TWO_TO_ONE(f4_value_mul, Plane, GSimd::f32_4::from_plane(p_val))
	FARRAY_DECL_VAL_TWO_TO_ONE(f4_value_div, Plane, GSimd::f32_4::from_plane(p_val))

	FARRAY_DECL_VAL_TWO_TO_ONE(f3_xform, Transform, p_val)
	FARRAY_DECL_VAL_TWO_TO_ONE(f3_inv_xform, Transform, p_val)

/////////////////////////////////////////////////////////////////////////////
#define FARRAY_DECL_TWO_TO_ONE(FUNC, DEF_SOURCE_STRIDE, DEF_SOURCE2_STRIDE, DEF_DEST_STRIDE)                                                                                                                                                                                                       \
	void FUNC(Object *p_source, uint32_t p_source_offset, Object *p_source2, uint32_t p_source_offset2, uint32_t p_dest_offset, uint32_t p_num_elements, uint32_t p_source_stride = DEF_SOURCE_STRIDE, uint32_t p_source_stride2 = DEF_SOURCE2_STRIDE, uint32_t p_dest_stride = DEF_DEST_STRIDE) { \
		FastArray *source_array = Object::cast_to<FastArray>(p_source);                                                                                                                                                                                                                            \
		ERR_FAIL_NULL(source_array);                                                                                                                                                                                                                                                               \
		FAParam source(source_array->get_access());                                                                                                                                                                                                                                                \
		source.set(p_source_offset, p_source_stride);                                                                                                                                                                                                                                              \
		FastArray *source_array2 = Object::cast_to<FastArray>(p_source2);                                                                                                                                                                                                                          \
		ERR_FAIL_NULL(source_array2);                                                                                                                                                                                                                                                              \
		FAParam source2(source_array2->get_access());                                                                                                                                                                                                                                              \
		source2.set(p_source_offset2, p_source_stride2);                                                                                                                                                                                                                                           \
		FAParam dest(get_access());                                                                                                                                                                                                                                                                \
		dest.set(p_dest_offset, p_dest_stride);                                                                                                                                                                                                                                                    \
		_access.FUNC(source, source2, dest, p_num_elements);                                                                                                                                                                                                                                       \
	}

	FARRAY_DECL_TWO_TO_ONE(f3_dot, 3, 3, 1)
	FARRAY_DECL_TWO_TO_ONE(f4_add, 4, 4, 4)
	FARRAY_DECL_TWO_TO_ONE(f4_sub, 4, 4, 4)
	FARRAY_DECL_TWO_TO_ONE(f4_mul, 4, 4, 4)
	FARRAY_DECL_TWO_TO_ONE(f4_div, 4, 4, 4)
	FARRAY_DECL_TWO_TO_ONE(f_add, 1, 1, 1)
	FARRAY_DECL_TWO_TO_ONE(f_sub, 1, 1, 1)
	FARRAY_DECL_TWO_TO_ONE(f_mul, 1, 1, 1)
	FARRAY_DECL_TWO_TO_ONE(f_div, 1, 1, 1)

#define FARRAY_DECL_ONE_TO_ONE(FUNC, DEF_SOURCE_STRIDE, DEF_DEST_STRIDE)                                                                                                                             \
	void FUNC(Object *p_source, uint32_t p_source_offset, uint32_t p_dest_offset, uint32_t p_num_elements, uint32_t p_source_stride = DEF_SOURCE_STRIDE, uint32_t p_dest_stride = DEF_DEST_STRIDE) { \
		FastArray *source_array = Object::cast_to<FastArray>(p_source);                                                                                                                              \
		ERR_FAIL_NULL(source_array);                                                                                                                                                                 \
		FAParam source(source_array->get_access());                                                                                                                                                  \
		source.set(p_source_offset, p_source_stride);                                                                                                                                                \
		FAParam dest(get_access());                                                                                                                                                                  \
		dest.set(p_dest_offset, p_dest_stride);                                                                                                                                                      \
		_access.FUNC(source, dest, p_num_elements);                                                                                                                                                  \
	}

	FARRAY_DECL_ONE_TO_ONE(f4_sqrt, 4, 4)
	FARRAY_DECL_ONE_TO_ONE(f4_inv_sqrt, 4, 4)
	FARRAY_DECL_ONE_TO_ONE(f4_reciprocal, 4, 4)
	FARRAY_DECL_ONE_TO_ONE(f_sqrt, 1, 1)
	FARRAY_DECL_ONE_TO_ONE(f_inv_sqrt, 1, 1)
	FARRAY_DECL_ONE_TO_ONE(f_reciprocal, 1, 1)
	FARRAY_DECL_ONE_TO_ONE(f3_length, 3, 1)
	FARRAY_DECL_ONE_TO_ONE(f3_length_squared, 3, 1)
	FARRAY_DECL_ONE_TO_ONE(f3_normalize, 3, 3)

	// element access
	Plane f4_read(uint32_t p_offset) const { return _access.f4_read(p_offset).to_plane(); }
	void f4_write(uint32_t p_offset, const Plane &p_val) { _access.f4_write(p_offset, GSimd::f32_4::from_plane(p_val)); }

	float f_read(uint32_t p_offset) const { return _access.f_read(p_offset); }
	void f_write(uint32_t p_offset, float p_value) { _access.f_write(p_offset, p_value); }

	float i_read(uint32_t p_offset) const { return _access.i_read(p_offset); }
	void i_write(uint32_t p_offset, float p_value) { _access.i_write(p_offset, p_value); }

	void bind_external_data(uint8_t *p_data, uint32_t p_data_size_bytes);

#define FARRAY_DECL_COPY_FROM_TO(FUNC_NAME, T)                                                                                             \
	void copy_from_##FUNC_NAME(const T &p_array, uint32_t p_source_offset, uint32_t p_dest_offset, uint32_t p_num_elements = UINT32_MAX) { \
		_access.copy_from_pool_vector(p_array, p_source_offset, p_dest_offset, p_num_elements);                                            \
	}                                                                                                                                      \
	T copy_to_##FUNC_NAME(T p_array, uint32_t p_source_offset, uint32_t p_dest_offset, uint32_t p_num_elements = UINT32_MAX) {             \
		_access.copy_to_pool_vector(p_array, p_source_offset, p_dest_offset, p_num_elements);                                              \
		return p_array;                                                                                                                    \
	}

	FARRAY_DECL_COPY_FROM_TO(pool_byte_array, PoolByteArray)
	FARRAY_DECL_COPY_FROM_TO(pool_color_array, PoolColorArray)
	FARRAY_DECL_COPY_FROM_TO(pool_int_array, PoolIntArray)
	FARRAY_DECL_COPY_FROM_TO(pool_real_array, PoolRealArray)
	FARRAY_DECL_COPY_FROM_TO(pool_vector2_array, PoolVector2Array)
	FARRAY_DECL_COPY_FROM_TO(pool_vector3_array, PoolVector3Array)

	/*
	#define FARRAY_DECL_BIND_POOL_ARRAY(FUNC_NAME, POOL_CLASS, SIZEOF_CLASS)                        \
		void                                                                                        \
		FUNC_NAME(const POOL_CLASS &p_array, bool p_read_only = false) {                            \
			_external_data_read_only = p_read_only;                                                 \
			if (p_read_only) {                                                                      \
				bind_external_data((uint8_t *)p_array.read().ptr(), p_array.size() * SIZEOF_CLASS); \
			} else {                                                                                \
				POOL_CLASS *array = (POOL_CLASS *)&p_array;                                         \
				bind_external_data((uint8_t *)array->write().ptr(), p_array.size() * SIZEOF_CLASS); \
			}                                                                                       \
		};

		FARRAY_DECL_BIND_POOL_ARRAY(bind_pool_byte_array, PoolByteArray, sizeof(uint8_t))
		FARRAY_DECL_BIND_POOL_ARRAY(bind_pool_color_array, PoolColorArray, sizeof(Color))
		FARRAY_DECL_BIND_POOL_ARRAY(bind_pool_int_array, PoolIntArray, sizeof(int))
		FARRAY_DECL_BIND_POOL_ARRAY(bind_pool_real_array, PoolRealArray, sizeof(real_t))
		FARRAY_DECL_BIND_POOL_ARRAY(bind_pool_string_array, PoolStringArray, sizeof(String))
		FARRAY_DECL_BIND_POOL_ARRAY(bind_pool_vector2_array, PoolVector2Array, sizeof(Vector2))
		FARRAY_DECL_BIND_POOL_ARRAY(bind_pool_vector3_array, PoolVector3Array, sizeof(Vector3))
		*/
};
