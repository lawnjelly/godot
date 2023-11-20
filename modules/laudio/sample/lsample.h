#pragma once

#include "core/ustring.h"
#include <stdint.h>

class LAudioFormat {
public:
	uint32_t num_channels = 1;
	uint32_t bytes_per_channel = 2;
	uint32_t sample_rate = 44100;
	uint32_t num_samples = 0;
	uint32_t total_bytes = 0;
};

class LSample {
	LAudioFormat _format;
	uint8_t *_data = nullptr;
	bool _owned_data = true;
	String _filename;

	bool _calculate_overlap(const LSample &p_dest, int32_t &r_num_samples, int32_t &r_dest_start_sample, int32_t &r_source_start_sample);
	void _mix_channel_to(LSample &r_dest, uint32_t p_channel_from, uint32_t p_channel_to, int32_t p_num_samples, int32_t p_dest_start_sample = 0, int32_t p_source_start_sample = 0, float p_volume = 1.0f);

public:
	void create(uint32_t p_bytes_per_channel, uint32_t p_num_channels, uint32_t p_sample_rate, uint32_t p_num_samples);
	void resize(uint32_t p_num_samples, bool p_repeat = false);
	bool load_wav(String p_filename);
	bool save_wav(String p_filename);
	void reset();
	void blank();
	const LAudioFormat &get_format() const { return _format; }
	bool convert_to(uint32_t p_bytes_per_channel);

	void normalize(float p_multiplier = 1.0f);

	// All these values are RELATIVE to the SAMPLES
	void mix_to(LSample &r_dest, int32_t p_num_samples, int32_t p_dest_start_sample = 0, int32_t p_source_start_sample = 0, float p_volume = 1.0f, float p_pan = 0.0f);
	void copy_to(LSample &r_dest, int32_t p_num_samples, int32_t p_dest_start_sample = 0, int32_t p_source_start_sample = 0);

	void mix_f(uint32_t p_sample, uint32_t p_channel, float p_value) {
		DEV_ASSERT(p_sample < _format.num_samples);
		DEV_ASSERT(_data);
		DEV_ASSERT(p_channel < _format.num_channels);
		DEV_ASSERT(_format.bytes_per_channel == 4);

		float *p = (float *)_data;
		p[(p_sample * _format.num_channels) + p_channel] += p_value;
	}

	void set_f(uint32_t p_sample, uint32_t p_channel, float p_value) {
		DEV_ASSERT(p_sample < _format.num_samples);
		DEV_ASSERT(_data);
		DEV_ASSERT(p_channel < _format.num_channels);
		DEV_ASSERT(_format.bytes_per_channel == 4);

		float *p = (float *)_data;
		p[(p_sample * _format.num_channels) + p_channel] = p_value;
	}

	void mix_16(uint32_t p_sample, uint32_t p_channel, float p_value) {
		DEV_ASSERT(p_sample < _format.num_samples);
		DEV_ASSERT(_data);
		DEV_ASSERT(p_channel < _format.num_channels);
		DEV_ASSERT(_format.bytes_per_channel == 2);

		int16_t *p = (int16_t *)_data;
		int32_t val = p_value * INT16_MAX;
		val = CLAMP(val, -INT16_MAX, INT16_MAX);

		int16_t &sample = p[(p_sample * _format.num_channels) + p_channel];
		sample = CLAMP((int32_t)val + sample, -INT16_MAX, INT16_MAX);
	}

	uint8_t *get_data(uint32_t p_sample, uint32_t p_channel = 0) {
		DEV_ASSERT(p_sample < _format.num_samples);
		DEV_ASSERT(_data);

		switch (_format.bytes_per_channel) {
			case 1: {
				int8_t *p = (int8_t *)_data;
				return (uint8_t *)&p[(p_sample * _format.num_channels) + p_channel];
			} break;
			case 2: {
				int16_t *p = (int16_t *)_data;
				return (uint8_t *)&p[(p_sample * _format.num_channels) + p_channel];
			} break;
			case 3: {
				uint8_t *p = (uint8_t *)_data;
				p += ((p_sample * _format.num_channels) + p_channel) * 3;
				return (uint8_t *)p;
			} break;
			case 4: {
				float *p = (float *)_data;
				return (uint8_t *)&p[(p_sample * _format.num_channels) + p_channel];
			} break;
			default: {
				// not supported
				ERR_FAIL_V(nullptr);
			} break;
		}
		return nullptr;
	}

	float get_f_interpolated(uint32_t p_sample, float p_fraction, uint32_t p_channel = 0, bool p_loop = true) const {
		float s0 = safe_get_f(p_sample, p_channel, p_loop);
		float s1 = safe_get_f(p_sample + 1, p_channel, p_loop);

		// Linear for now.
		return s0 + ((s1 - s0) * p_fraction);
	}

	//	float get_f_through_whole(float p_position, uint32_t p_channel = 0, bool p_loop = false) const {
	//		DEV_ASSERT(_format.num_samples);
	//		float f = p_position * _format.num_samples;
	//		uint32_t samp = f;
	//		f = f - samp;
	//		return get_f_interpolated(samp, f, p_channel, p_loop);
	//	}

	float safe_get_f(uint32_t p_sample, uint32_t p_channel = 0, bool p_loop = false) const {
		DEV_ASSERT(_format.num_samples);
		if (p_loop) {
			p_sample %= _format.num_samples;
		} else {
			p_sample = MIN(p_sample, _format.num_samples - 1);
		}
		return get_f(p_sample, p_channel);
	}

	float get_f(uint32_t p_sample, uint32_t p_channel = 0) const {
		DEV_ASSERT(p_sample < _format.num_samples);
		DEV_ASSERT(_data);

		switch (_format.bytes_per_channel) {
			case 1: {
				int8_t *p = (int8_t *)_data;
				int8_t val = p[(p_sample * _format.num_channels) + p_channel];
				float f = val;
				f /= -INT8_MIN;
				return f;
			} break;
			case 2: {
				int16_t *p = (int16_t *)_data;
				int16_t val = p[(p_sample * _format.num_channels) + p_channel];
				float f = val;
				// we lose the +1.0, because negative range is higher than positive
				f /= -INT16_MIN;
				return f;
			} break;
			case 3: {
				union s24 {
					uint8_t c[4];
					int32_t val;
				} s;

				uint8_t *p = (uint8_t *)_data;
				p += ((p_sample * _format.num_channels) + p_channel) * 3;
				s.c[0] = 0;
				s.c[1] = *p++;
				s.c[2] = *p++;
				s.c[3] = *p++;

				float f = s.val;
				// we lose the +1.0, because negative range is higher than positive
				f /= (-(int64_t)INT32_MIN);
				return f;
			} break;
			case 4: {
				float *p = (float *)_data;
				float f = p[(p_sample * _format.num_channels) + p_channel];
				return f;
			} break;
			default: {
				// not supported
				ERR_FAIL_V(0.0f);
			} break;
		}
		return 0.0f;
	}

	static double note_to_frequency(int32_t p_note);

	LSample();
	~LSample();
};
