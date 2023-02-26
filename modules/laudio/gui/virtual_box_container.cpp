#include "virtual_box_container.h"
#include "scene/resources/packed_scene.h"

void VirtualBoxContainer::set_line_scene(const Ref<PackedScene> &p_scene) {
	if (p_scene == data.line_scene) {
		return;
	}
	if (!data.line_scene.is_null()) {
		data.line_scene->unregister_owner(this);
	}

	// only allow node2d? NYI

	data.line_scene = p_scene;
	if (data.line_scene.is_valid()) {
		data.line_scene->register_owner(this);
	}

	_place_display_lines(true);
	update();
}

Ref<PackedScene> VirtualBoxContainer::get_line_scene() const {
	return data.line_scene;
}

void VirtualBoxContainer::update_bounds() {
	set_custom_minimum_size(Size2(256, data.total_height));
}

bool VirtualBoxContainer::refresh_line(uint32_t p_line_id) {
	int32_t offset = p_line_id - data.prev_first_line_id;
	if ((offset < 0) || (offset >= (int32_t)data.num_display_lines)) {
		// off screen
		return false;
	}

	ERR_FAIL_COND_V(offset >= get_child_count(), false);
	Control *child = Object::cast_to<Control>(get_child(offset));
	if (child) {
		child->call("_refresh_line", p_line_id);
	}
	return true;
}

void VirtualBoxContainer::refresh_all() {
	_place_display_lines(true);
}

void VirtualBoxContainer::_place_display_lines(bool p_force) {
	DEV_CHECK_ONCE(get_child_count() == (int)data.num_display_lines);

	// get the offset of the container
	float offset = -get_position().y;
	offset /= data.line_size;
	int first_id = offset;

	// if the id of the first hasn't changed, no need to update
	// UNLESS number of display items has changed and this is forced.
	if (!p_force && (first_id == data.prev_first_line_id)) {
		// no change
		return;
	}
	data.prev_first_line_id = first_id;

	for (int n = 0; n < get_child_count(); n++) {
		Control *child = Object::cast_to<Control>(get_child(n));
		if (child) {
			// out of range?
			int32_t line_id = first_id + n;
			if (line_id < (int32_t)data.num_lines) {
				child->set_visible(true);
				child->set_position(Point2(0, line_id * data.line_size));
				child->call("_refresh_line", line_id);
			} else {
				child->set_visible(false);
			}
		}
	}
}

void VirtualBoxContainer::_on_resize() {
	// Not ERR_FAIL because this may happen during normal events.
	if (!data.line_scene.is_valid()) {
		return;
	}

	// get the window height of the PARENT window
	Control *parent = Object::cast_to<Control>(get_parent());
	if (!parent) {
		return;
	}

	data.num_display_lines = (parent->get_size().y / data.line_size) + 1;

	// check we have the correct number of display lines
	int32_t diff = data.num_display_lines - get_child_count();

	if (diff > 0) {
		for (int32_t n = 0; n < diff; n++) {
			Node *scene = data.line_scene->instance();
			ERR_FAIL_NULL(scene);
			add_child(scene);
		}
	}
	if (diff < 0) {
		for (int32_t n = 0; n < -diff; n++) {
			get_child(n)->queue_delete();
		}
	}

	if (diff != 0) {
		_place_display_lines(true);
	}
}

void VirtualBoxContainer::set_line_size(uint32_t p_size) {
	if (p_size != data.line_size) {
		data.line_size = p_size;
		data.calc_total_height();
		update_bounds();
		_on_resize();
	}
}

void VirtualBoxContainer::set_num_lines(uint32_t p_num_lines) {
	if (p_num_lines != data.num_lines) {
		data.num_lines = p_num_lines;
		data.calc_total_height();
		update_bounds();
		_place_display_lines(true);
	}
}

void VirtualBoxContainer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			update_bounds();
		} break;
		case NOTIFICATION_RESIZED: {
			_on_resize();
		} break;
		case NOTIFICATION_DRAW: {
			_place_display_lines();
		} break;
		case NOTIFICATION_THEME_CHANGED: {
			//			queue_sort();
		} break;
		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (is_visible_in_tree()) {
				//				queue_sort();
			}
		} break;
	}
}

void VirtualBoxContainer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_line_size", "size"), &VirtualBoxContainer::set_line_size);
	ClassDB::bind_method(D_METHOD("set_num_lines", "num_lines"), &VirtualBoxContainer::set_num_lines);

	ClassDB::bind_method(D_METHOD("set_line_scene", "scene"), &VirtualBoxContainer::set_line_scene);
	ClassDB::bind_method(D_METHOD("get_line_scene"), &VirtualBoxContainer::get_line_scene);

	ClassDB::bind_method(D_METHOD("refresh_line", "line_id"), &VirtualBoxContainer::refresh_line);
	ClassDB::bind_method(D_METHOD("refresh_all"), &VirtualBoxContainer::refresh_all);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "line_scene", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_line_scene", "get_line_scene");
}

VirtualBoxContainer::VirtualBoxContainer(bool p_vertical) {
}
