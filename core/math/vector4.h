/**************************************************************************/
/*  vector4.h                                                             */
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

#include "core/error_macros.h"
#include "core/math/math_defs.h"
#include "core/typedefs.h"

class String;
struct Vector4i;

template <typename T>
struct [[nodiscard]] Vector4Base {
	static constexpr int AXIS_COUNT = 4;

	enum Axis {
		AXIS_X,
		AXIS_Y,
		AXIS_Z,
		AXIS_W,
	};

	union {
		// NOLINTBEGIN(modernize-use-default-member-init)
		struct {
			T x;
			T y;
			T z;
			T w;
		};
		T coord[4] = { 0, 0, 0, 0 };
		// NOLINTEND(modernize-use-default-member-init)
	};

	_FORCE_INLINE_ T &operator[](int p_axis) {
		DEV_ASSERT((unsigned int)p_axis < 4);
		return coord[p_axis];
	}
	_FORCE_INLINE_ const T &operator[](int p_axis) const {
		DEV_ASSERT((unsigned int)p_axis < 4);
		return coord[p_axis];
	}

	typename Vector4Base<T>::Axis min_axis_index() const;
	typename Vector4Base<T>::Axis max_axis_index() const;

	Vector4Base min(const Vector4Base &p_vector4) const {
		return Vector4Base(MIN(x, p_vector4.x), MIN(y, p_vector4.y), MIN(z, p_vector4.z), MIN(w, p_vector4.w));
	}

	Vector4Base minf(T p_scalar) const {
		return Vector4Base(MIN(x, p_scalar), MIN(y, p_scalar), MIN(z, p_scalar), MIN(w, p_scalar));
	}

	Vector4Base max(const Vector4Base &p_vector4) const {
		return Vector4Base(MAX(x, p_vector4.x), MAX(y, p_vector4.y), MAX(z, p_vector4.z), MAX(w, p_vector4.w));
	}

	Vector4Base maxf(T p_scalar) const {
		return Vector4Base(MAX(x, p_scalar), MAX(y, p_scalar), MAX(z, p_scalar), MAX(w, p_scalar));
	}

	_FORCE_INLINE_ T length_squared() const;
	bool is_equal_approx(const Vector4Base &p_vec4) const;
	bool is_zero_approx() const;
	bool is_finite() const;
	T length() const;
	void normalize();
	Vector4Base normalized() const;
	bool is_normalized() const;

	void zero() { x = y = z = w = 0; }

	T distance_to(const Vector4Base &p_to) const;
	T distance_squared_to(const Vector4Base &p_to) const;
	Vector4Base direction_to(const Vector4Base &p_to) const;

	Vector4Base abs() const;
	Vector4Base sign() const;
	Vector4Base floor() const;
	Vector4Base ceil() const;
	Vector4Base round() const;
	Vector4Base lerp(const Vector4Base &p_to, T p_weight) const;

	Vector4Base posmod(T p_mod) const;
	Vector4Base posmodv(const Vector4Base &p_modv) const;
	Vector4Base clamp(const Vector4Base &p_min, const Vector4Base &p_max) const;
	Vector4Base clampf(T p_min, T p_max) const;

	Vector4Base inverse() const;
	_FORCE_INLINE_ T dot(const Vector4Base &p_vec4) const;

	constexpr void operator+=(const Vector4Base &p_vec4);
	constexpr void operator-=(const Vector4Base &p_vec4);
	constexpr void operator*=(const Vector4Base &p_vec4);
	constexpr void operator/=(const Vector4Base &p_vec4);
	constexpr void operator*=(T p_s);
	constexpr void operator/=(T p_s);
	constexpr Vector4Base operator+(const Vector4Base &p_vec4) const;
	constexpr Vector4Base operator-(const Vector4Base &p_vec4) const;
	constexpr Vector4Base operator*(const Vector4Base &p_vec4) const;
	constexpr Vector4Base operator/(const Vector4Base &p_vec4) const;
	constexpr Vector4Base operator-() const;
	constexpr Vector4Base operator*(T p_s) const;
	constexpr Vector4Base operator/(T p_s) const;

	constexpr bool operator==(const Vector4Base &p_vec4) const;
	constexpr bool operator!=(const Vector4Base &p_vec4) const;
	constexpr bool operator>(const Vector4Base &p_vec4) const;
	constexpr bool operator<(const Vector4Base &p_vec4) const;
	constexpr bool operator>=(const Vector4Base &p_vec4) const;
	constexpr bool operator<=(const Vector4Base &p_vec4) const;

	explicit operator String() const;
	operator Vector4i() const;

	// Templated conversion constructor
	template <typename U>
	explicit Vector4Base(const Vector4Base<U> &other) :
			x(static_cast<T>(other.x)), y(static_cast<T>(other.y)), z(static_cast<T>(other.z)), w(static_cast<T>(other.w)) {}

	constexpr Vector4Base() :
			x(0), y(0), z(0), w(0) {}
	constexpr Vector4Base(T p_x, T p_y, T p_z, T p_w) :
			x(p_x), y(p_y), z(p_z), w(p_w) {}
};

template <typename T>
T Vector4Base<T>::dot(const Vector4Base &p_vec4) const {
	return x * p_vec4.x + y * p_vec4.y + z * p_vec4.z + w * p_vec4.w;
}

template <typename T>
T Vector4Base<T>::length_squared() const {
	return dot(*this);
}

