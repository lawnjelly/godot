#include "soft_surface.h"
#include "servers/visual/visual_server_raster.h"
#include "servers/visual_server.h"

void SoftSurface::create(uint32_t p_width, uint32_t p_height) {
	data.width = p_width;
	data.height = p_height;

	data.image->create(p_width, p_height, false, Image::FORMAT_RGBA8);
	data.zbuffer.resize(p_width * p_height);
	data.zbuffer.fill(0);

	data.gbuffer.resize(p_width * p_height);
	GData g;
	g.blank();
	data.gbuffer.fill(g);

	if (data.texture[data.write_texture_id].is_valid() && !data.texture_allocated) {
		for (uint32_t n = 0; n < SOFT_REND_NUM_BUFFERS; n++) {
			VisualServer::get_singleton()->texture_allocate(data.texture[n], p_width, p_height, 0, data.image->get_format(), VS::TEXTURE_TYPE_2D, 0);
		}
		data.texture_allocated = true;
	}
	data.write_texture_id = 0;
	data.read_texture_id = 1 % SOFT_REND_NUM_BUFFERS;
}

void SoftSurface::clear() {
	//data.image->fill(Color(0.15, 0.3, 0.7, 1));
	data.zbuffer.fill(2.0f);
	GData g;
	g.blank();
	data.gbuffer.fill(g);
}

void SoftSurface::update() {
	if (data.texture[data.write_texture_id].is_valid() && data.texture_allocated) {
		VisualServer::get_singleton()->texture_set_data(data.texture[data.write_texture_id], data.image);
		VisualServerRaster::undo_redraw_request();
	}

	data.read_texture_id = (data.read_texture_id + 1) % SOFT_REND_NUM_BUFFERS;
	data.write_texture_id = (data.write_texture_id + 1) % SOFT_REND_NUM_BUFFERS;
}

SoftSurface::SoftSurface() {
	data.image = Ref<Image>(memnew(Image));
	for (uint32_t n = 0; n < SOFT_REND_NUM_BUFFERS; n++) {
		data.texture[n] = RID_PRIME(VisualServer::get_singleton()->texture_create());
	}
}

SoftSurface::~SoftSurface() {
	for (uint32_t n = 0; n < SOFT_REND_NUM_BUFFERS; n++) {
		VisualServer::get_singleton()->free(data.texture[n]);
	}
}
