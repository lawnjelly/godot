#pragma once

#include "lhandle.h"

namespace LSon {
struct Node;
};

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
	uint32_t ticks_per_minute;

	// ticks per quarter note (we use tick terminology rather than pulses)
	uint32_t tpqn;
	uint32_t bpm;
	uint32_t sample_rate;
	uint32_t samples_pqn;
	uint32_t samples_pt;

	uint64_t samples_at_limit_ticks;

	// loops within the pattern view
	uint32_t transport_tick_left;
	uint32_t transport_tick_right;

	uint32_t song_length_ticks;

	uint32_t tick_to_sample(uint32_t p_tick) const {
		return p_tick * samples_pt;
	}

	bool load(LSon::Node *p_data);
	bool save(LSon::Node *p_root);

	uint32_t get_transport_left_sample() const { return tick_to_sample(transport_tick_left); }
	uint32_t get_transport_right_sample() const { return tick_to_sample(transport_tick_right); }

	void reset() {
		ticks_per_minute = 0;

		tpqn = 24;
		bpm = 120000;
		sample_rate = 44100;
		transport_tick_left = 0;
		transport_tick_right = 192;
		song_length_ticks = 192;
		recalculate();
	}

	uint32_t get_tick_sample(uint32_t p_tick) const {
		return ((uint64_t)sample_rate * 60 * p_tick) / ticks_per_minute;
	}

	void recalculate() {
		// derive ticks per minute from tpqn and bpm
		ticks_per_minute = ((uint64_t)tpqn * 2 * bpm) / 1000;

		// calculate number of samples required to do LIMIT ticks.
		// This gives us an accurate way to describe tempo over a long song.

		//	samples_pqn = ((sample_rate * 120) / bpm)) / (60 * 8);
		samples_pqn = (((uint64_t)sample_rate * 120000) / bpm) / 4;
		samples_pt = samples_pqn / tpqn;
		print_line("recalculated samples_pt " + itos(samples_pt));
		print_line("recalculated samples_pqn " + itos(samples_pqn));
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
