#include "lsynth.h"
#include "../lbus.h"

void LSynth::play(uint32_t p_key, int32_t p_song_sample_from, int32_t p_dest_num_samples, int32_t p_note_start_sample, int32_t p_note_num_samples) {
	LBus *bus = g_Buses.get_bus(_output_bus_handle);
	if (!bus) {
		return;
	}

	int32_t instrument_start_sample_offset = 0;
	int32_t dest_start_sample = 0;
	if (!bus->calculate_overlap(p_song_sample_from, p_dest_num_samples, p_note_start_sample, p_note_num_samples, dest_start_sample, instrument_start_sample_offset))
		return;

	double mult = 1.0 / bus->get_sample().get_format().sample_rate;
	double freq = LSample::note_to_frequency(p_key);
	mult *= freq * Math_TAU;

	LSample &dest = bus->get_sample();

	// phase offset
	int32_t phase = dest_start_sample + p_song_sample_from;

	for (int32_t n = 0; n < p_dest_num_samples; n++) {
		int32_t dest_sample_id = dest_start_sample + n;

		double secs = (n + phase) * mult;
		float f = sin(secs);

		dest.set_f(dest_sample_id, 0, f);
		dest.set_f(dest_sample_id, 1, f);
	}
}

bool LSynth::load(LSon::Node *p_data, const LocalVector<String> &p_include_paths) {
	return true;
}
