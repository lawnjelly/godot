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

///////////////////////////////////////////

template <typename VEC_T, typename T>
void PlaneT<VEC_T, T>::normalize() {
	T l = normal.length();
	if (l == 0) {
		*this = PlaneT(0, 0, 0, 0);
		return;
	}
	normal /= l;
	d /= l;
}

template <typename VEC_T, typename T>
VEC_T PlaneT<VEC_T, T>::get_any_perpendicular_normal() const {
	static const VEC_T p1 = VEC_T(1, 0, 0);
	static const VEC_T p2 = VEC_T(0, 1, 0);
	VEC_T p;

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

template <typename VEC_T, typename T>
bool PlaneT<VEC_T, T>::intersect_3(const PlaneT &p_plane1, const PlaneT &p_plane2, VEC_T *r_result) const {
	const PlaneT &p_plane0 = *this;
	VEC_T normal0 = p_plane0.normal;
	VEC_T normal1 = p_plane1.normal;
	VEC_T normal2 = p_plane2.normal;

	T denom = normal0.cross(normal1).dot(normal2);

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

template <typename VEC_T, typename T>
bool PlaneT<VEC_T, T>::intersects_ray(const VEC_T &p_from, const VEC_T &p_dir, VEC_T *p_intersection) const {
	VEC_T segment = p_dir;
	T den = normal.dot(segment);

	//printf("den is %i\n",den);
	if (Math::is_zero_approx(den)) {
		return false;
	}

	T dist = (normal.dot(p_from) - d) / den;
	//printf("dist is %i\n",dist);

	if (dist > (T)CMP_EPSILON) { //this is a ray, before the emitting pos (p_from) doesn't exist

		return false;
	}

	dist = -dist;
	*p_intersection = p_from + segment * dist;

	return true;
}

template <typename VEC_T, typename T>
bool PlaneT<VEC_T, T>::intersects_segment(const VEC_T &p_begin, const VEC_T &p_end, VEC_T *p_intersection) const {
	VEC_T segment = p_begin - p_end;
	T den = normal.dot(segment);

	//printf("den is %i\n",den);
	if (Math::is_zero_approx(den)) {
		return false;
	}

	T dist = (normal.dot(p_begin) - d) / den;
	//printf("dist is %i\n",dist);

	if (dist < (T)-CMP_EPSILON || dist > (1 + (T)CMP_EPSILON)) {
		return false;
	}

	dist = -dist;
	*p_intersection = p_begin + segment * dist;

	return true;
}

/* misc */

template <typename VEC_T, typename T>
bool PlaneT<VEC_T, T>::is_equal_approx(const PlaneT &p_plane) const {
	return normal.is_equal_approx(p_plane.normal) && Math::is_equal_approx(d, p_plane.d);
}

template <typename VEC_T, typename T>
PlaneT<VEC_T, T>::operator String() const {
	return normal.operator String() + ", " + rtos(d);
}
