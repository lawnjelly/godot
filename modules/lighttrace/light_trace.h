/*************************************************************************/
/*  light_trace.h                                                        */
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

#include "core/math/vector3.h"
#include "llight_types.h"
#include "lvector.h"
#include "lvoxel_tracer.h"
#include "scene/3d/spatial.h"

class LightTrace : public Spatial {
	GDCLASS(LightTrace, Spatial);

public:
	// adding scene
	// these are non-indexed triangles, they are stored in this form for cache reasons
	void add_triangle(const Vector3 &p_a, const Vector3 &p_b, const Vector3 &p_c);

	// build the voxels etc for tracing
	void finalize_scene(int p_voxel_density = 0);

	// clear all data, will be ready for another scene
	void reset();

	// returns triangle ID plus 1 or 0 for no hit, and the barycentric coords if a hit, and distance to hit
	// use this from c++
	uint32_t raytrace_find(const Vector3 &p_from, const Vector3 &p_to, Vector3 &r_hit_barycentric, float &r_dist);
	uint32_t raytrace_find_ray(const Vector3 &p_from, const Vector3 &p_direction_normalized, float p_max_distance, Vector3 &r_hit_barycentric, float &r_dist, float p_min_distance = 0.0f);

	// returns false if no hit
	bool raytrace_test(const Vector3 &p_from, const Vector3 &p_to, bool p_cull_back_faces);
	bool raytrace_test_ray(const Vector3 &p_from, const Vector3 &p_direction_normalized, float p_max_distance, bool p_cull_back_faces, float p_min_distance = 0.0f);

	// GDSCRIPT interface (not multithread safe)
	uint32_t raytrace(const Vector3 &p_from, const Vector3 &p_to);
	Vector3 get_hit_barycentric();
	float get_hit_distance();

protected:
	static void _bind_methods();

private:
	int find_intersect_ray(const LM::Ray &p_ray, Vector3 &r_bary, float &r_nearest_t, const LM::Vec3i *p_voxel_range);
	bool test_intersect_ray(const LM::Ray &p_ray, float p_max_dist, const LM::Vec3i &p_voxel_range, bool p_cull_back_faces);

	void process_voxel_hits(const LM::Ray &p_ray, const LM::PackedRay &p_packed_ray, const LM::Voxel &p_voxel, float &r_nearest_t, int &r_nearest_tri);
	bool test_voxel_hits(const LM::Ray &p_ray, const LM::PackedRay &p_packed_ray, const LM::Voxel &p_voxel, float p_max_dist, bool p_cull_back_faces);

	LM::LightGeometry _geometry;
	LM::VoxelTracer _tracer;

	Vector3 _temp_hit_bary;
	float _temp_hit_dist = 0.0f;
};
