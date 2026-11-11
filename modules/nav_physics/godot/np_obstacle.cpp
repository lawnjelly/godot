#include "np_obstacle.h"

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
}

void NPObstacle_Box::set_height(real_t p_height) {
	data.height = p_height;
}

///////////////

void NPObstacleInstance::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_obstacle", "obstacle"), &NPObstacleInstance::set_obstacle);
	ClassDB::bind_method(D_METHOD("get_obstacle"), &NPObstacleInstance::get_obstacle);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "obstacle", PROPERTY_HINT_RESOURCE_TYPE, "NPObstacle"), "set_obstacle", "get_obstacle");
}

void NPObstacleInstance::set_obstacle(const Ref<NPObstacle> &p_obstacle) {
	if (p_obstacle == data.obstacle) {
		return;
	}
	if (!data.obstacle.is_null()) {
		data.obstacle->unregister_owner(this);
	}
	data.obstacle = p_obstacle;
	if (data.obstacle.is_valid()) {
		data.obstacle->register_owner(this);
	}
}

Ref<NPObstacle> NPObstacleInstance::get_obstacle() const {
	return data.obstacle;
}

NPObstacleInstance::~NPObstacleInstance() {
	if (!data.obstacle.is_null()) {
		data.obstacle->unregister_owner(this);
	}
}
