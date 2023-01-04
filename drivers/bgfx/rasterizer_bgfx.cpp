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

RasterizerBGFX::RasterizerBGFX() {
	canvas.storage = &storage;
	scene.storage = &storage;

	storage.scene = &scene;
}

RasterizerBGFX::~RasterizerBGFX() {
}
