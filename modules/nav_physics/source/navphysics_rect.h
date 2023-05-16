#pragma once

#include "navphysics_pointf.h"
#include "navphysics_pointi.h"

namespace NavPhysics {

// Exclusive definition.
struct [[nodiscard]] IRect2 {
	IPoint2 position;
	IPoint2 size;

	i64 get_area() const { return (i64)size.x * (i64)size.y; }
	IPoint2 end() const { return position + size; }

	void zero() {
		position.zero();
		size.zero();
	}

	void expand_to(const IRect2 &o) {
		IPoint2 end = position + size;
		IPoint2 end_o = o.end();

		// The begin is easy, no plus 1...
		expand_to_fast(o.position);

		end.x = MAX(end.x, end_o.x);
		end.y = MAX(end.y, end_o.y);

		size = end - position;
	}

	void expand_to_fast(const IPoint2 &p_vector) {
		IPoint2 begin = position;
		IPoint2 end = position + size;

		for (u32 n = 0; n < 2; n++) {
			begin.coord[n] = MIN(begin.coord[n], p_vector.coord[n]);
			end.coord[n] = MAX(end.coord[n], p_vector.coord[n]);
		}

		position = begin;
		size = end - begin;
	}
	void increment_size() {
		size += IPoint2::make(1, 1);
	}
	bool contains_point(const IPoint2 &p_pt) const {
		IPoint2 pt = p_pt - position;
		for (u32 n = 0; n < 2; n++) {
			if (pt.coord[n] < 0) {
				return false;
			}
		}
		for (u32 n = 0; n < 2; n++) {
			if (pt.coord[n] >= size.coord[n]) {
				return false;
			}
		}

		return true;
	}
};

struct [[nodiscard]] Rect2 {
	FPoint2 position;
	FPoint2 size;

	freal get_area() const { return size.x * size.y; }

	void expand_to(const FPoint2 &p_vector) {
		FPoint2 begin = position;
		FPoint2 end = position + size;

		for (u32 n = 0; n < 2; n++) {
			begin.coord[n] = MIN(begin.coord[n], p_vector.coord[n]);
			end.coord[n] = MAX(end.coord[n], p_vector.coord[n]);
		}

		position = begin;
		size = end - begin;
	}
};

struct [[nodiscard]] AABB {
	FPoint3 position;
	FPoint3 size;

	freal get_area() const { return size.x * size.y * size.z; }
	void zero() {
		position.zero();
		size.zero();
	}
	freal end(u32 p_coord) const { return position.coord[p_coord] + size.coord[p_coord]; }

	void expand_to(const FPoint3 &p_vector) {
		FPoint3 begin = position;
		FPoint3 end = position + size;

		for (u32 n = 0; n < 3; n++) {
			begin.coord[n] = MIN(begin.coord[n], p_vector.coord[n]);
			end.coord[n] = MAX(end.coord[n], p_vector.coord[n]);
		}

		position = begin;
		size = end - begin;
	}

	// p_aabb is above, NOT this AABB.
	bool contains_aabb_or_above(const AABB &p_aabb) const {
		for (u32 n = 0; n < 3; n++) {
			if (p_aabb.end(n) < position.coord[n]) {
				return false;
			}
		}
		if (end(0) < p_aabb.position.x) {
			return false;
		}
		if (end(2) < p_aabb.position.z) {
			return false;
		}

		return true;
	}

	bool contains_point_or_above(const FPoint3 &p_pt) const {
		FPoint3 pt = p_pt - position;
		if (pt.x < 0) {
			return false;
		}
		if (pt.y < 0) {
			return false;
		}
		if (pt.z < 0) {
			return false;
		}
		if (pt.x > size.x) {
			return false;
		}
		if (pt.z > size.z) {
			return false;
		}

		return true;
	}

	bool contains_point(const FPoint3 &p_pt) const {
		FPoint3 pt = p_pt - position;
		for (u32 n = 0; n < 3; n++) {
			if (pt.coord[n] < 0) {
				return false;
			}
		}
		for (u32 n = 0; n < 3; n++) {
			if (pt.coord[n] > size.coord[n]) {
				return false;
			}
		}

		return true;
	}
};

} // namespace NavPhysics
