#include "llighttracer.h"
#include "llightscene.h"


using namespace LM;

void LightTracer::Create(const LightScene &scene)
{
	m_pScene = &scene;
	CalculateWorldBound();

	m_Dims.Set(10, 1, 10);

	m_iNumVoxels = m_Dims.x * m_Dims.y * m_Dims.z;
	m_DimsXTimesY = m_Dims.x * m_Dims.y;

	m_Voxels.resize(m_iNumVoxels);
	m_VoxelBounds.resize(m_iNumVoxels);

	m_VoxelSize.x = m_SceneWorldBound.size.x / m_Dims.x;
	m_VoxelSize.y = m_SceneWorldBound.size.y / m_Dims.y;
	m_VoxelSize.z = m_SceneWorldBound.size.z / m_Dims.z;

	// fill the bounds
	AABB aabb;
	aabb.size = m_VoxelSize;
	int count = 0;
	for (int z=0; z<m_Dims.z; z++)
	{
		aabb.position.z = m_SceneWorldBound.position.z + (z * m_VoxelSize.z);
		for (int y=0; y<m_Dims.y; y++)
		{
			aabb.position.y = m_SceneWorldBound.position.y + (y * m_VoxelSize.y);
			for (int x=0; x<m_Dims.x; x++)
			{
				aabb.position.x = m_SceneWorldBound.position.x + (x * m_VoxelSize.x);
				m_VoxelBounds[count++] = aabb;
			} // for x
		} // for y
	} // for z
}

// ray translated to voxel space
bool LightTracer::RayTrace_Start(const Ray &ray, Ray &voxel_ray, Vec3i &start_voxel)
{
	voxel_ray.o = ray.o - m_SceneWorldBound.position;
	voxel_ray.o.x /= m_VoxelSize.x;
	voxel_ray.o.y /= m_VoxelSize.y;
	voxel_ray.o.z /= m_VoxelSize.z;
	voxel_ray.d.x = ray.d.x / m_VoxelSize.x;
	voxel_ray.d.y = ray.d.y / m_VoxelSize.y;
	voxel_ray.d.z = ray.d.z / m_VoxelSize.z;
	voxel_ray.d.normalize();

	start_voxel.x = voxel_ray.o.x;
	start_voxel.y = voxel_ray.o.y;
	start_voxel.z = voxel_ray.o.z;

	m_TriHits.clear();

	// out of bounds?
	return VoxelWithinBounds(start_voxel);
}

bool LightTracer::RayTrace(const Ray &ray_orig, Ray &r, Vec3i &ptVoxel)
{
	if (!VoxelWithinBounds(ptVoxel))
		return false;

	// add the tris in this voxel
	const Voxel &vox = GetVoxel(ptVoxel);

	for (int n=0; n<vox.m_TriIDs.size(); n++)
	{
		m_TriHits.push_back(vox.m_TriIDs[n]);
	}

	// attempt to cross out of planes


	return true;
}

void LightTracer::FillVoxels()
{
	int nTris = m_pScene->m_TriPos_aabbs.size();
	int count = 0;
	for (int z=0; z<m_Dims.z; z++)
	{
		for (int y=0; y<m_Dims.y; y++)
		{
			for (int x=0; x<m_Dims.x; x++)
			{
				Voxel &vox = m_Voxels[count];
				const AABB &aabb = m_VoxelBounds[count++];

				// find all tris within
				for (int t=0; t<nTris; t++)
				{
					if (m_pScene->m_TriPos_aabbs[t].intersects(aabb))
					{
						// add tri to voxel
						vox.m_TriIDs.push_back(t);
					}
				}

			} // for x
		} // for y
	} // for z

}


void LightTracer::CalculateWorldBound()
{
	int nTris = m_pScene->m_Tris.size();
	if (!nTris)
		return;

	AABB &aabb = m_SceneWorldBound;
	aabb.position = m_pScene->m_Tris[0].pos[0];
	aabb.size = Vector3(0, 0, 0);

	for (int n=0; n<m_pScene->m_Tris.size(); n++)
	{
		const Tri &tri = m_pScene->m_Tris[n];
		aabb.expand_to(tri.pos[0]);
		aabb.expand_to(tri.pos[1]);
		aabb.expand_to(tri.pos[2]);
	}
}
