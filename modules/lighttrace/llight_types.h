/*************************************************************************/
/*  llight_types.h                                                       */
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
#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include "lvector.h"

namespace LM {

class Vec3i {
public:
	Vec3i() {}
	Vec3i(int xx, int yy, int zz) {
		x = xx;
		y = yy;
		z = zz;
	}
	int32_t x, y, z;

	Vec3i &operator-=(const Vec3i &v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}
	int square_length() const { return (x * x) + (y * y) + (z * z); }
	float length() const { return sqrtf(square_length()); }

	void set(int xx, int yy, int zz) {
		x = xx;
		y = yy;
		z = zz;
	}
	void set_round(const Vector3 &p) {
		x = (int)(p.x + 0.5f);
		y = (int)(p.y + 0.5f);
		z = (int)(p.z + 0.5f);
	}
	void to(Vector3 &p) const {
		p.x = x;
		p.y = y;
		p.z = z;
	}
	String to_string() const { return itos(x) + ", " + itos(y) + ", " + itos(z); }
};

class Tri {
public:
	Vector3 pos[3];

	Vector3 get_centre() const { return (pos[0] + pos[1] + pos[2]) / 3.0f; }
	void flip_winding() {
		Vector3 temp = pos[0];
		pos[0] = pos[2];
		pos[2] = temp;
	}
	void find_barycentric(const Vector3 &pt, float &u, float &v, float &w) const;
	void interpolate_barycentric(Vector3 &pt, const Vector3 &bary) const { interpolate_barycentric(pt, bary.x, bary.y, bary.z); }
	void interpolate_barycentric(Vector3 &pt, float u, float v, float w) const;
	void find_normal_edgeform(Vector3 &norm) const {
		norm = -pos[0].cross(pos[1]);
		norm.normalize();
	}
	void find_normal(Vector3 &norm) const {
		Vector3 e0 = pos[1] - pos[0];
		Vector3 e1 = pos[2] - pos[1];
		norm = -e0.cross(e1);
		norm.normalize();
	}
	void convert_to_edgeform() {
		Tri t = *this;
		// b - a
		pos[0] = t.pos[1] - t.pos[0];
		// c - a
		pos[1] = t.pos[2] - t.pos[0];
		// a
		pos[2] = t.pos[0];
	}

	void convert_from_edgeform() {
		Tri t = *this;
		pos[0] = t.pos[2];
		pos[1] = t.pos[2] + t.pos[0];
		pos[2] = t.pos[2] + t.pos[1];
	}

	float calculate_area() const {
		return sqrtf(calculate_twice_area_squared()) * 0.5f;
	}

	float calculate_twice_area_squared() const {
		// compute the area squared. Which is the result of the cross product of two edges. If it's near zero, bingo
		Vector3 edge1 = pos[1] - pos[0];
		Vector3 edge2 = pos[2] - pos[0];

		Vector3 vec = edge1.cross(edge2);
		return vec.length_squared();
	}
};

class LightGeometry {
public:
	void clear() {
		_tris.clear(true);
		_tris_edge_form.clear(true);
		_aabbs.clear(true);
	}

	LVector<LM::Tri> _tris;
	LVector<LM::Tri> _tris_edge_form;
	LVector<AABB> _aabbs; // of the triangles
};

class Ray {
public:
	Vector3 o; // origin
	Vector3 d; // direction

	bool test_intersect(const Tri &tri, float &t) const;
	bool test_intersect_edgeform(const Tri &tri_edge, float &t) const;

