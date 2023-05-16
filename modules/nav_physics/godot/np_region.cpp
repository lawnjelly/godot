#include "np_region.h"

NPRegion::NPRegion() {
}

NPRegion::~NPRegion() {
}

void NPRegion::_notification(int p_what) {
}

void NPRegion::set_meshes(const Array &p_meshes) {
	//	_meshes = p_meshes;
	for (int n = 0; n < _meshes.size(); n++) {
		//		if (!_meshes[n].is_null())
		{
			Ref<NPMesh> rmesh = _meshes[n];
			rmesh->unregister_owner(this);
		}
	}

	_meshes.resize(0);
	_meshes.resize(p_meshes.size());
	for (int n = 0; n < _meshes.size(); n++) {
		if (p_meshes[n]) {
			Ref<NPMesh> rmesh = p_meshes[n];
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
			ret[n] = _meshes[n];
		}
	}

	return ret;
	//	return _meshes;
}

/*
void NPRegion::set_meshes(Vector<Ref<NPMesh>> &p_meshes)
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

Ref<NPMesh> NPRegion::get_meshes() const
{
	return _meshes;
}
*/

void NPRegion::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_meshes", "meshes"), &NPRegion::set_meshes);
	ClassDB::bind_method(D_METHOD("get_meshes"), &NPRegion::get_meshes);

	//	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "meshes", PROPERTY_HINT_RESOURCE_TYPE, "NPMesh"), "set_meshes", "get_meshes");
	//	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "meshes", PROPERTY_HINT_RESOURCE_TYPE, "NPMesh"), "set_meshes", "get_meshes");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "meshes", PROPERTY_HINT_NONE, "NPMesh"), "set_meshes", "get_meshes");
}
