#pragma once

#include "../lvector.h"
#include "llighttypes.h"

namespace LM
{

class LightScene
{
public:
	void Create(const MeshInstance &mi);
	
	// returns triangle ID (or -1) and barycentric coords
	int IntersectRay(const Ray &r, float &u, float &v, float &w, float &nearest_t) const;
	void FindUVsBarycentric(int tri, Vector2 &uvs, float u, float v, float w) const
	{
		m_UVTris[tri].FindUVBarycentric(uvs, u, v, w);
	}

private:
	void Transform_Verts(const PoolVector<Vector3> &ptsLocal, PoolVector<Vector3> &ptsWorld, const Transform &tr) const;
	void Transform_Norms(const PoolVector<Vector3> &normsLocal, PoolVector<Vector3> &normsWorld, const Transform &tr) const;

	PoolVector<Vector3> m_ptPositions;
	PoolVector<Vector3> m_ptNormals;
	PoolVector<Vector2> m_UVs;
	PoolVector<int> m_Inds;

	LVector<Tri> m_Tris;
	LVector<UVTri> m_UVTris;


};



}
