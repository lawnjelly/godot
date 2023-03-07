#include "lsample.h"
#include "core/os/memory.h"
#include "lwav_importer.h"

void LSample::create(uint32_t p_bytes_per_channel, uint32_t p_num_channels, uint32_t p_sample_rate, uint32_t p_num_samples) {
	reset();

	// arbitrary limits to prevent duff params

	ERR_FAIL_COND((p_num_channels <= 0) || (p_num_channels > 8));
	ERR_FAIL_COND(p_sample_rate < 4096);
	ERR_FAIL_COND(p_sample_rate > (44100 * 4));
	ERR_FAIL_COND(p_num_samples == 0);
	ERR_FAIL_COND(p_num_samples > (UINT32_MAX / 16));
	ERR_FAIL_COND(p_bytes_per_channel == 0);
	ERR_FAIL_COND(p_bytes_per_channel > 4);
	ERR_FAIL_COND(p_bytes_per_channel == 3);

	_format.bytes_per_channel = p_bytes_per_channel;
	_format.num_channels = p_num_channels;
	_format.num_samples = p_num_samples;
	_format.sample_rate = p_sample_rate;

	_format.total_bytes = _format.bytes_per_channel * _format.num_channels * _format.num_samples;

	_data = (uint8_t *)memalloc(_format.total_bytes);
	blank();
}

void LSample::blank() {
	if (_data) {
		memset(_data, 0, _format.total_bytes);
	}
}

bool LSample::save_wav(String p_filename) {
	//	LSample temp;
	//	temp.create(2, 2, _format.sample_rate, _format.num_samples);
	//	mix_to(temp, _format.num_samples);

	Sound::LWAVImporter wav;

	return wav.save(p_filename, _data, _format.num_samples, _format.num_channels, _format.sample_rate, _format.bytes_per_channel);
	//return wav.save(p_filename, temp._data, temp._format.num_samples, temp._format.num_channels, temp._format.sample_rate, temp._format.bytes_per_channel);
}

bool LSample::load_wav(String p_filename) {
	_filename = "";

	Sound::LWAVImporter wav;
	_data = wav.load(p_filename, _format.num_samples, _format.num_channels, _format.sample_rate, _format.bytes_per_channel);

	if (!_data) {
		return false;
	}

	_filename = p_filename;
	_format.total_bytes = _format.bytes_per_channel * _format.num_channels * _format.num_samples;
	return true;
}

void LSample::reset() {
	if (_data && _owned_data) {
		memfree(_data);
		_data = nullptr;
	}
	_owned_data = true;
	_data = nullptr;
}

bool LSample::_calculate_overlap(const LSample &p_dest, int32_t &r_num_samples, int32_t &r_dest_start_sample, int32_t &r_source_start_sample) {
	// chop according to destination
	if (r_dest_start_sample < 0) {
		int32_t chop = -r_dest_start_sample;
		r_num_samples -= chop;
		r_dest_start_sample = 0;
		r_source_start_sample += chop;
	}
	if (r_num_samples <= 0)
		return false;

	int32_t dest_end_sample = r_dest_start_sample + r_num_samples;
	if (dest_end_sample > (int32_t)p_dest._format.num_samples) {
		int32_t chop = dest_end_sample - p_dest._format.num_samples;
		r_num_samples -= chop;
	}

	if (r_num_samples <= 0)
		return false;

	// chop according to source
	if (r_source_start_sample < 0) {
		int32_t chop = -r_source_start_sample;
		r_num_samples -= chop;
		//		r_source_start_sample = 0;
		r_source_start_sample = chop;
		//r_dest_start_sample += chop;
	}
	if (r_num_samples <= 0)
		return false;

	int32_t source_end_sample = r_source_start_sample + r_num_samples;
	if (source_end_sample > (int32_t)_format.num_samples) {
		int32_t chop = source_end_sample - _format.num_samples;
		r_num_samples -= chop;
	}
	if (r_num_samples <= 0)
		return false;

	return true;
}

