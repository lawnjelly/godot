#pragma once

#include "../lvector.h"
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

	bool RayTrace_Start(const Ray &ray, Ray &voxel_ray, Vec3i &start_voxel);
	bool RayTrace(const Ray &ray_orig, Ray &r, Vec3i &ptVoxel);
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
		return false;
	}

	LVector<Voxel> m_Voxels;
	LVector<AABB> m_VoxelBounds;

	AABB m_SceneWorldBound;
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
};

}
