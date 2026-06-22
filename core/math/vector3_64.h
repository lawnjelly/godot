/**************************************************************************/
/*  vector3.h                                                             */
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

#include "core/math/math_funcs.h"
#include "core/ustring.h"

struct _NO_DISCARD_CLASS_ Vector3_64 {
	static const int AXIS_COUNT = 3;

	enum Axis {
		AXIS_X,
		AXIS_Y,
		AXIS_Z,
	};

	union {
		struct {
			double x;
			double y;
			double z;
		};

		double coord[3];
	};

	_FORCE_INLINE_ const double &operator[](int p_axis) const {
		DEV_ASSERT((unsigned int)p_axis < 3);
		return coord[p_axis];
	}

	_FORCE_INLINE_ double &operator[](int p_axis) {
		DEV_ASSERT((unsigned int)p_axis < 3);
		return coord[p_axis];
	}

	void set_axis(int p_axis, double p_value);
	double get_axis(int p_axis) const;

	_FORCE_INLINE_ void set_all(double p_value) {
		x = y = z = p_value;
	}

	_FORCE_INLINE_ int min_axis() const {
		return x < y ? (x < z ? 0 : 2) : (y < z ? 1 : 2);
	}

	_FORCE_INLINE_ int max_axis() const {
		return x < y ? (y < z ? 2 : 1) : (x < z ? 2 : 0);
	}

	_FORCE_INLINE_ double length() const;
	_FORCE_INLINE_ double length_squared() const;

	_FORCE_INLINE_ void normalize();
	_FORCE_INLINE_ Vector3_64 normalized() const;
	_FORCE_INLINE_ bool is_normalized() const;
	_FORCE_INLINE_ Vector3_64 inverse() const;

	_FORCE_INLINE_ void zero();

	/* Static Methods between 2 vector3s */

	_FORCE_INLINE_ Vector3_64 linear_interpolate(const Vector3_64 &p_to, double p_weight) const;

	_FORCE_INLINE_ Vector3_64 cross(const Vector3_64 &p_b) const;
	_FORCE_INLINE_ double dot(const Vector3_64 &p_b) const;

	_FORCE_INLINE_ Vector3_64 abs() const;
	_FORCE_INLINE_ Vector3_64 floor() const;
	_FORCE_INLINE_ Vector3_64 sign() const;
	_FORCE_INLINE_ Vector3_64 ceil() const;
	_FORCE_INLINE_ Vector3_64 round() const;

	_FORCE_INLINE_ double distance_to(const Vector3_64 &p_to) const;
	_FORCE_INLINE_ double distance_squared_to(const Vector3_64 &p_to) const;

	bool is_equal_approx(const Vector3_64 &p_v) const;
	inline bool is_equal_approx(const Vector3_64 &p_v, double p_tolerance) const;
	bool is_zero_approx() const;

	/* Operators */

	_FORCE_INLINE_ Vector3_64 &operator+=(const Vector3_64 &p_v);
	_FORCE_INLINE_ Vector3_64 operator+(const Vector3_64 &p_v) const;
	_FORCE_INLINE_ Vector3_64 &operator-=(const Vector3_64 &p_v);
	_FORCE_INLINE_ Vector3_64 operator-(const Vector3_64 &p_v) const;
	_FORCE_INLINE_ Vector3_64 &operator*=(const Vector3_64 &p_v);
	_FORCE_INLINE_ Vector3_64 operator*(const Vector3_64 &p_v) const;
	_FORCE_INLINE_ Vector3_64 &operator/=(const Vector3_64 &p_v);
	_FORCE_INLINE_ Vector3_64 operator/(const Vector3_64 &p_v) const;

	_FORCE_INLINE_ Vector3_64 &operator*=(double p_scalar);
	_FORCE_INLINE_ Vector3_64 operator*(double p_scalar) const;
	_FORCE_INLINE_ Vector3_64 &operator/=(double p_scalar);
	_FORCE_INLINE_ Vector3_64 operator/(double p_scalar) const;

