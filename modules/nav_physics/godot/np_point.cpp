#include "np_point.h"
#include "np_mesh_instance.h"

void NPPoint::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PARENTED: {
			NPMeshInstance *mi = Object::cast_to<NPMeshInstance>(get_parent());
			if (mi) {
				mi->point_add(*this);
			}
		} break;
		case NOTIFICATION_UNPARENTED: {
			NPMeshInstance *mi = Object::cast_to<NPMeshInstance>(get_parent());
			if (mi) {
				mi->point_remove(*this);
			}
		} break;
		case NOTIFICATION_ENTER_TREE: {
			NPMeshInstance *mi = Object::cast_to<NPMeshInstance>(get_parent());
			if (mi) {
				mi->point_notify_transform(*this);
			}
		} break;
		case NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {
			if (is_inside_tree()) {
				NPMeshInstance *mi = Object::cast_to<NPMeshInstance>(get_parent());
				if (mi) {
					mi->point_notify_transform(*this);
				}
			}
		} break;
		default:
			break;
	}
}

void NPPoint::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_label", "label"), &NPPoint::set_label);
	ClassDB::bind_method(D_METHOD("get_label"), &NPPoint::get_label);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "label"), "set_label", "get_label");
}

void NPPoint::set_label(String p_label) {
	data.label = p_label;
}

String NPPoint::get_label() const {
	return data.label;
}

void NPPoint::set_type(int32_t p_type) {
	data.type = p_type;
}

int32_t NPPoint::get_type() const {
	return data.type;
}

NavPhysics::POI *NPPoint::get_poi() const {
	ERR_FAIL_COND_V(data.h_poi == 0, nullptr);
	return NavPhysics::g_world.safe_get_poi(data.h_poi);
}

NPPoint::NPPoint() {
	data.h_poi = NavPhysics::g_world.safe_poi_create();
	set_notify_local_transform(true);
}

NPPoint::~NPPoint() {
	if (data.h_poi) {
		NavPhysics::g_world.safe_poi_free(data.h_poi);
		data.h_poi = 0;
	}
}
