#pragma once

#include "simd_types.h"

namespace GSimd {

struct f32_3 {
	float32_t v[3];

	float dot(const f32_3 &o) const {
		return (v[0] * o.v[0]) + (v[1] * o.v[1]) + (v[2] * o.v[2]);
	}

	f32_3 cross(const f32_3 &o) const {
		f32_3 t;
		t.v[0] = (v[1] * o.v[2]);
		t.v[1] = (v[2] * o.v[0]);
		t.v[2] = (v[0] * o.v[1]);

		t.v[0] -= (v[2] * o.v[1]);
		t.v[1] -= (v[0] * o.v[2]);
		t.v[2] -= (v[1] * o.v[0]);

		return t;
	}

	f32_3 unit_cross(const f32_3 &o) const {
		return cross(o).normalize();
	}

	float normalize_in_place() {
		float l_squared = (v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]);
		if (unlikely(l_squared == 0)) {
			v[0] = v[1] = v[2] = 0;
			return 0;
		}
		float l = Math::sqrt(l_squared);

		v[0] /= l;
		v[1] /= l;
		v[2] /= l;

		return l;
	}

	f32_3 normalize() const {
		f32_3 t = *this;
		t.normalize_in_place();
		return t;
	}

	f32_3 inv_xform(const f32_transform &tr) const {
		f32_3 o;
		o.v[0] = v[0] - tr.origin[0];
		o.v[1] = v[1] - tr.origin[1];
		o.v[2] = v[2] - tr.origin[2];

		f32_3 t;
		t.v[0] = (tr.basis[0] * o.v[0]) + (tr.basis[1] * o.v[1]) + (tr.basis[2] * o.v[2]);
		t.v[1] = (tr.basis[4] * o.v[0]) + (tr.basis[5] * o.v[1]) + (tr.basis[6] * o.v[2]);
		t.v[2] = (tr.basis[8] * o.v[0]) + (tr.basis[9] * o.v[1]) + (tr.basis[10] * o.v[2]);

		return t;
	}

	f32_3 xform(const f32_transform &tr) const {
		f32_3 t;
		t.v[0] = (tr.basis[0] * v[0]) + (tr.basis[1] * v[1]) + (tr.basis[2] * v[2]) + tr.origin[0];
		t.v[1] = (tr.basis[4] * v[0]) + (tr.basis[5] * v[1]) + (tr.basis[6] * v[2]) + tr.origin[1];
		t.v[2] = (tr.basis[8] * v[0]) + (tr.basis[9] * v[1]) + (tr.basis[10] * v[2]) + tr.origin[2];
		return t;
	}

	float length_squared() const {
		return (v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]);
	}

	float length() const {
		float sl = length_squared();
		return Math::sqrt(sl);
	}
};

} //namespace GSimd
