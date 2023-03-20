#include "lsynth.h"
#include "../lbus.h"

void LSynth::play_ADSR(const SegmentParams &p_seg_params) {
	const SegmentParams &p = p_seg_params;
	float vol_a = p.vol_a;
	float vol_b = p.vol_b;

	int32_t instrument_start_sample_offset = 0;
	int32_t dest_start_sample = 0;
	int32_t num_samples_to_write = 0;
	if (!p.bus->calculate_overlap(p.song_sample_from, p.dest_num_samples, p.seg_start_sample, p.seg_num_samples, dest_start_sample, instrument_start_sample_offset, num_samples_to_write, true))
		return;

	// adjust volume a and b according to how much of the original note was clipped
	float orig_vol_a = vol_a;
	if (instrument_start_sample_offset > 0) {
		// cut off samples from beginning
		float cutoff = instrument_start_sample_offset / (float)p.seg_num_samples;
		vol_a = Math::lerp(vol_a, vol_b, cutoff);
	}
	int32_t cutoff_end = (p.seg_num_samples - instrument_start_sample_offset) - num_samples_to_write;
	if (cutoff_end > 0) {
		float fract = cutoff_end / (float)p.seg_num_samples;
		vol_b = Math::lerp(vol_b, orig_vol_a, fract);
	}
	//////////////////////////////////////////////////////////////////

	Oscillator osc;
	osc.set_freq(LSample::note_to_frequency(p.key), p.bus->get_sample().get_format().sample_rate);
	osc.set_wave(data.wave);

	LSample &dest = p.bus->get_sample();

	// phase offset
	osc.set_phase(dest_start_sample + p.bus->get_song_time_start());

	for (int32_t n = 0; n < num_samples_to_write; n++) {
		int32_t dest_sample_id = dest_start_sample + n;

		float f = osc.get_sample();

		// adjust volume between vol a and vol b
		f *= Math::lerp(vol_a, vol_b, n / (float)num_samples_to_write);

		dest.mix_f(dest_sample_id, 0, f);
		dest.mix_f(dest_sample_id, 1, f);
	}
}

void LSynth::play(const PlayParams &p_play_params) {
	LBus *bus = g_Buses.get_bus(_output_bus_handle);
	if (!bus) {
		return;
	}

	play_note_ADSR(bus, p_play_params);
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

		if (!load_idata(child, p_include_paths))
			return false;
	}
	return true;
}
