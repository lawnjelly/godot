#include "lsampler.h"

void LSampler::play(const PlayParams &p_play_params) {
	const PlayParams &p = p_play_params;

	// try and get sampler associated with this key
	uint32_t key = MIN(p.key, 127);
	uint32_t pool_id = _key_player_map[key];

	// no player for this key
	if (!pool_id) {
		return;
	}

	// +1 based
	pool_id--;
	LSamplePlayer &sp = _players[pool_id];

	//	int32_t offset_start_of_write = (int32_t)p_note_start_sample - (int32_t)p_song_sample_from;
	//	sp.play(offset_start_of_write, _output_bus_handle);
	sp.play(p.song_sample_from, p.dest_num_samples, p.note_start_sample, p.note_num_samples, _output_bus_handle);
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
