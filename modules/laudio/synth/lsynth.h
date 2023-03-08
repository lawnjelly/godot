#pragma once

#include "../player/linstrument.h"

class LBus;

class LSynth : public LInstrument {
	enum Wave {
		WAVE_SINE,
		WAVE_SAWTOOTH,
		WAVE_SQUARE,
	};

	struct Data {
		Wave wave = WAVE_SINE;
		float attack = 0.1;
		float release = 0.2;
		float decay = 0.5;
		float sustain = 1.0;
	} data;

	void play_ADSR(LBus *p_bus, uint32_t p_key, int32_t p_song_sample_from, int32_t p_dest_num_samples, int32_t p_note_start_sample, int32_t p_note_num_samples, float p_vol_a, float p_vol_b);

public:
	virtual const char *get_type_name() const { return "Synth"; }
	virtual void play(uint32_t p_key, int32_t p_song_sample_from, int32_t p_dest_num_samples, int32_t p_note_start_sample, int32_t p_note_num_samples);

	virtual bool load(LSon::Node *p_data, const LocalVector<String> &p_include_paths);
	virtual int32_t get_max_release_time() const { return 44100; } // in samples
};
