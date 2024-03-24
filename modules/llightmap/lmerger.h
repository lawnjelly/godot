#pragma once

#include "core/vector.h"
#include "scene/3d/mesh_instance.h"

namespace LM {

class Merger {
public:
	MeshInstance *merge(Spatial *pRoot, int padding);

private:
	Node *find_scene_root(Node *pNode) const;

	void find_meshes(Spatial *pNode);
	bool merge_meshes(MeshInstance &merged);
	void merge_mesh_instance(const MeshInstance &mi, PoolVector<Vector3> &verts, PoolVector<Vector3> &norms, PoolVector<int> &inds);
	void merge_mesh_instance_surface(const MeshInstance &mi, PoolVector<Vector3> &verts, PoolVector<Vector3> &norms, PoolVector<int> &inds, int p_surface_id);
	bool lightmap_unwrap(Ref<ArrayMesh> am, const Transform &trans);

#ifdef TOOLS_ENABLED
	static bool xatlas_unwrap(float p_texel_size, const float *p_vertices, const float *p_normals, int p_vertex_count, const int *p_indices, const int *p_face_materials, int p_index_count, float **r_uv, int **r_vertex, int *r_vertex_count, int **r_index, int *r_index_count, int *r_size_hint_x, int *r_size_hint_y);
#endif

public:
	static int _uv_padding;
	Vector<MeshInstance *> _meshes;
};

} //namespace LM
