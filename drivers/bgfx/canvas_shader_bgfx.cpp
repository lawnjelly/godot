#include "canvas_shader_bgfx.h"
#include "bgfx_funcs.h"

bgfx::VertexLayout CanvasShaderBGFX::CanvasVertex::layout;

//static CanvasShaderBGFX::CanvasVertex canvasVertices[] = {
//	{ -10.0f, 10.0f, 0.0f, 0xff000000, 0.0f, 0.0f },
//	{ 10.0f, 10.0f, 0.0f, 0xff0000ff, 0.0f, 1.0f },
//	{ -10.0f, -10.0f, 0.0f, 0xff00ff00, 1.0f, 1.0f },
//	{ 10.0f, -10.0f, 0.0f, 0xff00ffff, 1.0f, 0.0f },
//};

static const uint16_t canvasTriList[] = {
	0,
	1,
	2,
	0,
	2,
	3,
};

void CanvasShaderBGFX::_refresh_modulate() {
	if (data.modulate_dirty) {
		data.modulate_dirty = false;
		bgfx::setUniform(data.uniform_modulate, &data.modulate);
	}
}

void CanvasShaderBGFX::set_modulate(const Color &p_color) {
	if (data.modulate != p_color) {
		data.modulate = p_color;
		data.modulate_dirty = true;
	}
}

void CanvasShaderBGFX::prepare(bgfx::ViewId p_view_id, int p_viewport_width, int p_viewport_height) {
	//	p_viewport_width = 640;
	//	p_viewport_height = 480;

	data.current_view_id = p_view_id;

	data.viewport_width = p_viewport_width;
	data.viewport_height = p_viewport_height;

	// Set view rectangle for 0th view
	bgfx::setViewRect(get_view_id(), 0, 0, uint16_t(p_viewport_width), uint16_t(p_viewport_height));
	set_scissor_disable();

	// Clear the view rect
	//	bgfx::setViewClear(0,
	//			BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
	//			0xFF00FF00, 1.0f, 0);

	bgfx::touch(get_view_id());

	data.current_storage_texture = nullptr;
	data.current_texture = BGFX_INVALID_HANDLE;
	data.white_texture_bound = false;
	data.modulate_dirty = true;
	data.modulate = Color(1, 1, 1, 1);

	//	bgfx::setTexture(0, data.uniform_sampler_tex, data.white_texture);

	//	bgfx::setState(0 | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);
	data.state = BGFX_STATE_WRITE_RGB;
	bgfx::setState(data.state);

	data.scissor_active = false;
	data.scissor_dirty = true;
}

void CanvasShaderBGFX::set_scissor(int p_x, int p_y, int p_width, int p_height) {
	// bgfx seems v flipped for scissor
	p_y = data.viewport_height - (p_y + p_height);

	data.scissor_dirty = true;
	data.scissor_active = true;
	data.scissor_x = p_x;
	data.scissor_y = p_y;
	data.scissor_width = p_width;
	data.scissor_height = p_height;

	//	bgfx::setScissor(p_x, p_y, p_width, p_height);
	//	bgfx::setViewRect(get_view_id(), p_x, p_y, p_width, p_height);
	//bgfx::setScissor(0, 0, uint16_t(data.viewport_width/2), uint16_t(data.viewport_height/2));
}

void CanvasShaderBGFX::set_scissor_disable() {
	data.scissor_dirty = true;
	data.scissor_active = false;
	//	bgfx::setScissor(0, 0, 0, 0);
	//	bgfx::setViewRect(get_view_id(), 0, 0, uint16_t(data.viewport_width), uint16_t(data.viewport_height));
	//	bgfx::setViewScissor(get_view_id(), 0, 0, uint16_t(data.viewport_width/2), uint16_t(data.viewport_height/2));
}

void CanvasShaderBGFX::_refresh_scissor() {
	//	if (data.scissor_dirty) {
	data.scissor_dirty = false;

	if (data.scissor_active) {
		bgfx::setScissor(data.scissor_x, data.scissor_y, data.scissor_width, data.scissor_height);
	} else {
		//			bgfx::setScissor(0, 0, data.viewport_width, data.viewport_height);
	}
	//	}
}

void CanvasShaderBGFX::_refresh_state() {
	bgfx::setState(data.state);
}

