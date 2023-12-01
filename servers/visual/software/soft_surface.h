#pragma once

#include "core/image.h"

class SoftSurface {
	struct Data {
		RID texture;
		Ref<Image> image;
	} data;

public:
	void create(uint32_t p_width, uint32_t p_height);
	void update();

	SoftSurface();
	~SoftSurface();
};
