/*************************************************************************/
/*  lvoxel_tracer.h                                                      */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#pragma once

#include "core/math/aabb.h"
#include "llight_tests_simd.h"
#include "llight_types.h"
#include "lvector.h"
#include <limits.h>

//#define LIGHTTRACER_IGNORE_VOXELS

#define LIGHTTRACER_EXPANDED_BOUND (0.2f)
#define LIGHTTRACER_HALF_EXPANSION (LIGHTTRACER_EXPANDED_BOUND / 2.0f)

namespace LM {

class Voxel {
public:
	LVector<uint32_t> _tri_IDs;

	// a COPY of the triangles in SIMD format, edge form
	// contiguous in memory for faster testing
	LVector<PackedTriangles> _packed_triangles;
	int _num_triangles;
	unsigned int _SDF; // measured in voxels

	void reset() {
		_tri_IDs.clear();
		_packed_triangles.clear();
		_num_triangles = 0;
		_SDF = UINT_MAX;
	}

	void add_triangle(const Tri &tri, uint32_t tri_id) {
		_tri_IDs.push_back(tri_id);
		uint32_t packed = _num_triangles / 4;
		uint32_t mod = _num_triangles % 4;
		if ((int)packed >= _packed_triangles.size()) {
			PackedTriangles *pNew = _packed_triangles.request();
			pNew->create();
		}

		// fill the relevant packed triangle
		PackedTriangles &tris = _packed_triangles[packed];
		tris.set(mod, tri);
		_num_triangles++;
	}
	void finalize() {
		uint32_t packed = _num_triangles / 4;
		uint32_t mod = _num_triangles % 4;
		if (mod) {
			PackedTriangles &tris = _packed_triangles[packed];
			tris.finalize(mod);
		}
		if (_num_triangles)
			_SDF = 0; // seed the SDF
	}
};

class VoxelTracer {
public:
	friend class RayBank;

	void reset();
	void create(const LightGeometry &geom, int voxel_density);

	// translate a real world distance into a number of voxels in each direction
	// (this can be used to limit the distance in ray traces)
	void get_distance_in_voxels(float dist, Vec3i &ptVoxelDist) const;

	bool raytrace_start(Ray ray, Ray &voxel_ray, Vec3i &start_voxel);
	const Voxel *raytrace(const Ray &ray_orig, Ray &ray_out, Vec3i &ptVoxel);

	LVector<uint32_t> m_TriHits;

	bool _use_SIMD;
	bool _use_SDF;

	void find_nearest_voxel(const Vector3 &ptWorld, Vec3i &ptVoxel) const;
	const AABB &get_world_bound_expanded() const { return _scene_world_bound_expanded; }
	const AABB &get_world_bound_mid() const { return _scene_world_bound_mid; }
	const AABB &get_world_bound_contracted() const { return _scene_world_bound_contracted; }

	Vec3i estimate_voxel_dims(int voxel_density);

private:
	void calculate_world_bound();
	void calculate_voxel_dims(int voxel_density);
	void fill_voxels();
	void calculate_SDF();
	void debug_save_SDF();
	void calculate_SDF_voxel(const Vec3i &ptCentre);
	void calculate_SDF_assess_neighbour(const Vec3i &pt, unsigned int &min_SDF);
	bool voxel_within_bounds(Vec3i v) const {
		if (v.x < 0) return false;
		if (v.y < 0) return false;
		if (v.z < 0) return false;
		if (v.x >= _dims.x) return false;
		if (v.y >= _dims.y) return false;
		if (v.z >= _dims.z) return false;
		return true;
	}
	void debug_check_world_point_in_voxel(Vector3 pt, const Vec3i &ptVoxel);
	void debug_check_local_point_in_voxel(Vector3 pt, const Vec3i &ptVoxel) {
		//		assert ((int) (pt.x+0.5f) == ptVoxel.x);
		//		assert ((int) (pt.y+0.5f) == ptVoxel.y);
		//		assert ((int) (pt.z+0.5f) == ptVoxel.z);
	}

	LVector<Voxel> _voxels;
	LVector<AABB> _voxel_bounds;
	int _num_tris;

	// slightly expanded
	AABB _scene_world_bound_expanded;
	AABB _scene_world_bound_mid;

	// exact
	AABB _scene_world_bound_contracted;

	const LightGeometry *_geometry;

	Vec3i _dims;
	int _dims_X_times_Y;
	Vector3 _voxel_size;
	int _num_voxels;

	int get_voxel_num(const Vec3i &pos) const {
		int v = pos.z * _dims_X_times_Y;
		v += pos.y * _dims.x;
		v += pos.x;
		return v;
	}

	const Voxel &get_voxel(const Vec3i &pos) const {
		int v = get_voxel_num(pos);
		assert(v < _num_voxels);
		return _voxels[v];
	}
	Voxel &get_voxel(const Vec3i &pos) {
		int v = get_voxel_num(pos);
		assert(v < _num_voxels);
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
