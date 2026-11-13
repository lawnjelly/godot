#pragma once

#include "core/resource.h"
#include "scene/3d/spatial.h"
#include "thirdparty/navphysics/navphysics_defines.h"

class NPObstacle : public Resource {
	GDCLASS(NPObstacle, Resource);

protected:
	//	static void _bind_methods();

public:
	enum class Type {
		UNSET,
		BOX,
	};

	virtual Type get_obstacle_type() const { return Type::UNSET; }
};

class NPObstacle_Box : public NPObstacle {
	GDCLASS(NPObstacle_Box, NPObstacle);

	struct Data {
		real_t width = 1;
		real_t height = 1;
	} data;

protected:
	static void _bind_methods();

public:
	virtual Type get_obstacle_type() const { return Type::BOX; }

	void set_width(real_t p_width);
	real_t get_width() const { return data.width; }

	void set_height(real_t p_height);
	real_t get_height() const { return data.height; }
};

class NPMeshInstance;

class NPObstacleInstance : public Spatial {
	GDCLASS(NPObstacleInstance, Spatial);

	struct Data {
		Ref<NPObstacle> obstacle;
		np_handle h_obstacle;

		NPMeshInstance *parent = nullptr;
	} data;

	void _resource_changed();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_obstacle(const Ref<NPObstacle> &p_obstacle);
	Ref<NPObstacle> get_obstacle() const;

	np_handle get_handle_obstacle() const { return data.h_obstacle; }

	NPObstacleInstance();
	~NPObstacleInstance();
};
