#include "lbus.h"

LBuses g_Buses;

void LBus::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_handle"), &LBus::get_handle);
	ClassDB::bind_method(D_METHOD("save", "filename"), &LBus::save);
	ClassDB::bind_method(D_METHOD("resize", "num_samples"), &LBus::resize);
}

LBus::LBus() {
	_sample.create(4, 2, 44100, 44100);
	_handle = g_Buses.alloc_bus(this);
}

void LBus::resize(uint32_t p_num_samples) {
	_sample.create(4, 2, 44100, p_num_samples);
}

bool LBus::save(String p_filename) {
	return _sample.save_wav(p_filename);
}

bool LBus::calculate_overlap(int32_t p_song_sample_from, int32_t &r_dest_num_samples, int32_t p_note_start_sample, int32_t &r_note_num_samples, int32_t &r_dest_start_sample, int32_t &r_instrument_start_sample_offset, bool p_clamp_to_note_length) const {
	// check the offset
	DEV_ASSERT((p_song_sample_from - get_song_time_start()) == get_offset());

	// adjust the start sample
	r_instrument_start_sample_offset = p_note_start_sample - p_song_sample_from;
	r_dest_start_sample = get_offset();

	if (r_instrument_start_sample_offset > 0) {
		r_dest_start_sample += r_instrument_start_sample_offset;
		r_dest_num_samples -= r_instrument_start_sample_offset;
		r_instrument_start_sample_offset = 0;

		if (r_dest_num_samples <= 0)
			return false;
	} else if (r_instrument_start_sample_offset < 0) {
		r_instrument_start_sample_offset = -r_instrument_start_sample_offset;
		r_note_num_samples -= r_instrument_start_sample_offset;
		if (r_note_num_samples <= 0)
			return false;
	}

	// key off .. not yet supported as we need a release
	if (p_clamp_to_note_length && (r_note_num_samples < r_dest_num_samples)) {
		r_dest_num_samples = r_note_num_samples;
	}

	return true;
}

LBus::~LBus() {
	g_Buses.free_bus(_handle, this);
	_handle = UINT32_MAX;
}

//////////////////////

LBus *LBuses::get_bus(uint32_t p_bus_handle) {
	LBus **pp = _buses.get(BusHandle(p_bus_handle));
	if (pp) {
		return *pp;
	}
	return nullptr;
}

uint32_t LBuses::alloc_bus(LBus *p_bus) {
	BusHandle handle;
	LBus **pp = _buses.request(handle);
	DEV_ASSERT(pp);
	*pp = p_bus;
	return handle.get_value();
}

void LBuses::free_bus(uint32_t p_bus_handle, LBus *p_bus) {
	LBus **pp = _buses.get(BusHandle(p_bus_handle));
	if (pp) {
		DEV_ASSERT(*pp == p_bus);
		_buses.free(BusHandle(p_bus_handle));
		return;
	}
	ERR_FAIL_MSG("Bus handle not found.");
}
