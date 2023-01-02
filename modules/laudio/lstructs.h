#pragma once

#include <stdint.h>

struct LTiming {
	// ticks per quarter note (we use tick terminology rather than pulses)
	uint32_t tpqn = 192;
	uint32_t bpm = 120;
	uint32_t sample_rate = 44100;
	uint32_t samples_pqn = (sample_rate * bpm) / 60;
	uint32_t samples_pt = samples_pqn / tpqn;

	uint32_t tick_to_sample(uint32_t p_tick) const {
		return p_tick * samples_pt;
	}
};

struct LNote {
	int32_t tick_start;
	uint32_t tick_length;
	uint16_t note;
	uint16_t velocity;
};
