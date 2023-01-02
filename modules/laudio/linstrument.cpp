#include "linstrument.h"
#include "lbus.h"

void LInstrument::_bind_methods() {
	ClassDB::bind_method(D_METHOD("load", "filename"), &LInstrument::load);
	ClassDB::bind_method(D_METHOD("play", "dest_start_sample"), &LInstrument::play);
	ClassDB::bind_method(D_METHOD("set_output_bus", "bus_handle"), &LInstrument::set_output_bus);
}

bool LInstrument::load(String p_filename) {
	return _sample.load_wav(p_filename);
}

void LInstrument::play(uint32_t p_dest_start_sample) {
	LBus *bus = g_Buses.get_bus(_output_bus_handle);
	if (!bus) {
		return;
	}
	_sample.mix_to(bus->_sample, _sample.get_format().num_samples, p_dest_start_sample);
}

void LInstrument::set_output_bus(uint32_t p_bus_handle) {
	_output_bus_handle = p_bus_handle;
}

LInstrument::LInstrument() {
}
