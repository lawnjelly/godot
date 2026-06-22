/**************************************************************************/
/*  plane.h                                                               */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "core/math/vector3_64.h"

class _NO_DISCARD_CLASS_ Plane_64 {
public:
	union {
		struct {
			Vector3_64 normal;
			double d;
		};

		double coord[4];
	};

	void set_normal(const Vector3_64 &p_normal);
	_FORCE_INLINE_ Vector3_64 get_normal() const { return normal; }; ///Point is coplanar, CMP_EPSILON for precision

	void normalize();
	Plane_64 normalized() const;

	/* Plane_64-Point operations */

	_FORCE_INLINE_ Vector3_64 center() const { return normal * d; }
	Vector3_64 get_any_point() const;
	Vector3_64 get_any_perpendicular_normal() const;

	_FORCE_INLINE_ bool is_point_over(const Vector3_64 &p_point) const; ///< Point is over plane
	_FORCE_INLINE_ double distance_to(const Vector3_64 &p_point) const;
	_FORCE_INLINE_ bool has_point(const Vector3_64 &p_point, double _epsilon = CMP_EPSILON) const;

	/* intersections */

	bool intersect_3(const Plane_64 &p_plane1, const Plane_64 &p_plane2, Vector3_64 *r_result = nullptr) const;
	bool intersects_ray(const Vector3_64 &p_from, const Vector3_64 &p_dir, Vector3_64 *p_intersection) const;
	bool intersects_segment(const Vector3_64 &p_begin, const Vector3_64 &p_end, Vector3_64 *p_intersection) const;

	_FORCE_INLINE_ Vector3_64 project(const Vector3_64 &p_point) const {
		return p_point - normal * distance_to(p_point);
	}

	/* misc */

	Plane_64 operator-() const { return Plane_64(-normal, -d); }
	bool is_equal_approx(const Plane_64 &p_plane) const;

	_FORCE_INLINE_ bool operator==(const Plane_64 &p_plane) const;
	_FORCE_INLINE_ bool operator!=(const Plane_64 &p_plane) const;
	operator String() const;

	_FORCE_INLINE_ Plane_64() :
			d(0) {}
	_FORCE_INLINE_ Plane_64(double p_a, double p_b, double p_c, double p_d) :
			normal(p_a, p_b, p_c),
			d(p_d) {}

	_FORCE_INLINE_ Plane_64(const Vector3_64 &p_normal, double p_d);
	_FORCE_INLINE_ Plane_64(const Vector3_64 &p_point, const Vector3_64 &p_normal);
	_FORCE_INLINE_ Plane_64(const Vector3_64 &p_point1, const Vector3_64 &p_point2, const Vector3_64 &p_point3, ClockDirection p_dir = CLOCKWISE);
};

bool Plane_64::is_point_over(const Vector3_64 &p_point) const {
	return (normal.dot(p_point) > d);
}

double Plane_64::distance_to(const Vector3_64 &p_point) const {
	return (normal.dot(p_point) - d);
}

bool Plane_64::has_point(const Vector3_64 &p_point, double _epsilon) const {
	double dist = normal.dot(p_point) - d;
	dist = ABS(dist);
	return (dist <= _epsilon);
}

Plane_64::Plane_64(const Vector3_64 &p_normal, double p_d) :
		normal(p_normal),
		d(p_d) {
}

Plane_64::Plane_64(const Vector3_64 &p_point, const Vector3_64 &p_normal) :
		normal(p_normal),
		d(p_normal.dot(p_point)) {
}

Plane_64::Plane_64(const Vector3_64 &p_point1, const Vector3_64 &p_point2, const Vector3_64 &p_point3, ClockDirection p_dir) {
	if (p_dir == CLOCKWISE) {
		normal = (p_point1 - p_point3).cross(p_point1 - p_point2);
	} else {
		normal = (p_point1 - p_point2).cross(p_point1 - p_point3);
	}

	normal.normalize();
	d = normal.dot(p_point1);
}

bool Plane_64::operator==(const Plane_64 &p_plane) const {
	return normal == p_plane.normal && d == p_plane.d;
}

bool Plane_64::operator!=(const Plane_64 &p_plane) const {
	return normal != p_plane.normal || d != p_plane.d;
}
