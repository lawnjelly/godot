/*************************************************************************/
/*  light_trace.cpp                                                      */
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

#include "light_trace.h"
#include "core/math/aabb.h"

using namespace LM;

void LightTrace::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_triangle", "a", "b", "c"), &LightTrace::add_triangle);
	ClassDB::bind_method(D_METHOD("finalize_scene"), &LightTrace::finalize_scene);
	ClassDB::bind_method(D_METHOD("reset"), &LightTrace::reset);
	ClassDB::bind_method(D_METHOD("raytrace", "from", "to"), &LightTrace::raytrace);
	ClassDB::bind_method(D_METHOD("raytrace_test", "from", "to", "cull_backfaces"), &LightTrace::raytrace_test);
	ClassDB::bind_method(D_METHOD("get_hit_barycentric"), &LightTrace::get_hit_barycentric);
	ClassDB::bind_method(D_METHOD("get_hit_distance"), &LightTrace::get_hit_distance);
}

void LightTrace::add_triangle(const Vector3 &p_a, const Vector3 &p_b, const Vector3 &p_c) {

	// straight copy of the triangles
	LM::Tri *tri = _geometry._tris.request();
	tri->pos[0] = p_a;
	tri->pos[1] = p_b;
	tri->pos[2] = p_c;

	// edge form (which is needed by the tracer, this gets stored
	// in the voxels later)
	LM::Tri *tri_ef = _geometry._tris_edge_form.request();
	*tri_ef = *tri;

	tri_ef->convert_to_edgeform();

	// aabb for placing the triangle in the correct voxel
	AABB *bb = _geometry._aabbs.request();
	bb->position = p_a;
	bb->expand_to(p_b);
	bb->expand_to(p_c);
}

// build the voxels etc for tracing
void LightTrace::finalize_scene(int p_voxel_density) {
	if (p_voxel_density == 0) {
		// use some metric related to number of tris... NYI
		p_voxel_density = 20;
	}
	_tracer.create(_geometry, p_voxel_density);

	// the aabbs should no longer be used, so can delete these to save some memory
	// the triangles are still used for one step, maybe we can eliminate this...
	_geometry._aabbs.clear(true);
	_geometry._tris_edge_form.clear(true);
}

// clear all data, will be ready for another scene
void LightTrace::reset() {
	_geometry.clear();
	_tracer.reset();
}

// tracing

uint32_t LightTrace::raytrace(const Vector3 &p_from, const Vector3 &p_to) {
	return raytrace_find(p_from, p_to, _temp_hit_bary, _temp_hit_dist);
}

Vector3 LightTrace::get_hit_barycentric() {
	return _temp_hit_bary;
}

float LightTrace::get_hit_distance() {
	return _temp_hit_dist;
}

bool LightTrace::raytrace_test_ray(const Vector3 &p_from, const Vector3 &p_direction_normalized, float p_max_distance, bool p_cull_back_faces, float p_min_distance) {
	if (p_max_distance == 0.0f)
		return false;

	Ray r;
	r.o = p_from;
	r.d = p_direction_normalized;

	// apply minimum distance
	if (p_min_distance != 0.0f) {
		r.o += (r.d * p_min_distance);
		p_max_distance -= p_min_distance;
	}

	// limit distance
	Vec3i voxel_range;
	_tracer.get_distance_in_voxels(p_max_distance, voxel_range);

	return test_intersect_ray(r, p_max_distance, voxel_range, p_cull_back_faces);
}

bool LightTrace::raytrace_test(const Vector3 &p_from, const Vector3 &p_to, bool p_cull_back_faces) {
	Ray r;
	r.o = p_from;
	r.d = p_to - p_from;

	float max_dist = r.d.length();
	if (max_dist == 0.0f)
		return false;

	// normalize
	r.d = r.d / max_dist;

	// limit distance
	Vec3i voxel_range;
	_tracer.get_distance_in_voxels(max_dist, voxel_range);

	return test_intersect_ray(r, max_dist, voxel_range, p_cull_back_faces);
}

