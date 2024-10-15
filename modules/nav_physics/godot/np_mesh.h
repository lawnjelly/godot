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

public:
	struct Poly {
		uint32_t first_index = 0;
		uint32_t num_indices = 0;
	};

private:
	struct Data {
		np_handle h_mesh = 0;
	} data;

	static void _nav_physics_log_callback(const char *p_string);

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

	bool bake_load(const Vector3 *p_verts, uint32_t p_num_verts, const uint32_t *p_indices, uint32_t p_num_indices);

protected:
	static void _bind_methods();

public:
	NPMesh();
	~NPMesh();

	Vector<Vector3> get_vertices() const;
	Vector<int> get_indices() const;
	Vector<Poly> get_polys() const;
	Vector<int> get_wall_connection_indices() const;

	void set_data(const Vector<uint8_t> &p_data);
	Vector<uint8_t> get_data() const;

	bool bake(Node *p_node);
	bool clear();
	bool toggle_wall_connection(const Vector3 &p_start, const Vector3 &p_end);

	np_handle get_mesh_handle() { return data.h_mesh; }
};
