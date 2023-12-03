#include "soft_material.h"
#include "core/image.h"
#include "scene/resources/material.h"
#include "servers/visual/visual_server_globals.h"

void SoftTexture::create(Image *p_image) {
	_width = p_image->get_width();
	_height = p_image->get_height();
	_num_pixels = _width * _height;

	_pixels.resize(_num_pixels);

	p_image->lock();
	for (uint32_t y = 0; y < _height; y++) {
		for (uint32_t x = 0; x < _width; x++) {
			SoftRGBA &rgba = get(x, y);
			Color col = p_image->get_pixel(x, y);
			//col = Color(0, 0, 1, 1);
			//col = Color(1, 0, 0, 0);

			// The Godot color to_rgba32 seems to be the wrong way around?
			// Create an issue?
			rgba.rgba = col.to_abgr32();
			//col = Color(1, 0, 0, 0);
		}
	}
	p_image->unlock();
}

void SoftMaterial::create(RID p_mat) {
	godot_material = p_mat;
	texture_albedo = VSG::storage->material_get_param(godot_material, "texture_albedo");

	if (texture_albedo.is_valid()) {
		Ref<Image> image_albedo = VSG::storage->texture_get_data(texture_albedo);
		Ref<Image> temp = image_albedo->duplicate();
		temp->decompress();

		st_albedo.create(temp.ptr());
	}
}

uint32_t SoftMaterials::find_or_create_material(RID p_mat) {
	DEV_ASSERT(p_mat.is_valid());

	// Already present?
	for (uint32_t n = 0; n < materials.active_size(); n++) {
		uint32_t mat_id = materials.get_active_id(n);

		const SoftMaterial &mat = materials[mat_id];
		if (mat.godot_material == p_mat) {
			return mat_id;
		}
	}

	// Create
	uint32_t id;
	SoftMaterial *mat = materials.request(id);
	mat->create(p_mat);

	return id;
}
