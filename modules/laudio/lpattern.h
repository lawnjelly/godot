#pragma once

#include "core/local_vector.h"
#include "lstructs.h"
#include <stdint.h>

class LPattern {
	LocalVector<LNote> _notes;

public:
	struct Data {
		// the first note can be BEFORE the pattern start,
		// or after. It doesn't have to occur exactly at pattern start.
		int32_t tick_start = 0;
		int32_t tick_end = 0;
		uint32_t tick_length = 0;

		void reset() {
			tick_start = 0;
			tick_end = 0;
			tick_length = 0;
		}
	} _data;

	void calculate_length();
};