template <typename T>
constexpr void Vector4Base<T>::operator+=(const Vector4Base &p_vec4) {
	x += p_vec4.x;
	y += p_vec4.y;
	z += p_vec4.z;
	w += p_vec4.w;
}

template <typename T>
constexpr void Vector4Base<T>::operator-=(const Vector4Base &p_vec4) {
	x -= p_vec4.x;
	y -= p_vec4.y;
	z -= p_vec4.z;
	w -= p_vec4.w;
}

template <typename T>
constexpr void Vector4Base<T>::operator*=(const Vector4Base &p_vec4) {
	x *= p_vec4.x;
	y *= p_vec4.y;
	z *= p_vec4.z;
	w *= p_vec4.w;
}

template <typename T>
constexpr void Vector4Base<T>::operator/=(const Vector4Base &p_vec4) {
	x /= p_vec4.x;
	y /= p_vec4.y;
	z /= p_vec4.z;
	w /= p_vec4.w;
}
template <typename T>
constexpr void Vector4Base<T>::operator*=(T p_s) {
	x *= p_s;
	y *= p_s;
	z *= p_s;
	w *= p_s;
}

template <typename T>
constexpr void Vector4Base<T>::operator/=(T p_s) {
	x /= p_s;
	y /= p_s;
	z /= p_s;
	w /= p_s;
}

template <typename T>
constexpr Vector4Base<T> Vector4Base<T>::operator+(const Vector4Base &p_vec4) const {
	return Vector4Base(x + p_vec4.x, y + p_vec4.y, z + p_vec4.z, w + p_vec4.w);
}

template <typename T>
constexpr Vector4Base<T> Vector4Base<T>::operator-(const Vector4Base &p_vec4) const {
	return Vector4Base(x - p_vec4.x, y - p_vec4.y, z - p_vec4.z, w - p_vec4.w);
}

template <typename T>
constexpr Vector4Base<T> Vector4Base<T>::operator*(const Vector4Base &p_vec4) const {
	return Vector4Base(x * p_vec4.x, y * p_vec4.y, z * p_vec4.z, w * p_vec4.w);
}

template <typename T>
constexpr Vector4Base<T> Vector4Base<T>::operator/(const Vector4Base &p_vec4) const {
	return Vector4Base(x / p_vec4.x, y / p_vec4.y, z / p_vec4.z, w / p_vec4.w);
}

template <typename T>
constexpr Vector4Base<T> Vector4Base<T>::operator-() const {
	return Vector4Base(-x, -y, -z, -w);
}

template <typename T>
constexpr Vector4Base<T> Vector4Base<T>::operator*(T p_s) const {
	return Vector4Base(x * p_s, y * p_s, z * p_s, w * p_s);
}

template <typename T>
constexpr Vector4Base<T> Vector4Base<T>::operator/(T p_s) const {
	return Vector4Base(x / p_s, y / p_s, z / p_s, w / p_s);
}

template <typename T>
constexpr bool Vector4Base<T>::operator==(const Vector4Base &p_vec4) const {
	return x == p_vec4.x && y == p_vec4.y && z == p_vec4.z && w == p_vec4.w;
}

template <typename T>
constexpr bool Vector4Base<T>::operator!=(const Vector4Base &p_vec4) const {
	return x != p_vec4.x || y != p_vec4.y || z != p_vec4.z || w != p_vec4.w;
}

template <typename T>
constexpr bool Vector4Base<T>::operator<(const Vector4Base &p_v) const {
	if (x == p_v.x) {
		if (y == p_v.y) {
			if (z == p_v.z) {
				return w < p_v.w;
			}
			return z < p_v.z;
		}
		return y < p_v.y;
	}
	return x < p_v.x;
}

template <typename T>
constexpr bool Vector4Base<T>::operator>(const Vector4Base &p_v) const {
	if (x == p_v.x) {
		if (y == p_v.y) {
			if (z == p_v.z) {
				return w > p_v.w;
			}
			return z > p_v.z;
		}
		return y > p_v.y;
	}
	return x > p_v.x;
}

template <typename T>
constexpr bool Vector4Base<T>::operator<=(const Vector4Base &p_v) const {
	if (x == p_v.x) {
		if (y == p_v.y) {
			if (z == p_v.z) {
				return w <= p_v.w;
			}
			return z < p_v.z;
		}
		return y < p_v.y;
	}
	return x < p_v.x;
}

template <typename T>
constexpr bool Vector4Base<T>::operator>=(const Vector4Base &p_v) const {
	if (x == p_v.x) {
		if (y == p_v.y) {
			if (z == p_v.z) {
				return w >= p_v.w;
			}
			return z > p_v.z;
		}
		return y > p_v.y;
	}
	return x > p_v.x;
}

template <typename T>
constexpr Vector4Base<T> operator*(float p_scalar, const Vector4Base<T> &p_vec) {
	return p_vec * p_scalar;
}

template <typename T>
constexpr Vector4Base<T> operator*(double p_scalar, const Vector4Base<T> &p_vec) {
	return p_vec * p_scalar;
}

template <typename T>
constexpr Vector4Base<T> operator*(int32_t p_scalar, const Vector4Base<T> &p_vec) {
	return p_vec * p_scalar;
}

template <typename T>
constexpr Vector4Base<T> operator*(int64_t p_scalar, const Vector4Base<T> &p_vec) {
	return p_vec * p_scalar;
}

using Vector4 = Vector4Base<float>;
using Vector4_64 = Vector4Base<double>;
