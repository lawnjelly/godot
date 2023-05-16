// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#include "navphysics_plane.h"

namespace NavPhysics {

bool Plane::intersects_ray(const FPoint3 &p_from, const FPoint3 &p_dir, FPoint3 *p_intersection) const {
	freal den = normal.dot(p_dir);

	if (Math::is_zero_approx(den)) {
		return false;
	}

	freal dist = (normal.dot(p_from) - d) / den;

	if (dist > (freal)Math::NP_CMP_EPSILON) { //this is a ray, before the emitting pos (p_from) doesn't exist

		return false;
	}

	dist = -dist;
	*p_intersection = p_from + p_dir * dist;

	return true;
}

} // namespace NavPhysics
