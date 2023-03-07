#pragma once

#include "core/reference.h"
#include "lhandle.h"
#include "sample/lsample.h"

typedef Handle_24_8 BusHandle;

class LBus : public Reference {
	GDCLASS(LBus, Reference);

	LSample _sample;
	uint32_t _handle = UINT32_MAX;
	uint32_t _offset = 0;
	uint32_t _song_time_start = 0;

protected:
	static void _bind_methods();

public:
	bool calculate_overlap(int32_t p_song_sample_from, int32_t &r_dest_num_samples, int32_t p_note_start_sample, int32_t &r_note_num_samples, int32_t &r_dest_start_sample, int32_t &r_instrument_start_sample_offset, bool p_clamp_to_note_length = true) const;

	uint32_t get_handle() const { return _handle; }
	bool save(String p_filename);
	void resize(uint32_t p_num_samples);
	LSample &get_sample() { return _sample; }
	uint32_t get_offset() const { return _offset; }
	void set_offset(uint32_t p_offset) { _offset = p_offset; }
	void shift_offset(uint32_t p_shift) { _offset += p_shift; }

	void set_song_time_start(uint32_t p_time) { _song_time_start = p_time; }
	uint32_t get_song_time_start() const { return _song_time_start; }

	LBus();
	virtual ~LBus();
};

class LBuses {
	HandledPool<LBus *> _buses;

public:
	LBus *get_bus(uint32_t p_bus_handle);
	uint32_t alloc_bus(LBus *p_bus);
	void free_bus(uint32_t p_bus_handle, LBus *p_bus);
};

extern LBuses g_Buses;
