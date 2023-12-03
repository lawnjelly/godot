#pragma once

#include "core/image.h"

#define SOFT_REND_NUM_BUFFERS 1

class SoftRend;

class SoftSurface {
	friend class SoftRend;

	struct GData {
		//uint32_t tri_id_p1;
		uint32_t item_id_p1;
		Vector2 uv;
		void blank() {
			//tri_id_p1 = 0;
			item_id_p1 = 0;
			uv = Vector2();
		}
	};

	struct Data {
		RID texture[SOFT_REND_NUM_BUFFERS];
		bool texture_allocated = false;
		uint32_t write_texture_id = 0;
		uint32_t read_texture_id = 0;

		Ref<Image> image;
		LocalVector<float> zbuffer;
		LocalVector<GData> gbuffer;
		uint32_t width = 0;
		uint32_t height = 0;
	} data;

public:
	void create(uint32_t p_width, uint32_t p_height);
	void update();
	void clear();
	Image *get_image() { return data.image.ptr(); }
	RID get_read_texture() const { return data.texture[data.read_texture_id]; }

	const GData &get_g(uint32_t x, uint32_t y) const {
		uint32_t id = (y * data.width) + x;
		DEV_ASSERT(id < data.gbuffer.size());
		return data.gbuffer[id];
	}

	GData &get_g(uint32_t x, uint32_t y) {
		uint32_t id = (y * data.width) + x;
		DEV_ASSERT(id < data.gbuffer.size());
		return data.gbuffer[id];
	}

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
