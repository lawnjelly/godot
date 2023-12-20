#include "fast_access_base.h"

using namespace GSimd;

void FastAccessBase::copy_data_to(uint8_t *r_dest, uint32_t p_offset, uint32_t p_num_bytes) {
	ERR_FAIL_NULL(r_dest);
	ERR_FAIL_COND(!bounds_check(p_offset, 1, p_num_bytes, 1));
	const uint8_t *p = &_data[p_offset];
	memcpy(r_dest, p, p_num_bytes);
}

void FastAccessBase::copy_data_from(const uint8_t *p_source, uint32_t p_offset, uint32_t p_num_bytes) {
	ERR_FAIL_NULL(p_source);
	ERR_FAIL_COND(!bounds_check(p_offset, 1, p_num_bytes, 1));
	uint8_t *p = &_data[p_offset];
	memcpy(p, p_source, p_num_bytes);
}
