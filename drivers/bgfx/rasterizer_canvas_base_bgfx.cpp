#include "rasterizer_canvas_base_bgfx.h"

#include "core/os/os.h"
#include "servers/visual/visual_server_raster.h"

void RasterizerCanvasBaseBGFX::_set_uniforms() {
	CanvasShaderBGFX::_set_view_transform_2D(0, state.uniforms.modelview_matrix, state.uniforms.projection_matrix, state.uniforms.extra_matrix);
}

void RasterizerCanvasBaseBGFX::canvas_begin() {
	state.using_transparent_rt = false;

	// always start with light_angle unset
	state.using_light_angle = false;
	state.using_large_vertex = false;
	state.using_modulate = false;

	if (storage->frame.clear_request) {
		//		glClearColor(storage->frame.clear_request_color.r,
		//				storage->frame.clear_request_color.g,
		//				storage->frame.clear_request_color.b,
		//				state.using_transparent_rt ? storage->frame.clear_request_color.a : 1.0);
		//		glClear(GL_COLOR_BUFFER_BIT);
		storage->frame.clear_request = false;
	}

	reset_canvas();

	Transform canvas_transform;

	if (storage->frame.current_rt) {
		float csy = 1.0;
		if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_VFLIP]) {
			csy = -1.0;
		}
		canvas_transform.translate(-(storage->frame.current_rt->width / 2.0f), -(storage->frame.current_rt->height / 2.0f), 0.0f);
		canvas_transform.scale(Vector3(2.0f / storage->frame.current_rt->width, csy * -2.0f / storage->frame.current_rt->height, 1.0f));
	} else {
		Vector2 ssize = OS::get_singleton()->get_window_size();
		canvas_transform.translate(-(ssize.width / 2.0f), -(ssize.height / 2.0f), 0.0f);
		canvas_transform.scale(Vector3(2.0f / ssize.width, -2.0f / ssize.height, 1.0f));
	}

	state.uniforms.projection_matrix = canvas_transform;

	state.uniforms.final_modulate = Color(1, 1, 1, 1);

	state.uniforms.modelview_matrix = Transform2D();
	state.uniforms.extra_matrix = Transform2D();
	state.draw_rect.prepare();

	_set_uniforms();
}
void RasterizerCanvasBaseBGFX::canvas_end() {
	if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_DIRECT_TO_SCREEN]) {
		//reset viewport to full window size
		//		int viewport_width = OS::get_singleton()->get_window_size().width;
		//		int viewport_height = OS::get_singleton()->get_window_size().height;
		//		glViewport(0, 0, viewport_width, viewport_height);
		//		glScissor(0, 0, viewport_width, viewport_height);
	}

	state.using_texture_rect = false;
	state.using_skeleton = false;
	state.using_ninepatch = false;
	state.using_transparent_rt = false;
}

void RasterizerCanvasBaseBGFX::_copy_texscreen(const Rect2 &p_rect) {
}

void RasterizerCanvasBaseBGFX::_copy_screen(const Rect2 &p_rect) {
}

RasterizerStorageBGFX::Texture *RasterizerCanvasBaseBGFX::_bind_canvas_texture(const RID &p_texture, const RID &p_normal_map) {
	RasterizerStorageBGFX::Texture *tex_return = nullptr;

	if (p_texture.is_valid()) {
		RasterizerStorageBGFX::Texture *texture = storage->texture_owner.getornull(p_texture);

		if (!texture) {
			state.current_tex = RID();
			state.current_tex_ptr = nullptr;

			//			glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 1);
			//			glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);

		} else {
			if (texture->redraw_if_visible) {
				VisualServerRaster::redraw_request(false);
			}

			texture = texture->get_ptr();

			if (texture->render_target) {
				texture->render_target->used_in_frame = true;
			}

			//			glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 1);
			//			glBindTexture(GL_TEXTURE_2D, texture->tex_id);

			state.current_tex = p_texture;
			state.current_tex_ptr = texture;

			tex_return = texture;
		}
	} else {
		state.current_tex = RID();
		state.current_tex_ptr = nullptr;

		//		glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 1);
		//		glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);
	}

#if 0
	if (p_normal_map == state.current_normal) {
		//do none
		state.canvas_shader.set_uniform(CanvasShaderGLES2::USE_DEFAULT_NORMAL, state.current_normal.is_valid());

	} else if (p_normal_map.is_valid()) {
		RasterizerStorageGLES2::Texture *normal_map = storage->texture_owner.getornull(p_normal_map);

		if (!normal_map) {
			state.current_normal = RID();
			glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 2);
			glBindTexture(GL_TEXTURE_2D, storage->resources.normal_tex);
			state.canvas_shader.set_uniform(CanvasShaderGLES2::USE_DEFAULT_NORMAL, false);

		} else {
			if (normal_map->redraw_if_visible) { //check before proxy, because this is usually used with proxies
				VisualServerRaster::redraw_request(false);
			}

			normal_map = normal_map->get_ptr();

			glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 2);
			glBindTexture(GL_TEXTURE_2D, normal_map->tex_id);
			state.current_normal = p_normal_map;
			state.canvas_shader.set_uniform(CanvasShaderGLES2::USE_DEFAULT_NORMAL, true);
		}

	} else {
		state.current_normal = RID();
		glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 2);
		glBindTexture(GL_TEXTURE_2D, storage->resources.normal_tex);
		state.canvas_shader.set_uniform(CanvasShaderGLES2::USE_DEFAULT_NORMAL, false);
	}
#endif
	return tex_return;
}

void RasterizerCanvasBaseBGFX::_set_texture_rect_mode(bool p_texture_rect, bool p_light_angle, bool p_modulate, bool p_large_vertex) {
}

void RasterizerCanvasBaseBGFX::_draw_gui_primitive(int p_points, const Vector2 *p_vertices, const Color *p_colors, const Vector2 *p_uvs, const float *p_light_angles) {
	if (p_points == 4) {
		state.draw_rect.draw_rect(&state.canvas_shader, p_vertices, p_uvs, state.current_tex_ptr);
	}
}

RasterizerCanvasBaseBGFX::RasterizerCanvasBaseBGFX() {
	state.draw_rect.create();
}

RasterizerCanvasBaseBGFX::~RasterizerCanvasBaseBGFX() {
	state.draw_rect.destroy();
}
