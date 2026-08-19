#include "navphysics_transform.h"

namespace NavPhysics {

bool Basis::is_equal_approx(const Basis &p_o, f32 p_tolerance) const {
	for (u32 n = 0; n < 3; n++) {
		if (!elements[n].is_equal_approx(p_o.elements[n], p_tolerance)) {
			return false;
		}
	}
	return true;
}

} // namespace NavPhysics
