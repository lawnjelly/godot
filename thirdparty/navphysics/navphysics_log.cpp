#include "navphysics_log.h"
#include "navphysics_pointf.h"
#include "navphysics_pointi.h"
#include <iostream>

namespace NavPhysics {

np_log_callback g_log_callback = nullptr;

void set_log_callback(np_log_callback p_callback) {
	g_log_callback = p_callback;
}

void loga(String p_sz, u32 p_priority) {
	if (g_log_callback) {
		g_log_callback(p_sz.get_cstring());
	} else {
		std::cout << p_sz.get_cstring();
	}
}

void log(String p_sz, u32 p_depth, u32 p_priority) {
	String sz;
	for (u32 n = 0; n < p_depth; n++) {
		sz += "\t";
	}

	if (!g_log_callback) {
		loga(sz + p_sz + "\n", p_priority);
	} else {
		loga(sz + p_sz, p_priority);
	}
}

// Compatibility .. remove later.
void print_line(String p_sz) {
	log(p_sz);
}
String itos(i32 p_val) {
	return String(p_val);
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
