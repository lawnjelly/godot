#pragma once

#include "core/local_vector.h"
#include "core/pool_vector.h"
#include "simd_param.h"
#include "type_f32_3.h"
#include "type_f32_4.h"

class FAParam;

#define FACCESS_DECL_VAL_TWO_TO_ONE(FUNC, VAL_TYPE) \
	void FUNC(const VAL_TYPE &p_val, FAParam &p_source, FAParam &p_dest, uint32_t p_num_elements);

#define FACCESS_DECL_ONE_TO_ONE(FUNC) \
	void FUNC(FAParam &p_source, FAParam &p_dest, uint32_t p_num_elements);

#define FACCESS_DECL_TWO_TO_ONE(FUNC) \
	void FUNC(FAParam &p_source, FAParam &p_source2, FAParam &p_dest, uint32_t p_num_elements);

// Note that for SIMD (SSE / AVX / Neon) paths
// We always do the final element with reference method.
// This is because SIMD can read / write outside the bounds of the data,
// in the case of e.g. Vector3.
class FastAccessBase {
	friend class FAParam;

protected:
	uint32_t _size = 0;
	uint8_t *_data = nullptr;

	// Pre-store sizes in various common alignments.
	struct Data {
		uint32_t size_f = 0;
		uint32_t size_f4 = 0;
		uint32_t size_f3 = 0;
	} data;

	bool bounds_check(uint32_t p_offset, uint32_t p_word_size, uint32_t p_num = 1, uint32_t p_stride = 0) const {
		uint32_t end = p_offset + ((p_num - 1) * p_stride) + p_word_size;
		return end <= _size;
	}

public:
	void set(uint8_t *p_data, uint32_t p_size) {
		_data = p_data;
		_size = p_size;
		data.size_f = p_size / 4;
		data.size_f4 = data.size_f / 4;
		data.size_f3 = data.size_f / 3;
	}

	// Shortcut for using set with LocalVector.
	template <class T>
	void bind_local_vector(const LocalVector<T> &p_vector) {
		set(p_vector.ptr(), p_vector.size() * sizeof(T));
	}

	uint8_t *ptr() { return _data; }
	const uint8_t *ptr() const { return _data; }
	uint32_t size() const { return _size; }

	uint32_t size_f() const { return data.size_f; }
	uint32_t size_f4() const { return data.size_f4; }
	uint32_t size_f3() const { return data.size_f3; }

	// COPYING TO / FROM
	// Note these are unsafe as far as bounds checking external data destination / source.
	void copy_data_to(uint8_t *r_dest, uint32_t p_offset, uint32_t p_num_bytes);
	void copy_data_from(const uint8_t *p_source, uint32_t p_offset, uint32_t p_num_bytes);

#define FACCESS_DECL_COPY_FROM_GENERIC(VECTOR_NAME, VECTOR_CLASS, GET_POINTER_TOKEN)                                                                                \
	template <class T>                                                                                                                                              \
	void copy_from_##VECTOR_NAME(const VECTOR_CLASS<T> &p_source, uint32_t p_source_offset = 0, uint32_t p_dest_offset = 0, uint32_t p_num_elements = UINT32_MAX) { \
		if (p_num_elements == UINT32_MAX) {                                                                                                                         \
			p_num_elements = (_size / sizeof(T));                                                                                                                   \
			ERR_FAIL_COND(p_dest_offset > p_num_elements);                                                                                                          \
			p_num_elements -= p_dest_offset;                                                                                                                        \
			ERR_FAIL_COND((p_source.size() - p_source_offset) < p_num_elements);                                                                                    \
		}                                                                                                                                                           \
		ERR_FAIL_COND((p_source_offset + p_num_elements) > p_source.size());                                                                                        \
		ERR_FAIL_COND(((p_dest_offset + p_num_elements) * sizeof(T)) > _size);                                                                                      \
		auto r = p_source.read();                                                                                                                                   \
		memcpy(&_data[p_dest_offset * sizeof(T)], GET_POINTER_TOKEN + (p_source_offset * sizeof(T)), p_num_elements * sizeof(T));                                   \
	}

#define FACCESS_DECL_COPY_TO_GENERIC(VECTOR_NAME, VECTOR_CLASS, GET_POINTER_TOKEN)                                                                              \
	template <class T>                                                                                                                                          \
	void copy_to_##VECTOR_NAME(VECTOR_CLASS<T> &r_dest, uint32_t p_source_offset = 0, uint32_t p_dest_offset = 0, uint32_t p_num_elements = UINT32_MAX) const { \
		if (p_num_elements == UINT32_MAX) {                                                                                                                     \
			p_num_elements = (_size / sizeof(T));                                                                                                               \
			ERR_FAIL_COND(p_source_offset > p_num_elements);                                                                                                    \
			p_num_elements -= p_source_offset;                                                                                                                  \
			ERR_FAIL_COND((r_dest.size() - p_dest_offset) < p_num_elements);                                                                                    \
		}                                                                                                                                                       \
		ERR_FAIL_COND((p_dest_offset + p_num_elements) > r_dest.size());                                                                                        \
		ERR_FAIL_COND(((p_source_offset + p_num_elements) * sizeof(T)) > _size);                                                                                \
		memcpy(GET_POINTER_TOKEN + (p_dest_offset * sizeof(T)), &_data[p_source_offset * sizeof(T)], p_num_elements * sizeof(T));                               \
	}

	FACCESS_DECL_COPY_FROM_GENERIC(pool_vector, PoolVector, p_source.read().ptr())
	FACCESS_DECL_COPY_TO_GENERIC(pool_vector, PoolVector, r_dest.write().ptr())

	FACCESS_DECL_COPY_FROM_GENERIC(local_vector, LocalVector, p_source.ptr())
	FACCESS_DECL_COPY_TO_GENERIC(local_vector, LocalVector, r_dest.ptr())

	FACCESS_DECL_COPY_FROM_GENERIC(vector, Vector, p_source.ptr())
	FACCESS_DECL_COPY_TO_GENERIC(vector, Vector, r_dest.ptrw())
};

class FAParam {
	FastAccessBase &access;
	uint32_t offset_units = 0;

	SIMDParam param;

public:
	SIMDParam &get_param() { return param; }
	void set(uint32_t p_offset, uint32_t p_stride_units) {
		offset_units = p_offset;
		param.stride_units = p_stride_units;
	}

	FAParam(FastAccessBase &p_access) :
			access(p_access) {}
	bool prepare(uint32_t p_word_size, uint32_t p_alignment, uint32_t p_num_elements) {
		param.stride_bytes = param.stride_units * p_alignment;
		uint32_t offset_bytes = offset_units * p_alignment;

		uint32_t end = offset_bytes + ((p_num_elements - 1) * param.stride_bytes) + p_word_size;
		if (end <= access._size) {
			param.data = &access.ptr()[offset_bytes];
			return true;
		}
		param.data = nullptr;
		return false;
	}
	// This is used to fast forward through elements done by SIMD
	// until will get to the last element, which will be done manually.
	void fast_forward(uint32_t p_num_elements) {
		param.data += p_num_elements * param.stride_bytes;
	}
};
