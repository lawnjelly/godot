#include "rasterizer_canvas_base_bgfx.h"

#include "core/os/os.h"
#include "servers/visual/visual_server_raster.h"

void RasterizerCanvasBaseBGFX::_set_uniforms(bool set_projection) {
	if (set_projection)
		state.canvas_shader.set_view_transform(state.uniforms.projection_matrix);

	state.canvas_shader.set_model_transform(state.uniforms.modelview_matrix, state.uniforms.extra_matrix);
	//	CanvasShaderBGFX::_set_view_transform_2D(0, state.uniforms.modelview_matrix, state.uniforms.projection_matrix, state.uniforms.extra_matrix);
	state.canvas_shader.set_modulate(state.uniforms.final_modulate);
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
	int fwidth = 0;
	int fheight = 0;

	bgfx::ViewId view_id = 0;

	if (storage->frame.current_rt) {
		fwidth = storage->frame.current_rt->width;
		fheight = storage->frame.current_rt->height;

		view_id = storage->frame.current_rt->id_view;

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
		fwidth = ssize.x;
		fheight = ssize.y;
	}

	state.uniforms.projection_matrix = canvas_transform;

	state.uniforms.final_modulate = Color(1, 1, 1, 1);

	state.uniforms.modelview_matrix = Transform2D();
	state.uniforms.extra_matrix = Transform2D();
	//	state.draw_rect.prepare();
	state.canvas_shader.prepare(view_id, fwidth, fheight);
	storage->_bgfx_view_order.push_back(view_id);

	_set_uniforms(true);
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

void RasterizerCanvasBaseBGFX::draw_generic_textured_rect(RasterizerStorageBGFX::Texture *p_texture, const Rect2 &p_rect, const Rect2 &p_src) {
	Rect2 r = p_rect;

	r.position.x *= 2.0 / state.canvas_shader.data.viewport_width;
	r.position.y *= 2.0 / state.canvas_shader.data.viewport_height;
	r.size.x *= 2.0 / state.canvas_shader.data.viewport_width;
	r.size.y *= 2.0 / state.canvas_shader.data.viewport_height;

	r.position -= Vector2(1, 1);

	Vector2 pts[4];
	pts[0] = r.position;
	pts[1] = r.position + Vector2(0, r.size.y);
	pts[2] = r.position + r.size;
	pts[3] = r.position + Vector2(r.size.x, 0);

	Rect2 src = p_src;
	src.size.y = -src.size.y;
	Vector2 uvs[4];
	uvs[0] = src.position;
	uvs[1] = src.position + Vector2(0, src.size.y);
	uvs[2] = src.position + src.size;
	uvs[3] = src.position + Vector2(src.size.x, 0);

	state.canvas_shader.draw_rect(pts, uvs, p_texture, nullptr);
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
		//state.draw_rect.draw_rect(&state.canvas_shader, p_vertices, p_uvs, state.current_tex_ptr);
		state.canvas_shader.draw_rect(p_vertices, p_uvs, state.current_tex_ptr, p_colors);
	}
}

