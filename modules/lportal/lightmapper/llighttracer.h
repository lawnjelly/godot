#pragma once

#include "../lvector.h"
#include "../lbitfield_dynamic.h"
#include "core/math/aabb.h"
#include "llighttypes.h"


namespace LM
{

class LightScene;

class Voxel
{
public:
	LVector<uint32_t> m_TriIDs;
};

class LightTracer
{
public:
	void Create(const LightScene &scene);

	bool RayTrace_Start(Ray ray, Ray &voxel_ray, Vec3i &start_voxel);
	bool RayTrace(const Ray &ray_orig, Ray &ray_out, Vec3i &ptVoxel);
	LVector<uint32_t> m_TriHits;

private:
	void CalculateWorldBound();
	void FillVoxels();
	bool VoxelWithinBounds(Vec3i v) const
	{
		if (v.x < 0) return false;
		if (v.y < 0) return false;
		if (v.z < 0) return false;
		if (v.x >= m_Dims.x) return false;
		if (v.y >= m_Dims.y) return false;
		if (v.z >= m_Dims.z) return false;
		return true;
	}
	void DebugCheckWorldPointInVoxel(Vector3 pt, const Vec3i &ptVoxel);
	void DebugCheckLocalPointInVoxel(Vector3 pt, const Vec3i &ptVoxel)
	{
//		assert ((int) (pt.x+0.5f) == ptVoxel.x);
//		assert ((int) (pt.y+0.5f) == ptVoxel.y);
//		assert ((int) (pt.z+0.5f) == ptVoxel.z);
	}

	LVector<Voxel> m_Voxels;
	LVector<AABB> m_VoxelBounds;
	Lawn::LBitField_Dynamic m_BFTrisHit;
	int m_iNumTris;

	// slightly expanded
	AABB m_SceneWorldBound;
	// exact
	AABB m_SceneWorldBound_contracted;

	const LightScene * m_pScene;

	int GetVoxelNum(const Vec3i &pos) const
	{
		int v = pos.z * m_DimsXTimesY;
		v += pos.y * m_Dims.x;
		v += pos.x;
		return v;
	}

	const Voxel &GetVoxel(const Vec3i &pos) const
	{
		int v = GetVoxelNum(pos);
		assert (v < m_iNumVoxels);
		return m_Voxels[v];
	}

	Vec3i m_Dims;
	int m_DimsXTimesY;
	Vector3 m_VoxelSize;
	int m_iNumVoxels;
	Plane m_VoxelPlanes[6];

	void IntersectAAPlane(const Ray &ray, int axis, Vector3 &pt, float &nearest_hit, int plane_id, int &nearest_plane_id) const
	{
		if (ray.IntersectAAPlane(axis, pt))
		{
			Vector3 offset = (pt - ray.o);
			float dist = offset.length_squared();
			if (dist < nearest_hit)
			{
				nearest_hit = dist;
				nearest_plane_id = plane_id;
			}
		}
	}

	bool IntersectRayAABB(const Ray &ray, const AABB &aabb, Vector3 &ptInter);
	void IntersectPlane(const Ray &ray, int plane_id, Vector3 &ptIntersect, float constant, float &nearest_hit, int &nearest_plane_id)
	{
		m_VoxelPlanes[plane_id].d = constant;
		bool bHit = m_VoxelPlanes[plane_id].intersects_ray(ray.o, ray.d, &ptIntersect);
		if (bHit)
		{
			Vector3 offset = (ptIntersect - ray.o);
			float dist = offset.length_squared();
			if (dist < nearest_hit)
			{
				nearest_hit = dist;
				nearest_plane_id = plane_id;
			}
		} // if hit
	}
};

}
