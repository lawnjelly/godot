#pragma once

#include "core/resource.h"
#include "scene/3d/spatial.h"
#include "thirdparty/navphysics/navphysics_defines.h"

class NPShape : public Resource {
	GDCLASS(NPShape, Resource);

protected:
	//	static void _bind_methods();

public:
	enum class Type {
		UNSET,
		BOX,
	};

	virtual Type get_shape_type() const { return Type::UNSET; }
};

class NPShape_Box : public NPShape {
	GDCLASS(NPShape_Box, NPShape);

	struct Data {
		real_t width = 1;
		real_t height = 1;
	} data;

protected:
	static void _bind_methods();

public:
	virtual Type get_shape_type() const { return Type::BOX; }

	void set_width(real_t p_width);
	real_t get_width() const { return data.width; }

	void set_height(real_t p_height);
	real_t get_height() const { return data.height; }
};

class NPMeshInstance;

class NPBlock : public Spatial {
	GDCLASS(NPBlock, Spatial);

	struct Data {
		Ref<NPShape> shape;
		np_handle h_block;

		NPMeshInstance *parent = nullptr;
		bool enabled = true;
	} data;

	void _resource_changed();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_shape(const Ref<NPShape> &p_obstacle);
	Ref<NPShape> get_shape() const;

	void set_enabled(bool p_enable);
	bool is_enabled() const { return data.enabled; }

	Vector2 get_dimensions() const;

	np_handle get_handle_block() const { return data.h_block; }

	NPBlock();
	~NPBlock();
};