void CanvasShaderBGFX::set_blend_state(uint64_t p_state) {
	data.state &= ~BGFX_STATE_BLEND_MASK;
	data.state &= ~BGFX_STATE_BLEND_EQUATION_MASK;
	data.state |= p_state;
}

void CanvasShaderBGFX::set_view_transform(const CameraMatrix &p_projection_matrix) {
	float view[16];
	float proj[16];
	BGFX::transform_to_mat16(Transform(), view);
	BGFX::camera_matrix_to_mat16(p_projection_matrix, proj);

	bgfx::setViewTransform(get_view_id(), view, proj);
}

void CanvasShaderBGFX::set_model_transform(const Transform2D &p_view_matrix, const Transform2D &p_extra_matrix) {
	//	Transform2D m = p_view_matrix * p_extra_matrix;
	Transform2D m = p_extra_matrix * p_view_matrix;
	//	Transform2D m = p_view_matrix;

	Transform tr;
	tr.origin.x = m.get_origin().x;
	tr.origin.y = m.get_origin().y;

	//	tr = p_view_matrix;
	tr.basis.elements[0].x = m.elements[0].x;
	tr.basis.elements[1].x = m.elements[0].y;
	tr.basis.elements[0].y = m.elements[1].x;
	tr.basis.elements[1].y = m.elements[1].y;

	float view[16];
	BGFX::transform_to_mat16(tr, view);

	bgfx::setTransform(view);
}

//void CanvasShaderBGFX::_set_view_transform_2D(bgfx::ViewId p_view_id, const Transform2D &p_view_matrix, const CameraMatrix &p_projection_matrix, const Transform2D &p_extra_matrix) {
//	float view[16];
//	float proj[16];
//	_transform2D_to_mat16(p_view_matrix, view);
//	_camera_matrix_to_mat16(p_projection_matrix, proj);

//	//	bgfx::setViewTransform(p_view_id, view, proj);
//}

void CanvasShaderBGFX::draw_polygon(const int16_t *p_indices, uint32_t p_index_count, const Vector2 *p_vertices, uint32_t p_vertex_count, const Color *p_colors, const Color *p_single_color) {
	if (!bgfx::getAvailTransientVertexBuffer(p_vertex_count, CanvasVertex::layout)) {
		print_line("draw_polygon : out of transient vertex buffers");
		return;
	}

	if (!bgfx::getAvailTransientIndexBuffer(p_index_count)) {
		print_line("draw_polygon : out of transient index buffers");
		return;
	}

	bgfx::TransientVertexBuffer tvb;
	bgfx::allocTransientVertexBuffer(&tvb, p_vertex_count, CanvasVertex::layout);

	bgfx::TransientIndexBuffer tib;
	bgfx::allocTransientIndexBuffer(&tib, p_index_count);
	memcpy(tib.data, p_indices, p_index_count * sizeof(uint16_t));

	CanvasVertex *cvs = (CanvasVertex *)tvb.data;
	if (!cvs) {
		return;
	}

	if (p_single_color) {
		uint32_t col = UINT32_MAX;
		if (p_colors) {
			col = p_colors->to_abgr32();
		}

		for (uint32_t n = 0; n < p_vertex_count; n++) {
			CanvasVertex &cv = cvs[n];
			cv.x = p_vertices[n].x;
			cv.y = p_vertices[n].y;
			cv.abgr = col;
			cv.u = 0;
			cv.v = 0;
		}
	} else {
		for (uint32_t n = 0; n < p_vertex_count; n++) {
			CanvasVertex &cv = cvs[n];
			cv.x = p_vertices[n].x;
			cv.y = p_vertices[n].y;
			cv.abgr = p_colors[n].to_abgr32();
			cv.u = 0;
			cv.v = 0;
		}
	}

	bgfx::setVertexBuffer(0, &tvb);
	bgfx::setIndexBuffer(&tib, 0, p_index_count);

	_refresh_modulate();
	_refresh_state();
	_refresh_scissor();

	//	if (true) {
	if (!data.white_texture_bound) {
		data.white_texture_bound = true;
		data.current_storage_texture = nullptr;
		data.current_texture = data.white_texture;
		bgfx::setTexture(0, data.uniform_sampler_tex, data.current_texture);
	}

	bgfx::submit(get_view_id(), data.program);
}

