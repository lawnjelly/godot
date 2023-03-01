#include "lsampler.h"

void LSampler::play(uint32_t p_key, uint32_t p_dest_start_sample, uint32_t p_dest_num_samples, uint32_t p_note_start_sample, uint32_t p_note_num_samples) {
	// try and get sampler associated with this key
	p_key = MIN(p_key, 127);
	uint32_t pool_id = _key_player_map[p_key];

	// no player for this key
	if (!pool_id) {
		return;
	}

	// +1 based
	pool_id--;
	LSamplePlayer &sp = _players[pool_id];

	int32_t offset_start = (int32_t)p_note_start_sample - (int32_t)p_dest_start_sample;

	sp.play(offset_start, _output_bus_handle);
}

bool LSampler::load(LSon::Node *p_data, const LocalVector<String> &p_include_paths) {
	String extra_path;

	for (uint32_t c = 0; c < p_data->children.size(); c++) {
		LSon::Node *child = p_data->get_child(c);

		if (child->name == "path") {
			print_line("path is " + child->string);
			extra_path = child->string;
		}

		if (child->name == "sample_player") {
			uint32_t sample_player_id;
			LSamplePlayer *player = _players.request(sample_player_id);

			if (!player->load(child, p_include_paths, extra_path))
				return false;

			for (uint32_t k = player->key_range_lo; k <= player->key_range_hi; k++) {
				_key_player_map[k] = sample_player_id + 1;
			}
		}
	}
	return true;
}
