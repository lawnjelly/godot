#include "llightprobe.h"
#include "llightmapper.h"

using namespace LM;


void LightProbes::Create(LightMapper &lm)
{
	// get the aabb (smallest) of the world bound.
	AABB bb = lm.GetTracer().GetWorldBound_contracted();

	// can't make probes for a non existant world
	if (bb.get_shortest_axis_size() <= 0.0f)
		return;

	// start hard coded
	Vec3i dims;
	dims.Set(4, 4, 1);

	Vector3 voxel_dims;
	dims.To(voxel_dims);

	// voxel dimensions and start point
	Vector3 voxel_size;
	voxel_size = bb.size / voxel_dims;

	// start point is offset by half a voxel
	Vector3 ptMin = bb.position + (voxel_size * 0.5f);


}