	_FORCE_INLINE_ Vector3_64 operator-() const;

	_FORCE_INLINE_ bool operator==(const Vector3_64 &p_v) const;
	_FORCE_INLINE_ bool operator!=(const Vector3_64 &p_v) const;
	_FORCE_INLINE_ bool operator<(const Vector3_64 &p_v) const;
	_FORCE_INLINE_ bool operator<=(const Vector3_64 &p_v) const;
	_FORCE_INLINE_ bool operator>(const Vector3_64 &p_v) const;
	_FORCE_INLINE_ bool operator>=(const Vector3_64 &p_v) const;

	operator String() const;

	_FORCE_INLINE_ Vector3_64(double p_x, double p_y, double p_z) {
		x = p_x;
		y = p_y;
		z = p_z;
	}
	_FORCE_INLINE_ Vector3_64() { x = y = z = 0; }
};

Vector3_64 Vector3_64::cross(const Vector3_64 &p_b) const {
	Vector3_64 ret(
			(y * p_b.z) - (z * p_b.y),
			(z * p_b.x) - (x * p_b.z),
			(x * p_b.y) - (y * p_b.x));

	return ret;
}

double Vector3_64::dot(const Vector3_64 &p_b) const {
	return x * p_b.x + y * p_b.y + z * p_b.z;
}

Vector3_64 Vector3_64::abs() const {
	return Vector3_64(Math::abs(x), Math::abs(y), Math::abs(z));
}

Vector3_64 Vector3_64::sign() const {
	return Vector3_64(SGN(x), SGN(y), SGN(z));
}

Vector3_64 Vector3_64::floor() const {
	return Vector3_64(Math::floor(x), Math::floor(y), Math::floor(z));
}

Vector3_64 Vector3_64::ceil() const {
	return Vector3_64(Math::ceil(x), Math::ceil(y), Math::ceil(z));
}

Vector3_64 Vector3_64::round() const {
	return Vector3_64(Math::round(x), Math::round(y), Math::round(z));
}

Vector3_64 Vector3_64::linear_interpolate(const Vector3_64 &p_to, double p_weight) const {
	return Vector3_64(
			x + (p_weight * (p_to.x - x)),
			y + (p_weight * (p_to.y - y)),
			z + (p_weight * (p_to.z - z)));
}

double Vector3_64::distance_to(const Vector3_64 &p_to) const {
	return (p_to - *this).length();
}

double Vector3_64::distance_squared_to(const Vector3_64 &p_to) const {
	return (p_to - *this).length_squared();
}

/* Operators */

Vector3_64 &Vector3_64::operator+=(const Vector3_64 &p_v) {
	x += p_v.x;
	y += p_v.y;
	z += p_v.z;
	return *this;
}

Vector3_64 Vector3_64::operator+(const Vector3_64 &p_v) const {
	return Vector3_64(x + p_v.x, y + p_v.y, z + p_v.z);
}

Vector3_64 &Vector3_64::operator-=(const Vector3_64 &p_v) {
	x -= p_v.x;
	y -= p_v.y;
	z -= p_v.z;
	return *this;
}
Vector3_64 Vector3_64::operator-(const Vector3_64 &p_v) const {
	return Vector3_64(x - p_v.x, y - p_v.y, z - p_v.z);
}

Vector3_64 &Vector3_64::operator*=(const Vector3_64 &p_v) {
	x *= p_v.x;
	y *= p_v.y;
	z *= p_v.z;
	return *this;
}
Vector3_64 Vector3_64::operator*(const Vector3_64 &p_v) const {
	return Vector3_64(x * p_v.x, y * p_v.y, z * p_v.z);
}

Vector3_64 &Vector3_64::operator/=(const Vector3_64 &p_v) {
	x /= p_v.x;
	y /= p_v.y;
	z /= p_v.z;
	return *this;
}

