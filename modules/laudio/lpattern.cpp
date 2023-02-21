#include "lpattern.h"

void LPattern::calculate_length() {
	if (!notes.size()) {
		data.reset();
		return;
	}

	data.tick_start = INT32_MAX;
	int32_t tick_end = INT32_MIN;
	data.tick_length = 0;

	for (uint32_t n = 0; n < notes.size(); n++) {
		const LNote &note = notes[n];
		int32_t start = note.tick_start;
		int32_t end = start + note.tick_length;
		data.tick_start = MIN(data.tick_start, start);
		tick_end = MAX(tick_end, end);
	}

	if (tick_end > data.tick_start) {
		data.tick_length = tick_end - data.tick_start;
	}
}
