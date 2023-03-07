#pragma once

#include "core/reference.h"

namespace LSon {
struct Node;
};

class LSample;

class LInstrument : public Reference {
	GDCLASS(LInstrument, Reference);

	struct InstrumentData {
		CharString name = "Undefined";
	} idata;

protected:
	uint32_t _output_bus_handle = 0;
	static void _bind_methods();

public:
	virtual void play(uint32_t p_key, int32_t p_song_sample_from, int32_t p_dest_num_samples, int32_t p_note_start_sample, int32_t p_note_num_samples) {}
	virtual void set_output_bus(uint32_t p_bus_handle);
	const char *get_name() const {
		if (idata.name.size())
			return idata.name.get_data();
		else
			return get_type_name();
	}
	void set_name(String p_name) { idata.name = CharString(p_name.utf8()); }
	virtual const char *get_type_name() const { return "Undefined Type"; }
	virtual bool load(LSon::Node *p_data, const LocalVector<String> &p_include_paths) { return true; }
};

//class LInstrument_Holder {
//	friend class LPlayer;

//	Ref<LInstrument> _instrument;

//protected:
//	//	static void _bind_methods();

//public:
//	bool load(String p_filename);
//	bool is_valid() const { return _instrument.is_valid(); }
//	Ref<LInstrument> get_instrument() { return _instrument; }
//	const Ref<LInstrument> get_instrument() const { return _instrument; }
//};