Vector3_64 Vector3_64::operator/(const Vector3_64 &p_v) const {
	return Vector3_64(x / p_v.x, y / p_v.y, z / p_v.z);
}

Vector3_64 &Vector3_64::operator*=(double p_scalar) {
	x *= p_scalar;
	y *= p_scalar;
	z *= p_scalar;
	return *this;
}

_FORCE_INLINE_ Vector3_64 operator*(double p_scalar, const Vector3_64 &p_vec) {
	return p_vec * p_scalar;
}

Vector3_64 Vector3_64::operator*(double p_scalar) const {
	return Vector3_64(x * p_scalar, y * p_scalar, z * p_scalar);
}

Vector3_64 &Vector3_64::operator/=(double p_scalar) {
	x /= p_scalar;
	y /= p_scalar;
	z /= p_scalar;
	return *this;
}

Vector3_64 Vector3_64::operator/(double p_scalar) const {
	return Vector3_64(x / p_scalar, y / p_scalar, z / p_scalar);
}

Vector3_64 Vector3_64::operator-() const {
	return Vector3_64(-x, -y, -z);
}

bool Vector3_64::operator==(const Vector3_64 &p_v) const {
	return x == p_v.x && y == p_v.y && z == p_v.z;
}

bool Vector3_64::operator!=(const Vector3_64 &p_v) const {
	return x != p_v.x || y != p_v.y || z != p_v.z;
}

bool Vector3_64::operator<(const Vector3_64 &p_v) const {
	if (x == p_v.x) {
		if (y == p_v.y) {
			return z < p_v.z;
		} else {
			return y < p_v.y;
		}
	} else {
		return x < p_v.x;
	}
}

bool Vector3_64::operator>(const Vector3_64 &p_v) const {
	if (x == p_v.x) {
		if (y == p_v.y) {
			return z > p_v.z;
		} else {
			return y > p_v.y;
		}
	} else {
		return x > p_v.x;
	}
}

bool Vector3_64::operator<=(const Vector3_64 &p_v) const {
	if (x == p_v.x) {
		if (y == p_v.y) {
			return z <= p_v.z;
		} else {
			return y < p_v.y;
		}
	} else {
		return x < p_v.x;
	}
}

bool Vector3_64::operator>=(const Vector3_64 &p_v) const {
	if (x == p_v.x) {
		if (y == p_v.y) {
			return z >= p_v.z;
		} else {
			return y > p_v.y;
		}
	} else {
		return x > p_v.x;
	}
}

double Vector3_64::length() const {
	double x2 = x * x;
	double y2 = y * y;
	double z2 = z * z;

	return Math::sqrt(x2 + y2 + z2);
}

double Vector3_64::length_squared() const {
	double x2 = x * x;
	double y2 = y * y;
	double z2 = z * z;

	return x2 + y2 + z2;
}

void Vector3_64::normalize() {
	double lengthsq = length_squared();
	if (lengthsq == 0) {
		x = y = z = 0;
	} else {
		double length = Math::sqrt(lengthsq);
		x /= length;
		y /= length;
		z /= length;
	}
}

Vector3_64 Vector3_64::normalized() const {
	Vector3_64 v = *this;
	v.normalize();
	return v;
}

bool Vector3_64::is_normalized() const {
	// use length_squared() instead of length() to avoid sqrt(), makes it more stringent.
	return Math::is_equal_approx(length_squared(), 1, (double)UNIT_EPSILON);
}

Vector3_64 Vector3_64::inverse() const {
	return Vector3_64(1 / x, 1 / y, 1 / z);
}

void Vector3_64::zero() {
	x = y = z = 0;
}

bool Vector3_64::is_equal_approx(const Vector3_64 &p_v, double p_tolerance) const {
	return Math::is_equal_approx(x, p_v.x, p_tolerance) && Math::is_equal_approx(y, p_v.y, p_tolerance) && Math::is_equal_approx(z, p_v.z, p_tolerance);
}
