#include "lstructs.h"
#include "lson/lson.h"

bool LTiming::load(LSon::Node *p_data) {
	for (uint32_t c = 0; c < p_data->children.size(); c++) {
		LSon::Node *child = p_data->get_child(c);

		if (child->name == "tpqn") {
			if (!child->get_s64(tpqn))
				return false;
		}
		if (child->name == "bpm") {
			if (!child->get_s64(bpm))
				return false;
		}
		if (child->name == "sample_rate") {
			if (!child->get_s64(sample_rate))
				return false;
		}
		if (child->name == "transport_tick_left") {
			if (!child->get_s64(transport_tick_left))
				return false;
		}
		if (child->name == "transport_tick_right") {
			if (!child->get_s64(transport_tick_right))
				return false;
		}
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

	node->request_child_s64("transport_tick_left", transport_tick_left);
	node->request_child_s64("transport_tick_right", transport_tick_right);

	return true;
}
