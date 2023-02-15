#pragma once

#include "core/local_vector.h"
#include "lstructs.h"
#include <stdint.h>

class LPattern {
	LocalVector<LNote> _notes;

public:
	struct Data {
		int32_t refcount = 0;

		// the first note can be BEFORE the pattern start,
		// or after. It doesn't have to occur exactly at pattern start.
		int32_t tick_start = 0;
		int32_t tick_length = 256;
		int32_t tick_end() const { return tick_start + tick_length; }

		String name = "unnamed";
		LHandle handle;

		void reset() {
			tick_start = 0;
			tick_length = 0;
		}
	} data;

	void calculate_length();
	String get_name() const { return data.name; }

	bool release() {
		data.refcount--;
		DEV_ASSERT(data.refcount >= 0);
		return data.refcount == 0;
	}
	void reference() {
		data.refcount++;
	}
};
