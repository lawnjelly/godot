#include "soft_surface.h"
#include "servers/visual_server.h"

void SoftSurface::create(uint32_t p_width, uint32_t p_height) {
	data.image->create(p_width, p_height, false, Image::FORMAT_RGBA8);
	VisualServer::get_singleton()->texture_allocate(data.texture, p_width, p_height, 0, data.image->get_format(), VS::TEXTURE_TYPE_2D, 0);
}

void SoftSurface::update() {
	VisualServer::get_singleton()->texture_set_data(data.texture, data.image);
}

SoftSurface::SoftSurface() {
	data.image = Ref<Image>(memnew(Image));
	data.texture = RID_PRIME(VisualServer::get_singleton()->texture_create());
}

SoftSurface::~SoftSurface() {
	VisualServer::get_singleton()->free(data.texture);
}
