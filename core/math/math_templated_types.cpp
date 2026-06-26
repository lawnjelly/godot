#include "math_templated_types.h"

#include "core/math/vector3i.h"
#include "core/ustring.h"

template <typename T>
Vector3T<T>::operator String() const {
	return "(" + rtos(x) + ", " + rtos(y) + ", " + rtos(z) + ")";
}

template <typename T>
Vector3T<T>::Vector3T(const Vector3i &p_v) {
	x = p_v.x;
	y = p_v.y;
	z = p_v.z;
}

template <typename T>
Vector3T<T>::operator Vector3i() const {
	return Vector3i(x, y, z);
}

///////////////////////////////////////////////////

template <typename T>
bool Vector4T<T>::is_equal_approx(const Vector4T &p_vec4) const {
	return Math::is_equal_approx(x, p_vec4.x) && Math::is_equal_approx(y, p_vec4.y) && Math::is_equal_approx(z, p_vec4.z) && Math::is_equal_approx(w, p_vec4.w);
}

template <typename T>
bool Vector4T<T>::is_zero_approx() const {
	return Math::is_zero_approx(x) && Math::is_zero_approx(y) && Math::is_zero_approx(z) && Math::is_zero_approx(w);
}

template <typename T>
bool Vector4T<T>::is_finite() const {
	return Math::is_finite(x) && Math::is_finite(y) && Math::is_finite(z) && Math::is_finite(w);
}

template <typename T>
void Vector4T<T>::normalize() {
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
Vector4T<T> Vector4T<T>::normalized() const {
	Vector4T v = *this;
	v.normalize();
	return v;
}

template <typename T>
Vector4T<T>::operator String() const {
	return "(" + rtos(x) + ", " + rtos(y) + ", " + rtos(z) + ", " + rtos(w) + ")";
}

template struct Vector4T<real_t>;
static_assert(sizeof(Vector4T<real_t>) == 4 * sizeof(real_t));
