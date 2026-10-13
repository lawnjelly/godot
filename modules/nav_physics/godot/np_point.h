#pragma once

#include "scene/3d/spatial.h"

class NPPoint : public Spatial {
	GDCLASS(NPPoint, Spatial);

	struct Data {
		String label;
		int32_t type = 0;

		// ID of the POI in the navphysics.
		// UINT32_MAX if unset.
		uint32_t id = UINT32_MAX;
	} data;

	void _resync_parent();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_label(String p_label);
	String get_label() const;

	void set_type(int32_t p_type);
	int32_t get_type() const;
};
