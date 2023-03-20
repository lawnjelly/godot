#pragma once

#include "core/reference.h"

namespace LSon {
struct Node;
};

class LSample;
class LBus;

class LInstrument : public Reference {
	GDCLASS(LInstrument, Reference);

protected:
	struct InstrumentData {
		CharString name = "Undefined";
		float volume = 1.0;

		float attack = 0.1;
		float release = 0.2;
		float decay = 0.5;
		float sustain = 1.0;

	} idata;

	uint32_t _output_bus_handle = 0;
	static void _bind_methods();

	virtual bool load_idata(LSon::Node *p_node, const LocalVector<String> &p_include_paths);

	void play_note_ADSR(LBus *p_bus, uint32_t p_key, uint32_t p_velocity, int32_t p_song_sample_from, int32_t p_dest_num_samples, int32_t p_note_start_sample, int32_t p_note_num_samples);

	virtual void play_ADSR(LBus *p_bus, uint32_t p_key, int32_t p_song_sample_from, int32_t p_dest_num_samples, int32_t p_note_start_sample, int32_t p_note_num_samples, float p_vol_a, float p_vol_b) {}

public:
	virtual void play(uint32_t p_key, uint32_t p_velocity, int32_t p_song_sample_from, int32_t p_dest_num_samples, int32_t p_note_start_sample, int32_t p_note_num_samples) {}
	virtual void set_output_bus(uint32_t p_bus_handle);
	const char *get_name() const {
		if (idata.name.size())
			return idata.name.get_data();
		else
			return get_type_name();
	}
	void set_name(String p_name) { idata.name = CharString(p_name.utf8()); }
	void set_volume(float p_volume) { idata.volume = p_volume; }
	virtual const char *get_type_name() const { return "Undefined Type"; }
	virtual bool load(LSon::Node *p_data, const LocalVector<String> &p_include_paths) { return true; }
	virtual int32_t get_max_release_time() const { return 0; } // in samples
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
