#pragma once

#include "core/hashfuncs.h"
#include "core/math/math_funcs.h"

class String;
struct Vector3;
struct Vector3_64;

struct _NO_DISCARD_CLASS_ Vector3i {
	static const int AXIS_COUNT = 3;

	enum Axis {
		AXIS_X,
		AXIS_Y,
		AXIS_Z,
	};

	union {
		struct {
			int32_t x;
			int32_t y;
			int32_t z;
		};

		int32_t coord[3] = { 0 };
	};

	_FORCE_INLINE_ const int32_t &operator[](uint32_t p_axis) const {
		DEV_ASSERT(p_axis < 3);
		return coord[p_axis];
	}

	_FORCE_INLINE_ int32_t &operator[](uint32_t p_axis) {
		DEV_ASSERT(p_axis < 3);
		return coord[p_axis];
	}

	/* Operators */

	constexpr Vector3i &operator+=(const Vector3i &p_v);
	constexpr Vector3i operator+(const Vector3i &p_v) const;
	constexpr Vector3i &operator-=(const Vector3i &p_v);
	constexpr Vector3i operator-(const Vector3i &p_v) const;
	constexpr Vector3i &operator*=(const Vector3i &p_v);
	constexpr Vector3i operator*(const Vector3i &p_v) const;
	constexpr Vector3i &operator/=(const Vector3i &p_v);
	constexpr Vector3i operator/(const Vector3i &p_v) const;
	constexpr Vector3i &operator%=(const Vector3i &p_v);
	constexpr Vector3i operator%(const Vector3i &p_v) const;

	constexpr Vector3i &operator*=(int32_t p_scalar);
	constexpr Vector3i operator*(int32_t p_scalar) const;
	constexpr Vector3i &operator/=(int32_t p_scalar);
	constexpr Vector3i operator/(int32_t p_scalar) const;
	constexpr Vector3i &operator%=(int32_t p_scalar);
	constexpr Vector3i operator%(int32_t p_scalar) const;

	constexpr Vector3i operator-() const;

	constexpr bool operator==(const Vector3i &p_v) const;
	constexpr bool operator!=(const Vector3i &p_v) const { return !(*this == p_v); }
	constexpr bool operator<(const Vector3i &p_v) const;
	constexpr bool operator<=(const Vector3i &p_v) const;
	constexpr bool operator>(const Vector3i &p_v) const;
	constexpr bool operator>=(const Vector3i &p_v) const;

	int64_t length_squared() const;
	int64_t distance_squared_to(const Vector3i &p_to) const;

	double length() const;
	double distance_to(const Vector3i &p_to) const;

	double calculate_triangle_area(const Vector3i &p_a, const Vector3i &p_b) const;

	Vector3i abs() const;

	operator String() const;
	operator Vector3() const;
	operator Vector3_64() const;

#if 0
#define HASH_MURMUR3_SEED 0x7F07C65
	// Murmurhash3 32-bit version.
	// All MurmurHash versions are public domain software, and the author disclaims all copyright to their code.

	uint32_t hash_murmur3_one_32(uint32_t p_in, uint32_t p_seed = HASH_MURMUR3_SEED) const {
		p_in *= 0xcc9e2d51;
		p_in = (p_in << 15) | (p_in >> 17);
		p_in *= 0x1b873593;

		p_seed ^= p_in;
		p_seed = (p_seed << 13) | (p_seed >> 19);
		p_seed = p_seed * 5 + 0xe6546b64;

		return p_seed;
	}

	uint32_t hash_fmix32(uint32_t h) const {
		h ^= h >> 16;
		h *= 0x85ebca6b;
		h ^= h >> 13;
		h *= 0xc2b2ae35;
		h ^= h >> 16;

		return h;
	}
#endif

	uint32_t hash() const {
		uint32_t h = hash_murmur3_one_32(uint32_t(x));
		h = hash_murmur3_one_32(uint32_t(y), h);
		h = hash_murmur3_one_32(uint32_t(z), h);
		return hash_fmix32(h);
	}

	constexpr Vector3i(int32_t p_x, int32_t p_y, int32_t p_z) {
		x = p_x;
		y = p_y;
		z = p_z;
	}
	constexpr Vector3i() { x = y = z = 0; }
};

/* Operators */

inline constexpr Vector3i &Vector3i::operator+=(const Vector3i &p_v) {
	x += p_v.x;
	y += p_v.y;
	z += p_v.z;
	return *this;
}

inline constexpr Vector3i Vector3i::operator+(const Vector3i &p_v) const {
	return Vector3i(x + p_v.x, y + p_v.y, z + p_v.z);
}

inline constexpr Vector3i &Vector3i::operator-=(const Vector3i &p_v) {
	x -= p_v.x;
	y -= p_v.y;
	z -= p_v.z;
	return *this;
}

inline constexpr Vector3i Vector3i::operator-(const Vector3i &p_v) const {
	return Vector3i(x - p_v.x, y - p_v.y, z - p_v.z);
}

inline constexpr Vector3i &Vector3i::operator*=(const Vector3i &p_v) {
	x *= p_v.x;
	y *= p_v.y;
	z *= p_v.z;
	return *this;
}

inline constexpr Vector3i Vector3i::operator*(const Vector3i &p_v) const {
	return Vector3i(x * p_v.x, y * p_v.y, z * p_v.z);
}

inline constexpr Vector3i &Vector3i::operator/=(const Vector3i &p_v) {
	x /= p_v.x;
	y /= p_v.y;
	z /= p_v.z;
	return *this;
}

inline constexpr Vector3i Vector3i::operator/(const Vector3i &p_v) const {
	return Vector3i(x / p_v.x, y / p_v.y, z / p_v.z);
}

inline constexpr Vector3i &Vector3i::operator%=(const Vector3i &p_v) {
	x %= p_v.x;
	y %= p_v.y;
	z %= p_v.z;
	return *this;
}

inline constexpr Vector3i Vector3i::operator%(const Vector3i &p_v) const {
	return Vector3i(x % p_v.x, y % p_v.y, z % p_v.z);
}

inline constexpr Vector3i &Vector3i::operator*=(int32_t p_scalar) {
	x *= p_scalar;
	y *= p_scalar;
	z *= p_scalar;
	return *this;
}

inline constexpr Vector3i Vector3i::operator*(int32_t p_scalar) const {
	return Vector3i(x * p_scalar, y * p_scalar, z * p_scalar);
}

inline constexpr Vector3i &Vector3i::operator/=(int32_t p_scalar) {
	x /= p_scalar;
	y /= p_scalar;
	z /= p_scalar;
	return *this;
}

inline int64_t Vector3i::length_squared() const {
	return x * (int64_t)x + y * (int64_t)y + z * (int64_t)z;
}

inline int64_t Vector3i::distance_squared_to(const Vector3i &p_to) const {
	return (p_to - *this).length_squared();
}

inline double Vector3i::length() const {
	return Math::sqrt((double)length_squared());
}

inline double Vector3i::distance_to(const Vector3i &p_to) const {
	return (p_to - *this).length();
}

inline Vector3i Vector3i::abs() const {
	return Vector3i(Math::abs(x), Math::abs(y), Math::abs(z));
}

inline constexpr bool Vector3i::operator==(const Vector3i &p_v) const {
	return x == p_v.x && y == p_v.y && z == p_v.z;
}
