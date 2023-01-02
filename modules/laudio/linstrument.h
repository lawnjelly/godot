#pragma once

#include "core/reference.h"
#include "lsample.h"

class LInstrument : public Reference {
	GDCLASS(LInstrument, Reference);

	LSample _sample;
	uint32_t _output_bus_handle = 0;

protected:
	static void _bind_methods();

public:
	LInstrument();

	bool load(String p_filename);
	void play(uint32_t p_dest_start_sample = 0);
	void set_output_bus(uint32_t p_bus_handle);
};
