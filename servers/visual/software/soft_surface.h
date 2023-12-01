#pragma once

#include "core/image.h"

#define SOFT_REND_NUM_BUFFERS 1

class SoftRend;

class SoftSurface {
	friend class SoftRend;

	struct Data {
		RID texture[SOFT_REND_NUM_BUFFERS];
		bool texture_allocated = false;
		uint32_t write_texture_id = 0;
		uint32_t read_texture_id = 0;

		Ref<Image> image;
		uint32_t width = 0;
		uint32_t height = 0;
	} data;

public:
	void create(uint32_t p_width, uint32_t p_height);
	void update();
	void clear();
	Image *get_image() { return data.image.ptr(); }
	RID get_read_texture() const { return data.texture[data.read_texture_id]; }

	SoftSurface();
	~SoftSurface();
};
