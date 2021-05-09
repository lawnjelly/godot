#pragma once

#include "scene/3d/mesh_instance.h"
#include "core/vector.h"


namespace LM {

class Merger
{
public:
	MeshInstance * Merge(Spatial * pRoot);


private:
	void FindMeshes(Spatial * pNode);
	void MergeMeshes(MeshInstance &merged);
	void Merge_MeshInstance(const MeshInstance &mi, PoolVector<Vector3> &verts, PoolVector<Vector3> &norms, PoolVector<int> &inds);
	bool LightmapUnwrap(Ref<ArrayMesh> am, const Transform &trans);

	static bool xatlas_unwrap(float p_texel_size, const float *p_vertices, const float *p_normals, int p_vertex_count, const int *p_indices, const int *p_face_materials, int p_index_count, float **r_uv, int **r_vertex, int *r_vertex_count, int **r_index, int *r_index_count, int *r_size_hint_x, int *r_size_hint_y);


public:
	Vector<MeshInstance *> m_Meshes;

};


} // namespace
