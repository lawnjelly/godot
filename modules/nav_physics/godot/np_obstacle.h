#pragma once

#include "core/resource.h"
#include "scene/3d/spatial.h"
#include "thirdparty/navphysics/navphysics_defines.h"

class NPObstacle : public Resource {
	GDCLASS(NPObstacle, Resource);

protected:
	//	static void _bind_methods();
};

class NPObstacle_Box : public NPObstacle {
	GDCLASS(NPObstacle_Box, NPObstacle);

	struct Data {
		real_t width = 1;
		real_t height = 1;
	} data;

protected:
	static void _bind_methods();

	void set_width(real_t p_width);
	real_t get_width() const { return data.width; }

	void set_height(real_t p_height);
	real_t get_height() const { return data.height; }
};

class NPObstacleInstance : public Spatial {
	GDCLASS(NPObstacleInstance, Spatial);

	struct Data {
		Ref<NPObstacle> obstacle;
	} data;

protected:
	static void _bind_methods();

public:
	void set_obstacle(const Ref<NPObstacle> &p_obstacle);
	Ref<NPObstacle> get_obstacle() const;

	~NPObstacleInstance();
};
