#pragma once

#include "core/reference.h"
#include "lhandle.h"
#include "sample/lsample.h"

typedef Handle_24_8 BusHandle;

class LBus : public Reference {
	GDCLASS(LBus, Reference);
	friend class LInstrument;

	LSample _sample;
	uint32_t _handle = UINT32_MAX;

protected:
	static void _bind_methods();

public:
	uint32_t get_handle() const { return _handle; }
	bool save(String p_filename);
	void resize(uint32_t p_num_samples);

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
