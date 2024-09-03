#pragma once

#include "np_mesh.h"
#include "scene/3d/spatial.h"

class NPRegion : public Spatial {
	GDCLASS(NPRegion, Spatial);

	//Vector<NPMesh> _meshes;
	Vector<Variant> _meshes;

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	//	void set_meshes(Vector<NPMesh> &p_meshes);
	//	Vector<NPMesh> get_meshes() const;

	void set_meshes(Vector<Variant> &p_meshes);
	Vector<Variant> get_meshes() const;

	//	void set_meshes(const Array &p_meshes);
	//	Array get_meshes() const;

	NPRegion();
	~NPRegion();
};
