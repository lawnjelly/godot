#include "navphysics_plane.h"

namespace NavPhysics {

bool Plane::intersects_ray(const FPoint3 &p_from, const FPoint3 &p_dir, FPoint3 *p_intersection) const {
	FPoint3 segment = p_dir;
	freal den = normal.dot(segment);

	if (Math::is_zero_approx(den)) {
		return false;
	}

	freal dist = (normal.dot(p_from) - d) / den;

	if (dist > (freal)Math::CMP_EPSILON) { //this is a ray, before the emitting pos (p_from) doesn't exist

		return false;
	}

	dist = -dist;
	*p_intersection = p_from + segment * dist;

	return true;
}

} // namespace NavPhysics
