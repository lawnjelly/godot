#pragma once

#include "core/reference.h"

namespace LSon {
struct Node;
};

class LSample;
class LBus;

class LInstrument : public Reference {
	GDCLASS(LInstrument, Reference);

public:
	struct PlayParams {
		LBus *bus = nullptr;
		uint32_t key = 0;
		uint32_t velocity = 127;
		int32_t song_sample_from = 0;
		int32_t dest_num_samples = 0;
		int32_t note_start_sample = 0;
		int32_t note_num_samples = 0;
		float vol_a = 1;
		float vol_b = 1;
	};

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

	void play_note_ADSR(const PlayParams &p_play_params);

	virtual void play_ADSR(const PlayParams &p_play_params) {}

public:
	virtual void play(const PlayParams &p_play_params) {}
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
