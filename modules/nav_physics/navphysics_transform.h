#pragma once

#include "navphysics_pointf.h"

namespace NavPhysics {

struct [[nodiscard]] Basis {
	FPoint3 elements[3];

	void init() {
		elements[0] = FPoint3::make(1, 0, 0);
		elements[1] = FPoint3::make(0, 1, 0);
		elements[2] = FPoint3::make(0, 0, 1);
	}

	const FPoint3 &operator[](int p_axis) const {
		return elements[p_axis];
	}
	FPoint3 &operator[](int p_axis) {
		return elements[p_axis];
	}

	void set(freal p_xx, freal p_xy, freal p_xz, freal p_yx, freal p_yy, freal p_yz, freal p_zx, freal p_zy, freal p_zz) {
		elements[0][0] = p_xx;
		elements[0][1] = p_xy;
		elements[0][2] = p_xz;
		elements[1][0] = p_yx;
		elements[1][1] = p_yy;
		elements[1][2] = p_yz;
		elements[2][0] = p_zx;
		elements[2][1] = p_zy;
		elements[2][2] = p_zz;
	}

#define cofac(row1, col1, row2, col2) \
	(elements[row1][col1] * elements[row2][col2] - elements[row1][col2] * elements[row2][col1])

	void invert() {
		freal co[3] = {
			cofac(1, 1, 2, 2), cofac(1, 2, 2, 0), cofac(1, 0, 2, 1)
		};
		freal det = elements[0][0] * co[0] +
				elements[0][1] * co[1] +
				elements[0][2] * co[2];
#ifdef MATH_CHECKS
		NP_NP_ERR_FAIL_COND(det == 0);
#endif
		freal s = 1 / det;

		set(co[0] * s, cofac(0, 2, 2, 1) * s, cofac(0, 1, 1, 2) * s,
				co[1] * s, cofac(0, 0, 2, 2) * s, cofac(0, 2, 1, 0) * s,
				co[2] * s, cofac(0, 1, 2, 0) * s, cofac(0, 0, 1, 1) * s);
	}

#undef cofac

	Basis inverse() const {
		Basis inv = *this;
		inv.invert();
		return inv;
	}
	void transpose() {
		SWAP(elements[0][1], elements[1][0]);
		SWAP(elements[0][2], elements[2][0]);
		SWAP(elements[1][2], elements[2][1]);
	}

	Basis transposed() const {
		Basis tr = *this;
		tr.transpose();
		return tr;
	}

	FPoint3 xform(const FPoint3 &p_vector) const {
		return FPoint3::make(
				elements[0].dot(p_vector),
				elements[1].dot(p_vector),
				elements[2].dot(p_vector));
	}

	// The following normal xform functions are correct for non-uniform scales.
	// Use these two functions in combination to xform a series of normals.
	// First use get_normal_xform_basis() to precalculate the inverse transpose.
	// Then apply xform_normal_fast() multiple times using the inverse transpose basis.
	Basis get_normal_xform_basis() const { return inverse().transposed(); }

	// N.B. This only does a normal transform if the basis used is the inverse transpose!
	// Otherwise use xform_normal().
	FPoint3 xform_normal_fast(const FPoint3 &p_vector) const { return xform(p_vector).normalized(); }

	// This function does the above but for a single normal vector. It is considerably slower, so should usually
	// only be used in cases of single normals, or when the basis changes each time.
	FPoint3 xform_normal(const FPoint3 &p_vector) const { return get_normal_xform_basis().xform_normal_fast(p_vector); }

	bool is_equal_approx(const Basis &p_o, f32 p_tolerance = Math::CMP_EPSILON) const;
};

struct [[nodiscard]] Transform {
	Basis basis;
	FPoint3 origin;

	void init() {
		basis.init();
		origin.zero();
	}

	FPoint3 xform(const FPoint3 &p_vector) const {
		return FPoint3::make(
				basis[0].dot(p_vector) + origin.x,
				basis[1].dot(p_vector) + origin.y,
				basis[2].dot(p_vector) + origin.z);
	}

	bool is_equal_approx(const Transform &p_o, f32 p_tolerance = Math::CMP_EPSILON) const { return basis.is_equal_approx(p_o.basis, p_tolerance) && origin.is_equal_approx(p_o.origin, p_tolerance); }

	void affine_invert() {
		basis.invert();
		origin = basis.xform(-origin);
	}

	Transform affine_inverse() const {
		Transform ret = *this;
		ret.affine_invert();
		return ret;
	}
};

} // namespace NavPhysics
