#include "rasterizer_bgfx.h"

#include "core/os/os.h"

#include "thirdparty/bgfx/bgfx/include/bgfx/bgfx.h"
#include "thirdparty/bgfx/bgfx/include/bgfx/platform.h"

void RasterizerBGFX::initialize() {
}

void RasterizerBGFX::finalize() {
}

void RasterizerBGFX::begin_frame(double frame_step) {
	time_total += frame_step * time_scale;

	if (frame_step == 0) {
		//to avoid hiccups
		frame_step = 0.001;
	}

	double time_roll_over = GLOBAL_GET("rendering/limits/time/time_rollover_secs");
	time_total = Math::fmod(time_total, time_roll_over);

	storage.frame.time[0] = time_total;
	storage.frame.time[1] = Math::fmod(time_total, 3600);
	storage.frame.time[2] = Math::fmod(time_total, 900);
	storage.frame.time[3] = Math::fmod(time_total, 60);
	storage.frame.count++;
	storage.frame.delta = frame_step;

	storage.update_dirty_resources();

	//	storage.info.render_final = storage->info.render;
	//	storage.info.render.reset();

	//	scene.iteration();
}

void RasterizerBGFX::end_frame(bool p_swap_buffers) {
	if (p_swap_buffers) {
		OS::get_singleton()->swap_buffers();
	}
}

void RasterizerBGFX::set_current_render_target(RID p_render_target) {
	if (!p_render_target.is_valid() && storage.frame.current_rt && storage.frame.clear_request) {
		// pending clear request. Do that first.
		//		glBindFramebuffer(GL_FRAMEBUFFER, storage->frame.current_rt->fbo);
		//		glClearColor(storage->frame.clear_request_color.r,
		//				storage->frame.clear_request_color.g,
		//				storage->frame.clear_request_color.b,
		//				storage->frame.clear_request_color.a);
		//		glClear(GL_COLOR_BUFFER_BIT);
	}

	if (p_render_target.is_valid()) {
		RasterizerStorageBGFX::RenderTarget *rt = storage.render_target_owner.getornull(p_render_target);
		storage.frame.current_rt = rt;
		ERR_FAIL_COND(!rt);
		storage.frame.clear_request = false;

		// keep drawrect up to date with view id
		canvas.state.draw_rect.set_current_view_id(rt->id_view);

		BGFX::scene.prepare(rt->id_view);

		//		glViewport(0, 0, rt->width, rt->height);
	} else {
		storage.frame.current_rt = nullptr;
		storage.frame.clear_request = false;
		//		glViewport(0, 0, OS::get_singleton()->get_window_size().width, OS::get_singleton()->get_window_size().height);
		//		glBindFramebuffer(GL_FRAMEBUFFER, RasterizerStorageGLES2::system_fbo);
	}
}

void RasterizerBGFX::restore_render_target(bool p_3d_was_drawn) {
	ERR_FAIL_COND(storage.frame.current_rt == nullptr);
	RasterizerStorageBGFX::RenderTarget *rt = storage.frame.current_rt;
	//	glBindFramebuffer(GL_FRAMEBUFFER, rt->fbo);
	//	glViewport(0, 0, rt->width, rt->height);
	storage._render_target_set_viewport(rt, 0, 0, rt->width, rt->height);
}

void RasterizerBGFX::clear_render_target(const Color &p_color) {
	ERR_FAIL_COND(!storage.frame.current_rt);

	storage.frame.clear_request = true;
	storage.frame.clear_request_color = p_color;

	storage._render_target_set_view_clear(storage.frame.current_rt, p_color);
}

void RasterizerBGFX::blit_render_target_to_screen(RID p_render_target, const Rect2 &p_screen_rect, int p_screen) {
	ERR_FAIL_COND(storage.frame.current_rt);

	RasterizerStorageBGFX::RenderTarget *rt = storage.render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	RasterizerStorageBGFX::Texture *texture = nullptr;
	if (rt->texture.is_valid()) {
		texture = storage.texture_owner.getornull(rt->texture);
	}
	ERR_FAIL_NULL(texture);
	ERR_FAIL_COND(!bgfx::isValid(texture->bg_handle));

	canvas._set_texture_rect_mode(true);

	canvas.state.canvas_shader.set_custom_shader(0);
	//	canvas->state.canvas_shader.set_conditional(CanvasShaderGLES2::LINEAR_TO_SRGB, rt->flags[RasterizerStorage::RENDER_TARGET_KEEP_3D_LINEAR]);
	canvas.state.canvas_shader.bind();

	// This is assuming the source render target dimensions match the screen,
	// which may be wrong, and is just to get working initially..
	// FIXME
	canvas.state.canvas_shader.prepare(0, rt->width, rt->height);
	storage._bgfx_view_order.push_back(0);

	//	canvas.state.canvas_shader.data.current_storage_texture = texture;
	//	canvas.state.canvas_shader.data.current_texture = texture->bg_handle;
	//	canvas.state.canvas_shader.data.white_texture_bound = false;
	//	bgfx::setTexture(0, canvas.state.canvas_shader.data.uniform_sampler_tex, canvas.state.canvas_shader.data.current_texture);

	/*
	canvas.canvas_begin();
	glDisable(GL_BLEND);
	glBindFramebuffer(GL_FRAMEBUFFER, RasterizerStorageGLES2::system_fbo);
	glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 1);
	if (rt->external.fbo != 0) {
		glBindTexture(GL_TEXTURE_2D, rt->external.color);
	} else {
		glBindTexture(GL_TEXTURE_2D, rt->color);
	}

	// TODO normals
*/

	canvas.draw_generic_textured_rect(texture, p_screen_rect, Rect2(0, 0, 1, -1));

	/*
	glBindTexture(GL_TEXTURE_2D, 0);
	canvas->canvas_end();

	canvas->state.canvas_shader.set_conditional(CanvasShaderGLES2::LINEAR_TO_SRGB, false);
	*/

	// send view order to BGFX (this might be done at a better place if blits will be called multiple times per frame)
	if (storage._bgfx_view_order.size()) {
		//		storage._bgfx_view_order.invert();
		//		bgfx::setViewOrder(0, storage._bgfx_view_order.size(), storage._bgfx_view_order.ptr());
	}

	storage._bgfx_view_order.clear();
}

RasterizerBGFX::RasterizerBGFX() {
	canvas.storage = &storage;
	scene.storage = &storage;

	storage.scene = &scene;
}

RasterizerBGFX::~RasterizerBGFX() {
}
