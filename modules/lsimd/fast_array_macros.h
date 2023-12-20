#pragma once

// When using SIMD we want to omit the last element, and do this last element manually.
#define FA_SIMD_IF(FLAG)                                    \
	if (g_GodotCPU.HasFlag(FLAG) && (p_num_elements > 1)) { \
		p_num_elements--;
#define FA_SIMD_IF_END_SINGLE_SOURCE       \
	p_source.fast_forward(p_num_elements); \
	p_dest.fast_forward(p_num_elements);   \
	p_num_elements = 1;                    \
	}
#define FA_SIMD_IF_END_DOUBLE_SOURCE        \
	p_source.fast_forward(p_num_elements);  \
	p_source2.fast_forward(p_num_elements); \
	p_dest.fast_forward(p_num_elements);    \
	p_num_elements = 1;                     \
	}

/////////////////////////////////////////////////////////////////////////////
// All implementations with value and second input and one output can use the same macros.

#define FA_IMPL_VAL_TWO_TO_ONE_A(FULL_FUNC, TYPE_VALUE, PREPARE_SIZEOF_SOURCE, PREPARE_SIZEOF_DEST, ALIGNMENT)          \
	void FastAccessF::FULL_FUNC(const TYPE_VALUE &p_val, FAParam &p_source, FAParam &p_dest, uint32_t p_num_elements) { \
		ERR_FAIL_COND(!p_source.prepare(PREPARE_SIZEOF_SOURCE, ALIGNMENT, p_num_elements));                             \
		ERR_FAIL_COND(!p_dest.prepare(PREPARE_SIZEOF_DEST, ALIGNMENT, p_num_elements));

#define FA_IMPL_VAL_TWO_TO_ONE_B(FUNC, TYPE_SOURCE, TYPE_DEST) \
	uint8_t *source = p_source.get_param().data;               \
	uint8_t *dest = p_dest.get_param().data;                   \
	for (uint32_t n = 0; n < p_num_elements; n++) {            \
		const TYPE_SOURCE &s = *(TYPE_SOURCE *)source;         \
		TYPE_DEST &d = *(TYPE_DEST *)dest;                     \
		d = s.FUNC(p_val);                                     \
		source += p_source.get_param().stride_bytes;           \
		dest += p_dest.get_param().stride_bytes;               \
	}                                                          \
	}

/////////////////////////////////////////////////////////////////////////////
// All implementations with two inputs and one output can use the same macros.

#define FA_IMPL_TWO_TO_ONE_A(FULL_FUNC, PREPARE_SIZEOF_SOURCE, PREPARE_SIZEOF_SOURCE2, PREPARE_SIZEOF_DEST, ALIGNMENT) \
	void FastAccessF::FULL_FUNC(FAParam &p_source, FAParam &p_source2, FAParam &p_dest, uint32_t p_num_elements) {     \
		ERR_FAIL_COND(!p_source.prepare(PREPARE_SIZEOF_SOURCE, ALIGNMENT, p_num_elements));                            \
		ERR_FAIL_COND(!p_source2.prepare(PREPARE_SIZEOF_SOURCE2, ALIGNMENT, p_num_elements));                          \
		ERR_FAIL_COND(!p_dest.prepare(PREPARE_SIZEOF_DEST, ALIGNMENT, p_num_elements));

#define FA_IMPL_TWO_TO_ONE_B(FUNC, TYPE_SOURCE, TYPE_SOURCE2, TYPE_DEST) \
	uint8_t *source = p_source.get_param().data;                         \
	uint8_t *source2 = p_source2.get_param().data;                       \
	uint8_t *dest = p_dest.get_param().data;                             \
	for (uint32_t n = 0; n < p_num_elements; n++) {                      \
		const TYPE_SOURCE &s = *(TYPE_SOURCE *)source;                   \
		const TYPE_SOURCE2 &s2 = *(TYPE_SOURCE2 *)source2;               \
		TYPE_DEST &d = *(TYPE_DEST *)dest;                               \
		d = s.FUNC(s2);                                                  \
		source += p_source.get_param().stride_bytes;                     \
		source2 += p_source2.get_param().stride_bytes;                   \
		dest += p_dest.get_param().stride_bytes;                         \
	}                                                                    \
	}

/////////////////////////////////////////////////////////////////////////////
// All implementations with one input and one output can use the same macros.

