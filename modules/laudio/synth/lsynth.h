#pragma once

#include "../player/linstrument.h"

class LSynth : public LInstrument {
public:
	virtual const char *get_type_name() const { return "Synth"; }
	virtual void play(uint32_t p_key, int32_t p_song_sample_from, int32_t p_dest_num_samples, int32_t p_note_start_sample, int32_t p_note_num_samples);

	virtual bool load(LSon::Node *p_data, const LocalVector<String> &p_include_paths);
};
