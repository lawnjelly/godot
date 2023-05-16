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

		if (p_vector.x < begin.x) {
			begin.x = p_vector.x;
		}
		if (p_vector.y < begin.y) {
			begin.y = p_vector.y;
		}

		if (p_vector.x > end.x) {
			end.x = p_vector.x;
		}
		if (p_vector.y > end.y) {
			end.y = p_vector.y;
		}

		position = begin;
		size = end - begin;
	}
};

} // namespace NavPhysics
