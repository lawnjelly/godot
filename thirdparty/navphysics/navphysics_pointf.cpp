// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#include "navphysics_pointf.h"

namespace NavPhysics {

freal FPoint3::distance_point_from_triangle(const FPoint3 &p_a, const FPoint3 &p_b, const FPoint3 &p_c) const {
	const FPoint3 &pt = *this;

	FPoint3 ab = p_b - p_a;
	FPoint3 ac = p_c - p_a;
	FPoint3 ap = pt - p_a;

	// Check if P in vertex region outside A.
	freal d1 = ab.dot(ap);
	freal d2 = ac.dot(ap);
	if (d1 <= 0 && d2 <= 0) {
		// Barycentric coordinates (1,0,0).
		return (pt - p_a).length();
	}

	// Check if P in vertex region outside B.
	FPoint3 bp = pt - p_b;
	freal d3 = ab.dot(bp);
	freal d4 = ac.dot(bp);
	if (d3 >= 0 && d4 <= d3) {
		// Barycentric coordinates (0,1,0).
		return (pt - p_b).length();
	}

	// Check if P in edge region of AB, if so return distance to segment AB.
	freal vc = d1 * d4 - d3 * d2;
	if (vc <= 0 && d1 >= 0 && d3 <= 0) {
		freal v = d1 / (d1 - d3);
		return (pt - (p_a + ab * v)).length();
	}

	// Check if P in vertex region outside C.
	FPoint3 cp = pt - p_c;
	freal d5 = ab.dot(cp);
	freal d6 = ac.dot(cp);
	if (d6 >= 0 && d5 <= d6) {
		// Barycentric coordinates (0,0,1).
		return (pt - p_c).length();
	}

	// Check if P in edge region of AC, if so return distance to segment AC.
	freal vb = d5 * d2 - d1 * d6;
	if (vb <= 0 && d2 >= 0 && d6 <= 0) {
		freal w = d2 / (d2 - d6);
		return (pt - (p_a + ac * w)).length();
	}

	// Check if P in edge region of BC, if so return distance to segment BC.
	freal va = d3 * d6 - d5 * d4;
	if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) {
		freal w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
		return (pt - (p_b + (p_c - p_b) * w)).length();
	}

	// P is inside face region. Compute closest point using barycentric coordinates (u, v, w).
	freal denom = 1 / (va + vb + vc);
	freal v = vb * denom;
	freal w = vc * denom;
	FPoint3 closestPoint = p_a + ab * v + ac * w;

	return (pt - closestPoint).length();
}

} // namespace NavPhysics
