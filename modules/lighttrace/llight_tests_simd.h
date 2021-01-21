/*************************************************************************/
/*  llight_tests_simd.h                                                  */
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

#if defined(__x86_64__) || defined(_M_X64)
#define LLIGHTMAPPER_USE_SIMD
#endif

#ifdef LLIGHTMAPPER_USE_SIMD
#include <emmintrin.h>
#endif

#include "llight_types.h"

namespace LM {

union u_m128 {
#ifdef LLIGHTMAPPER_USE_SIMD
	__m128 mm128;
#endif
	struct
	{
		float v[4];
	};
	struct
	{
		uint32_t ui[4];
	};

	static u_m128 set1_ps(float f) {
		u_m128 r;
		r.v[0] = f;
		r.v[1] = f;
		r.v[2] = f;
		r.v[3] = f;
		return r;
	}

	static u_m128 mul_ps(const u_m128 &a, const u_m128 &b) {
		u_m128 r;
		for (int n = 0; n < 4; n++) {
			r.v[n] = a.v[n] * b.v[n];
		}
		return r;
	}
	static u_m128 div_ps(const u_m128 &a, const u_m128 &b) {
		u_m128 r;
		for (int n = 0; n < 4; n++) {
			r.v[n] = a.v[n] / b.v[n];
		}
		return r;
	}
	static u_m128 add_ps(const u_m128 &a, const u_m128 &b) {
		u_m128 r;
		for (int n = 0; n < 4; n++) {
			r.v[n] = a.v[n] + b.v[n];
		}
		return r;
	}
	static u_m128 sub_ps(const u_m128 &a, const u_m128 &b) {
		u_m128 r;
		for (int n = 0; n < 4; n++) {
			r.v[n] = a.v[n] - b.v[n];
		}
		return r;
	}

	static u_m128 ref_cmple_ps(const u_m128 &a, const u_m128 &b) {
		u_m128 res;
		for (int n = 0; n < 4; n++) {
			if (a.v[n] <= b.v[n])
				res.ui[n] = 0xFFFFFFFF;
			else
				res.ui[n] = 0;
		}
		return res;
	}
	static u_m128 ref_cmpge_ps(const u_m128 &a, const u_m128 &b) {
		u_m128 res;
		for (int n = 0; n < 4; n++) {
			if (a.v[n] >= b.v[n])
				res.ui[n] = 0xFFFFFFFF;
			else
				res.ui[n] = 0;
		}
		return res;
	}

	int movemask_ps() const { // possibly needs this order reversing
		int m = 0;
		if (ui[0]) m |= 1;
		if (ui[1]) m |= 2;
		if (ui[2]) m |= 4;
		if (ui[3]) m |= 8;
		return m;
	}

	static u_m128 mm_or(const u_m128 &a, const u_m128 &b) {
		u_m128 r;
		for (int n = 0; n < 4; n++) {
			r.ui[n] = a.ui[n] | b.ui[n];
		}
		return r;
	}
	static u_m128 mm_and(const u_m128 &a, const u_m128 &b) {
		u_m128 r;
		for (int n = 0; n < 4; n++) {
			r.ui[n] = a.ui[n] & b.ui[n];
		}
		return r;
	}

	static void multi_sub(u_m128 result[3], const u_m128 a[3], const u_m128 b[3]) {
		for (int n = 0; n < 3; n++)
			result[n] = sub_ps(a[n], b[n]);
	}

	static void multi_cross(u_m128 result[3], const u_m128 a[3], const u_m128 b[3]) {
		u_m128 tmp;
		u_m128 tmp2;

		tmp = mul_ps(a[1], b[2]);
		tmp2 = mul_ps(b[1], a[2]);
		result[0] = sub_ps(tmp, tmp2);

		tmp = mul_ps(a[2], b[0]);
		tmp2 = mul_ps(b[2], a[0]);
		result[1] = sub_ps(tmp, tmp2);

		tmp = mul_ps(a[0], b[1]);
		tmp2 = mul_ps(b[0], a[1]);
		result[2] = sub_ps(tmp, tmp2);
	}

	static u_m128 multi_dot(const u_m128 a[3], const u_m128 b[3]) {
		u_m128 r;
		u_m128 tmp, tmp2, tmp3, tmp4;

		tmp = mul_ps(a[0], b[0]);
		tmp2 = mul_ps(a[1], b[1]);
		tmp3 = mul_ps(a[2], b[2]);

		tmp4 = add_ps(tmp, tmp2);
		r = add_ps(tmp4, tmp3); // this may be dodgy, as argument is same as result
		return r;
	}
};

struct PackedTriangles {
	// four triangles in edge form
	// i.e. edge1, edge2 and a vertex
	u_m128 e1[3];
	u_m128 e2[3];
	u_m128 v0[3];

	// not all the four triangles may be filled,
	// so we have an inactive mask that indicates
	// empty triangles. These won't produce a trace hit
	u_m128 inactiveMask;

	void create() { memset(this, 0, sizeof(PackedTriangles)); }
	void set(int which_tri, const Tri &tri) {
		set_e1(which_tri, tri);
		set_e2(which_tri, tri);
		set_v0(which_tri, tri);
	}
	void extract_triangle(int w, Tri &tri) const // debug
	{
		for (int m = 0; m < 3; m++) {
			tri.pos[0].coord[m] = e1[m].v[w];
			tri.pos[1].coord[m] = e2[m].v[w];
			tri.pos[2].coord[m] = v0[m].v[w];
		}
	}
	void finalize(int num_tris) {
		for (int n = num_tris; n < 4; n++) {
			inactiveMask.v[n] = 1.0f;
		}
	}
	void make_inactive(int which) {
		inactiveMask.v[which] = 1.0f;
	}

	void set_e1(int which_tri, const Tri &tri) {
		for (int m = 0; m < 3; m++) {
			e1[m].v[which_tri] = tri.pos[0].coord[m];
		}
	}
	void set_e2(int which_tri, const Tri &tri) {
		for (int m = 0; m < 3; m++) {
			e2[m].v[which_tri] = tri.pos[1].coord[m];
		}
	}
	void set_v0(int which_tri, const Tri &tri) {
		for (int m = 0; m < 3; m++) {
			v0[m].v[which_tri] = tri.pos[2].coord[m];
		}
	}
};

struct PackedRay {
	u_m128 _origin[3];
	u_m128 _direction[3];
	u_m128 _length;

	void create(const Ray &ray);
	int intersect(const PackedTriangles &packedTris, float &nearest_dist) const;
	bool intersect_test(const PackedTriangles &packedTris, float max_dist) const;
	bool intersect_test_cullbackfaces(const PackedTriangles &packedTris, float max_dist) const;
};

class LightTests_SIMD {
public:
	bool test_intersect4(const Tri *tris[4], const Ray &ray, float &r_nearest_t, int &r_winner) const;
	bool test_intersect4_packed(const PackedTriangles &ptris, const Ray &ray, float &r_nearest_t, int &r_winner) const;
};

} // namespace LM