uint32_t LightTrace::raytrace_find_ray(const Vector3 &p_from, const Vector3 &p_direction_normalized, float p_max_distance, Vector3 &r_hit_barycentric, float &r_dist, float p_min_distance) {
	if (p_max_distance == 0.0f)
		return 0;

	Ray r;
	r.o = p_from;
	r.d = p_direction_normalized;

	// apply minimum distance
	if (p_min_distance != 0.0f) {
		r.o += (r.d * p_min_distance);
		p_max_distance -= p_min_distance;
	}

	// limit distance
	Vec3i voxel_range;
	_tracer.get_distance_in_voxels(p_max_distance, voxel_range);

	uint32_t hittri_p1 = find_intersect_ray(r, r_hit_barycentric, r_dist, &voxel_range) + 1;

	// is it further away than the max...
	if (hittri_p1) {
		if (r_dist > p_max_distance) {
			// same as no hit
			return 0;
		}
	}

	return hittri_p1;
}

// returns triangle ID +1 or 0, and the barycentric coords if a hit
uint32_t LightTrace::raytrace_find(const Vector3 &p_from, const Vector3 &p_to, Vector3 &r_hit_barycentric, float &r_dist) {
	Ray r;
	r.o = p_from;
	r.d = p_to - p_from;

	float max_dist = r.d.length();
	if (max_dist == 0.0f)
		return 0;

	// normalize
	r.d = r.d / max_dist;

	// limit distance
	Vec3i voxel_range;
	_tracer.get_distance_in_voxels(max_dist, voxel_range);

	uint32_t hittri_p1 = find_intersect_ray(r, r_hit_barycentric, r_dist, &voxel_range) + 1;

	// is it further away than the max...
	if (hittri_p1) {
		if (r_dist > max_dist) {
			// same as no hit
			return 0;
		}
	}

	return hittri_p1;
}

int LightTrace::find_intersect_ray(const Ray &p_ray, Vector3 &r_bary, float &r_nearest_t, const Vec3i *p_voxel_range) {
	r_nearest_t = FLT_MAX;
	int nearest_tri = -1;

	Ray voxel_ray;
	Vec3i pt_voxel;

	// prepare voxel trace
	if (!_tracer.raytrace_start(p_ray, voxel_ray, pt_voxel))
		return nearest_tri;

	bool bFirstHit = false;
	Vec3i pt_voxel_first_hit;

	// if we have specified a (optional) maximum range for the trace in voxels
	Vec3i pt_voxel_start = pt_voxel;

	// create the packed ray as a once off and reuse it for each voxel
	PackedRay packed_ray;
	packed_ray.create(p_ray);

	// keep track of when we need to expand the bounds of the trace
	int nearest_tri_so_far = -1;
	int square_length_from_start_to_terminate = INT_MAX;

	while (true) {
		const Voxel *pVoxel = _tracer.raytrace(voxel_ray, voxel_ray, pt_voxel);
		if (!pVoxel)
			break;

		process_voxel_hits(p_ray, packed_ray, *pVoxel, r_nearest_t, nearest_tri);

		// count number of tests for stats
		//int nHits = m_Tracer.m_TriHits.size();
		//num_tests += nHits;

		// if there is a nearest hit, calculate the voxel in which the hit occurs.
		// if we have travelled more than 1 voxel more than this, no need to traverse further.
		if (nearest_tri != nearest_tri_so_far) {
			nearest_tri_so_far = nearest_tri;
			Vector3 ptNearestHit = p_ray.o + (p_ray.d * r_nearest_t);
			_tracer.find_nearest_voxel(ptNearestHit, pt_voxel_first_hit);
			bFirstHit = true;

			// length in voxels to nearest hit
			Vec3i voxel_diff = pt_voxel_first_hit;
			voxel_diff -= pt_voxel_start;
			float voxel_length_to_nearest_hit = voxel_diff.length();
			// add a bit
			voxel_length_to_nearest_hit += 2.0f;

			// square length
			voxel_length_to_nearest_hit *= voxel_length_to_nearest_hit;

			// plus 1 for rounding up.
			square_length_from_start_to_terminate = voxel_length_to_nearest_hit + 1;
		}

		// first hit?
		if (!bFirstHit) {
			// check for voxel range
			if (p_voxel_range) {
				if (abs(pt_voxel.x - pt_voxel_start.x) > p_voxel_range->x)
					break;
				if (abs(pt_voxel.y - pt_voxel_start.y) > p_voxel_range->y)
					break;
				if (abs(pt_voxel.z - pt_voxel_start.z) > p_voxel_range->z)
					break;
			} // if voxel range
		} else {
			// check the range to this voxel. Have we gone further than the terminate voxel distance?
			Vec3i voxel_diff = pt_voxel;
			voxel_diff -= pt_voxel_start;
			int sl = voxel_diff.square_length();
			if (sl >= square_length_from_start_to_terminate)
				break;
		}

#ifdef LIGHTTRACER_IGNORE_VOXELS
		break;
#endif

	} // while

	if (nearest_tri != -1) {
		// non edge form triangle
		// annoying that we need to retain this data,
		// presumably it is stored on the client... we could alternatively have e.g.
		// a callback or something to get this data from the client
		const Tri &tri = _geometry._tris[nearest_tri];
		p_ray.find_intersect(tri, r_nearest_t, r_bary.x, r_bary.y, r_bary.z);
	}

	return nearest_tri;
}