#define FA_IMPL_ONE_TO_ONE_A(FULL_FUNC, PREPARE_SIZEOF_SOURCE, PREPARE_SIZEOF_DEST, ALIGNMENT) \
	void FastAccessF::FULL_FUNC(FAParam &p_source, FAParam &p_dest, uint32_t p_num_elements) { \
		ERR_FAIL_COND(!p_source.prepare(PREPARE_SIZEOF_SOURCE, ALIGNMENT, p_num_elements));    \
		ERR_FAIL_COND(!p_dest.prepare(PREPARE_SIZEOF_DEST, ALIGNMENT, p_num_elements));

#define FA_IMPL_ONE_TO_ONE_B(FUNC, TYPE_SOURCE, TYPE_DEST) \
	uint8_t *source = p_source.get_param().data;           \
	uint8_t *dest = p_dest.get_param().data;               \
	for (uint32_t n = 0; n < p_num_elements; n++) {        \
		const TYPE_SOURCE &s = *(TYPE_SOURCE *)source;     \
		TYPE_DEST &d = *(TYPE_DEST *)dest;                 \
		d = s.FUNC();                                      \
		source += p_source.get_param().stride_bytes;       \
		dest += p_dest.get_param().stride_bytes;           \
	}                                                      \
	}

/////////////////////////////////////////////////////////////////////////////

#define FA_IMPL_TWO_TO_ONE_QUADS(FULL_FUNC, QUAD_FUNC, OP, ALIGNMENT)                                              \
	void FastAccessF::FULL_FUNC(FAParam &p_source, FAParam &p_source2, FAParam &p_dest, uint32_t p_num_elements) { \
		ERR_FAIL_COND(p_source.get_param().stride_bytes != sizeof(float));                                         \
		ERR_FAIL_COND(p_source2.get_param().stride_bytes != sizeof(float));                                        \
		ERR_FAIL_COND(p_dest.get_param().stride_bytes != sizeof(float));                                           \
		ERR_FAIL_COND(!p_source.prepare(sizeof(float), ALIGNMENT, p_num_elements));                                \
		ERR_FAIL_COND(!p_source2.prepare(sizeof(float), ALIGNMENT, p_num_elements));                               \
		ERR_FAIL_COND(!p_dest.prepare(sizeof(float), ALIGNMENT, p_num_elements));                                  \
		uint32_t num_quads = p_num_elements / 4;                                                                   \
		QUAD_FUNC(p_source, p_source2, p_dest, num_quads);                                                         \
		uint32_t leftover = p_num_elements % 4;                                                                    \
		if (!leftover) {                                                                                           \
			return;                                                                                                \
		}                                                                                                          \
		uint32_t quads_offset = num_quads * sizeof(f32_4);                                                         \
		uint8_t *source = p_source.get_param().data + quads_offset;                                                \
		uint8_t *source2 = p_source2.get_param().data + quads_offset;                                              \
		uint8_t *dest = p_dest.get_param().data + quads_offset;                                                    \
		for (uint32_t n = 0; n < leftover; n++) {                                                                  \
			float &d = *(float *)dest;                                                                             \
			const float &s = *(const float *)source;                                                               \
			const float &s2 = *(const float *)source2;                                                             \
			d = s;                                                                                                 \
			d OP s2;                                                                                               \
			dest += sizeof(float);                                                                                 \
			source += sizeof(float);                                                                               \
			source2 += sizeof(float);                                                                              \
		}                                                                                                          \
	}

/////////////////////////////////////////////////////////////////////////////
#define FA_IMPL_ONE_TO_ONE_QUADS(FULL_FUNC, QUAD_FUNC, OP, ALIGNMENT)                          \
	void FastAccessF::FULL_FUNC(FAParam &p_source, FAParam &p_dest, uint32_t p_num_elements) { \
		ERR_FAIL_COND(!p_source.prepare(sizeof(float), ALIGNMENT, p_num_elements));            \
		ERR_FAIL_COND(!p_dest.prepare(sizeof(float), ALIGNMENT, p_num_elements));              \
		uint32_t num_quads = p_num_elements / 4;                                               \
		QUAD_FUNC(p_source, p_dest, num_quads);                                                \
		uint32_t leftover = p_num_elements % 4;                                                \
		if (!leftover) {                                                                       \
			return;                                                                            \
		}                                                                                      \
		uint32_t quads_offset = num_quads * sizeof(f32_4);                                     \
		uint8_t *source = p_source.get_param().data + quads_offset;                            \
		uint8_t *dest = p_dest.get_param().data + quads_offset;                                \
		for (uint32_t n = 0; n < leftover; n++) {                                              \
			float &d = *(float *)dest;                                                         \
			const float &s = *(const float *)source;                                           \
			d = s;                                                                             \
			d = (OP);                                                                          \
			dest += sizeof(float);                                                             \
			source += sizeof(float);                                                           \
		}                                                                                      \
	}
