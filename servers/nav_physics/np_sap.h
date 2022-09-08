#pragma once

#include "core/local_vector.h"
#include <stdint.h>

namespace NavPhysics {

class SAP {
public:
	struct Intersection {
		uint32_t agent_id_a = UINT32_MAX;
		uint32_t agent_id_b = UINT32_MAX;
	};

private:
	struct Item {
		real_t axis_min = 0;
		real_t axis_max = 0;
		uint32_t agent_id = UINT32_MAX;

		bool operator<(const Item &p_o) const {
			return axis_min < p_o.axis_min;
		}
	};
	LocalVector<Item> _items;
	LocalVector<Intersection> _intersections;
	int _separating_axis = 0;

public:
	void add_item(uint32_t p_agent_id);
	void remove_item(uint32_t p_agent_id);

	void update();
	uint32_t get_num_intersections() const { return _intersections.size(); }
	const Intersection &get_intersection(uint32_t p_which) const { return _intersections[p_which]; }
};

} //namespace NavPhysics
