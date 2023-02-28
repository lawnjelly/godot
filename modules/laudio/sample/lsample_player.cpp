#include "lsample_player.h"
#include "../lbus.h"
#include "../lson/lson.h"

bool LSamplePlayer::load(LSon::Node *p_node, const LocalVector<String> &p_include_paths, String p_extra_path) {
	for (uint32_t c = 0; c < p_node->children.size(); c++) {
		LSon::Node *child = p_node->get_child(c);

		if (child->name == "key") {
			key_range_lo = child->val.u64;
			key_range_hi = key_range_lo;
			is_single = true;

			print_line("single key is " + itos(key_range_lo));
		}

		if (child->name == "wav") {
			print_line("wav is " + child->string);

			// find one that exists with the include paths
			for (uint32_t n = 0; n < p_include_paths.size(); n++) {
				String path = p_include_paths[n] + p_extra_path + child->string + ".wav";

				print_line("Testing path exists:\n" + path);
				if (FileAccess::exists(path)) {
					if (!load_wav(path)) {
						ERR_PRINT("Error loading: " + path);
					}
					break;
				}
			}
		}
	}

	return true;
}

bool LSamplePlayer::load_wav(String p_filename) {
	return _sample.load_wav(p_filename);
}

void LSamplePlayer::play(int32_t p_dest_start_sample, uint32_t p_output_bus_handle) {
	LBus *bus = g_Buses.get_bus(p_output_bus_handle);
	if (!bus) {
		return;
	}
	_sample.mix_to(bus->get_sample(), _sample.get_format().num_samples, p_dest_start_sample);
}
