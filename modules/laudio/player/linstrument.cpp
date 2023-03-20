#include "linstrument.h"

bool LInstrument::load_idata(LSon::Node *p_node, const LocalVector<String> &p_include_paths) {
	LSon::Node *c = p_node;

	if (c->name == "attack") {
		if (!c->get_f32(idata.attack))
			return false;
	}
	if (c->name == "release") {
		if (!c->get_f32(idata.release))
			return false;
	}
	if (c->name == "decay") {
		if (!c->get_f32(idata.decay))
			return false;
	}
	if (c->name == "sustain") {
		if (!c->get_f32(idata.sustain))
			return false;
	}

	return true;
}

void LInstrument::play_note_ADSR(LBus *p_bus, uint32_t p_key, uint32_t p_velocity, int32_t p_song_sample_from, int32_t p_dest_num_samples, int32_t p_note_start_sample, int32_t p_note_num_samples) {
	if (!p_bus || !p_note_num_samples)
		return;

	uint32_t sample_rate = p_bus->get_sample().get_format().sample_rate;
	float volume = idata.volume;
	volume *= (p_velocity / 127.0f);

	//	play_ADSR(bus, p_key, p_song_sample_from, p_dest_num_samples, p_note_start_sample, p_note_num_samples, volume, 0);
	//	return;

	// ATTACK
	//	int32_t note_start = p_note_start_sample - p_song_sample_from;
	//	int32_t note_start = p_note_start_sample - bus->get_song_time_start();
	int32_t note_start = p_note_start_sample;
	int32_t note_length = p_note_num_samples;

	int32_t attack_length = idata.attack * sample_rate;

	int32_t decay_start = attack_length;
	int32_t decay_length = idata.decay * sample_rate;
	int32_t sustain_start = decay_start + decay_length;

	// note this can be negative if we never got to sustain
	int32_t sustain_length = note_length - sustain_start;
	float sustain_volume = idata.sustain * idata.volume;

	// Most common case, the attack is fully used
	if (note_length >= attack_length) {
		play_ADSR(p_bus, p_key, p_song_sample_from, p_dest_num_samples, note_start, attack_length, 0, volume);
	} else {
		// fraction of full volume?
		volume *= (p_note_num_samples / (float)attack_length);
		play_ADSR(p_bus, p_key, p_song_sample_from, p_dest_num_samples, note_start, note_length, 0, volume);
		goto RELEASE;
	}

	// DECAY

	// Whole decay
	if (sustain_length >= 0) {
		play_ADSR(p_bus, p_key, p_song_sample_from, p_dest_num_samples, note_start + decay_start, decay_length, volume, sustain_volume);
		volume = sustain_volume;
	} else {
		// fraction through to sustain volume
		float fract = (note_length - decay_start) / (float)decay_length;
		sustain_volume = Math::lerp(volume, sustain_volume, fract);
		play_ADSR(p_bus, p_key, p_song_sample_from, p_dest_num_samples, note_start + decay_start, note_length - decay_start, volume, sustain_volume);
		volume = sustain_volume;
		goto RELEASE;
	}

	// SUSTAIN
	play_ADSR(p_bus, p_key, p_song_sample_from, p_dest_num_samples, note_start + sustain_start, note_length - sustain_start, volume, volume);

RELEASE:
	// RELEASE
	int32_t release_start = note_length;
	int32_t release_length = idata.release * sample_rate;
	play_ADSR(p_bus, p_key, p_song_sample_from, p_dest_num_samples, note_start + release_start, release_length, volume, 0);
}

void LInstrument::_bind_methods() {
	//	ClassDB::bind_method(D_METHOD("play", "key", "dest_start_sample"), &LInstrument::play);
	//	ClassDB::bind_method(D_METHOD("set_output_bus", "bus_handle"), &LInstrument::set_output_bus);
}

void LInstrument::set_output_bus(uint32_t p_bus_handle) {
	_output_bus_handle = p_bus_handle;
}

///////////////////////////

//void LInstrument_Holder::_bind_methods() {
//	ClassDB::bind_method(D_METHOD("load", "filename"), &LInstrument_Holder::load);
//	ClassDB::bind_method(D_METHOD("get_instrument"), &LInstrument_Holder::get_instrument);
//}

//bool LInstrument_Holder::load(String p_filename) {
//	//	return _sample.load_wav(p_filename);
//	return true;
//}
