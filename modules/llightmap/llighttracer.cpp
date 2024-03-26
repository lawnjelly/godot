#include "llighttracer.h"
#include "core/os/os.h"
#include "llightscene.h"

using namespace LM;

void LightTracer::reset() {
	_voxels.clear(true);
	_voxel_bounds.clear(true);
	_BF_tris_hit.Blank();
	_num_tris = 0;
}

void LightTracer::create(const LightScene &scene, int voxel_density) {
	_use_SDF = true;

	_p_scene = &scene;
	_num_tris = _p_scene->_tris.size();

	calculate_world_bound();
	calculate_voxel_dims(voxel_density);

	_num_voxels = _dims.x * _dims.y * _dims.z;
	_dims_x_times_y = _dims.x * _dims.y;

	_voxels.resize(_num_voxels);
	_voxel_bounds.resize(_num_voxels);
	_BF_tris_hit.Create(_num_tris);

	_voxel_size.x = _scene_world_bound_expanded.size.x / _dims.x;
	_voxel_size.y = _scene_world_bound_expanded.size.y / _dims.y;
	_voxel_size.z = _scene_world_bound_expanded.size.z / _dims.z;

	// calculate the maximum allowable test distance so as not to overflow 32 bit voxel bounds
	_max_test_dist = _scene_world_bound_expanded.get_longest_axis_size() * 3.0; // a bit for luck

	// fill the bounds
	AABB aabb;
	aabb.size = _voxel_size;
	int count = 0;
	for (int z = 0; z < _dims.z; z++) {
		aabb.position.z = _scene_world_bound_expanded.position.z + (z * _voxel_size.z);
		for (int y = 0; y < _dims.y; y++) {
			aabb.position.y = _scene_world_bound_expanded.position.y + (y * _voxel_size.y);
			for (int x = 0; x < _dims.x; x++) {
				aabb.position.x = _scene_world_bound_expanded.position.x + (x * _voxel_size.x);
				_voxel_bounds[count++] = aabb;
			} // for x
		} // for y
	} // for z

	fill_voxels();
}

void LightTracer::find_nearest_voxel(const Vector3 &ptWorld, Vec3i &ptVoxel) const {
	Vector3 pt = ptWorld;
	pt -= _scene_world_bound_expanded.position;
	pt.x /= _voxel_size.x;
	pt.y /= _voxel_size.y;
	pt.z /= _voxel_size.z;

	ptVoxel.set(pt.x, pt.y, pt.z);
}

void LightTracer::get_distance_in_voxels(float dist, Vec3i &ptVoxelDist) const {
	// note this will screw up with zero voxel size
	if ((_voxel_size.x == 0.0f) || (_voxel_size.y == 0.0f) || (_voxel_size.z == 0.0f)) {
		ptVoxelDist.set(0, 0, 0);
		return;
	}

	// bug .. if dist is FLT_MAX, we can get an overflow in the integer ptVoxelDist.
	// we need to account for this.
	dist = MIN(dist, _max_test_dist);

	ptVoxelDist.x = (dist / _voxel_size.x) + 1; //+1;
	ptVoxelDist.y = (dist / _voxel_size.y) + 1; //+1;
	ptVoxelDist.z = (dist / _voxel_size.z) + 1; //+1;
}

// ray translated to voxel space
bool LightTracer::ray_trace_start(Ray ray, Ray &voxel_ray, Vec3i &start_voxel) {
	// if tracing from outside, try to trace to the edge of the world bound
	if (!_scene_world_bound_expanded.has_point(ray.o)) {
		Vector3 clip;
		//if (!IntersectRayAABB(ray, m_SceneWorldBound_contracted, clip))
		if (!intersect_ray_AABB(ray, _scene_world_bound_expanded, clip))
			return false;

		// does hit the world bound
		ray.o = clip;
	}

	//	m_BFTrisHit.Blank();

	voxel_ray.o = ray.o - _scene_world_bound_expanded.position;
	voxel_ray.o.x /= _voxel_size.x;
	voxel_ray.o.y /= _voxel_size.y;
	voxel_ray.o.z /= _voxel_size.z;
	voxel_ray.d.x = ray.d.x / _voxel_size.x;
	voxel_ray.d.y = ray.d.y / _voxel_size.y;
	voxel_ray.d.z = ray.d.z / _voxel_size.z;
	voxel_ray.d.normalize();

	start_voxel.x = voxel_ray.o.x;
	start_voxel.y = voxel_ray.o.y;
	start_voxel.z = voxel_ray.o.z;

	//	m_TriHits.clear();

	// out of bounds?
	bool within = voxel_within_bounds(start_voxel);

	// instead of trying to calculate these on the fly with intersection
	// tests we can use simple linear addition to calculate them all quickly.
	if (within) {
		//PrecalcRayCuttingPoints(voxel_ray);
	}
	//	if (within)
	//		DebugCheckPointInVoxel(voxel_ray.o, start_voxel);

	// debug check the voxel number and bounding box are correct

	//	if (m_bUseSDF)
	//		print_line("Ray start");

	return within;
}

