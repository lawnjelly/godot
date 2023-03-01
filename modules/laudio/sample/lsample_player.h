#pragma once

#include "../sample/lsample.h"
#include "core/local_vector.h"

namespace LSon {
struct Node;
};

class LSamplePlayer {
	LSample _sample;

public:
	uint16_t key_range_lo = 0;
	uint16_t key_range_hi = 127;
	bool is_single = true;

	bool load_wav(String p_filename);
	bool load(LSon::Node *p_node, const LocalVector<String> &p_include_paths, String p_extra_path);
	void play(int32_t p_dest_start_sample, uint32_t p_output_bus_handle);
};
