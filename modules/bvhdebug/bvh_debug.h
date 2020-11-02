#pragma once

#include "scene/3d/immediate_geometry.h"

class BVHDebug : public ImmediateGeometry
{
	GDCLASS(BVHDebug, ImmediateGeometry);
public:

protected:
	static void _bind_methods();
	void _notification(int p_what);
};
