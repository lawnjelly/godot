#include "lsynth.h"
#include "../lbus.h"

void LSynth::play_ADSR(LBus *p_bus, uint32_t p_key, int32_t p_song_sample_from, int32_t p_dest_num_samples, int32_t p_note_start_sample, int32_t p_note_num_samples, float p_vol_a, float p_vol_b) {
	int32_t instrument_start_sample_offset = 0;
	int32_t dest_start_sample = 0;
	int32_t num_samples_to_write = 0;
	if (!p_bus->calculate_overlap(p_song_sample_from, p_dest_num_samples, p_note_start_sample, p_note_num_samples, dest_start_sample, instrument_start_sample_offset, num_samples_to_write, true))
		return;

	// adjust volume a and b according to how much of the original note was clipped
	float orig_vol_a = p_vol_a;
	if (instrument_start_sample_offset > 0) {
		// cut off samples from beginning
		float cutoff = instrument_start_sample_offset / (float)p_note_num_samples;
		p_vol_a = Math::lerp(p_vol_a, p_vol_b, cutoff);
	}
	int32_t cutoff_end = (p_note_num_samples - instrument_start_sample_offset) - num_samples_to_write;
	if (cutoff_end > 0) {
		float fract = cutoff_end / (float)p_note_num_samples;
		p_vol_b = Math::lerp(p_vol_b, orig_vol_a, fract);
	}
	//////////////////////////////////////////////////////////////////

	double mult = 1.0 / p_bus->get_sample().get_format().sample_rate;
	double freq = LSample::note_to_frequency(p_key);
	mult *= freq;
	if (data.wave == WAVE_SINE) {
		mult *= Math_TAU;
	}

	LSample &dest = p_bus->get_sample();

	// phase offset
	//	int32_t phase = dest_start_sample + p_song_sample_from;
	int32_t phase = dest_start_sample + p_bus->get_song_time_start();
	//print_line("phase " + itos(phase));

	for (int32_t n = 0; n < num_samples_to_write; n++) {
		int32_t dest_sample_id = dest_start_sample + n;

		double secs = (n + phase) * mult;

		float f;

		switch (data.wave) {
			case WAVE_SAWTOOTH: {
				f = Math::fmod(secs, 1.0);
				f *= 2.0;
				f -= 1.0;
			} break;
			case WAVE_SQUARE: {
				int32_t i = secs;
				f = ((i % 2) == 0) ? 1.0 : -1.0;
			} break;
			default: {
				f = sin(secs);
			} break;
		}

		// adjust volume between vol a and vol b
		f *= Math::lerp(p_vol_a, p_vol_b, n / (float)num_samples_to_write);

		dest.mix_f(dest_sample_id, 0, f);
		dest.mix_f(dest_sample_id, 1, f);
	}
}

void LSynth::play(uint32_t p_key, int32_t p_song_sample_from, int32_t p_dest_num_samples, int32_t p_note_start_sample, int32_t p_note_num_samples) {
	LBus *bus = g_Buses.get_bus(_output_bus_handle);
	if (!bus) {
		return;
	}

	if (!p_note_num_samples)
		return;

	uint32_t sample_rate = bus->get_sample().get_format().sample_rate;
	float volume = idata.volume;

	//	play_ADSR(bus, p_key, p_song_sample_from, p_dest_num_samples, p_note_start_sample, p_note_num_samples, volume, 0);
	//	return;

	// ATTACK
	//	int32_t note_start = p_note_start_sample - p_song_sample_from;
	//	int32_t note_start = p_note_start_sample - bus->get_song_time_start();
	int32_t note_start = p_note_start_sample;
	int32_t note_length = p_note_num_samples;

	int32_t attack_length = data.attack * sample_rate;

	int32_t decay_start = attack_length;
	int32_t decay_length = data.decay * sample_rate;
	int32_t sustain_start = decay_start + decay_length;

	// note this can be negative if we never got to sustain
	int32_t sustain_length = note_length - sustain_start;
	float sustain_volume = data.sustain * idata.volume;

	// Most common case, the attack is fully used
	if (note_length >= attack_length) {
		play_ADSR(bus, p_key, p_song_sample_from, p_dest_num_samples, note_start, attack_length, 0, volume);
	} else {
		// fraction of full volume?
		volume *= (p_note_num_samples / (float)attack_length);
		play_ADSR(bus, p_key, p_song_sample_from, p_dest_num_samples, note_start, note_length, 0, volume);
		goto RELEASE;
	}

	// DECAY

	// Whole decay
	if (sustain_length >= 0) {
		play_ADSR(bus, p_key, p_song_sample_from, p_dest_num_samples, note_start + decay_start, decay_length, volume, sustain_volume);
		volume = sustain_volume;
	} else {
		// fraction through to sustain volume
		float fract = (note_length - decay_start) / (float)decay_length;
		sustain_volume = Math::lerp(volume, sustain_volume, fract);
		play_ADSR(bus, p_key, p_song_sample_from, p_dest_num_samples, note_start + decay_start, note_length - decay_start, volume, sustain_volume);
		volume = sustain_volume;
		goto RELEASE;
	}

	// SUSTAIN
	play_ADSR(bus, p_key, p_song_sample_from, p_dest_num_samples, note_start + sustain_start, note_length - sustain_start, volume, volume);

RELEASE:
	// RELEASE
	int32_t release_start = note_length;
	int32_t release_length = data.release * sample_rate;
	play_ADSR(bus, p_key, p_song_sample_from, p_dest_num_samples, note_start + release_start, release_length, volume, 0);
}

bool LSynth::load(LSon::Node *p_data, const LocalVector<String> &p_include_paths) {
	for (uint32_t c = 0; c < p_data->children.size(); c++) {
		LSon::Node *child = p_data->get_child(c);

		if (child->name == "wave") {
			print_line("wave is " + child->string);
			String sz;
			if (!child->get_string(sz))
				return false;

			if (sz == "sine") {
				data.wave = WAVE_SINE;
			}
			if (sz == "sawtooth") {
				data.wave = WAVE_SAWTOOTH;
			}
			if (sz == "square") {
				data.wave = WAVE_SQUARE;
			}
		}

		if (child->name == "attack") {
			if (!child->get_f32(data.attack))
				return false;
		}
		if (child->name == "release") {
			if (!child->get_f32(data.release))
				return false;
		}
		if (child->name == "decay") {
			if (!child->get_f32(data.decay))
				return false;
		}
		if (child->name == "sustain") {
			if (!child->get_f32(data.sustain))
				return false;
		}
	}
	return true;
}
