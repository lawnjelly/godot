#pragma once

#include "core/local_vector.h"
#include "lstructs.h"
#include <stdint.h>

class LPattern {
public:
	LocalVector<LNote> notes;

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
	LNote *get_note(uint32_t p_id) {
		if (p_id < notes.size()) {
			return &notes[p_id];
		}
		return nullptr;
	}

	uint32_t create_note(uint32_t p_after_note) {
		if (p_after_note < notes.size()) {
			LNote old = notes[p_after_note];
			notes.insert(p_after_note, old);
			return p_after_note + 1;
		}

		LNote newnote;
		notes.insert(0, newnote);
		return 0;
	}

	void sort_notes() {
		notes.sort();
	}

	bool release() {
		data.refcount--;
		DEV_ASSERT(data.refcount >= 0);
		return data.refcount == 0;
	}
	void reference() {
		data.refcount++;
	}
};
