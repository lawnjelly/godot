/**************************************************************************/
/*  vector4.cpp                                                           */
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

#include "vector4.h"

#include "core/math/math_funcs.h"
#include "core/ustring.h"

template <typename T>
typename Vector4Base<T>::Axis Vector4Base<T>::min_axis_index() const {
	uint32_t min_index = 0;
	T min_value = x;
	for (uint32_t i = 1; i < 4; i++) {
		if (operator[](i) <= min_value) {
			min_index = i;
			min_value = operator[](i);
		}
	}
	return Vector4Base<T>::Axis(min_index);
}

template <typename T>
typename Vector4Base<T>::Axis Vector4Base<T>::max_axis_index() const {
	uint32_t max_index = 0;
	T max_value = x;
	for (uint32_t i = 1; i < 4; i++) {
		if (operator[](i) > max_value) {
			max_index = i;
			max_value = operator[](i);
		}
	}
	return Vector4Base<T>::Axis(max_index);
}

template <typename T>
bool Vector4Base<T>::is_equal_approx(const Vector4Base &p_vec4) const {
	return Math::is_equal_approx(x, p_vec4.x) && Math::is_equal_approx(y, p_vec4.y) && Math::is_equal_approx(z, p_vec4.z) && Math::is_equal_approx(w, p_vec4.w);
}

template <typename T>
bool Vector4Base<T>::is_zero_approx() const {
	return Math::is_zero_approx(x) && Math::is_zero_approx(y) && Math::is_zero_approx(z) && Math::is_zero_approx(w);
}

template <typename T>
bool Vector4Base<T>::is_finite() const {
	return Math::is_finite(x) && Math::is_finite(y) && Math::is_finite(z) && Math::is_finite(w);
}

template <typename T>
T Vector4Base<T>::length() const {
	return Math::sqrt(length_squared());
}

template <typename T>
void Vector4Base<T>::normalize() {
	if (!is_finite()) {
#ifdef MATH_CHECKS
		WARN_PRINT("Vector4 cannot be normalized, the elements must be finite. Making (0, 0, 0, 0) as a fallback.");
#endif // MATH_CHECKS
		zero();
		return;
	}

	T l = length_squared();
	if (l == 0) {
		zero();
	} else {
		l = Math::sqrt(l);
		x /= l;
		y /= l;
		z /= l;
		w /= l;
	}
}

template <typename T>
Vector4Base<T> Vector4Base<T>::normalized() const {
	Vector4Base v = *this;
	v.normalize();
	return v;
}

template <typename T>
bool Vector4Base<T>::is_normalized() const {
	return Math::is_equal_approx(length_squared(), (T)1, (T)UNIT_EPSILON);
}

template <typename T>
T Vector4Base<T>::distance_to(const Vector4Base &p_to) const {
	return (p_to - *this).length();
}

template <typename T>
T Vector4Base<T>::distance_squared_to(const Vector4Base &p_to) const {
	return (p_to - *this).length_squared();
}

template <typename T>
Vector4Base<T> Vector4Base<T>::direction_to(const Vector4Base &p_to) const {
	Vector4Base ret(p_to.x - x, p_to.y - y, p_to.z - z, p_to.w - w);
	ret.normalize();
	return ret;
}

template <typename T>
Vector4Base<T> Vector4Base<T>::abs() const {
	return Vector4Base(Math::abs(x), Math::abs(y), Math::abs(z), Math::abs(w));
}

template <typename T>
Vector4Base<T> Vector4Base<T>::sign() const {
	return Vector4Base(SIGN(x), SIGN(y), SIGN(z), SIGN(w));
}

template <typename T>
Vector4Base<T> Vector4Base<T>::floor() const {
	return Vector4Base(Math::floor(x), Math::floor(y), Math::floor(z), Math::floor(w));
}

template <typename T>
Vector4Base<T> Vector4Base<T>::ceil() const {
	return Vector4Base(Math::ceil(x), Math::ceil(y), Math::ceil(z), Math::ceil(w));
}

template <typename T>
Vector4Base<T> Vector4Base<T>::round() const {
	return Vector4Base(Math::round(x), Math::round(y), Math::round(z), Math::round(w));
}

template <typename T>
Vector4Base<T> Vector4Base<T>::lerp(const Vector4Base &p_to, T p_weight) const {
	Vector4Base res = *this;
	res.x = Math::lerp(res.x, p_to.x, p_weight);
	res.y = Math::lerp(res.y, p_to.y, p_weight);
	res.z = Math::lerp(res.z, p_to.z, p_weight);
	res.w = Math::lerp(res.w, p_to.w, p_weight);
	return res;
}

template <typename T>
Vector4Base<T> Vector4Base<T>::posmod(T p_mod) const {
	return Vector4Base(Math::fposmod(x, p_mod), Math::fposmod(y, p_mod), Math::fposmod(z, p_mod), Math::fposmod(w, p_mod));
}

template <typename T>
Vector4Base<T> Vector4Base<T>::posmodv(const Vector4Base &p_modv) const {
	return Vector4Base(Math::fposmod(x, p_modv.x), Math::fposmod(y, p_modv.y), Math::fposmod(z, p_modv.z), Math::fposmod(w, p_modv.w));
}

template <typename T>
Vector4Base<T> Vector4Base<T>::inverse() const {
	return Vector4Base(1.0f / x, 1.0f / y, 1.0f / z, 1.0f / w);
}

template <typename T>
Vector4Base<T> Vector4Base<T>::clamp(const Vector4Base &p_min, const Vector4Base &p_max) const {
	return Vector4Base(
			CLAMP(x, p_min.x, p_max.x),
			CLAMP(y, p_min.y, p_max.y),
			CLAMP(z, p_min.z, p_max.z),
			CLAMP(w, p_min.w, p_max.w));
}

template <typename T>
Vector4Base<T> Vector4Base<T>::clampf(T p_min, T p_max) const {
	return Vector4Base(
			CLAMP(x, p_min, p_max),
			CLAMP(y, p_min, p_max),
			CLAMP(z, p_min, p_max),
			CLAMP(w, p_min, p_max));
}

template <typename T>
Vector4Base<T>::operator String() const {
	return "(" + rtos(x) + ", " + rtos(y) + ", " + rtos(z) + ", " + rtos(w) + ")";
}

template struct Vector4Base<real_t>;
static_assert(sizeof(Vector4Base<real_t>) == 4 * sizeof(real_t));
