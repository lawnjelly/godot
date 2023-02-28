#pragma once

#include "core/local_vector.h"
#include "lstructs.h"
#include <stdint.h>

class LSong;

class LPattern {
public:
	LocalVector<LNote> notes;

	struct Data {
		int32_t refcount = 0;

		// the first note can be BEFORE the pattern start,
		// or after. It doesn't have to occur exactly at pattern start.
		int32_t tick_start = 0;
		int32_t tick_length = 192;
		int32_t tick_end() const { return tick_start + tick_length; }

		int32_t player_a = 0;
		int32_t player_b = 0;
		int32_t player_c = 0;
		int32_t player_d = 0;

		int32_t transpose = 0;

		int32_t time_sig_micro = 6;
		int32_t time_sig_minor = 8;
		int32_t time_sig_major = 16;

		String name = "unnamed";
		LHandle handle;

		void reset() {
			tick_start = 0;
			tick_length = 192;
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

	uint32_t sort_notes(uint32_t p_old_selected_note);

	bool play(LSong &p_song, uint32_t p_output_bus_handle, uint32_t p_start_sample, uint32_t p_num_samples, uint32_t p_samples_per_tick, uint32_t p_pattern_start_tick) const;

	bool release() {
		data.refcount--;
		DEV_ASSERT(data.refcount >= 0);
		return data.refcount == 0;
	}
	void reference() {
		data.refcount++;
	}
};
