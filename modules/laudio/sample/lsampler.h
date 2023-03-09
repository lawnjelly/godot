#pragma once

#include "../player/linstrument.h"
#include "lsample_player.h"

class LSampler : public LInstrument {
	PooledList<LSamplePlayer> _players;
	uint32_t _key_player_map[128] = {};

public:
	virtual void play(uint32_t p_key, uint32_t p_velocity, int32_t p_song_sample_from, int32_t p_dest_num_samples, int32_t p_note_start_sample, int32_t p_note_num_samples);
	virtual const char *get_type_name() const { return "Sampler"; }
	virtual bool load(LSon::Node *p_data, const LocalVector<String> &p_include_paths);
};
