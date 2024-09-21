#include "np_region.h"

#include "../source/navphysics_loader.h"
#include "../source/navphysics_log.h"
#include "../source/navphysics_map.h"
#include "../source/navphysics_pointf.h"
#include "../source/navphysics_pointi.h"
#include "../source/navphysics_vector.h"

NPRegion::NPRegion() {
	data.h_region = NavPhysics::g_world.safe_region_create();
}

NPRegion::~NPRegion() {
	if (data.h_region) {
		NavPhysics::g_world.safe_region_free(data.h_region);
		data.h_region = 0;
	}
}

void NPRegion::_notification(int p_what) {
}

void NPRegion::set_meshes(const Array &p_meshes) {
	//	_meshes = p_meshes;
	for (int n = 0; n < _meshes.size(); n++) {
		//		if (!_meshes[n].is_null())
		{
			Ref<NPMesh> rmesh = _meshes[n];
			if (rmesh.is_valid()) {
				rmesh->unregister_owner(this);
			}
		}
	}

	_meshes.resize(0);
	_meshes.resize(p_meshes.size());
	for (int n = 0; n < _meshes.size(); n++) {
		bool valid = false;

		if (p_meshes[n]) {
			Ref<NPMesh> rmesh = p_meshes[n];

			if (rmesh.is_valid()) {
				valid = true;
				_meshes.set(n, rmesh);
				rmesh->register_owner(this);
			}
			//NPMesh *mesh = Object::cast_to<NPMesh>(p_meshes[n]);

			//_meshes[n] = *mesh;
		}

		if (!valid) {
			// If there isn't one, create by default.
			Ref<NPMesh> rmesh;
			rmesh.instance();
			_meshes.set(n, rmesh);
			rmesh->register_owner(this);
		}
	}
}

Array NPRegion::get_meshes() const {
	Array ret;
	ret.resize(_meshes.size());

	for (int n = 0; n < _meshes.size(); n++) {
		//		if (_meshes[n].is_valid())
		{
			ret[n] = Variant(_meshes[n]);
		}
	}

	return ret;
	//	return _meshes;
}

/*
void NPRegion::set_meshes(Vector<Variant> &p_meshes)
//void NPRegion::set_meshes(Vector<NPMesh> &p_meshes)
//void NPRegion::set_mesh(const Ref<NPMesh> &p_mesh)
{
	for (int n=0; n<_meshes.size(); n++)
	{
		if (!_meshes[n].is_null())
		{
			_meshes[n]->unregister_owner(this);
		}
	}

	_meshes = p_meshes;
	for (int n=0; n<_meshes.size(); n++)
	{
		if (_meshes[n].is_valid())
		{
			_meshes[n]->register_owner(this);
		}
	}

	if (p_mesh == _mesh) {
		return;
	}
	if (!_mesh.is_null()) {
		_mesh->unregister_owner(this);
	}
	_mesh = p_mesh;
	if (_mesh.is_valid()) {
		_mesh->register_owner(this);

//		if (is_inside_world() && get_world().is_valid()) {
//			if (_occluder_instance.is_valid()) {
//				VisualServer::get_singleton()->occluder_instance_link_resource(_occluder_instance, p_shape->get_rid());
//			}
//		}
	}
	update_gizmo();
	update_configuration_warning();
}

//Vector<NPMesh> NPRegion::get_meshes() const
Vector<Variant> NPRegion::get_meshes() const {
	return _meshes;
}
*/

void NPRegion::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_meshes", "meshes"), &NPRegion::set_meshes);
	ClassDB::bind_method(D_METHOD("get_meshes"), &NPRegion::get_meshes);

	//	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "meshes", PROPERTY_HINT_RESOURCE_TYPE, "NPMesh"), "set_meshes", "get_meshes");
	//	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "meshes", PROPERTY_HINT_RESOURCE_TYPE, "NPMesh"), "set_meshes", "get_meshes");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "meshes", PROPERTY_HINT_RESOURCE_TYPE, "NPMesh"), "set_meshes", "get_meshes");
}
