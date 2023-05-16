#pragma once

#include "scene/3d/spatial.h"
#include "thirdparty/navphysics/navphysics_defines.h"

namespace NavPhysics {
class POI;
}

class NPPoint : public Spatial {
	GDCLASS(NPPoint, Spatial);

	struct Data {
		np_handle h_poi = 0;

		String label;
		int32_t type = 0;
	} data;

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	NPPoint();
	~NPPoint();

	void set_label(String p_label);
	String get_label() const;

	void set_type(int32_t p_type);
	int32_t get_type() const;

	np_handle get_handle_poi() const { return data.h_poi; }
	NavPhysics::POI *get_poi() const;
};
