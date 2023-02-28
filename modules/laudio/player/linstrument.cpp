#include "linstrument.h"

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
