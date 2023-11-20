#pragma once

#include "../sample/lsample.h"
#include "core/reference.h"

class LAnalyzer : public Reference {
	GDCLASS(LAnalyzer, Reference);

	struct Wave {
		uint32_t start = 0;
		uint32_t length = 0;
		//int32_t phase_offset = 0;
		float ramp = 0;
		float start_offset = 0;
		float end_offset = 0;
		float loudest_sample = 0;

		// Derived from the original length and the new length
		float get_frequency() const { return (num_waves / (float)length); }
		//		float get_frequency(uint32_t p_out_rate) const { return (wavedata_length / (float)length) / wavedata_length; }
		//float get_frequency() const {return (length / (float) wavedata_length);}

		uint32_t wavedata_start = 0;
		uint32_t wavedata_length = 0;
		uint32_t num_waves = 1;

		float get_f(const LSample &p_sample, float p_phase) const {
			p_phase = Math::fmod(p_phase, (float)num_waves);
			p_phase /= num_waves;
			float f = wavedata_length * p_phase;
			uint32_t pos = f;
			f -= pos;
			return p_sample.get_f_interpolated(wavedata_start + pos, f);
		}
	};

	struct Data {
		LSample source_sample;
		LocalVector<Wave> waves;
		LSample wavedata;
		float best_fit = 0;
		uint32_t best_window = 0;
	} data;

	bool does_crossing_phase_match_existing(const LSample &p_sample, uint32_t p_start);
	float calculate_error(const Wave &p_wave, const LSample &p_sample, uint32_t p_start);
	uint32_t _find_next_crossing_flip(const LSample &p_sample, uint32_t p_start, float &r_s0, float &r_s1, float p_threshold = 0);
	uint32_t find_next_crossing(const LSample &p_sample, uint32_t p_start, Wave &r_wave);
	bool autocorrelate(const LSample &p_sample, uint32_t p_start, uint32_t p_max_length, Wave &r_wave);
	bool autocorrelate_find_end_offset(const LSample &p_sample, uint32_t p_start, uint32_t p_length, Wave &r_wave);
	void reconstruct();
	String itof(float f) const { return String(Variant(f)); }

	void create_wavedata();
	void create_dummy_data();
	void count_waves(Wave &r_wave);
	void count_waves_from_source(const LSample &p_sample, Wave &r_wave);

	void output_waves();

	float mix_waves(const Wave &p_wave_a, const Wave &p_wave_b, float p_fraction, float p_relative_volume);
	float get_source_wave_f(const Wave &p_wave, float p_fraction);
	float get_wave_f(const Wave &p_wave, float p_fraction);

	void log(String p_sz);

protected:
	static void _bind_methods();

public:
	bool load_sample(String p_filename);
	void finalize();
	int32_t process_sample(int32_t p_pos);

	int32_t get_sample_length();
	float get_sample(int32_t p_pos);
	int32_t get_num_waves();
	int32_t get_wave_start(int32_t p_wave);
	int32_t get_wave_length(int32_t p_wave);
};