	void find_intersect(const Tri &tri, float t, float &u, float &v, float &w) const;
	bool intersect_AA_plane(int axis, Vector3 &pt, float epsilon = 0.0001f) const;
};

bool barycentric_inside(float u, float v, float w) {
	if ((u < 0.0f) || (u > 1.0f) ||
			(v < 0.0f) || (v > 1.0f) ||
			(w < 0.0f) || (w > 1.0f))
		return false;

	return true;
}

bool barycentric_inside(const Vector3 &bary) {
	return barycentric_inside(bary.x, bary.y, bary.z);
}

float triangle_calculate_twice_area_squared(const Vector3 &a, const Vector3 &b, const Vector3 &c) {
	// compute the area squared. Which is the result of the cross product of two edges. If it's near zero, bingo
	Vector3 edge1 = b - a;
	Vector3 edge2 = c - a;

	Vector3 vec = edge1.cross(edge2);
	return vec.length_squared();
}

bool triangle_is_degenerate(const Vector3 &a, const Vector3 &b, const Vector3 &c, float epsilon) {
	// not interested in the actual area, but numerical stability
	Vector3 edge1 = b - a;
	Vector3 edge2 = c - a;

	edge1 *= 1024.0f;
	edge2 *= 1024.0f;

	Vector3 vec = edge1.cross(edge2);
	float sl = vec.length_squared();

	if (sl <= epsilon)
		return true;

	return false;
}

inline void Tri::find_barycentric(const Vector3 &pt, float &u, float &v, float &w) const {
	const Vector3 &a = pos[0];
	const Vector3 &b = pos[1];
	const Vector3 &c = pos[2];

	// we can alternatively precache
	Vector3 v0 = b - a, v1 = c - a, v2 = pt - a;
	float d00 = v0.dot(v0);
	float d01 = v0.dot(v1);
	float d11 = v1.dot(v1);
	float d20 = v2.dot(v0);
	float d21 = v2.dot(v1);
	float invDenom = 1.0f / (d00 * d11 - d01 * d01);
	v = (d11 * d20 - d01 * d21) * invDenom;
	w = (d00 * d21 - d01 * d20) * invDenom;
	u = 1.0f - v - w;
}

inline void Tri::interpolate_barycentric(Vector3 &pt, float u, float v, float w) const {
	const Vector3 &a = pos[0];
	const Vector3 &b = pos[1];
	const Vector3 &c = pos[2];
	pt.x = (a.x * u) + (b.x * v) + (c.x * w);
	pt.y = (a.y * u) + (b.y * v) + (c.y * w);
	pt.z = (a.z * u) + (b.z * v) + (c.z * w);
}

// returns barycentric, assumes we have already tested and got a collision
inline void Ray::find_intersect(const Tri &tri, float t, float &u, float &v, float &w) const {
	Vector3 pt = o + (d * t);
	tri.find_barycentric(pt, u, v, w);
}

// returns false if doesn't cut x.
// the relevant axis should be filled on entry (0 is x, 1 is y, 2 is z) with the axis aligned plane constant.
bool Ray::intersect_AA_plane(int axis, Vector3 &pt, float epsilon) const {
	// x(t) = o.x + (d.x * t)
	// y(t) = o.y + (d.y * t)
	// z(t) = o.z + (d.z * t)

	// solve for x
	// find t
	// d.x * t = x(t) - o.x
	// t = (x(t) - o.x) / d.x

	// just aliases
	//	const Vector3 &o = ray.o;
	//	const Vector3 &d = ray.d;

	switch (axis) {
		case 0: {
			if (fabsf(d.x) < epsilon)
				return false;

			float t = pt.x - o.x;
			t /= d.x;

			pt.y = o.y + (d.y * t);
			pt.z = o.z + (d.z * t);
		} break;
		case 1: {
			if (fabsf(d.y) < epsilon)
				return false;

			float t = pt.y - o.y;
			t /= d.y;

			pt.x = o.x + (d.x * t);
			pt.z = o.z + (d.z * t);
		} break;
		case 2: {
			if (fabsf(d.z) < epsilon)
				return false;

			float t = pt.z - o.z;
			t /= d.z;

			// now we have t, we can calculate y and z
			pt.x = o.x + (d.x * t);
			pt.y = o.y + (d.y * t);
		} break;
		default:
			break;
	}

	return true;
}

// same as below but with edges pre calced
inline bool Ray::test_intersect_edgeform(const Tri &tri_edge, float &t) const {
	//Edge1, Edge2
	const Vector3 &e1 = tri_edge.pos[0];
	const Vector3 &e2 = tri_edge.pos[1];
	const Vector3 &a = tri_edge.pos[2];

	const float cfEpsilon = 0.000001f;
	Vector3 P, Q, T;
	float det, inv_det, u, v;

	//Begin calculating determinant - also used to calculate u parameter
	P = d.cross(e2);

	//if determinant is near zero, ray lies in plane of triangle
	det = e1.dot(P);

	//NOT CULLING
	if (det > -cfEpsilon && det < cfEpsilon) return false; //IR_NONE;
	inv_det = 1.f / det;

	//calculate distance from V1 to ray origin
	T = o - a;

	//Calculate u parameter and test bound
	u = T.dot(P) * inv_det;

	//The intersection lies outside of the triangle
	if (u < 0.f || u > 1.f) return false; // IR_NONE;

	//Prepare to test v parameter
	Q = T.cross(e1);

	//Calculate V parameter and test bound
	v = d.dot(Q) * inv_det;

	//The intersection lies outside of the triangle
	if (v < 0.f || u + v > 1.f) return false; //IR_NONE;

	t = e2.dot(Q) * inv_det;

	if (t > cfEpsilon) { //ray intersection
		//	*out = t;
		return true; //IR_HIT;
	}

	// No hit, no win
	return false; //IR_BEHIND;
}

// moller and trumbore test (actually calculated uv and t but we are not returning uv)
inline bool Ray::test_intersect(const Tri &tri, float &t) const {
	const Vector3 &a = tri.pos[0];
	const Vector3 &b = tri.pos[1];
	const Vector3 &c = tri.pos[2];

	const float cfEpsilon = 0.000001f;
	//#define EPSILON 0.000001
	Vector3 e1, e2; //Edge1, Edge2
	Vector3 P, Q, T;
	float det, inv_det, u, v;
	//	float t;

	//Find vectors for two edges sharing V1
	e1 = b - a;
	e2 = c - a;

	//Begin calculating determinant - also used to calculate u parameter
	P = d.cross(e2);

	//if determinant is near zero, ray lies in plane of triangle
	det = e1.dot(P);

	//NOT CULLING
	if (det > -cfEpsilon && det < cfEpsilon) return false; //IR_NONE;
	inv_det = 1.f / det;

	//calculate distance from V1 to ray origin
	T = o - a;

	//Calculate u parameter and test bound
	u = T.dot(P) * inv_det;

	//The intersection lies outside of the triangle
	if (u < 0.f || u > 1.f) return false; // IR_NONE;

	//Prepare to test v parameter
	Q = T.cross(e1);

	//Calculate V parameter and test bound
	v = d.dot(Q) * inv_det;

	//The intersection lies outside of the triangle
	if (v < 0.f || u + v > 1.f) return false; //IR_NONE;

	t = e2.dot(Q) * inv_det;

	if (t > cfEpsilon) { //ray intersection
		//	*out = t;
		return true; //IR_HIT;
	}

	// No hit, no win
	return false; //IR_BEHIND;
}

} // namespace LM
