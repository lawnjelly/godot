#include "np_mesh_instance.h"

#include "../source/navphysics_loader.h"
#include "../source/navphysics_log.h"
#include "../source/navphysics_map.h"
#include "../source/navphysics_mesh_instance.h"
#include "../source/navphysics_pointf.h"
#include "../source/navphysics_pointi.h"
#include "../source/navphysics_transform.h"
#include "../source/navphysics_vector.h"

NPMeshInstance::NPMeshInstance() {
	data.h_mesh_instance = NavPhysics::g_world.safe_mesh_instance_create();

	NavPhysics::g_world.safe_link_mesh_instance(data.h_mesh_instance, NavPhysics::g_world.get_handle_default_map());

	set_notify_transform(true);
}

NPMeshInstance::~NPMeshInstance() {
	if (data.h_mesh_instance) {
		NavPhysics::g_world.safe_unlink_mesh_instance(data.h_mesh_instance, NavPhysics::g_world.get_handle_default_map());
		NavPhysics::g_world.safe_mesh_instance_free(data.h_mesh_instance);
		data.h_mesh_instance = 0;
	}
	if (!data.mesh.is_null()) {
		data.mesh->unregister_owner(this);
	}
}

void NPMeshInstance::_update_server() {
	if (data.h_mesh_instance) {
		NavPhysics::MeshInstance *mi = NavPhysics::g_world.safe_get_mesh_instance(data.h_mesh_instance);
		if (mi) {
			Transform tr = get_global_transform();
			mi->set_transform(*(NavPhysics::Transform *)&tr);
		}
	}
}

void NPMeshInstance::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_update_server();
		} break;
		case NOTIFICATION_TRANSFORM_CHANGED: {
			_update_server();
		} break;
	}
}

void NPMeshInstance::resource_changed(RES res) {
	update_gizmo();
}

void NPMeshInstance::set_mesh(const Ref<NPMesh> &p_mesh) {
	if (p_mesh == data.mesh) {
		return;
	}
	if (!data.mesh.is_null()) {
		NavPhysics::g_world.safe_link_mesh(data.h_mesh_instance, 0);

		data.mesh->unregister_owner(this);
	}
	data.mesh = p_mesh;
	if (data.mesh.is_valid()) {
		data.mesh->register_owner(this);

		NavPhysics::g_world.safe_link_mesh(data.h_mesh_instance, data.mesh->get_mesh_handle());

		//		if (is_inside_world() && get_world().is_valid()) {
		//			if (_occluder_instance.is_valid()) {
		//				VisualServer::get_singleton()->occluder_instance_link_resource(_occluder_instance, p_shape->get_rid());
		//			}
		//		}
	}

	update_gizmo();
	update_configuration_warning();
}
Ref<NPMesh> NPMeshInstance::get_mesh() const {
	return data.mesh;
}

String NPMeshInstance::get_configuration_warning() const {
	String warning = Spatial::get_configuration_warning();

	if (!data.mesh.is_valid()) {
		if (!warning.empty()) {
			warning += "\n\n";
		}
		warning += TTR("No mesh is set.");
		return warning;
	}

	return warning;
}

void NPMeshInstance::_bind_methods() {
	ClassDB::bind_method(D_METHOD("resource_changed", "resource"), &NPMeshInstance::resource_changed);
	ClassDB::bind_method(D_METHOD("set_mesh", "mesh"), &NPMeshInstance::set_mesh);
	ClassDB::bind_method(D_METHOD("get_mesh"), &NPMeshInstance::get_mesh);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "NPMesh"), "set_mesh", "get_mesh");
}
