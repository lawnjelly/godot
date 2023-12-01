#pragma once

#include "core/image.h"

class SoftRend;

class SoftSurface {
	friend class SoftRend;

	struct Data {
		RID texture;
		bool texture_allocated = false;

		Ref<Image> image;
		LocalVector<float> zbuffer;
		uint32_t width = 0;
		uint32_t height = 0;
	} data;

public:
	void create(uint32_t p_width, uint32_t p_height);
	void update();
	void clear();
	Image *get_image() { return data.image.ptr(); }
	RID get_texture() const { return data.texture; }

	float *get_z(uint32_t x, uint32_t y) {
		uint32_t id = (y * data.width) + x;
		if (id >= data.zbuffer.size()) {
			return nullptr;
		}
		return &data.zbuffer[id];
	}

	SoftSurface();
	~SoftSurface();
};