void LightTrace::process_voxel_hits(const Ray &p_ray, const PackedRay &p_packed_ray, const Voxel &p_voxel, float &r_nearest_t, int &r_nearest_tri) {
	//#define LLIGHTMAPPED_DEBUG_COMPARE_SIMD

	// groups of 4
	int quads = p_voxel._packed_triangles.size();

	for (int q = 0; q < quads; q++) {
		// get pointers to 4 triangles
		const PackedTriangles &ptris = p_voxel._packed_triangles[q];

		//  we need to deal with the special case of the ignore_triangle being set. This will normally only occur on the first voxel.
		int winner = p_packed_ray.intersect(ptris, r_nearest_t);

#ifdef LLIGHTSCENE_TRACE_VERBOSE
		String sz = "\ttesting tris ";
		for (int t = 0; t < 4; t++) {
			int test_tri = (q * 4) + t;
			if (test_tri < voxel.m_TriIDs.size()) {
				sz += itos(voxel.m_TriIDs[(q * 4) + t]);
				sz += ", ";
			}
		}
		print_line(sz);
#endif

		if (winner != 0) {
			winner--;
			int winner_tri_index = (q * 4) + winner;
			int winner_tri = p_voxel._tri_IDs[winner_tri_index];

#ifdef LLIGHTSCENE_TRACE_VERBOSE
			const AABB &tri_bb = m_TriPos_aabbs[winner_tri];
			print_line("\t\tWINNER tri : " + itos(winner_tri) + " at dist " + String(Variant(r_nearest_t)) + " aabb " + String(tri_bb));
#endif

			r_nearest_tri = winner_tri;
		}
	}

	// print result
	//		if (r_nearest_tri != -1)
	//			print_line("SIMD\tr_nearest_tri " + itos (r_nearest_tri) + " dist " + String(Variant(r_nearest_t)));
}

bool LightTrace::test_intersect_ray(const LM::Ray &p_ray, float p_max_dist, const LM::Vec3i &p_voxel_range, bool p_cull_back_faces) {
	Ray voxel_ray;
	Vec3i pt_voxel;

	// prepare voxel trace
	if (!_tracer.raytrace_start(p_ray, voxel_ray, pt_voxel))
		return false;

	// if we have specified a (optional) maximum range for the trace in voxels
	Vec3i pt_voxel_start = pt_voxel;

	// create the packed ray as a once off and reuse it for each voxel
	PackedRay packed_ray;
	packed_ray.create(p_ray);

	while (true) {

		const Voxel *voxel = _tracer.raytrace(voxel_ray, voxel_ray, pt_voxel);
		if (!voxel)
			break;

		if (test_voxel_hits(p_ray, packed_ray, *voxel, p_max_dist, p_cull_back_faces))
			return true;

		// check for voxel range
		if (abs(pt_voxel.x - pt_voxel_start.x) > p_voxel_range.x)
			break;
		if (abs(pt_voxel.y - pt_voxel_start.y) > p_voxel_range.y)
			break;
		if (abs(pt_voxel.z - pt_voxel_start.z) > p_voxel_range.z)
			break;

	} // while

	return false;
}

bool LightTrace::test_voxel_hits(const Ray &p_ray, const PackedRay &p_packed_ray, const Voxel &p_voxel, float p_max_dist, bool p_cull_back_faces) {
	int quads = p_voxel._packed_triangles.size();

	if (!p_cull_back_faces) {
		for (int q = 0; q < quads; q++) {
			// get pointers to 4 triangles
			const PackedTriangles &ptris = p_voxel._packed_triangles[q];
			if (p_packed_ray.intersect_test(ptris, p_max_dist))
				return true;
		}
	} else {
		for (int q = 0; q < quads; q++) {
			// get pointers to 4 triangles
			const PackedTriangles &ptris = p_voxel._packed_triangles[q];
			if (p_packed_ray.intersect_test_cullbackfaces(ptris, p_max_dist))
				return true;
		}
	}

	return false;
}
