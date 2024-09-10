#pragma once

#include "navphysics_error.h"
#include "navphysics_math.h"
#include "navphysics_typedefs.h"

namespace NavPhysics {
struct [[nodiscard]] FPoint2 {
	static const i32 AXIS_COUNT = 2;
	enum {
		FP_RANGE = 65535,
	};

	union {
		struct
		{
			freal x, y;
		};
		freal coord[2];
	};

	NAVPHYSICS_DECLARE_OPERATORS_2(FPoint2, freal)
	freal length_squared() const { return (x * x) + (y * y); }
	freal length() const { return Math::sqrt_real(length_squared()); }
	freal distance_to(const FPoint2 &p_v) const { return (p_v - *this).length(); }
	void normalize() {
		freal sl = length_squared();
		if (sl != 0) {
			freal l = Math::sqrt_real(sl);
			freal inv_l = 1 / l;
			(*this) *= inv_l;
		}
	}
	FPoint2 normalized() const {
		FPoint2 res = *this;
		res.normalize();
		return res;
	}
	freal dot(const FPoint2 &p_v) const { return (x * p_v.x) + (y * p_v.y); }
	freal cross(const FPoint2 &p_v) const { return (x * p_v.y) - (y * p_v.x); }
	freal angle() const { return Math::atan2_real(y, x); }
	freal angle_to_point(const FPoint2 &p_v) const { return (p_v - (*this)).angle(); }
	bool is_equal_approx(const FPoint2 &p_v, f32 p_tolerance = Math::NP_CMP_EPSILON) const { return Math::is_equal_approx(x, p_v.x, p_tolerance) && Math::is_equal_approx(y, p_v.y, p_tolerance); }
};

struct [[nodiscard]] FPoint3 {
	static const i32 AXIS_COUNT = 3;

	union {
		struct
		{
			freal x, y, z;
		};
		freal coord[3];
	};

	NAVPHYSICS_DECLARE_OPERATORS_3(FPoint3, freal)
	freal length_squared() const { return (x * x) + (y * y) + (z * z); }
	freal length() const { return Math::sqrt_real(length_squared()); }
	freal distance_to(const FPoint3 &p_v) const { return (p_v - *this).length(); }
	void normalize() {
		freal sl = length_squared();
		if (sl != 0) {
			freal l = Math::sqrt_real(sl);
			freal inv_l = 1 / l;
			(*this) *= inv_l;
		}
	}
	FPoint3 normalized() const {
		FPoint3 res = *this;
		res.normalize();
		return res;
	}
	freal dot(const FPoint3 &p_v) const { return (x * p_v.x) + (y * p_v.y) + (z * p_v.z); }
	bool is_equal_approx(const FPoint3 &p_v, f32 p_tolerance = Math::NP_CMP_EPSILON) const { return Math::is_equal_approx(x, p_v.x, p_tolerance) && Math::is_equal_approx(y, p_v.y, p_tolerance) && Math::is_equal_approx(z, p_v.z, p_tolerance); }
};

} // namespace NavPhysics
