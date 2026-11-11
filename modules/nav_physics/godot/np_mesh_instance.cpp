#include "np_mesh_instance.h"

#include "core/engine.h"

#include "np_agent.h"
#include "np_defines.h"
#include "np_point.h"
#include "thirdparty/navphysics/navphysics_loader.h"
#include "thirdparty/navphysics/navphysics_log.h"
#include "thirdparty/navphysics/navphysics_map.h"
#include "thirdparty/navphysics/navphysics_mesh_instance.h"
#include "thirdparty/navphysics/navphysics_pointf.h"
#include "thirdparty/navphysics/navphysics_pointi.h"
#include "thirdparty/navphysics/navphysics_transform.h"
#include "thirdparty/navphysics/navphysics_vector.h"

NPMeshInstance::NPMeshInstance() {
	data.h_mesh_instance = NavPhysics::g_world.safe_mesh_instance_create();
	DEV_ASSERT(data.h_mesh_instance.is_valid());

	NavPhysics::g_world.safe_link_mesh_instance(data.h_mesh_instance, NavPhysics::g_world.get_handle_default_map());
	NavPhysics::MeshInstance *mi = NavPhysics::g_world.safe_get_mesh_instance(data.h_mesh_instance);
	DEV_ASSERT(mi);
	mi->set_user_data(get_instance_id());

	set_notify_transform(true);
	NPTicker::_ref_count++;
}

NPMeshInstance::~NPMeshInstance() {
	NPTicker::_ref_count--;

	set_debug_visuals(false);

	if (data.h_mesh_instance.is_valid()) {
		NavPhysics::g_world.safe_unlink_mesh_instance(data.h_mesh_instance, NavPhysics::g_world.get_handle_default_map());
		NavPhysics::g_world.safe_mesh_instance_free(data.h_mesh_instance);
		data.h_mesh_instance = np_handle();
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
	if (data.h_mesh_instance.is_valid()) {
		NavPhysics::MeshInstance *mi = NavPhysics::g_world.safe_get_mesh_instance(data.h_mesh_instance);
		if (mi) {
			Transform tr = get_global_transform();
			mi->set_transform(NPHelper::transform_to_np_transform(tr));
		}
	}
}

void NPMeshInstance::_update_active(bool p_exiting_tree) {
	ERR_FAIL_COND(!data.h_mesh_instance.is_valid());

	if (is_inside_tree() && !p_exiting_tree) {
		if (data.is_active_desired && is_visible_in_tree()) {
			if (!data.is_active_state) {
				NPWORLD.safe_set_mesh_instance_active(data.h_mesh_instance, true);
				data.is_active_state = true;
			}
		} else {
			if (data.is_active_state) {
				NPWORLD.safe_set_mesh_instance_active(data.h_mesh_instance, false);
				data.is_active_state = false;
			}
		}
	} else {
		if (data.is_active_state) {
			NPWORLD.safe_set_mesh_instance_active(data.h_mesh_instance, false);
			data.is_active_state = false;
		}
	}
}

void NPMeshInstance::point_add(NPPoint &p_point) {
	NavPhysics::MeshInstance *mi = NavPhysics::g_world.safe_get_mesh_instance(data.h_mesh_instance);
	ERR_FAIL_NULL(mi);

	//Vector3 origin = p_point.get_global_transform().origin;

	NavPhysics::POI *poi = NavPhysics::g_world.safe_get_poi(p_point.get_handle_poi());
	DEV_ASSERT(poi);
	//mi->add_poi(poi, *(NavPhysics::FPoint3 *)&origin);
	mi->poi_add(poi);

	// Back pointer to the scene side.
	poi->user_data = (u64)&p_point;

	PointRef ref;
	ref.point = &p_point;
	data.points.push_back(ref);
}

void NPMeshInstance::point_remove(NPPoint &p_point) {
	NavPhysics::MeshInstance *mi = NavPhysics::g_world.safe_get_mesh_instance(data.h_mesh_instance);
	ERR_FAIL_NULL(mi);

	NavPhysics::POI *poi = NavPhysics::g_world.safe_get_poi(p_point.get_handle_poi());
	DEV_ASSERT(poi);
	mi->poi_remove(poi);

	PointRef ref;
	ref.point = &p_point;
	ERR_FAIL_COND(data.points.erase_unordered(ref) == false);
}

void NPMeshInstance::point_notify_transform(NPPoint &p_point) {
#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
#endif

	NavPhysics::MeshInstance *mi = NavPhysics::g_world.safe_get_mesh_instance(data.h_mesh_instance);
	ERR_FAIL_NULL(mi);

	Vector3 origin = p_point.get_global_transform().origin;

	NavPhysics::POI *poi = NavPhysics::g_world.safe_get_poi(p_point.get_handle_poi());
	DEV_ASSERT(poi);

	mi->poi_set_position(poi, NPHelper::vector3_to_np_fpoint3(origin));
}

void NPMeshInstance::resync_points() {
	data.points.clear();

	if (data.h_mesh_instance.is_valid()) {
		NavPhysics::MeshInstance *mi = NavPhysics::g_world.safe_get_mesh_instance(data.h_mesh_instance);
		if (mi) {
			mi->pois_clear();

			int num_children = get_child_count();

			for (int n = 0; n < num_children; n++) {
				NPPoint *point = Object::cast_to<NPPoint>(get_child(n));
				if (point) {
					PointRef ref;
					ref.point = point;

					//Vector3 origin = point->get_global_transform().origin;

					NavPhysics::POI *poi = NavPhysics::g_world.safe_get_poi(point->get_handle_poi());
					DEV_ASSERT(poi);
					//mi->poi_add(poi, *(NavPhysics::FPoint3 *)&origin);
					mi->poi_add(poi);

					// Back pointer to the scene side.
					poi->user_data = (u64)point;

					data.points.push_back(ref);
				}
			}
		} // if mi
	}
}

void NPMeshInstance::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_update_active();
			_update_server();
			_refresh_debug_visuals();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			_update_active(true);
		} break;
		case NOTIFICATION_TRANSFORM_CHANGED: {
			if (is_visible_in_tree()) {
				_update_server();
			}
		} break;
		case NOTIFICATION_VISIBILITY_CHANGED: {
			_update_active();
		} break;
		case NOTIFICATION_CHILD_ORDER_CHANGED: {
		} break;
	}
}

void NPMeshInstance::resource_changed(RES res) {
	update_gizmo();
}

void NPMeshInstance::set_active(bool p_enable) {
	if (p_enable == data.is_active_desired) {
		return;
	}
	data.is_active_desired = p_enable;
	_update_active();
}

void NPMeshInstance::set_mesh(const Ref<NPMesh> &p_mesh) {
	if (p_mesh == data.mesh) {
		return;
	}
	if (!data.mesh.is_null()) {
		NavPhysics::g_world.safe_link_mesh(data.h_mesh_instance, np_handle());

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
	if (data.h_mesh_instance.is_valid()) {
		NavPhysics::MeshInstance *mi = NavPhysics::g_world.safe_get_mesh_instance(data.h_mesh_instance);
		if (mi) {
			NavPhysics::FPoint3 pos = mi->choose_random_location();
			return NPHelper::np_fpoint3_to_vector3(pos);
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

	ClassDB::bind_method(D_METHOD("set_active", "enable"), &NPMeshInstance::set_active);
	ClassDB::bind_method(D_METHOD("is_active"), &NPMeshInstance::is_active);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "active"), "set_active", "is_active");

	ADD_GROUP("Debug", "debug_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_visuals"), "set_debug_visuals", "has_debug_visuals");

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "NPMesh"), "set_mesh", "get_mesh");
}
