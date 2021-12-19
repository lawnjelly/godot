#pragma once

#include "vector2.h"
#include <cstdint>

class Vec2i {
public:
	Vec2i() = default;
	Vec2i(int xx, int yy) {
		x = xx;
		y = yy;
	}
	int32_t x, y;

	Vector2 vec2() const { return Vector2(x, y); }

	Vec2i operator+(const Vec2i &p_v) const {
		return Vec2i(x + p_v.x, y + p_v.y);
	}
	void operator+=(const Vec2i &p_v) {
		x += p_v.x;
		y += p_v.y;
	}
	Vec2i operator-(const Vec2i &p_v) const {
		return Vec2i(x - p_v.x, y - p_v.y);
	}
	void operator-=(const Vec2i &p_v) {
		x -= p_v.x;
		y -= p_v.y;
	}

	bool operator==(const Vec2i &p_o) const { return x == p_o.x && y == p_o.y; }
	bool operator!=(const Vec2i &p_o) const { return (*this == p_o) == false; }
	bool operator<(const Vec2i &p_o) const { return x == p_o.x ? (y < p_o.y) : (x < p_o.x); }

	int32_t cross(const Vec2i &A, const Vec2i &B) const {
		return (A.x - x) * (B.y - y) - (A.y - y) * (B.x - x);
	}

	int64_t length_squared() const { return ((int64_t)x * x) + ((int64_t)y * y); }
	real_t length() const { return Math::sqrt((double)length_squared()); }

	// https://www.geeksforgeeks.org/check-if-two-given-line-segments-intersect/

	// Given three collinear points p, q, r, the function checks if
	// point q lies on line segment 'pr'
	static bool on_segment(Vec2i p, Vec2i q, Vec2i r) {
		if (q.x <= MAX(p.x, r.x) && q.x >= MIN(p.x, r.x) &&
				q.y <= MAX(p.y, r.y) && q.y >= MIN(p.y, r.y))
			return true;

		return false;
	}

	// To find orientation of ordered triplet (p, q, r).
	// The function returns following values
	// 0 --> p, q and r are collinear
	// 1 --> Clockwise
	// 2 --> Counterclockwise
	static int orientation(Vec2i p, Vec2i q, Vec2i r) {
		// See https://www.geeksforgeeks.org/orientation-3-ordered-points/
		// for details of below formula.
		int val = (q.y - p.y) * (r.x - q.x) -
				(q.x - p.x) * (r.y - q.y);

		if (val == 0)
			return 0; // collinear

		return (val > 0) ? 1 : 2; // clock or counterclock wise
	}

	// The main function that returns true if line segment 'p1q1'
	// and 'p2q2' intersect.
	static bool intersect_test_lines(Vec2i p1, Vec2i q1, Vec2i p2, Vec2i q2) {
		// Find the four orientations needed for general and
		// special cases
		int o1 = orientation(p1, q1, p2);
		int o2 = orientation(p1, q1, q2);
		int o3 = orientation(p2, q2, p1);
		int o4 = orientation(p2, q2, q1);

		// General case
		if (o1 != o2 && o3 != o4)
			return true;

		// Special Cases
		// p1, q1 and p2 are collinear and p2 lies on segment p1q1
		if (o1 == 0 && on_segment(p1, p2, q1))
			return true;

		// p1, q1 and q2 are collinear and q2 lies on segment p1q1
		if (o2 == 0 && on_segment(p1, q2, q1))
			return true;

		// p2, q2 and p1 are collinear and p1 lies on segment p2q2
		if (o3 == 0 && on_segment(p2, p1, q2))
			return true;

		// p2, q2 and q1 are collinear and q1 lies on segment p2q2
		if (o4 == 0 && on_segment(p2, q1, q2))
			return true;

		return false; // Doesn't fall in any of the above cases
	}
};
