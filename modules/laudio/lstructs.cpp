#include "lstructs.h"
#include "lson/lson.h"

bool LTiming::load(LSon::Node *p_data) {
	for (uint32_t c = 0; c < p_data->children.size(); c++) {
		LSon::Node *child = p_data->get_child(c);

		LSON_LOAD_S64(child, "tpqn", tpqn)
		LSON_LOAD_S64(child, "bpm", bpm)
		LSON_LOAD_S64(child, "sample_rate", sample_rate)
		LSON_LOAD_S64(child, "time_sig_micro", time_sig_micro)
		LSON_LOAD_S64(child, "time_sig_minor", time_sig_minor)
		LSON_LOAD_S64(child, "time_sig_major", time_sig_major)
		LSON_LOAD_S64(child, "transport_tick_left", transport_tick_left)
		LSON_LOAD_S64(child, "transport_tick_right", transport_tick_right)
	}

	recalculate();
	return true;
}

bool LTiming::save(LSon::Node *p_root) {
	LSon::Node *node = p_root->request_child();
	node->set_name("timing");

	node->request_child_s64("tpqn", tpqn);
	node->request_child_s64("bpm", bpm);
	node->request_child_s64("sample_rate", sample_rate);

	node->request_child_s64("time_sig_micro", time_sig_micro);
	node->request_child_s64("time_sig_minor", time_sig_minor);
	node->request_child_s64("time_sig_major", time_sig_major);

	node->request_child_s64("transport_tick_left", transport_tick_left);
	node->request_child_s64("transport_tick_right", transport_tick_right);

	return true;
}
