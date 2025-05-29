#include "np_mesh_instance.h"

#include "core/engine.h"

#include "../source/navphysics_loader.h"
#include "../source/navphysics_log.h"
#include "../source/navphysics_map.h"
#include "../source/navphysics_mesh_instance.h"
#include "../source/navphysics_pointf.h"
#include "../source/navphysics_pointi.h"
#include "../source/navphysics_transform.h"
#include "../source/navphysics_vector.h"
#include "np_agent.h"

NPMeshInstance::NPMeshInstance() {
	data.h_mesh_instance = NavPhysics::g_world.safe_mesh_instance_create();

	NavPhysics::g_world.safe_link_mesh_instance(data.h_mesh_instance, NavPhysics::g_world.get_handle_default_map());

	set_notify_transform(true);
}

NPMeshInstance::~NPMeshInstance() {
	set_debug_visuals(false);

	if (data.h_mesh_instance) {
		NavPhysics::g_world.safe_unlink_mesh_instance(data.h_mesh_instance, NavPhysics::g_world.get_handle_default_map());
		NavPhysics::g_world.safe_mesh_instance_free(data.h_mesh_instance);
		data.h_mesh_instance = 0;
	}
	if (!data.mesh.is_null()) {
		data.mesh->unregister_owner(this);
	}
}

void NPMeshInstance::set_debug_visuals(bool p_enable) {
	debug_data.show_debug_visuals = p_enable;
	_refresh_debug_visuals();
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

void NPMeshInstance::_update_visibility() {
	if (is_inside_tree()) {
		if (data.h_mesh_instance) {
			NPWORLD.safe_set_mesh_instance_active(data.h_mesh_instance, is_visible_in_tree());
		}
	}
}

void NPMeshInstance::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			NavPhysics::World::set_timestep(1.0 / Engine::get_singleton()->get_iterations_per_second());
			_update_visibility();
			_update_server();
			_refresh_debug_visuals();
		} break;
		case NOTIFICATION_TRANSFORM_CHANGED: {
			if (is_visible_in_tree()) {
				_update_server();
			}
		} break;
		case NOTIFICATION_VISIBILITY_CHANGED: {
			_update_visibility();
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
	_refresh_debug_visuals();
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

Vector3 NPMeshInstance::choose_random_location() const {
	if (data.h_mesh_instance) {
		NavPhysics::MeshInstance *mi = NavPhysics::g_world.safe_get_mesh_instance(data.h_mesh_instance);
		if (mi) {
			NavPhysics::FPoint3 pos = mi->choose_random_location();
			return Vector3(pos.x, pos.y, pos.z);
		}
	}

	return Vector3();
}

bool NPMeshInstance::_refresh_debug_visuals() {
	bool show = debug_data.show_debug_visuals;

	if (Engine::get_singleton()->is_editor_hint()) {
		return false;
	}

	if (!data.mesh.is_valid()) {
		show = false;
	}

	RID &rid_mesh_instance = debug_data.debug_polys;
	VisualServer *vs = VisualServer::get_singleton();

	// Hiding, free the meshinstance RID before the mesh.
	if (!show) {
		if (rid_mesh_instance.is_valid()) {
			vs->free(rid_mesh_instance);
			rid_mesh_instance = RID();
		}

		// Never try and delete debug geometry from mesh once shown,
		// because another MeshInstance may be using it.
		// We could alternatively refcount here, but probably overkill.
		return true;
	}

	if (!is_inside_tree()) {
		return false;
	}

	RID rid_mesh = data.mesh->_refresh_debug_geometry(true);

	if (!rid_mesh_instance.is_valid()) {
		RID scenario = get_tree()->get_root()->get_world()->get_scenario();
		rid_mesh_instance = vs->instance_create2(rid_mesh, scenario);
	}

	vs->instance_set_transform(rid_mesh_instance, get_global_transform());

	return true;
}

void NPMeshInstance::_bind_methods() {
	ClassDB::bind_method(D_METHOD("resource_changed", "resource"), &NPMeshInstance::resource_changed);
	ClassDB::bind_method(D_METHOD("set_mesh", "mesh"), &NPMeshInstance::set_mesh);
	ClassDB::bind_method(D_METHOD("get_mesh"), &NPMeshInstance::get_mesh);
	ClassDB::bind_method(D_METHOD("choose_random_location"), &NPMeshInstance::choose_random_location);
	ClassDB::bind_method(D_METHOD("set_debug_visuals", "enable"), &NPMeshInstance::set_debug_visuals);
	ClassDB::bind_method(D_METHOD("has_debug_visuals"), &NPMeshInstance::has_debug_visuals);

	ADD_GROUP("Debug", "debug_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_visuals"), "set_debug_visuals", "has_debug_visuals");

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "NPMesh"), "set_mesh", "get_mesh");
}

/////////////////////////////////

void NPMap::_agent_callback(uint64_t p_user_data, const NavPhysics::FPoint3 &p_position, const NavPhysics::FPoint3 &p_velocity) {
	NPAgent *agent = (NPAgent *)p_user_data;
	ERR_FAIL_NULL(agent);
	Transform tr = agent->get_transform();
	//Transform tr;
	tr.origin = *(Vector3 *)&p_position;

	agent->data.vel = *(Vector3 *)&p_velocity;

	//print_line("vel " + String(Variant(agent->data.vel)));

	// Calculate yaw
	agent->update_yaw();

	//agent->data.vel.zero();

	//tr.basis = Basis(Vector3(0, Math::randf(), 0));
	tr.basis = Basis(Vector3(0, (Math_PI / 2) - agent->data.yaw, 0));
	agent->set_transform(tr);
}

void NPMap::_notification(int p_what) {
	switch (p_what) {
		default: {
		} break;
		case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			if (!Engine::get_singleton()->is_editor_hint()) {
				NPWORLD.tick_update(Engine::get_singleton()->get_physics_frames(), get_physics_process_delta_time());
			}
		} break;
	}
}

void NPMap::_bind_methods() {
}

NPMap::NPMap() {
	set_process_priority(100);
	if (!Engine::get_singleton()->is_editor_hint()) {
		set_physics_process_internal(true);
	}
	NavPhysics::World::set_agent_callback(&_agent_callback);
}
