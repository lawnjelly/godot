// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#pragma once

#include "navphysics_error.h"
#include "navphysics_log.h"
#include "navphysics_pointf.h"
#include "navphysics_typedefs.h"

namespace NavPhysics {

struct [[nodiscard]] IPoint2 {
	static const i32 AXIS_COUNT = 2;
	static const i32 NORMALIZE_RANGE = 65535;

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
	static i64 which_side(const IPoint2 &a, const IPoint2 &b, const IPoint2 &c) {
		NP_CHECK_32(b.x - a.x);
		NP_CHECK_32(b.y - a.y);
		IPoint2 wall_vec = b - a;
		NP_CHECK_32(c.x - a.x);
		NP_CHECK_32(c.y - a.y);
		IPoint2 pt_vec = c - a;

		// log(String("wall vec ") + wall_vec + ", pt_vec " + pt_vec + ", cross " + wall_vec.cross(pt_vec));

		return wall_vec.cross(pt_vec);
	}
	freal angle() const { return Math::atan2_real(y, x); }
	//	static IPoint2 make(const FPoint2 &p_v) {
	//		return IPoint2::make(p_v.x, p_v.y);
	//	}
	void round_from(const FPoint2 &p_v) {
		x = Math::round32(p_v.x);
		y = Math::round32(p_v.y);
	}
	void from_angle(freal p_angle) {
		FPoint2 pt;
		pt.from_angle(p_angle);
		pt *= 65535.0f;
		round_from(pt);
	}
	freal normalize_to_scale(freal p_scale) {
		FPoint2 temp = to_f32();
		freal length = temp.normalize();
		temp *= p_scale;
		round_from(temp);
		return length;
	}
	freal normalize() { return normalize_to_scale(NORMALIZE_RANGE); }

	i64 dot(const IPoint2 &p_v) const { return ((i64)x * p_v.x) + ((i64)y * p_v.y); }

	freal dot_normalized(const IPoint2 &p_o) const {
		FPoint2 v0 = to_f32();
		FPoint2 v1 = p_o.to_f32();
		v0.normalize();
		v1.normalize();
		return v0.dot(v1);
	}

	void lerp(const IPoint2 &p_o, f32 p_fraction) {
		FPoint2 a = to_f32();
		FPoint2 b = p_o.to_f32();
		a.lerp(b, p_fraction);

		round_from(a);
	}

	bool is_zero() const { return (x == 0) && (y == 0); }

	// Just for easy reading of large fixed point numbers.
	IPoint2 readable() const { return *this / 1000000; }

	IPoint2() {
		zero();
	}
	IPoint2(i32 p_x, i32 p_y) {
		x = p_x;
		y = p_y;
	}
	IPoint2(const FPoint2 &p_pt) {
		round_from(p_pt);
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
	void normalize_to_scale(freal p_scale) {
		FPoint3 temp = to_f32();
		temp.normalize();
		temp *= p_scale;
		round_from(temp);
	}
	void normalize() { normalize_to_scale(IPoint2::NORMALIZE_RANGE); }
	bool is_zero() const { return (x == 0) && (y == 0) && (z == 0); }
	
	void clamp_to_fp_range()
	{
		const i32 r = NAVPHYSICS_MESH_FP_RANGE_WITH_LIP;
		x = CLAMP(x,	-r, r);
		y = CLAMP(y,	-r, r);
		z = CLAMP(z,	-r, r);
	}

	void round_from(const FPoint3 &p_v) {
		x = Math::round32(p_v.x);
		y = Math::round32(p_v.y);
		z = Math::round32(p_v.z);
	}
	IPoint2 xz() const { return IPoint2(x, z); }
	static IPoint3 from(const IPoint2 &p_pt) { return IPoint3(p_pt.x, 0, p_pt.y); }

	IPoint3() {
		zero();
	}

	IPoint3(i32 p_x, i32 p_y, i32 p_z) {
		x = p_x;
		y = p_y;
		z = p_z;
	}

	IPoint3(const FPoint3 &p_v) {
		round_from(p_v);
	}
};

// A way of storing a navigation mesh position WITH the pre-found poly ID.
struct [[nodiscard]] LocalPoint3 {
	IPoint3 pos;
	u32 poly_id = UINT32_MAX;

	void blank() {
		pos.zero();
		poly_id = UINT32_MAX;
	}
};

} // namespace NavPhysics
