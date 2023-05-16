// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#include "navphysics_transform.h"

namespace NavPhysics {

freal Basis::calculate_yaw() const
{
	const FPoint3 &forward = elements[2];
	return Math::atan2_real(forward.x, forward.z);
}

bool Basis::is_equal_approx(const Basis &p_o, f32 p_tolerance) const {
	for (u32 n = 0; n < 3; n++) {
		if (!elements[n].is_equal_approx(p_o.elements[n], p_tolerance)) {
			return false;
		}
	}
	return true;
}

} // namespace NavPhysics
