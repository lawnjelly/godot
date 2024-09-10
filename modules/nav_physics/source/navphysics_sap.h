#pragma once

#include "navphysics_vector.h"

namespace NavPhysics {

class SAP {
public:
	struct Intersection {
		u32 agent_id_a = UINT32_MAX;
		u32 agent_id_b = UINT32_MAX;
	};

private:
	struct Item {
		freal axis_min = 0;
		freal axis_max = 0;
		u32 agent_id = UINT32_MAX;

		bool operator<(const Item &p_o) const {
			return axis_min < p_o.axis_min;
		}
	};
	Vector<Item> _items;
	Vector<Intersection> _intersections;
	int _separating_axis = 0;

public:
	void add_item(u32 p_agent_id);
	void remove_item(u32 p_agent_id);

	void update();
	u32 get_num_intersections() const { return _intersections.size(); }
	const Intersection &get_intersection(u32 p_which) const { return _intersections[p_which]; }
};

} //namespace NavPhysics
