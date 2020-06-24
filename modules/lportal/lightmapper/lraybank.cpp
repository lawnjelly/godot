#include "lraybank.h"
#include "llightscene.h"


using namespace LM;

void RayBank::RayBank_Reset()
{
	m_Data_RB.m_Voxels.clear(true);

}

//void RayBank::Create(Vec3i voxel_dims, LightScene &light_scene)
void RayBank::RayBank_Create()
{
//	m_pLightScene = &light_scene;

	// dimensions and quick lookup stuff
//	m_Dims = voxel_dims;
//	m_iNumVoxels = m_Dims.x * m_Dims.y * m_Dims.z;
//	m_DimsXTimesY = m_Dims.x * m_Dims.y;

//	m_Data_RB.m_Voxels.resize(m_iNumVoxels);

}


// either we know the start voxel or we find it during this routine (or it doesn't cut the world)
FRay * RayBank::RayBank_RequestNewRay(const Ray &ray, const Vec3i * pStartVoxel)
{

	return 0;
}
