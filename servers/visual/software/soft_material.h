#pragma once

#include "core/color.h"
#include "core/math/vector2.h"
#include "core/pooled_list.h"
#include "core/rid.h"

class Material;
class Image;

struct SoftRGBA {
	union {
		uint32_t rgba;
		struct
		{
			uint8_t r, g, b, a;
		};
	};
};

class SoftTexture {
	LocalVector<SoftRGBA> _pixels;
	uint32_t _width = 0;
	uint32_t _height = 0;
	uint32_t _num_pixels = 0;

public:
	void create(Image *p_image);
	SoftRGBA &get(uint32_t x, uint32_t y) {
		DEV_ASSERT(((y * _width) + x) < _num_pixels);
		return _pixels[(y * _width) + x];
	}
	const SoftRGBA &get(uint32_t x, uint32_t y) const {
		DEV_ASSERT(((y * _width) + x) < _num_pixels);
		return _pixels[(y * _width) + x];
	}
	uint32_t get8(const Vector2 &p_uv) const {
		int32_t x = (p_uv.x * _width);
		int32_t y = (p_uv.y * _height);
		x %= _width;
		y %= _height;
		//x = CLAMP(x, 0, _width - 1);
		//y = CLAMP(y, 0, _height - 1);
		const SoftRGBA &rgba = get(x, y);
		return rgba.rgba;
	}
	Color get_color(const Vector2 &p_uv) const {
		int32_t x = (p_uv.x * _width);
		int32_t y = (p_uv.y * _height);
		x %= _width;
		y %= _height;
		const SoftRGBA &rgba = get(x, y);
		Color col;
		col.set_rgba32(rgba.rgba);
		return col;
	}
};

class SoftMaterial {
public:
	RID godot_material;

	RID texture_albedo;
	SoftTexture st_albedo;

	void create(RID p_mat);
};

class SoftMaterials {
	TrackedPooledList<SoftMaterial> materials;

public:
	uint32_t find_or_create_material(RID p_mat);
	const SoftMaterial &get(uint32_t p_id) const {
		return materials[p_id];
	}
};
