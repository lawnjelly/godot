#pragma once

#include "../source/navphysics_defines.h"
#include "core/resource.h"
#include <Recast.h>

class NavigationMeshInstance;
class NPBakeParams;

#ifdef TOOLS_ENABLED
struct EditorProgress;
#endif

class NPMesh : public Resource {
	//class NPMesh : public Reference {
	//	GDCLASS(NPMesh, Reference);
	GDCLASS(NPMesh, Resource);
	OBJ_SAVE_TYPE(NPMesh);
	RES_BASE_EXTENSION("npmesh");

	struct Data {
		np_handle h_mesh = 0;

		Vector<Vector3> verts;
		Vector<int> indices;
	} data;

	void _build_recast_navigation_mesh(
			const NPBakeParams &p_params,
#ifdef TOOLS_ENABLED
			EditorProgress *ep,
#endif
			rcHeightfield *hf,
			rcCompactHeightfield *chf,
			rcContourSet *cset,
			rcPolyMesh *poly_mesh,
			rcPolyMeshDetail *detail_mesh,
			Vector<float> &vertices,
			Vector<int> &indices);
	void _convert_detail_mesh_to_native_navigation_mesh(const rcPolyMeshDetail *p_detail_mesh);

	bool load(const Vector3 *p_verts, uint32_t p_num_verts, const uint32_t *p_indices, uint32_t p_num_indices);
	bool load(const NavigationMeshInstance &p_nav_mesh_instance);

	void _update_mesh();

protected:
	static void _bind_methods();

public:
	NPMesh();
	~NPMesh();

	void set_vertices(const Vector<Vector3> &p_verts);
	void set_indices(const Vector<int> &p_indices);

	Vector<Vector3> get_vertices() const { return data.verts; }
	Vector<int> get_indices() const { return data.indices; }

	bool bake(Node *p_node);
	bool clear();

	np_handle get_mesh_handle() { return data.h_mesh; }
};