void LightTracer::debug_check_world_point_in_voxel(Vector3 pt, const Vec3i &ptVoxel) {
	int iVoxelNum = get_voxel_num(ptVoxel);
	AABB bb = _voxel_bounds[iVoxelNum];
	bb.grow_by(0.01f);
	assert(bb.has_point(pt));
}

//bool LightTracer::RayTrace(const Ray &ray_orig, Ray &ray_out, Vec3i &ptVoxel)
const Voxel *LightTracer::ray_trace(const Ray &ray_orig, Ray &ray_out, Vec3i &ptVoxel) {
	//m_TriHits.clear();

#ifdef LIGHTTRACER_IGNORE_VOXELS
	for (int n = 0; n < m_iNumTris; n++) {
		m_TriHits.push_back(n);
	}
	return true;
#endif

	if (!voxel_within_bounds(ptVoxel))
		return 0;

	// debug check
	debug_check_local_point_in_voxel(ray_orig.o, ptVoxel);

	// add the tris in this voxel
	int iVoxelNum = get_voxel_num(ptVoxel);
	const Voxel &vox = _voxels[iVoxelNum];
	const Voxel *pCurrVoxel = &vox;

	//	const AABB &bb = m_VoxelBounds[iVoxelNum];
	//	print_line("Checking Voxel " + ptVoxel.ToString() + " bound " + String(bb));

	if (!_use_SIMD) {
		/*
		for (int n=0; n<vox.m_TriIDs.size(); n++)
		{
			unsigned int id = vox.m_TriIDs[n];

			// check bitfield, the tri may already have been added
			if (m_BFTrisHit.CheckAndSet(id))
				m_TriHits.push_back(id);
		}
		*/
	}

//#define LLIGHT_USE_SDF
#ifdef LLIGHT_USE_SDF
	int sdf = vox.m_SDF;

	sdf -= 1;
	if (sdf >= 0) {
		;
	} else {
		sdf = 0;
	}
	//	sdf = 0;

	if (!m_bUseSDF) {
		sdf = 0;
	}

	//		print_line("\tvoxel (" + itos(ptVoxel.x) + " " + itos(ptVoxel.y) + " " + itos(ptVoxel.z) +
	//		"),	SDF " + itos(vox.m_SDF) + " num_tris " + itos (vox.m_iNumTriangles) + ", jump " + itos(sdf));

	int change_withsdf = sdf + 1;

	//	sdf = 0;
	// SDF is now how many planes to cross.

	// PLANES are in INTEGER space (voxel space)
	// attempt to cross out of planes
	//	const AABB &bb = m_VoxelBounds[iVoxelNum];
	Vector3 mins = Vector3(ptVoxel.x - sdf, ptVoxel.y - sdf, ptVoxel.z - sdf);
	Vector3 maxs = mins + Vector3(change_withsdf, change_withsdf, change_withsdf);

	// add a bit of bias to ensure we cross the boundary into another voxel and don't get float error
	// (note we may also have to catch triangles slightly outside the voxel to compensate for this)
	const float plane_bias = 0.001f;
	Vector3 plane_bias3 = Vector3(plane_bias, plane_bias, plane_bias);
	mins -= plane_bias3;
	maxs += plane_bias3;
#else
	Vector3 mins = Vector3(ptVoxel.x, ptVoxel.y, ptVoxel.z);
	Vector3 maxs = mins + Vector3(1, 1, 1);
#endif

	//	const Vector3 &mins = bb.position;
	//	Vector3 maxs = bb.position + bb.size;
	const Vector3 &dir = ray_orig.d;

	// the 3 intersection points
	Vector3 ptIntersect[3];
	float nearest_hit = FLT_MAX;
	int nearest_hit_plane = -1;

	//Vector3 ptBias;

	// planes from constants
	if (dir.x >= 0.0f) {
		ptIntersect[0].x = maxs.x;
		intersect_AA_plane(ray_orig, 0, ptIntersect[0], nearest_hit, 0, nearest_hit_plane);
		//ptBias.x = 0.5f;
	} else {
		ptIntersect[0].x = mins.x;
		intersect_AA_plane(ray_orig, 0, ptIntersect[0], nearest_hit, 1, nearest_hit_plane);
		//ptBias.x = -0.5f;
	}

	if (dir.y >= 0.0f) {
		ptIntersect[1].y = maxs.y;
		intersect_AA_plane(ray_orig, 1, ptIntersect[1], nearest_hit, 2, nearest_hit_plane);
		//ptBias.y = 0.5f;
	} else {
		ptIntersect[1].y = mins.y;
		intersect_AA_plane(ray_orig, 1, ptIntersect[1], nearest_hit, 3, nearest_hit_plane);
		//ptBias.y = -0.5f;
	}

	if (dir.z >= 0.0f) {
		ptIntersect[2].z = maxs.z;
		intersect_AA_plane(ray_orig, 2, ptIntersect[2], nearest_hit, 4, nearest_hit_plane);
		//ptBias.z = 0.5f;
	} else {
		ptIntersect[2].z = mins.z;
		intersect_AA_plane(ray_orig, 2, ptIntersect[2], nearest_hit, 5, nearest_hit_plane);
		//ptBias.z = -0.5f;
	}

	// ray out
	ray_out.d = ray_orig.d;
	ray_out.o = ptIntersect[nearest_hit_plane / 2];

#ifdef LLIGHT_USE_SDF
	const Vector3 out = ray_out.o; // + ptBias;
	ptVoxel.x = floorf(out.x);
	ptVoxel.y = floorf(out.y);
	ptVoxel.z = floorf(out.z);
#else

	switch (nearest_hit_plane) {
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
			assert(0 && "LightTracer::RayTrace");
			break;
	}

#endif
	return pCurrVoxel;
}

