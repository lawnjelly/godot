#pragma once

#include "../lvector.h"
#include "llighttypes.h"
#include "llighttracer.h"
#include "llightimage.h"
#include "scene/3d/mesh_instance.h"



namespace LM
{

class LightMapper_Base;

class LightScene
{
	friend class LLightMapper;
	friend class LightTracer;
public:

	// each texel can have more than 1 triangle crossing it.
	// this is important for sub texel anti-aliasing, so we need a quick record
	// of all the triangles to test
	struct TexelTriList
	{
		enum {MAX_TRIS = 11};
		uint32_t tri_ids[MAX_TRIS];
		uint32_t num_tris;
	};

	void Reset();
	bool Create(const MeshInstance &mi, int width, int height, const Vec3i &voxel_dims);
	
	// returns triangle ID (or -1) and barycentric coords
	int IntersectRay(const Ray &r, float &u, float &v, float &w, float &nearest_t, const Vec3i * pVoxelRange, int &num_tests);//, int ignore_tri_p1 = 0);
	int IntersectRay_old(const Ray &ray, float &u, float &v, float &w, float &nearest_t) const;


	void FindUVsBarycentric(int tri, Vector2 &uvs, float u, float v, float w) const
	{
		m_UVTris[tri].FindUVBarycentric(uvs, u, v, w);
	}
	//int FindTriAtUV(float x, float y, float &u, float &v, float &w) const;

	// setup
	void RasterizeTriangleIDs(LightMapper_Base &base, LightImage<uint32_t> &im_p1, LightImage<uint32_t> &im2_p1, LightImage<Vector3> &im_bary);
	int GetNumTris() const {return m_UVTris.size();}

private:
	void FindCuts(LightMapper_Base &base);
	void FindCuts_Texel(LightMapper_Base &base, int tx, int ty, int tri_id, const Vector3 &bary);
	void FindCuts_TangentTrace(LightMapper_Base &base, int tx, int ty, Ray r);

	void Transform_Verts(const PoolVector<Vector3> &ptsLocal, PoolVector<Vector3> &ptsWorld, const Transform &tr) const;
	void Transform_Norms(const PoolVector<Vector3> &normsLocal, PoolVector<Vector3> &normsWorld, const Transform &tr) const;
	void ProcessVoxelHits(const Ray &ray, const PackedRay &pray, const Voxel &voxel, float &r_nearest_t, int &r_nearest_tri); // int ignore_triangle_id_p1);
	void ProcessVoxelHits_Old(const Ray &ray, const Voxel &voxel, float &r_nearest_t, int &r_nearest_tri);

	PoolVector<Vector3> m_ptPositions;
	PoolVector<Vector3> m_ptNormals;
	PoolVector<Vector2> m_UVs;
	PoolVector<int> m_Inds;

	LVector<Rect2> m_TriUVaabbs;
	LVector<AABB> m_TriPos_aabbs;

protected:
public:
	LVector<UVTri> m_UVTris;

	LightTracer m_Tracer;

	LVector<Tri> m_Tris;
	LVector<Tri> m_TriNormals;

	// we maintain a list of tris in the form of 2 edges plus a point. These
	// are precalculated as they are used in the intersection test.
	LVector<Tri> m_Tris_EdgeForm;

	Vec3i m_VoxelRange;

	bool m_bUseSIMD;
};



}
