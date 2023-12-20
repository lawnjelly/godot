#pragma once

#include <stdint.h>

struct SIMDParam {
	uint8_t *data = nullptr; // Taking into account offset.
	uint32_t stride_bytes = 0;
	uint32_t stride_units = 0; // 32 bit floats etc, NOT the 4 vec.
};