void RasterizerCanvasBaseBGFX::_draw_polygon(const int *p_indices, int p_index_count, int p_vertex_count, const Vector2 *p_vertices, const Vector2 *p_uvs, const Color *p_colors, bool p_singlecolor, const float *p_weights, const int *p_bones) {
	ERR_FAIL_COND(!p_index_count);
	ERR_FAIL_COND(!p_vertex_count);

	// convert inds to 16 bit
	int16_t *inds = (int16_t *)alloca(p_index_count * 2);

	for (int n = 0; n < p_index_count; n++) {
		DEV_CHECK_ONCE((p_indices[n] >= 0) && (p_indices[n] < 65536));
		inds[n] = p_indices[n];
	}

	bool opaque = false;
	if (true) {
		if (!p_colors) {
			opaque = true;
		} else {
			if (p_singlecolor && p_colors->a >= 0.999f) {
				opaque = true;
			}
		}
	}

	uint64_t bs = 0;
	if (opaque) {
		bs = state.canvas_shader.get_blend_state();
		state.canvas_shader.set_blend_state();
	}
	state.canvas_shader.draw_polygon(inds, p_index_count, p_vertices, p_vertex_count, p_colors, p_singlecolor ? p_colors : nullptr);

	if (opaque) {
		state.canvas_shader.set_blend_state(bs);
	}

#if 0
	glBindBuffer(GL_ARRAY_BUFFER, data.polygon_buffer);

	uint32_t buffer_ofs = 0;
	uint32_t buffer_ofs_after = buffer_ofs + (sizeof(Vector2) * p_vertex_count);
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND(buffer_ofs_after > data.polygon_buffer_size);
#endif

	storage->buffer_orphan_and_upload(data.polygon_buffer_size, 0, sizeof(Vector2) * p_vertex_count, p_vertices, GL_ARRAY_BUFFER, _buffer_upload_usage_flag, true);

	glEnableVertexAttribArray(VS::ARRAY_VERTEX);
	glVertexAttribPointer(VS::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(Vector2), nullptr);
	buffer_ofs = buffer_ofs_after;

	if (p_singlecolor) {
		glDisableVertexAttribArray(VS::ARRAY_COLOR);
		Color m = *p_colors;
		glVertexAttrib4f(VS::ARRAY_COLOR, m.r, m.g, m.b, m.a);
	} else if (!p_colors) {
		glDisableVertexAttribArray(VS::ARRAY_COLOR);
		glVertexAttrib4f(VS::ARRAY_COLOR, 1, 1, 1, 1);
	} else {
		RAST_FAIL_COND(!storage->safe_buffer_sub_data(data.polygon_buffer_size, GL_ARRAY_BUFFER, buffer_ofs, sizeof(Color) * p_vertex_count, p_colors, buffer_ofs_after));
		glEnableVertexAttribArray(VS::ARRAY_COLOR);
		glVertexAttribPointer(VS::ARRAY_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(Color), CAST_INT_TO_UCHAR_PTR(buffer_ofs));
		buffer_ofs = buffer_ofs_after;
	}

	if (p_uvs) {
		RAST_FAIL_COND(!storage->safe_buffer_sub_data(data.polygon_buffer_size, GL_ARRAY_BUFFER, buffer_ofs, sizeof(Vector2) * p_vertex_count, p_uvs, buffer_ofs_after));
		glEnableVertexAttribArray(VS::ARRAY_TEX_UV);
		glVertexAttribPointer(VS::ARRAY_TEX_UV, 2, GL_FLOAT, GL_FALSE, sizeof(Vector2), CAST_INT_TO_UCHAR_PTR(buffer_ofs));
		buffer_ofs = buffer_ofs_after;
	} else {
		glDisableVertexAttribArray(VS::ARRAY_TEX_UV);
	}

	if (p_weights && p_bones) {
		RAST_FAIL_COND(!storage->safe_buffer_sub_data(data.polygon_buffer_size, GL_ARRAY_BUFFER, buffer_ofs, sizeof(float) * 4 * p_vertex_count, p_weights, buffer_ofs_after));
		glEnableVertexAttribArray(VS::ARRAY_WEIGHTS);
		glVertexAttribPointer(VS::ARRAY_WEIGHTS, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, CAST_INT_TO_UCHAR_PTR(buffer_ofs));
		buffer_ofs = buffer_ofs_after;

		RAST_FAIL_COND(!storage->safe_buffer_sub_data(data.polygon_buffer_size, GL_ARRAY_BUFFER, buffer_ofs, sizeof(int) * 4 * p_vertex_count, p_bones, buffer_ofs_after));
		glEnableVertexAttribArray(VS::ARRAY_BONES);
		glVertexAttribPointer(VS::ARRAY_BONES, 4, GL_UNSIGNED_INT, GL_FALSE, sizeof(int) * 4, CAST_INT_TO_UCHAR_PTR(buffer_ofs));
		buffer_ofs = buffer_ofs_after;

	} else {
		glDisableVertexAttribArray(VS::ARRAY_WEIGHTS);
		glDisableVertexAttribArray(VS::ARRAY_BONES);
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.polygon_index_buffer);

	if (storage->config.support_32_bits_indices) { //should check for
#ifdef DEBUG_ENABLED
		ERR_FAIL_COND((sizeof(int) * p_index_count) > data.polygon_index_buffer_size);
#endif
		storage->buffer_orphan_and_upload(data.polygon_index_buffer_size, 0, sizeof(int) * p_index_count, p_indices, GL_ELEMENT_ARRAY_BUFFER, _buffer_upload_usage_flag, true);
		glDrawElements(GL_TRIANGLES, p_index_count, GL_UNSIGNED_INT, nullptr);
		storage->info.render._2d_draw_call_count++;
	} else {
#ifdef DEBUG_ENABLED
		ERR_FAIL_COND((sizeof(uint16_t) * p_index_count) > data.polygon_index_buffer_size);
#endif
		uint16_t *index16 = (uint16_t *)alloca(sizeof(uint16_t) * p_index_count);
		for (int i = 0; i < p_index_count; i++) {
			index16[i] = uint16_t(p_indices[i]);
		}
		storage->buffer_orphan_and_upload(data.polygon_index_buffer_size, 0, sizeof(uint16_t) * p_index_count, index16, GL_ELEMENT_ARRAY_BUFFER, _buffer_upload_usage_flag, true);
		glDrawElements(GL_TRIANGLES, p_index_count, GL_UNSIGNED_SHORT, nullptr);
		storage->info.render._2d_draw_call_count++;
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#endif
}

RasterizerCanvasBaseBGFX::RasterizerCanvasBaseBGFX() {
	//state.draw_rect.create();
}

RasterizerCanvasBaseBGFX::~RasterizerCanvasBaseBGFX() {
	//state.draw_rect.destroy();
}