void LightTracer::debug_save_SDF() {
	int width = _dims.x;
	int height = _dims.z;

	Ref<Image> image = memnew(Image(width, height, false, Image::FORMAT_RGBA8));
	image->lock();
	int y = _dims.y - 1;

	for (int z = 0; z < _dims.z; z++) {
		//		for (int y=0; y<m_Dims.y; y++)
		//		{
		for (int x = 0; x < _dims.x; x++) {
			Vec3i pt;
			pt.set(x, y, z);

			int sdf = get_voxel(pt).SDF;
			float f = sdf * 0.25f;
			image->set_pixel(x, z, Color(f, f, f, 1.0f));
		}
		//		}
	}

	image->unlock();
	image->save_png("sdf.png");
}

void LightTracer::calculate_SDF() {
	return;

	print_line("Calculating SDF");

	// look at the surrounding neighbours. We should be at a minimum, the lowest neighbour +1
	int iters = _dims.x;
	if (_dims.y > iters)
		iters = _dims.y;
	if (_dims.z > iters)
		iters = _dims.z;

	for (int i = 0; i < iters; i++) {
		// SDF is seeded with zero in the filled voxels
		for (int z = 0; z < _dims.z; z++) {
			for (int y = 0; y < _dims.y; y++) {
				for (int x = 0; x < _dims.x; x++) {
					Vec3i pt;
					pt.set(x, y, z);
					calculate_SDF_Voxel(pt);
				}
			}
		}

	} // for iteration

	// debug print sdf
	//	Debug_SaveSDF();
}

void LightTracer::calculate_SDF_assess_neighbour(const Vec3i &pt, unsigned int &min_SDF) {
	// on map?
	if (!voxel_within_bounds(pt))
		return;

	const Voxel &vox = get_voxel(pt);
	if (vox.SDF < min_SDF)
		min_SDF = vox.SDF;
}

void LightTracer::calculate_SDF_Voxel(const Vec3i &ptCentre) {
	Voxel &vox = get_voxel(ptCentre);

	unsigned int lowest = UINT_MAX - 1;

	for (int nz = -1; nz <= 1; nz++) {
		for (int ny = -1; ny <= 1; ny++) {
			for (int nx = -1; nx <= 1; nx++) {
				if ((nx == 0) && (ny == 0) && (nz == 0)) {
				} else {
					Vec3i pt;
					pt.set(ptCentre.x + nx, ptCentre.y + ny, ptCentre.z + nz);
					calculate_SDF_assess_neighbour(pt, lowest);
				}
			} // for nx
		} // for ny
	} // for nz

	lowest += 1;
	if (vox.SDF > lowest)
		vox.SDF = lowest;
}

