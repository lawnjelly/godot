#pragma once

// Most Godot Geometric Types are derived from `real_t`.
// However for some math, we need to specify 64 bit.
// Here we supply minimal templated versions which can be used for 64 bit.

#include "core/error_macros.h"

class String;
struct Vector3i;

template <typename T>
struct Vector3T {
	T x = 0, y = 0, z = 0;

	Vector3T() = default;
	Vector3T(T x, T y, T z) :
			x(x), y(y), z(z) {}
	Vector3T(const Vector3i &p_v);

	bool operator==(const Vector3T<T> &p_v) const {
		return x == p_v.x && y == p_v.y && z == p_v.z;
	}

	bool operator!=(const Vector3T<T> &p_v) const { return !(*this == p_v); }

	Vector3T<T> operator+(const Vector3T<T> &p_v) const {
		return Vector3T<T>(x + p_v.x, y + p_v.y, z + p_v.z);
	}

	Vector3T<T> operator-(const Vector3T<T> &p_v) const {
		return Vector3T<T>(x - p_v.x, y - p_v.y, z - p_v.z);
	}
	Vector3T<T> operator-() const {
		return Vector3T<T>(-x, -y, -z);
	}

	Vector3T<T> &operator-=(const Vector3T<T> &p_v) {
		x -= p_v.x;
		y -= p_v.y;
		z -= p_v.z;
		return *this;
	}

	Vector3T<T> operator*(const Vector3T<T> &p_v) const {
		return Vector3T<T>(x * p_v.x, y * p_v.y, z * p_v.z);
	}

	Vector3T<T> &operator*=(T p_scalar) {
		x *= p_scalar;
		y *= p_scalar;
		z *= p_scalar;
		return *this;
	}

	Vector3T<T> operator*(T p_scalar) const {
		return Vector3T<T>(x * p_scalar, y * p_scalar, z * p_scalar);
	}

	Vector3T<T> &operator/=(T p_scalar) {
		x /= p_scalar;
		y /= p_scalar;
		z /= p_scalar;
		return *this;
	}

	Vector3T<T> operator/(T p_scalar) const {
		return Vector3T<T>(x / p_scalar, y / p_scalar, z / p_scalar);
	}

	Vector3T<T> cross(const Vector3T<T> &p_v) const {
		return Vector3T<T>(
				y * p_v.z - z * p_v.y,
				z * p_v.x - x * p_v.z,
				x * p_v.y - y * p_v.x);
	}

	T dot(const Vector3T<T> &p_v) const {
		return x * p_v.x + y * p_v.y + z * p_v.z;
	}

	T length_squared() const {
		return x * x + y * y + z * z;
	}

	T length() const {
		return Math::sqrt(length_squared());
	}

	void normalize() {
		T lengthsq = length_squared();
		if (lengthsq == 0) {
			x = y = z = 0;
		} else {
			T length = Math::sqrt(lengthsq);
			x /= length;
			y /= length;
			z /= length;
		}
	}

	Vector3T<T> normalized() const {
		Vector3T<T> v = *this;
		v.normalize();
		return v;
	}

	bool is_equal_approx(const Vector3T<T> &p_v) const {
		return Math::is_equal_approx(x, p_v.x) && Math::is_equal_approx(y, p_v.y) && Math::is_equal_approx(z, p_v.z);
	}

	explicit operator String() const;
	explicit operator Vector3i() const;
};

using Vector3_64 = Vector3T<double>;

///////////////////////////////////////////////////

struct Vector4i;

template <typename T>
struct [[nodiscard]] Vector4T {
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

	T &operator[](int p_axis) {
		DEV_ASSERT((unsigned int)p_axis < 4);
		return coord[p_axis];
	}
	const T &operator[](int p_axis) const {
		DEV_ASSERT((unsigned int)p_axis < 4);
		return coord[p_axis];
	}

	T length_squared() const { return dot(*this); }
	bool is_equal_approx(const Vector4T &p_vec4) const;
	bool is_zero_approx() const;
	bool is_finite() const;
	T length() const { return Math::sqrt(length_squared()); }
	void normalize();
	Vector4T normalized() const;
	bool is_normalized() const { return Math::is_equal_approx(length_squared(), (T)1, (T)UNIT_EPSILON); }

	void zero() { x = y = z = w = 0; }

	T distance_to(const Vector4T &p_to) const { return (p_to - *this).length(); }
	T distance_squared_to(const Vector4T &p_to) const { return (p_to - *this).length_squared(); }

	T dot(const Vector4T &p_vec4) const { return x * p_vec4.x + y * p_vec4.y + z * p_vec4.z + w * p_vec4.w; }

	constexpr void operator+=(const Vector4T &p_vec4) {
		x += p_vec4.x;
		y += p_vec4.y;
		z += p_vec4.z;
		w += p_vec4.w;
	}
	constexpr void operator-=(const Vector4T &p_vec4) {
		x -= p_vec4.x;
		y -= p_vec4.y;
		z -= p_vec4.z;
		w -= p_vec4.w;
	}
	constexpr void operator*=(const Vector4T &p_vec4) {
		x *= p_vec4.x;
		y *= p_vec4.y;
		z *= p_vec4.z;
		w *= p_vec4.w;
	}
	constexpr void operator/=(const Vector4T &p_vec4) {
		x /= p_vec4.x;
		y /= p_vec4.y;
		z /= p_vec4.z;
		w /= p_vec4.w;
	}
	constexpr void operator*=(T p_s) {
		x *= p_s;
		y *= p_s;
		z *= p_s;
		w *= p_s;
	}
	constexpr void operator/=(T p_s) {
		x /= p_s;
		y /= p_s;
		z /= p_s;
		w /= p_s;
	}
	constexpr Vector4T operator+(const Vector4T &p_vec4) const { return Vector4T(x + p_vec4.x, y + p_vec4.y, z + p_vec4.z, w + p_vec4.w); }
	constexpr Vector4T operator-(const Vector4T &p_vec4) const { return Vector4T(x - p_vec4.x, y - p_vec4.y, z - p_vec4.z, w - p_vec4.w); }
	constexpr Vector4T operator*(const Vector4T &p_vec4) const { return Vector4T(x * p_vec4.x, y * p_vec4.y, z * p_vec4.z, w * p_vec4.w); }
	constexpr Vector4T operator/(const Vector4T &p_vec4) const { return Vector4T(x / p_vec4.x, y / p_vec4.y, z / p_vec4.z, w / p_vec4.w); }
	constexpr Vector4T operator-() const { return Vector4T(-x, -y, -z, -w); }

