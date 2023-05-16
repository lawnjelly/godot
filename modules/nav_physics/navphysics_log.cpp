#include "navphysics_log.h"
#include "navphysics_pointf.h"
#include "navphysics_pointi.h"
#include <iostream>

namespace NavPhysics {

void loga(String p_sz, u32 p_priority) {
	std::cout << p_sz.get_cstring();
}

#define NAVPHYSICS_IMPL_STR(T)                    \
	String str(const T &p_pt) {                   \
		String sz = "{ ";                         \
		for (u32 n = 0; n < T::AXIS_COUNT; n++) { \
			sz += String(p_pt.coord[n]);          \
			if (n != T::AXIS_COUNT - 1) {         \
				sz += ", ";                       \
			}                                     \
		}                                         \
		sz += " }";                               \
		return sz;                                \
	}

NAVPHYSICS_IMPL_STR(IPoint2)
NAVPHYSICS_IMPL_STR(IPoint3)
NAVPHYSICS_IMPL_STR(FPoint2)
NAVPHYSICS_IMPL_STR(FPoint3)

} // namespace NavPhysics