void LightTracer::fill_voxels() {
	print_line("FillVoxels : Num AABBs " + itos(_p_scene->_tri_pos_aabbs.size()));
	print_line("\tnum tris : " + itos(_num_tris));

	uint32_t tris_added = 0;

	for (int t = 0; t < _num_tris; t++) {
		AABB tri_aabb = _p_scene->_tri_pos_aabbs[t];

		// expand a little for float error
		tri_aabb.grow_by(0.001f);

		Vector3 mins = tri_aabb.position;
		Vector3 maxs = mins + tri_aabb.size;

		// find all voxels it intersects
		// by changing the AABB to voxel coords.
		_world_coord_to_voxel_coord(mins);
		_world_coord_to_voxel_coord(maxs);

		Vec3i mins_i;
		Vec3i maxs_i;

		mins_i.set_floor(mins);
		maxs_i.set_ceil(maxs);

		mins_i.x = CLAMP(mins_i.x, 0, _dims.x);
		mins_i.y = CLAMP(mins_i.y, 0, _dims.y);
		mins_i.z = CLAMP(mins_i.z, 0, _dims.z);
		maxs_i.x = CLAMP(maxs_i.x, 0, _dims.x);
		maxs_i.y = CLAMP(maxs_i.y, 0, _dims.y);
		maxs_i.z = CLAMP(maxs_i.z, 0, _dims.z);

		/*
		if ((t % 1024) == 0) {
			String sz = "Tri " + itos(t) + " :\t";
			sz += mins_i.to_string();
			sz += " to ";
			sz += maxs_i.to_string();
			sz += " ... total tris added " + itos(tris_added);
			print_line(sz);
		}
*/

		for (int z = mins_i.z; z < maxs_i.z; z++) {
			for (int y = mins_i.y; y < maxs_i.y; y++) {
				for (int x = mins_i.x; x < maxs_i.x; x++) {
					// Voxel number
					int v = get_voxel_num(x, y, z);
					Voxel &vox = get_voxel(v);

					vox.add_triangle(_p_scene->_tris_edge_form[t], t);
					tris_added++;
				}
			}
		}
	}

	print_line("\ttotal voxel tris added : " + itos(tris_added));

	int count = 0;
	for (int z = 0; z < _dims.z; z++) {
		for (int y = 0; y < _dims.y; y++) {
			for (int x = 0; x < _dims.x; x++) {
				Voxel &vox = _voxels[count++];
				vox.finalize();
			}
		}
	}

	calculate_SDF();
}

/*
void LightTracer::fill_voxels_old() {
	print_line("FillVoxels : Num AABBs " + itos(_p_scene->_tri_pos_aabbs.size()));
	print_line("NumTris " + itos(_num_tris));

	uint32_t tris_added = 0;

	int count = 0;
	for (int z = 0; z < _dims.z; z++) {
		for (int y = 0; y < _dims.y; y++) {
			for (int x = 0; x < _dims.x; x++) {
				Voxel &vox = _voxels[count];
				vox.reset();
				AABB aabb = _voxel_bounds[count++];

				// expand the aabb just a little to account for float error
				aabb.grow_by(0.001f);

				//				if ((z == 1) && (y == 1) && (x == 0))
				//				{
				//					print_line("AABB voxel is " + String(Variant(aabb)));
				//				}

				// find all tris within
				for (int t = 0; t < _num_tris; t++) {
					if (_p_scene->_tri_pos_aabbs[t].intersects(aabb)) {
						// add tri to voxel
						//vox.m_TriIDs.push_back(t);
						vox.add_triangle(_p_scene->_tris_edge_form[t], t);
						tris_added++;

						//						if ((z == 1) && (y == 1) && (x == 0))
						//						{
						//							print_line("tri " + itos (t) + " AABB " + String(Variant(m_pScene->m_TriPos_aabbs[t])));
						//						}
					}
				}
				vox.finalize();

				//				if ((z == 1) && (y == 1) && (x == 0))
				//				{
				//					print_line("\tvoxel line x " + itos(x) + ", " + itos(vox.m_TriIDs.size()) + " tris.");
				//				}
			} // for x
		} // for y
	} // for z

	print_line("Total tris added : " + itos(tris_added));

	calculate_SDF();
}
*/

