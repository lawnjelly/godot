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
	}

	return true;
}

bool LTiming::save(LSon::Node *p_root) {
	LSon::Node *node = p_root->request_child();
	node->set_name("timing");

	node->request_child_s64("tpqn", tpqn);
	node->request_child_s64("bpm", bpm);
	node->request_child_s64("sample_rate", sample_rate);

	return true;
}
