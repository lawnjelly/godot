#include "np_block.h"

#include "np_mesh_instance.h"
#include <core/engine.h>

#include "thirdparty/navphysics/navphysics_poi.h"

//void NPObstacle::_bind_methods() {
//}

void NPShape_Box::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_width", "width"), &NPShape_Box::set_width);
	ClassDB::bind_method(D_METHOD("get_width"), &NPShape_Box::get_width);

	ClassDB::bind_method(D_METHOD("set_height", "height"), &NPShape_Box::set_height);
	ClassDB::bind_method(D_METHOD("get_height"), &NPShape_Box::get_height);

	ADD_PROPERTY(PropertyInfo(Variant::REAL, "width", PROPERTY_HINT_RANGE, "0,1024,0.01,or_greater"), "set_width", "get_width");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "height", PROPERTY_HINT_RANGE, "0,1024,0.01,or_greater"), "set_height", "get_height");
}

void NPShape_Box::set_width(real_t p_width) {
	data.width = p_width;
	emit_changed();
}

void NPShape_Box::set_height(real_t p_height) {
	data.height = p_height;
	emit_changed();
}

///////////////

void NPBlock::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_shape", "shape"), &NPBlock::set_shape);
	ClassDB::bind_method(D_METHOD("get_shape"), &NPBlock::get_shape);

	ClassDB::bind_method(D_METHOD("_resource_changed"), &NPBlock::_resource_changed);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "shape", PROPERTY_HINT_RESOURCE_TYPE, "NPShape"), "set_shape", "get_shape");
}

void NPBlock::_notification(int p_what) {
	if (!Engine::get_singleton()->is_editor_hint()) {
		switch (p_what) {
			case NOTIFICATION_PARENTED: {
				data.parent = Object::cast_to<NPMeshInstance>(get_parent());
				if (data.parent) {
					data.parent->block_add(*this);
				}
			} break;
			case NOTIFICATION_UNPARENTED: {
				if (data.parent) {
					data.parent->block_remove(*this);
				}
			} break;
			case NOTIFICATION_ENTER_TREE: {
				if (data.parent) {
					data.parent->block_notify_transform(*this);
				}
			} break;
			case NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {
				if (is_inside_tree()) {
					if (data.parent) {
						data.parent->block_notify_transform(*this);
					}
				}
			} break;
			default:
				break;
		}
	}
}

void NPBlock::_resource_changed() {
	if (data.shape.is_null() || !data.h_block.is_valid()) {
		return;
	}

	NavPhysics::Block *obstacle = NPWORLD.safe_get_block(data.h_block);

	if (data.shape->get_shape_type() == NPShape::Type::BOX) {
		Ref<NPShape_Box> box = data.shape;
		if (box.is_valid()) {
			obstacle->set_width(box->get_width());
			obstacle->set_height(box->get_height());
		}
	}

	print_line("was changed");
}

void NPBlock::set_shape(const Ref<NPShape> &p_obstacle) {
	if (p_obstacle == data.shape) {
		return;
	}

	bool is_editor = Engine::get_singleton()->is_editor_hint();

	if (!data.shape.is_null()) {
		if (!is_editor) {
			data.shape->disconnect("changed", this, "_resource_changed");
		}
		data.shape->unregister_owner(this);
	}
	data.shape = p_obstacle;
	if (data.shape.is_valid()) {
		data.shape->register_owner(this);
		if (!is_editor) {
			// Ensure that the server is kept up to date with the changed data.
			_resource_changed();
			data.shape->connect("changed", this, "_resource_changed");
		}
	}
}

Ref<NPShape> NPBlock::get_shape() const {
	return data.shape;
}

NPBlock::NPBlock() {
	if (!Engine::get_singleton()->is_editor_hint()) {
		data.h_block = NPWORLD.safe_block_create();
		set_notify_local_transform(true);
	}
}

NPBlock::~NPBlock() {
	bool is_editor = Engine::get_singleton()->is_editor_hint();

	if (!data.shape.is_null()) {
		if (!is_editor) {
			data.shape->disconnect("changed", this, "_resource_changed");
		}
		data.shape->unregister_owner(this);
	}

	if (!is_editor) {
		NPWORLD.safe_block_free(data.h_block);
	}
}
