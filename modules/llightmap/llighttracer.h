#pragma once

#include "core/math/aabb.h"
#include "lbitfield_dynamic.h"
#include "llighttests_simd.h"
#include "llighttypes.h"
#include "lvector.h"
#include <limits.h>

//#define LIGHTTRACER_IGNORE_VOXELS

#define LIGHTTRACER_EXPANDED_BOUND (0.2f)
#define LIGHTTRACER_HALF_EXPANSION (LIGHTTRACER_EXPANDED_BOUND / 2.0f)

namespace LM {

class LightScene;

class Voxel {
public:
	void reset() {
		tri_ids.clear();
		packed_triangles.clear();
		num_tris = 0;
		SDF = UINT_MAX;
	}
	LVector<uint32_t> tri_ids;

	// a COPY of the triangles in SIMD format, edge form
	// contiguous in memory for faster testing
	LVector<PackedTriangles> packed_triangles;
	int num_tris;
	unsigned int SDF; // measured in voxels

	void add_triangle(const Tri &tri, uint32_t tri_id) {
		tri_ids.push_back(tri_id);
		uint32_t packed = num_tris / 4;
		uint32_t mod = num_tris % 4;
		if (packed >= packed_triangles.size()) {
			PackedTriangles *pNew = packed_triangles.request();
			pNew->Create();
		}

		// fill the relevant packed triangle
		PackedTriangles &tris = packed_triangles[packed];
		tris.Set(mod, tri);
		num_tris++;
	}
	void finalize() {
		uint32_t packed = num_tris / 4;
		uint32_t mod = num_tris % 4;
		if (mod) {
			PackedTriangles &tris = packed_triangles[packed];
			tris.Finalize(mod);
		}
		if (num_tris)
			SDF = 0; // seed the SDF
	}
};

class LightTracer {
public:
	friend class RayBank;

	void reset();
	//	void Create(const LightScene &scene, const Vec3i &voxel_dims);
	void create(const LightScene &scene, int voxel_density);

	// translate a real world distance into a number of voxels in each direction
	// (this can be used to limit the distance in ray traces)
	void get_distance_in_voxels(float dist, Vec3i &ptVoxelDist) const;

	bool ray_trace_start(Ray ray, Ray &voxel_ray, Vec3i &start_voxel);
	const Voxel *ray_trace(const Ray &ray_orig, Ray &ray_out, Vec3i &ptVoxel);

	LVector<uint32_t> _tri_hits;

	bool _use_SIMD;
	bool _use_SDF;

	void find_nearest_voxel(const Vector3 &ptWorld, Vec3i &ptVoxel) const;
	const AABB &get_world_bound_expanded() const { return _scene_world_bound_expanded; }
	const AABB &get_world_bound_mid() const { return _scene_world_bound_mid; }
	const AABB &get_world_bound_contracted() const { return _scene_world_bound_contracted; }

	//	void ClampVoxelToBounds(Vec3i &v) const
	//	{
	//		v.x = CLAMP(v.x, 0, m_Dims.x-1);
	//		v.y = CLAMP(v.y, 0, m_Dims.y-1);
	//		v.z = CLAMP(v.z, 0, m_Dims.z-1);
	//	}

	Vec3i estimate_voxel_dims(int voxel_density);

	int get_voxel_num(const Vec3i &pos) const {
		return get_voxel_num(pos.x, pos.y, pos.z);
	}

