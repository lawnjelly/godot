#include "soft_surface.h"
#include "servers/visual_server.h"

void SoftSurface::create(uint32_t p_width, uint32_t p_height) {
	data.width = p_width;
	data.height = p_height;

	data.image->create(p_width, p_height, false, Image::FORMAT_RGBA8);
	data.zbuffer.resize(p_width * p_height);
	data.zbuffer.fill(0);

	if (data.texture.is_valid() && !data.texture_allocated) {
		VisualServer::get_singleton()->texture_allocate(data.texture, p_width, p_height, 0, data.image->get_format(), VS::TEXTURE_TYPE_2D, 0);
		data.texture_allocated = true;
	}
}

void SoftSurface::clear() {
	data.image->fill(Color(0.15, 0.3, 0.7, 1));
	data.zbuffer.fill(2.0f);
}

void SoftSurface::update() {
	if (data.texture.is_valid() && data.texture_allocated) {
		VisualServer::get_singleton()->texture_set_data(data.texture, data.image);
	}
}

SoftSurface::SoftSurface() {
	data.image = Ref<Image>(memnew(Image));
	data.texture = RID_PRIME(VisualServer::get_singleton()->texture_create());
}

SoftSurface::~SoftSurface() {
	VisualServer::get_singleton()->free(data.texture);
}
