#include "np_sap.h"
#include "np_map.h"

namespace NavPhysics {

void SAP::add_item(uint32_t p_agent_id) {
	Agent &agent = g_world.get_body(p_agent_id);

	Item item;
	item.agent_id = p_agent_id;

	real_t pos = agent.fpos3[_separating_axis];

	item.axis_min = pos - agent.radius;
	item.axis_max = pos + agent.radius;

	_items.push_back(item);
}

void SAP::remove_item(uint32_t p_agent_id) {
	for (uint32_t n = 0; n < _items.size(); n++) {
		if (_items[n].agent_id == p_agent_id) {
			_items.remove_unordered(n);
			return;
		}
	}
	ERR_FAIL_MSG("Item not found.");
}

void SAP::update() {
	_intersections.clear();

	if (!_items.size()) {
		return;
	}

	// find best separating axis for next tick
	// (there will be a tick delay, but this shouldn't matter as it will rarely change)

	// Initialize a position within the array (this is to optimize the main loop without a branch)
	Rect2 bound;
	{
		const Agent &agent = g_world.get_body(_items[0].agent_id);
		const Vector3 &pos3 = agent.fpos3;
		bound.position = Vector2(pos3.x, pos3.z);
	}

	// update the items positions
	for (uint32_t i = 0; i < _items.size(); i++) {
		Item &item = _items[i];
		const Agent &agent = g_world.get_body(item.agent_id);
		const Vector3 &pos3 = agent.fpos3;

		bound.expand_to(Vector2(pos3.x, pos3.z));

		real_t pos = pos3[_separating_axis];

		item.axis_min = pos - agent.radius;
		item.axis_max = pos + agent.radius;
	}

	// sort the items
	_items.sort();

	// find collisions
	for (uint32_t i = 0; i < _items.size(); i++) {
		const Item &a = _items[i];
		Agent &agent_a = g_world.get_body(a.agent_id);

		for (uint32_t j = i + 1; j < _items.size(); j++) {
			const Item &b = _items[j];

			if (b.axis_min >= a.axis_max) {
				break;
			}

			// possible collision, do in depth check
			Agent &agent_b = g_world.get_body(b.agent_id);

			real_t combined_radii = agent_a.radius + agent_b.radius;
			combined_radii *= combined_radii;

			real_t dist = (agent_b.fpos3 - agent_a.fpos3).length_squared();
			if (dist < combined_radii) {
				// hit
				Intersection it;
				it.agent_id_a = a.agent_id;
				it.agent_id_b = b.agent_id;
				_intersections.push_back(it);
			}
		}
	}

	// store the new best separating axis
	_separating_axis = bound.size.x > bound.size.y ? 0 : 2;
}

} //namespace NavPhysics
