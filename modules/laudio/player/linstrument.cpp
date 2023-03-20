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

void LInstrument::play_note_ADSR(const PlayParams &p_play_params) {
	const PlayParams &p = p_play_params;

	if (!p.bus || !p.note_num_samples)
		return;

	uint32_t sample_rate = p.bus->get_sample().get_format().sample_rate;
	float volume = idata.volume;
	volume *= (p.velocity / 127.0f);

	PlayParams pp = p;

	// ATTACK
	int32_t note_start = p.note_start_sample;
	int32_t note_length = p.note_num_samples;

	int32_t attack_length = idata.attack * sample_rate;

	int32_t decay_start = attack_length;
	int32_t decay_length = idata.decay * sample_rate;
	int32_t sustain_start = decay_start + decay_length;

	// note this can be negative if we never got to sustain
	int32_t sustain_length = note_length - sustain_start;
	float sustain_volume = idata.sustain * idata.volume;

	// Most common case, the attack is fully used
	if (note_length >= attack_length) {
		pp.vol_a = 0;
		pp.vol_b = volume;
		pp.note_num_samples = attack_length;
		play_ADSR(pp);
	} else {
		// fraction of full volume?
		volume *= (p.note_num_samples / (float)attack_length);
		pp.vol_a = 0;
		pp.vol_b = volume;
		play_ADSR(pp);
		goto RELEASE;
	}

	// DECAY

	// Whole decay
	if (sustain_length >= 0) {
		pp.vol_a = volume;
		pp.vol_b = sustain_volume;
		pp.note_start_sample = note_start + decay_start;
		pp.note_num_samples = decay_length;
		play_ADSR(pp);
		volume = sustain_volume;
	} else {
		// fraction through to sustain volume
		float fract = (note_length - decay_start) / (float)decay_length;
		sustain_volume = Math::lerp(volume, sustain_volume, fract);

		pp.vol_a = volume;
		pp.vol_b = sustain_volume;
		pp.note_start_sample = note_start + decay_start;
		pp.note_num_samples = note_length - decay_start;
		play_ADSR(pp);
		volume = sustain_volume;
		goto RELEASE;
	}

	// SUSTAIN
	pp.vol_a = volume;
	pp.vol_b = volume;
	pp.note_start_sample = note_start + sustain_start;
	pp.note_num_samples = note_length - sustain_start;
	play_ADSR(pp);

RELEASE:
	// RELEASE
	int32_t release_start = note_length;
	int32_t release_length = idata.release * sample_rate;

	pp.vol_a = volume;
	pp.vol_b = 0;
	pp.note_start_sample = note_start + release_start;
	pp.note_num_samples = release_length;

	play_ADSR(pp);
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