	Voxel &get_voxel(int v) {
		DEV_ASSERT(v < _num_voxels);
		return _voxels[v];
	}

private:
	void calculate_world_bound();
	void calculate_voxel_dims(int voxel_density);
	void fill_voxels();
	//void fill_voxels_old();
	void calculate_SDF();
	void debug_save_SDF();
	void calculate_SDF_Voxel(const Vec3i &ptCentre);
	void calculate_SDF_assess_neighbour(const Vec3i &pt, unsigned int &min_SDF);
	bool voxel_within_bounds(Vec3i v) const {
		if (v.x < 0)
			return false;
		if (v.y < 0)
			return false;
		if (v.z < 0)
			return false;
		if (v.x >= _dims.x)
			return false;
		if (v.y >= _dims.y)
			return false;
		if (v.z >= _dims.z)
			return false;
		return true;
	}
	void debug_check_world_point_in_voxel(Vector3 pt, const Vec3i &ptVoxel);
	void debug_check_local_point_in_voxel(Vector3 pt, const Vec3i &ptVoxel) {
		//		DEV_ASSERT ((int) (pt.x+0.5f) == ptVoxel.x);
		//		DEV_ASSERT ((int) (pt.y+0.5f) == ptVoxel.y);
		//		DEV_ASSERT ((int) (pt.z+0.5f) == ptVoxel.z);
	}

	LVector<Voxel> _voxels;
	LVector<AABB> _voxel_bounds;
	LBitField_Dynamic _BF_tris_hit;
	int _num_tris;

	// slightly expanded
	AABB _scene_world_bound_expanded;
	AABB _scene_world_bound_mid;

	// exact
	AABB _scene_world_bound_contracted;

	const LightScene *_p_scene;

	Vec3i _dims;
	int _dims_x_times_y;
	Vector3 _voxel_size;
	int _num_voxels;
	//	Plane m_VoxelPlanes[6];

	// to prevent integer overflow in the voxels, we calculate the maximum possible distance
	real_t _max_test_dist;

	void _world_coord_to_voxel_coord(Vector3 &r_pt) const {
		DEV_ASSERT(_voxel_size.x > 0);
		DEV_ASSERT(_voxel_size.y > 0);
		DEV_ASSERT(_voxel_size.z > 0);
		r_pt.x -= _scene_world_bound_expanded.position.x;
		r_pt.x /= _voxel_size.x;
		r_pt.y -= _scene_world_bound_expanded.position.y;
		r_pt.y /= _voxel_size.y;
		r_pt.z -= _scene_world_bound_expanded.position.z;
		r_pt.z /= _voxel_size.z;
	}

	int get_voxel_num(int x, int y, int z) const {
		int v = z * _dims_x_times_y;
		v += y * _dims.x;
		v += x;
		return v;
	}

	const Voxel &get_voxel(const Vec3i &pos) const {
		int v = get_voxel_num(pos);
		DEV_ASSERT(v < _num_voxels);
		return _voxels[v];
	}
	Voxel &get_voxel(const Vec3i &pos) {
		int v = get_voxel_num(pos);
		DEV_ASSERT(v < _num_voxels);
		return _voxels[v];
	}

	// the pt is both the output intersection point, and the input plane constant (in the correct tuple)
	void intersect_AA_plane_only_within_AABB(const AABB &aabb, const Ray &ray, int axis, Vector3 &pt, float &nearest_hit, int plane_id, int &nearest_plane_id, float aabb_epsilon = 0.0f) const {
		if (ray.intersect_AA_plane(axis, pt)) {
			Vector3 offset = (pt - ray.o);
			float dist = offset.length_squared();
			if (dist < nearest_hit) {
				// must be within aabb
				if (aabb.has_point(pt)) {
					nearest_hit = dist;
					nearest_plane_id = plane_id;
				}
			}
		}
	}

	// the pt is both the output intersection point, and the input plane constant (in the correct tuple)
	void intersect_AA_plane(const Ray &ray, int axis, Vector3 &pt, float &nearest_hit, int plane_id, int &nearest_plane_id) const {
		if (ray.intersect_AA_plane(axis, pt)) {
			Vector3 offset = (pt - ray.o);
			float dist = offset.length_squared();
			if (dist < nearest_hit) {
				nearest_hit = dist;
				nearest_plane_id = plane_id;
			}
		}
	}

	bool intersect_ray_AABB(const Ray &ray, const AABB &aabb, Vector3 &ptInter);
};

} // namespace LM
