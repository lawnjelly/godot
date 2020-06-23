#pragma once

#include "llighttypes.h"
#include "../lvector.h"


namespace LM
{

struct RB_Voxel
{
	LVector<FRay> m_Rays;
};

class RayBank
{
public:
	void Reset();
	void Create(Vec3i voxel_dims, LightScene &light_scene);

private:

	Vec3i m_Dims;
	int m_DimsXTimesY;
	int m_iNumVoxels;
	LVector<RB_Voxel> m_Voxels;

	LightScene * m_pLightScene;
};


} // namespace
