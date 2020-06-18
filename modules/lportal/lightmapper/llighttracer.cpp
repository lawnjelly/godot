#include "llighttracer.h"
#include "llightscene.h"



using namespace LM;

void LightTracer::Reset()
{
	m_Voxels.clear();
	m_VoxelBounds.clear();
	m_BFTrisHit.Blank();
	m_iNumTris = 0;


}


void LightTracer::Create(const LightScene &scene)
{
	m_pScene = &scene;
	m_iNumTris = m_pScene->m_Tris.size();

	// precalc voxel planes
	m_VoxelPlanes[0].normal = Vector3(-1, 0, 0);
	m_VoxelPlanes[1].normal = Vector3(1, 0, 0);
	m_VoxelPlanes[2].normal = Vector3(0, -1, 0);
	m_VoxelPlanes[3].normal = Vector3(0, 1, 0);
	m_VoxelPlanes[4].normal = Vector3(0, 0, -1);
	m_VoxelPlanes[5].normal = Vector3(0, 0, 1);

	CalculateWorldBound();

	m_Dims.Set(40, 10, 40);

	m_iNumVoxels = m_Dims.x * m_Dims.y * m_Dims.z;
	m_DimsXTimesY = m_Dims.x * m_Dims.y;

	m_Voxels.resize(m_iNumVoxels);
	m_VoxelBounds.resize(m_iNumVoxels);
	m_BFTrisHit.Create(m_iNumTris);

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

	FillVoxels();
}

// ray translated to voxel space
bool LightTracer::RayTrace_Start(Ray ray, Ray &voxel_ray, Vec3i &start_voxel)
{
	// if tracing from outside, try to trace to the edge of the world bound
	if (!m_SceneWorldBound.has_point(ray.o))
	{
		Vector3 clip;
		if (!IntersectRayAABB(ray, m_SceneWorldBound_contracted, clip))
			return false;

		// does hit the world bound
		ray.o = clip;
	}

	m_BFTrisHit.Blank();

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
	bool within = VoxelWithinBounds(start_voxel);

	// instead of trying to calculate these on the fly with intersection
	// tests we can use simple linear addition to calculate them all quickly.
	if (within)
	{
		//PrecalcRayCuttingPoints(voxel_ray);
	}
//	if (within)
//		DebugCheckPointInVoxel(voxel_ray.o, start_voxel);

	// debug check the voxel number and bounding box are correct


	return within;
}

void LightTracer::DebugCheckWorldPointInVoxel(Vector3 pt, const Vec3i &ptVoxel)
{
	int iVoxelNum = GetVoxelNum(ptVoxel);
	AABB bb = m_VoxelBounds[iVoxelNum];
	bb.grow_by(0.01f);
	assert (bb.has_point(pt));
}


bool LightTracer::RayTrace(const Ray &ray_orig, Ray &ray_out, Vec3i &ptVoxel)
{
	m_TriHits.clear();

#ifdef LIGHTTRACER_IGNORE_VOXELS
	for (int n=0; n<m_iNumTris; n++)
	{
		m_TriHits.push_back(n);
	}
	return true;
#endif

	if (!VoxelWithinBounds(ptVoxel))
		return false;

	// debug check
	DebugCheckLocalPointInVoxel(ray_orig.o, ptVoxel);

	// add the tris in this voxel
	int iVoxelNum = GetVoxelNum(ptVoxel);
	const Voxel &vox = m_Voxels[iVoxelNum];

	for (int n=0; n<vox.m_TriIDs.size(); n++)
	{
		unsigned int id = vox.m_TriIDs[n];

		// check bitfield, the tri may already have been added
		if (m_BFTrisHit.CheckAndSet(id))
			m_TriHits.push_back(id);
	}

	// PLANES are in INTEGER space (voxel space)
	// attempt to cross out of planes
//	const AABB &bb = m_VoxelBounds[iVoxelNum];
	Vector3 mins = Vector3(ptVoxel.x, ptVoxel.y, ptVoxel.z);
	Vector3 maxs = mins + Vector3(1, 1, 1);
//	const Vector3 &mins = bb.position;
//	Vector3 maxs = bb.position + bb.size;
	const Vector3 &dir = ray_orig.d;

	// the 3 intersection points
	Vector3 ptIntersect[3];
	float nearest_hit = FLT_MAX;
	int nearest_hit_plane = -1;

//#define OLD_PLANE_METHOD

	// planes from constants
	if (dir.x >= 0.0f)
	{
#ifdef OLD_PLANE_METHOD
		IntersectPlane(ray_orig, 0, ptIntersect[0], -maxs.x, nearest_hit, nearest_hit_plane);
#else
		// test new routine
		ptIntersect[0].x = maxs.x;
		IntersectAAPlane(ray_orig, 0, ptIntersect[0], nearest_hit, 0, nearest_hit_plane);
#endif
	}
	else
	{
#ifdef OLD_PLANE_METHOD
		IntersectPlane(ray_orig, 1, ptIntersect[0], mins.x, nearest_hit, nearest_hit_plane);
#else
		// test new routine
		ptIntersect[0].x = mins.x;
		IntersectAAPlane(ray_orig, 0, ptIntersect[0], nearest_hit, 1, nearest_hit_plane);
#endif
	}

	if (dir.y >= 0.0f)
	{
#ifdef OLD_PLANE_METHOD
		IntersectPlane(ray_orig, 2, ptIntersect[1], -maxs.y, nearest_hit, nearest_hit_plane);
#else
		// test new routine
		ptIntersect[1].y = maxs.y;
		IntersectAAPlane(ray_orig, 1, ptIntersect[1], nearest_hit, 2, nearest_hit_plane);
#endif
	}
	else
	{
#ifdef OLD_PLANE_METHOD
		IntersectPlane(ray_orig, 3, ptIntersect[1], mins.y, nearest_hit, nearest_hit_plane);
#else
		// test new routine
		ptIntersect[1].y = mins.y;
		IntersectAAPlane(ray_orig, 1, ptIntersect[1], nearest_hit, 3, nearest_hit_plane);
#endif
	}

	if (dir.z >= 0.0f)
	{
#ifdef OLD_PLANE_METHOD
		IntersectPlane(ray_orig, 4, ptIntersect[2], -maxs.z, nearest_hit, nearest_hit_plane);
#else
		// test new routine
		ptIntersect[2].z = maxs.z;
		IntersectAAPlane(ray_orig, 2, ptIntersect[2], nearest_hit, 4, nearest_hit_plane);
#endif
	}
	else
	{
#ifdef OLD_PLANE_METHOD
		IntersectPlane(ray_orig, 5, ptIntersect[2], mins.z, nearest_hit, nearest_hit_plane);
#else
		// test new routine
		ptIntersect[2].z = mins.z;
		IntersectAAPlane(ray_orig, 2, ptIntersect[2], nearest_hit, 5, nearest_hit_plane);
#endif
	}

	// ray out
	ray_out.d = ray_orig.d;
	ray_out.o = ptIntersect[nearest_hit_plane/2];

	switch (nearest_hit_plane)
	{
	case 0:
		ptVoxel.x += 1;
		break;
	case 1:
		ptVoxel.x -= 1;
		break;
	case 2:
		ptVoxel.y += 1;
		break;
	case 3:
		ptVoxel.y -= 1;
		break;
	case 4:
		ptVoxel.z += 1;
		break;
	case 5:
		ptVoxel.z -= 1;
		break;
	default:
		assert (0 && "LightTracer::RayTrace");
		break;
	}

	return true;
}




