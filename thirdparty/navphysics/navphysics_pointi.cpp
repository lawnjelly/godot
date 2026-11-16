// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#include "navphysics_pointi.h"

namespace NavPhysics {

void IPoint2::slide_velocity_on_wall_vec(const IPoint2 &p_wall_vec) {
	// The dot contains the polarity (relative to the wall) and the magnitude of the slide.
	freal dot = p_wall_vec.dot_normalized(*this);
	freal vel_mag = lengthf();
	*this = p_wall_vec;
	normalize_to_scale(vel_mag * dot);
}

void IPoint2::slide_velocity_on_wall(const IPoint2 &p_wall_a, const IPoint2 &p_wall_b) {
	IPoint2 wall_vec = p_wall_b - p_wall_a;
	slide_velocity_on_wall_vec(wall_vec);
}

} //namespace NavPhysics
