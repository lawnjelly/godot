#pragma once

#include "navphysics_pointf.h"

namespace NavPhysics {

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

	bool contains_point_or_above(const FPoint3 &p_pt) const {
		FPoint3 pt = p_pt - position;
		if (pt.x < 0) {
			return false;
		}
		if (pt.y < 0) {
			//return false;
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
