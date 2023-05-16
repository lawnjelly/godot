#pragma once

#include "np_mesh.h"
#include "scene/3d/spatial.h"

class NPRegion : public Spatial {
	GDCLASS(NPRegion, Spatial);

	Vector<Ref<NPMesh>> _meshes;

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	//	void set_meshes(Vector<Ref<NPMesh>> &p_meshes);
	//	Vector<Ref<NPMesh>> get_meshes() const;
	void set_meshes(const Array &p_meshes);
	Array get_meshes() const;

	NPRegion();
	~NPRegion();
};
