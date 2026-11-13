#include "np_obstacle.h"

#include "np_mesh_instance.h"

#include "thirdparty/navphysics/navphysics_poi.h"

//void NPObstacle::_bind_methods() {
//}

void NPObstacle_Box::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_width", "width"), &NPObstacle_Box::set_width);
	ClassDB::bind_method(D_METHOD("get_width"), &NPObstacle_Box::get_width);

	ClassDB::bind_method(D_METHOD("set_height", "height"), &NPObstacle_Box::set_height);
	ClassDB::bind_method(D_METHOD("get_height"), &NPObstacle_Box::get_height);

	ADD_PROPERTY(PropertyInfo(Variant::REAL, "width", PROPERTY_HINT_RANGE, "0,1024,0.01,or_greater"), "set_width", "get_width");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "height", PROPERTY_HINT_RANGE, "0,1024,0.01,or_greater"), "set_height", "get_height");
}

void NPObstacle_Box::set_width(real_t p_width) {
	data.width = p_width;
	emit_changed();
}

void NPObstacle_Box::set_height(real_t p_height) {
	data.height = p_height;
	emit_changed();
}

///////////////

void NPObstacleInstance::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_obstacle", "obstacle"), &NPObstacleInstance::set_obstacle);
	ClassDB::bind_method(D_METHOD("get_obstacle"), &NPObstacleInstance::get_obstacle);

	ClassDB::bind_method(D_METHOD("_resource_changed"), &NPObstacleInstance::_resource_changed);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "obstacle", PROPERTY_HINT_RESOURCE_TYPE, "NPObstacle"), "set_obstacle", "get_obstacle");
}

void NPObstacleInstance::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PARENTED: {
			data.parent = Object::cast_to<NPMeshInstance>(get_parent());
			if (data.parent) {
				data.parent->obstacle_add(*this);
			}
		} break;
		case NOTIFICATION_UNPARENTED: {
			if (data.parent) {
				data.parent->obstacle_remove(*this);
			}
		} break;
		case NOTIFICATION_ENTER_TREE: {
			if (data.parent) {
				data.parent->obstacle_notify_transform(*this);
			}
		} break;
		case NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {
			if (is_inside_tree()) {
				if (data.parent) {
					data.parent->obstacle_notify_transform(*this);
				}
			}
		} break;
		default:
			break;
	}
}

void NPObstacleInstance::_resource_changed() {
	if (data.obstacle.is_null() || !data.h_obstacle.is_valid()) {
		return;
	}

	NavPhysics::Obstacle *obstacle = NPWORLD.safe_get_obstacle(data.h_obstacle);

	if (data.obstacle->get_obstacle_type() == NPObstacle::Type::BOX) {
		Ref<NPObstacle_Box> box = data.obstacle;
		if (box.is_valid()) {
			obstacle->set_width(box->get_width());
			obstacle->set_height(box->get_height());
		}
	}

	print_line("was changed");
}

void NPObstacleInstance::set_obstacle(const Ref<NPObstacle> &p_obstacle) {
	if (p_obstacle == data.obstacle) {
		return;
	}
	if (!data.obstacle.is_null()) {
		data.obstacle->disconnect("changed", this, "_resource_changed");
		data.obstacle->unregister_owner(this);
	}
	data.obstacle = p_obstacle;
	if (data.obstacle.is_valid()) {
		data.obstacle->register_owner(this);
		data.obstacle->connect("changed", this, "_resource_changed");
	}
}

Ref<NPObstacle> NPObstacleInstance::get_obstacle() const {
	return data.obstacle;
}

NPObstacleInstance::NPObstacleInstance() {
	data.h_obstacle = NPWORLD.safe_obstacle_create();
	set_notify_local_transform(true);
}

NPObstacleInstance::~NPObstacleInstance() {
	if (!data.obstacle.is_null()) {
		data.obstacle->disconnect("changed", this, "_resource_changed");
		data.obstacle->unregister_owner(this);
	}
}