	constexpr Vector4T operator*(T p_s) const { return Vector4T(x * p_s, y * p_s, z * p_s, w * p_s); }
	constexpr Vector4T operator/(T p_s) const { return Vector4T(x / p_s, y / p_s, z / p_s, w / p_s); }

	constexpr bool operator==(const Vector4T &p_vec4) const { return x == p_vec4.x && y == p_vec4.y && z == p_vec4.z && w == p_vec4.w; }
	constexpr bool operator!=(const Vector4T &p_vec4) const { return x != p_vec4.x || y != p_vec4.y || z != p_vec4.z || w != p_vec4.w; }

	explicit operator String() const;
	operator Vector4i() const;

	// Templated conversion constructor
	template <typename U>
	explicit Vector4T(const Vector4T<U> &other) :
			x(static_cast<T>(other.x)), y(static_cast<T>(other.y)), z(static_cast<T>(other.z)), w(static_cast<T>(other.w)) {}

	constexpr Vector4T() :
			x(0), y(0), z(0), w(0) {}
	constexpr Vector4T(T p_x, T p_y, T p_z, T p_w) :
			x(p_x), y(p_y), z(p_z), w(p_w) {}
};

template <typename T>
constexpr Vector4T<T> operator*(float p_scalar, const Vector4T<T> &p_vec) {
	return p_vec * p_scalar;
}

template <typename T>
constexpr Vector4T<T> operator*(double p_scalar, const Vector4T<T> &p_vec) {
	return p_vec * p_scalar;
}

template <typename T>
constexpr Vector4T<T> operator*(int32_t p_scalar, const Vector4T<T> &p_vec) {
	return p_vec * p_scalar;
}

template <typename T>
constexpr Vector4T<T> operator*(int64_t p_scalar, const Vector4T<T> &p_vec) {
	return p_vec * p_scalar;
}

using Vector4_64 = Vector4T<double>;

///////////////////////////////////////////////////

template <typename VEC_T, typename T>
class _NO_DISCARD_CLASS_ PlaneT {
public:
	union {
		struct {
			VEC_T normal;
			T d;
		};

		T coord[4];
	};

	void set_normal(const VEC_T &p_normal) { normal = p_normal; }
	PlaneT get_normal() const { return normal; } ///Point is coplanar, CMP_EPSILON for precision

	void normalize();
	PlaneT normalized() const {
		PlaneT p = *this;
		p.normalize();
		return p;
	}

	/* Plane_64-Point operations */

	VEC_T center() const { return normal * d; }
	VEC_T get_any_point() const { return get_normal() * d; }
	VEC_T get_any_perpendicular_normal() const;

	bool is_point_over(const VEC_T &p_point) const { return (normal.dot(p_point) > d); } ///< Point is over plane
	T distance_to(const VEC_T &p_point) const { return (normal.dot(p_point) - d); }
	bool has_point(const VEC_T &p_point, T _epsilon = CMP_EPSILON) const {
		double dist = normal.dot(p_point) - d;
		dist = ABS(dist);
		return (dist <= _epsilon);
	}

	/* intersections */

	bool intersect_3(const PlaneT &p_plane1, const PlaneT &p_plane2, VEC_T *r_result = nullptr) const;
	bool intersects_ray(const VEC_T &p_from, const VEC_T &p_dir, VEC_T *p_intersection) const;
	bool intersects_segment(const VEC_T &p_begin, const VEC_T &p_end, VEC_T *p_intersection) const;

	VEC_T project(const VEC_T &p_point) const {
		return p_point - normal * distance_to(p_point);
	}

	/* misc */

	PlaneT operator-() const { return PlaneT(-normal, -d); }
	bool is_equal_approx(const PlaneT &p_plane) const;

	bool operator==(const PlaneT &p_plane) const { return normal == p_plane.normal && d == p_plane.d; }
	bool operator!=(const PlaneT &p_plane) const { return !(*this == p_plane); }
	operator String() const;

	PlaneT() :
			d(0) {}
	PlaneT(double p_a, double p_b, double p_c, double p_d) :
			normal(p_a, p_b, p_c),
			d(p_d) {}

	PlaneT(const VEC_T &p_normal, double p_d) :
			normal(p_normal),
			d(p_d) {
	}
	PlaneT(const VEC_T &p_point, const VEC_T &p_normal) :
			normal(p_normal),
			d(p_normal.dot(p_point)) {
	}
	PlaneT(const VEC_T &p_point1, const VEC_T &p_point2, const VEC_T &p_point3, ClockDirection p_dir = CLOCKWISE) {
		if (p_dir == CLOCKWISE) {
			normal = (p_point1 - p_point3).cross(p_point1 - p_point2);
		} else {
			normal = (p_point1 - p_point2).cross(p_point1 - p_point3);
		}

		normal.normalize();
		d = normal.dot(p_point1);
	}
};

using Plane_64 = PlaneT<Vector3_64, double>;