void CanvasShaderBGFX::draw_rect(const Vector2 *p_points, const Vector2 *p_uvs, RasterizerStorageBGFX::Texture *p_texture, const Color *p_colors) {
	if (!bgfx::getAvailTransientVertexBuffer(4, CanvasVertex::layout)) {
		print_line("draw_rect : out of transient buffers");
		return;
	}

	bgfx::TransientVertexBuffer tb;
	bgfx::allocTransientVertexBuffer(&tb, 4, CanvasVertex::layout);

	CanvasVertex *cvs = (CanvasVertex *)tb.data;
	if (!cvs) {
		return;
	}

//#define GODOT_LOG_DRAW_RECT
#ifdef GODOT_LOG_DRAW_RECT
	String sz;
#endif

	uint32_t col = UINT32_MAX;
	if (p_colors) {
		col = p_colors[0].to_abgr32();
	}

	for (int n = 0; n < 4; n++) {
		CanvasVertex &cv = cvs[n];
		cv.x = p_points[n].x;
		cv.y = p_points[n].y;

		cv.abgr = col;
#ifdef GODOT_LOG_DRAW_RECT
		sz += String(Variant(p_points[n])) + ", ";
#endif
	}
#ifdef GODOT_LOG_DRAW_RECT
	print_line("draw_rect: " + sz);
#endif

	if (p_uvs) {
		for (int n = 0; n < 4; n++) {
			CanvasVertex &cv = cvs[n];
			cv.u = p_uvs[n].x;
			cv.v = p_uvs[n].y;
		}
	} else {
		cvs[0].u = 0;
		cvs[0].v = 0;
		cvs[1].u = 0;
		cvs[1].v = 1;
		cvs[2].u = 1;
		cvs[2].v = 1;
		cvs[3].u = 1;
		cvs[3].v = 0;
	}

	//	bgfx::update(data.vbh, 0, bgfx::makeRef(canvasVertices, sizeof(canvasVertices)));

	bgfx::setVertexBuffer(0, &tb);
	//	bgfx::setVertexBuffer(0, data.vbh);
	bgfx::setIndexBuffer(data.ibh);

	if (data.current_storage_texture != p_texture) {
		//	if (p_texture->bg_handle != data.current_texture)
		//	{
		data.current_storage_texture = p_texture;
		if (p_texture) {
			data.current_texture = p_texture->bg_handle;
			data.white_texture_bound = false;
			bgfx::setTexture(0, data.uniform_sampler_tex, data.current_texture);
		} else {
			data.white_texture_bound = true;
			data.current_storage_texture = nullptr;
			data.current_texture = data.white_texture;
			bgfx::setTexture(0, data.uniform_sampler_tex, data.current_texture);
		}
	}

	_refresh_modulate();
	_refresh_state();
	_refresh_scissor();

	bgfx::submit(get_view_id(), data.program);
	//bgfx::submit(0, data.program, BGFX_DISCARD_NONE);
}

void CanvasShaderBGFX::create() {
	bgfx::ShaderHandle vsh = BGFX::loadShader("v_canvas.bin");
	bgfx::ShaderHandle fsh = BGFX::loadShader("f_canvas.bin");
	data.program = bgfx::createProgram(vsh, fsh, true);
	data.uniform_sampler_tex = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
	data.uniform_modulate = bgfx::createUniform("u_modulate", bgfx::UniformType::Vec4);

	CanvasVertex::init();

	//data.vbh = bgfx::createDynamicVertexBuffer(bgfx::makeRef(canvasVertices, sizeof(canvasVertices)), CanvasVertex::layout);
	data.ibh = bgfx::createIndexBuffer(bgfx::makeRef(canvasTriList, sizeof(canvasTriList)));

	// create a default white texture

	const bgfx::Memory *mem = bgfx::alloc(4);
	uint32_t white = UINT32_MAX;
	memcpy(mem->data, &white, 4);
	data.white_texture = bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8, 0, mem);
}

void CanvasShaderBGFX::destroy() {
	//	BGFX_DESTROY(data.vsh);
	//	BGFX_DESTROY(data.fsh);
	BGFX_DESTROY(data.program);
	BGFX_DESTROY(data.white_texture);
}
