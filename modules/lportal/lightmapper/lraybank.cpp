#include "lraybank.h"
#include "llightscene.h"


using namespace LM;

void RayBank::Reset()
{
	m_Voxels.clear(true);

}

void RayBank::Create(Vec3i voxel_dims, LightScene &light_scene)
{
	m_pLightScene = &light_scene;

	// dimensions and quick lookup stuff
	m_Dims = voxel_dims;
	m_iNumVoxels = m_Dims.x * m_Dims.y * m_Dims.z;
	m_DimsXTimesY = m_Dims.x * m_Dims.y;

	m_Voxels.resize(m_iNumVoxels);

}
