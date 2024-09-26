#pragma once

#include "navphysics_error.h"
#include "navphysics_pointf.h"
#include "navphysics_typedefs.h"

namespace NavPhysics {

struct [[nodiscard]] IPoint2 {
	static const i32 AXIS_COUNT = 2;
	static const i32 FP_RANGE = 65535;

	union {
		struct
		{
			i32 x, y;
		};
		i32 coord[2];
	};

	NAVPHYSICS_DECLARE_OPERATORS_2(IPoint2, i32)
	u64 length_squared() const { return ((i64)x * x) + ((i64)y * y); }
	freal lengthf_squared() const { return FPoint2::make(x, y).length_squared(); }
	freal lengthf() const { return Math::sqrt_real(lengthf_squared()); }
	freal distancef_to(const IPoint2 &p_v) const { return (p_v - *this).lengthf(); }
	FPoint2 to_f32() const { return FPoint2::make(x, y); }
	i64 cross(const IPoint2 &p_v) const {
		return ((i64)x * p_v.y) - ((i64)y * p_v.x);
	}
	static IPoint2 make(const FPoint2 &p_v) {
		return IPoint2::make(p_v.x, p_v.y);
	}
	void from_f32(const FPoint2 &p_v) {
		x = p_v.x;
		y = p_v.y;
	}
	void normalize_to_scale(freal p_scale) {
		FPoint2 temp = to_f32();
		temp.normalize();
		temp *= p_scale;
		from_f32(temp);
	}
	void normalize() { normalize_to_scale(FP_RANGE); }
	freal dot(const IPoint2 &p_o) const {
		FPoint2 v0 = to_f32();
		FPoint2 v1 = p_o.to_f32();
		v0.normalize();
		v1.normalize();
		return v0.dot(v1);
	}

	IPoint2() {
		zero();
	}
};

struct [[nodiscard]] IPoint3 {
	static const i32 AXIS_COUNT = 3;

	union {
		struct
		{
			i32 x, y, z;
		};
		i32 coord[3];
	};

	NAVPHYSICS_DECLARE_OPERATORS_3(IPoint3, i32)
	u64 length_squared() const { return ((i64)x * x) + ((i64)y * y) + ((i64)z * z); }
	freal lengthf_squared() const { return FPoint3::make(x, y, z).length_squared(); }
	freal lengthf() const { return Math::sqrt_real(lengthf_squared()); }
	freal distancef_to(const IPoint3 &p_v) const { return (p_v - *this).lengthf(); }
	FPoint3 to_f32() const { return FPoint3::make(x, y, z); }
	void from_f32(const FPoint3 &p_v) {
		x = p_v.x;
		y = p_v.y;
		z = p_v.z;
	}
	void normalize_to_scale(freal p_scale) {
		FPoint3 temp = to_f32();
		temp.normalize();
		temp *= p_scale;
		from_f32(temp);
	}
	void normalize() { normalize_to_scale(IPoint2::FP_RANGE); }
	IPoint3() {
		zero();
	}
};

} // namespace NavPhysics