void LSample::_mix_channel_to(LSample &r_dest, uint32_t p_channel_from, uint32_t p_channel_to, int32_t p_num_samples, int32_t p_dest_start_sample, int32_t p_source_start_sample, float p_volume) {
	//print_line("writing to channel " + itos(p_channel_to));

	switch (r_dest._format.bytes_per_channel) {
		case 4: {
			for (int32_t n = 0; n < p_num_samples; n++) {
				float f = get_f(p_source_start_sample + n, p_channel_from);
				f *= p_volume;
				r_dest.set_f(p_dest_start_sample + n, p_channel_to, f);
				//print_line("\twriting " + String(Variant(f)));
#ifdef DEV_ENABLED
//			float comp = 	r_dest.get_f(p_dest_start_sample + n, p_channel_to);
//			DEV_ASSERT(comp == f);
#endif
			}
		} break;
		case 2: {
			for (int32_t n = 0; n < p_num_samples; n++) {
				float f = get_f(p_source_start_sample + n, p_channel_from);
				f *= p_volume;
				r_dest.set_16(p_dest_start_sample + n, p_channel_to, f);
				//print_line("\twriting " + String(Variant(f)));
			}
		} break;

		default: {
			DEV_ASSERT(0);
		} break;
	}
}

void LSample::copy_to(LSample &r_dest, int32_t p_num_samples, int32_t p_dest_start_sample, int32_t p_source_start_sample) {
	// reduce num samples and start sample etc to get overlap
	if (!_calculate_overlap(r_dest, p_num_samples, p_dest_start_sample, p_source_start_sample))
		return;

	memcpy(r_dest.get_data(p_dest_start_sample), get_data(p_source_start_sample), p_num_samples * _format.bytes_per_channel * _format.num_channels);
}

void LSample::mix_to(LSample &r_dest, int32_t p_num_samples, int32_t p_dest_start_sample, int32_t p_source_start_sample, float p_volume, float p_pan) {
	//ERR_FAIL_COND(r_dest._format.bytes_per_channel != 4);

	// reduce num samples and start sample etc to get overlap
	if (!_calculate_overlap(r_dest, p_num_samples, p_dest_start_sample, p_source_start_sample))
		return;

	switch (r_dest._format.num_channels) {
		case 2: {
			float vol_left = p_pan > 0.0f ? p_volume * (1.0f - p_pan) : p_volume;
			float vol_right = p_pan < 0.0f ? p_volume * (1.0f + p_pan) : p_volume;

			switch (_format.num_channels) {
				case 2: {
					_mix_channel_to(r_dest, 0, 0, p_num_samples, p_dest_start_sample, p_source_start_sample, vol_left);
					_mix_channel_to(r_dest, 1, 1, p_num_samples, p_dest_start_sample, p_source_start_sample, vol_right);
				} break;
				case 1: {
					_mix_channel_to(r_dest, 0, 0, p_num_samples, p_dest_start_sample, p_source_start_sample, vol_left);
					_mix_channel_to(r_dest, 0, 1, p_num_samples, p_dest_start_sample, p_source_start_sample, vol_right);
				} break;
				default: {
					ERR_FAIL_MSG("num channels must be 1 or 2.");
				} break;
			} // switch our num channels
		} break;
		case 1: {
			switch (_format.num_channels) {
				case 2: {
					_mix_channel_to(r_dest, 0, 0, p_num_samples, p_dest_start_sample, p_source_start_sample, p_volume * 0.5f);
					_mix_channel_to(r_dest, 1, 0, p_num_samples, p_dest_start_sample, p_source_start_sample, p_volume * 0.5f);
				} break;
				case 1: {
					_mix_channel_to(r_dest, 0, 0, p_num_samples, p_dest_start_sample, p_source_start_sample, p_volume);
				} break;
				default: {
					ERR_FAIL_MSG("num channels must be 1 or 2.");
				} break;
			} // switch our num channels

		} break;
		default: {
			ERR_FAIL_MSG("num channels must be 1 or 2.");
		} break;
	}
}

double LSample::note_to_frequency(int32_t p_note) {
	//	const uint32_t midi_note_at_zero_volts = 12;
	const int32_t a4_midi_note = 69;
	const double a4_frequency = 440.0;
	const double semitones_per_octave = 12.0;
	//	const float volts_per_semitone = 1.0f / semitones_per_octave;

	double semitones_away_from_a4 = p_note - a4_midi_note;
	return Math::pow(2.0, semitones_away_from_a4 / semitones_per_octave) * a4_frequency;
}

LSample::LSample() {
}

LSample::~LSample() {
	reset();
}