void LightTracer::FillVoxels()
{
	print_line("FillVoxels");
	int count = 0;
	for (int z=0; z<m_Dims.z; z++)
	{
		for (int y=0; y<m_Dims.y; y++)
		{
			for (int x=0; x<m_Dims.x; x++)
			{
				Voxel &vox = m_Voxels[count];
				AABB aabb = m_VoxelBounds[count++];

				// expand the aabb just a little to account for float error
				aabb.grow_by(0.001f);

				// find all tris within
				for (int t=0; t<m_iNumTris; t++)
				{
					if (m_pScene->m_TriPos_aabbs[t].intersects(aabb))
					{
						// add tri to voxel
						vox.m_TriIDs.push_back(t);
					}
				}

				//print_line("\tvoxel line x " + itos(x) + ", " + itos(vox.m_TriIDs.size()) + " tris.");
			} // for x
		} // for y
	} // for z

}


void LightTracer::CalculateWorldBound()
{
	if (!m_iNumTris)
		return;

	AABB &aabb = m_SceneWorldBound;
	aabb.position = m_pScene->m_Tris[0].pos[0];
	aabb.size = Vector3(0, 0, 0);

	for (int n=0; n<m_iNumTris; n++)
	{
		const Tri &tri = m_pScene->m_Tris[n];
		aabb.expand_to(tri.pos[0]);
		aabb.expand_to(tri.pos[1]);
		aabb.expand_to(tri.pos[2]);
	}

	// exact
	m_SceneWorldBound_contracted = aabb;

	// expanded
	aabb.grow_by(0.01f);
}

bool LightTracer::IntersectRayAABB(const Ray &ray, const AABB &aabb, Vector3 &ptInter)
{
	const Vector3 &mins = aabb.position;
	Vector3 maxs = aabb.position + aabb.size;

	// the 3 intersection points
	Vector3 ptIntersect[3];
	float nearest_hit = FLT_MAX;
	int nearest_hit_plane = -1;
	const Vector3 &dir = ray.d;

	// planes from constants
	if (dir.x >= 0.0f)
		IntersectPlane(ray, 0, ptIntersect[0], -mins.x, nearest_hit, nearest_hit_plane);
	else
		IntersectPlane(ray, 1, ptIntersect[0], maxs.x, nearest_hit, nearest_hit_plane);

	if (dir.y >= 0.0f)
		IntersectPlane(ray, 2, ptIntersect[1], -mins.y, nearest_hit, nearest_hit_plane);
	else
		IntersectPlane(ray, 3, ptIntersect[1], maxs.y, nearest_hit, nearest_hit_plane);

	if (dir.z >= 0.0f)
		IntersectPlane(ray, 4, ptIntersect[2], -mins.z, nearest_hit, nearest_hit_plane);
	else
		IntersectPlane(ray, 5, ptIntersect[2], maxs.z, nearest_hit, nearest_hit_plane);

	if (nearest_hit_plane == -1)
		return false;

	// intersection point
	ptInter = ptIntersect[nearest_hit_plane/2];
	return true;
}
