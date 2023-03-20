#pragma once

#include "../player/linstrument.h"

class LBus;

class LSynth : public LInstrument {
	enum Wave {
		WAVE_SINE,
		WAVE_SAWTOOTH,
		WAVE_SQUARE,
	};

	class Oscillator {
		Wave wave = WAVE_SINE;
		double phase = 0;
		double phase_increment = 0;

	public:
		void set_wave(Wave p_wave) { wave = p_wave; }
		void set_freq(double p_freq, uint32_t p_sample_rate) {
			phase_increment = (1.0 / p_sample_rate) * p_freq;
		}
		void set_phase(uint32_t p_sample) {
			phase = phase_increment * p_sample;
			phase = Math::fmod(phase, 1.0);
		}
		float get_sample() {
			float f;
			switch (wave) {
				case WAVE_SAWTOOTH: {
					f = phase;
					f *= 2.0;
					f -= 1.0;
				} break;
				case WAVE_SQUARE: {
					f = (phase < 0.5) ? 1.0 : -1.0;
				} break;
				default: {
					f = sin(phase * Math_TAU);
				} break;
			}

			// increment phase
			phase += phase_increment;
			if (phase >= 1.0) {
				phase -= 1.0;
			}
			return f;
		}
	};

	struct Data {
		Wave wave = WAVE_SINE;
	} data;

	virtual void play_ADSR(const PlayParams &p_play_params);

public:
	virtual const char *get_type_name() const { return "Synth"; }
	virtual void play(const PlayParams &p_play_params);

	virtual bool load(LSon::Node *p_data, const LocalVector<String> &p_include_paths);
	virtual int32_t get_max_release_time() const { return 44100; } // in samples
};
