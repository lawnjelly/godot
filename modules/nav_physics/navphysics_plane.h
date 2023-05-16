#pragma once

#include "navphysics_pointf.h"

namespace NavPhysics {

struct [[nodiscard]] Plane {
	FPoint3 normal;
	freal d;

	bool intersects_ray(const FPoint3 &p_from, const FPoint3 &p_dir, FPoint3 *p_intersection) const;
	void zero() {
		normal.zero();
		d = 0;
	}
	static Plane make(const FPoint3 &p_normal, freal p_d) {
		Plane p;
		p.set(p_normal, p_d);
		return p;
	}
	void set(const FPoint3 &p_normal, freal p_d) {
		normal = p_normal;
		d = p_d;
	}
	void set(const FPoint3 &p_point, const FPoint3 &p_normal) {
		normal = p_normal;
		d = normal.dot(p_point);
	}
};

} // namespace NavPhysics
