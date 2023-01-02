#include "lpattern.h"

void LPattern::calculate_length() {
	if (!_notes.size()) {
		_data.reset();
		return;
	}

	_data.tick_start = INT32_MAX;
	_data.tick_end = INT32_MIN;
	_data.tick_length = 0;

	for (uint32_t n = 0; n < _notes.size(); n++) {
		const LNote &note = _notes[n];
		int32_t start = note.tick_start;
		int32_t end = start + note.tick_length;
		_data.tick_start = MIN(_data.tick_start, start);
		_data.tick_end = MAX(_data.tick_end, end);
	}

	if (_data.tick_end > _data.tick_start) {
		_data.tick_length = _data.tick_end - _data.tick_start;
	}
}
