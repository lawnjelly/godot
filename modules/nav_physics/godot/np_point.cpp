#include "np_point.h"
#include "np_mesh_instance.h"

void NPPoint::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_EXIT_TREE: {
			_resync_parent();
		} break;
		case NOTIFICATION_ENTER_TREE: {
		} break;
		case NOTIFICATION_PARENTED: {
			_resync_parent();
		} break;
		default:
			break;
	}
}

void NPPoint::_resync_parent() {
	// We will be getting a new ID from the navphysics.
	data.id = UINT32_MAX;

	NPMeshInstance *mi = Object::cast_to<NPMeshInstance>(get_parent_spatial());
	if (mi) {
		mi->resync_points();
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
