#pragma once

#include "llighttypes.h"
#include "../lvector.h"
#include "llightmapper_base.h"


namespace LM
{

struct RB_Voxel
{
	LVector<FRay> m_Rays;
};

class RayBank : public LightMapper_Base
{
public:
	void RayBank_Reset();
	void RayBank_Create();

	FRay * RayBank_RequestNewRay(const Ray &ray, const Vec3i * pStartVoxel);


private:

//	Vec3i m_Dims;
//	int m_DimsXTimesY;
//	int m_iNumVoxels;
	struct RayBank_Data
	{
		LVector<RB_Voxel> m_Voxels;
	} m_Data_RB;

//	LightScene * m_pLightScene;
};


} // namespace
