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

uint32_t LPattern::sort_notes(uint32_t p_old_selected_note) {
	// keep the current note ID up to date
	uint32_t note_id = p_old_selected_note;

	bool moved = true;
	while (moved) {
		moved = false;

		const LNote &curr = notes[note_id];
		if (note_id > 0) {
			// try moving back
			const LNote &prev = notes[note_id - 1];
			if (prev.tick_start > curr.tick_start) {
				SWAP(notes[note_id], notes[note_id - 1]);
				note_id--;
				moved = true;
				continue;
			}
		}
		if (note_id < notes.size() - 1) {
			// try moving forward
			const LNote &next = notes[note_id + 1];
			if (next.tick_start < curr.tick_start) {
				SWAP(notes[note_id], notes[note_id + 1]);
				note_id++;
				moved = true;
				continue;
			}
		}
	}
	//notes.sort();
	return note_id;
}
