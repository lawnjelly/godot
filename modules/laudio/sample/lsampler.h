#pragma once

#include "../player/linstrument.h"
#include "lsample_player.h"

class LSampler : public LInstrument {
	PooledList<LSamplePlayer> _players;
	uint32_t _key_player_map[128] = {};

public:
	virtual void play(const PlayParams &p_play_params);
	virtual const char *get_type_name() const { return "Sampler"; }
	virtual bool load(LSon::Node *p_data, const LocalVector<String> &p_include_paths);
};
