/**************************************************************************/
/*  plane.cpp                                                             */
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

#include "plane_64.h"

#include "core/math/math_funcs.h"

void Plane_64::set_normal(const Vector3_64 &p_normal) {
	normal = p_normal;
}

void Plane_64::normalize() {
	double l = normal.length();
	if (l == 0) {
		*this = Plane_64(0, 0, 0, 0);
		return;
	}
	normal /= l;
	d /= l;
}

Plane_64 Plane_64::normalized() const {
	Plane_64 p = *this;
	p.normalize();
	return p;
}

Vector3_64 Plane_64::get_any_point() const {
	return get_normal() * d;
}

Vector3_64 Plane_64::get_any_perpendicular_normal() const {
	static const Vector3_64 p1 = Vector3_64(1, 0, 0);
	static const Vector3_64 p2 = Vector3_64(0, 1, 0);
	Vector3_64 p;

	if (ABS(normal.dot(p1)) > 0.99f) { // if too similar to p1
		p = p2; // use p2
	} else {
		p = p1; // use p1
	}

	p -= normal * normal.dot(p);
	p.normalize();

	return p;
}

/* intersections */

bool Plane_64::intersect_3(const Plane_64 &p_plane1, const Plane_64 &p_plane2, Vector3_64 *r_result) const {
	const Plane_64 &p_plane0 = *this;
	Vector3_64 normal0 = p_plane0.normal;
	Vector3_64 normal1 = p_plane1.normal;
	Vector3_64 normal2 = p_plane2.normal;

	double denom = normal0.cross(normal1).dot(normal2);

	if (Math::is_zero_approx(denom)) {
		return false;
	}

	if (r_result) {
		*r_result = ((normal1.cross(normal2) * p_plane0.d) +
							(normal2.cross(normal0) * p_plane1.d) +
							(normal0.cross(normal1) * p_plane2.d)) /
				denom;
	}

	return true;
}

bool Plane_64::intersects_ray(const Vector3_64 &p_from, const Vector3_64 &p_dir, Vector3_64 *p_intersection) const {
	Vector3_64 segment = p_dir;
	double den = normal.dot(segment);

	//printf("den is %i\n",den);
	if (Math::is_zero_approx(den)) {
		return false;
	}

	double dist = (normal.dot(p_from) - d) / den;
	//printf("dist is %i\n",dist);

	if (dist > (double)CMP_EPSILON) { //this is a ray, before the emitting pos (p_from) doesn't exist

		return false;
	}

	dist = -dist;
	*p_intersection = p_from + segment * dist;

	return true;
}

bool Plane_64::intersects_segment(const Vector3_64 &p_begin, const Vector3_64 &p_end, Vector3_64 *p_intersection) const {
	Vector3_64 segment = p_begin - p_end;
	double den = normal.dot(segment);

	//printf("den is %i\n",den);
	if (Math::is_zero_approx(den)) {
		return false;
	}

	double dist = (normal.dot(p_begin) - d) / den;
	//printf("dist is %i\n",dist);

	if (dist < (double)-CMP_EPSILON || dist > (1 + (double)CMP_EPSILON)) {
		return false;
	}

	dist = -dist;
	*p_intersection = p_begin + segment * dist;

	return true;
}

/* misc */

bool Plane_64::is_equal_approx(const Plane_64 &p_plane) const {
	return normal.is_equal_approx(p_plane.normal) && Math::is_equal_approx(d, p_plane.d);
}

Plane_64::operator String() const {
	return normal.operator String() + ", " + rtos(d);
}