Vec3i LightTracer::estimate_voxel_dims(int voxel_density) {
	Vec3i dims;

	const AABB &aabb = _scene_world_bound_expanded;
	float max_length = aabb.get_longest_axis_size();

	dims.x = ((aabb.size.x / max_length) * voxel_density) + 0.01f;
	dims.y = ((aabb.size.y / max_length) * voxel_density) + 0.01f;
	dims.z = ((aabb.size.z / max_length) * voxel_density) + 0.01f;

	// minimum of 1
	dims.x = MAX(dims.x, 1);
	dims.y = MAX(dims.y, 1);
	dims.z = MAX(dims.z, 1);

	return dims;
}

void LightTracer::calculate_voxel_dims(int voxel_density) {
	_dims = estimate_voxel_dims(voxel_density);
	print_line("voxels dims : " + itos(_dims.x) + ", " + itos(_dims.y) + ", " + itos(_dims.z));
}

void LightTracer::calculate_world_bound() {
	if (!_num_tris)
		return;

	AABB &aabb = _scene_world_bound_expanded;
	aabb.position = _p_scene->_tris[0].pos[0];
	aabb.size = Vector3(0, 0, 0);

	for (int n = 0; n < _num_tris; n++) {
		const Tri &tri = _p_scene->_tris[n];
		aabb.expand_to(tri.pos[0]);
		aabb.expand_to(tri.pos[1]);
		aabb.expand_to(tri.pos[2]);
	}

	// exact
	_scene_world_bound_contracted = aabb;

	// expanded

	// it is CRUCIAL that the expansion here is more than the push in provided
	// by LightTracer::IntersectRayAABB
	// otherwise triangles at the very edges of the world will be missed by the ray tracing.

	aabb.grow_by(LIGHTTRACER_EXPANDED_BOUND);

	_scene_world_bound_mid = _scene_world_bound_contracted;
	_scene_world_bound_mid.grow_by(LIGHTTRACER_HALF_EXPANSION);
}

bool LightTracer::intersect_ray_AABB(const Ray &ray, const AABB &aabb, Vector3 &ptInter) {
	// the 3 intersection points
	const Vector3 &mins = aabb.position;
	Vector3 maxs = aabb.position + aabb.size;

	Vector3 ptIntersect[3];
	float nearest_hit = FLT_MAX;
	int nearest_hit_plane = -1;
	const Vector3 &dir = ray.d;

	// planes from constants
	if (dir.x <= 0.0f) {
		ptIntersect[0].x = maxs.x;
		intersect_AA_plane_only_within_AABB(aabb, ray, 0, ptIntersect[0], nearest_hit, 0, nearest_hit_plane);
	} else {
		ptIntersect[0].x = mins.x;
		intersect_AA_plane_only_within_AABB(aabb, ray, 0, ptIntersect[0], nearest_hit, 1, nearest_hit_plane);
	}

	if (dir.y <= 0.0f) {
		ptIntersect[1].y = maxs.y;
		intersect_AA_plane_only_within_AABB(aabb, ray, 1, ptIntersect[1], nearest_hit, 2, nearest_hit_plane);
	} else {
		ptIntersect[1].y = mins.y;
		intersect_AA_plane_only_within_AABB(aabb, ray, 1, ptIntersect[1], nearest_hit, 3, nearest_hit_plane);
	}

	if (dir.z <= 0.0f) {
		ptIntersect[2].z = maxs.z;
		intersect_AA_plane_only_within_AABB(aabb, ray, 2, ptIntersect[2], nearest_hit, 4, nearest_hit_plane);
	} else {
		ptIntersect[2].z = mins.z;
		intersect_AA_plane_only_within_AABB(aabb, ray, 2, ptIntersect[2], nearest_hit, 5, nearest_hit_plane);
	}

	ptInter = ptIntersect[nearest_hit_plane / 2];

	if (nearest_hit_plane == -1)
		return false;

	// recalculate intersect using distance plus epsilon
	float nearest_length = sqrtf(nearest_hit);

	// this epsilon MUST be less than the world expansion in LightTracer::CalculateWorldBound
	ptInter = ray.o + (ray.d * (nearest_length + LIGHTTRACER_HALF_EXPANSION));

	if (aabb.has_point(ptInter))
		return true;

	return false;
}
