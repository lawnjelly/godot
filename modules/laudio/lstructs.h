#pragma once

#include "lhandle.h"

typedef Handle_24_8 LHandle;

struct LTrack {
	String name;
	bool active = true;
};

struct LTracks {
	enum { MAX_TRACKS = 32 };
	LTrack tracks[MAX_TRACKS];

	LTracks() {
		reset();
	}

	void reset() {
		for (uint32_t n = 0; n < MAX_TRACKS; n++) {
			tracks[n].name = "Track " + itos(n);
			tracks[n].active = true;
		}
	}
};

struct LTiming {
	// ticks per quarter note (we use tick terminology rather than pulses)
	uint32_t tpqn;
	uint32_t bpm;
	uint32_t sample_rate;
	uint32_t samples_pqn;
	uint32_t samples_pt;

	uint32_t tick_to_sample(uint32_t p_tick) const {
		return p_tick * samples_pt;
	}

	void reset() {
		tpqn = 192;
		bpm = 120;
		sample_rate = 44100;
		samples_pqn = (sample_rate * bpm) / 60;
		samples_pt = samples_pqn / tpqn;
	}
	LTiming() {
		reset();
	}
};

struct LNote {
	int32_t tick_start = 0;
	int32_t tick_length = 32;
	int32_t note = 60;
	int32_t velocity = 100;
	int32_t player = 0;
	bool operator<(const LNote &p_other) const {
		return tick_start < p_other.tick_start;
	}
};
