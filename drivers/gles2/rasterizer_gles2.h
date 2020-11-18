#pragma once

#include "drivers/gles_common/rasterizer_version.h"
#include "rasterizer_canvas_gles2.h"
#include "rasterizer_scene_gles2.h"
#include "rasterizer_storage_gles2.h"
#include "servers/rendering/rasterizer.h"

class RasterizerGLES2 : public Rasterizer {
private:
	uint64_t frame = 1;
	float delta = 0;

protected:
	RasterizerCanvasGLES2 canvas;
	RasterizerStorageGLES2 storage;
	RasterizerSceneGLES2 scene;

	void _blit_render_target_to_screen(RID p_render_target, const Rect2 &p_screen_rect);

public:
	RasterizerStorage *get_storage() { return &storage; }
	RasterizerCanvas *get_canvas() { return &canvas; }
	RasterizerScene *get_scene() { return &scene; }

	void set_boot_image(const Ref<Image> &p_image, const Color &p_color, bool p_scale, bool p_use_filter = true) {}

	void initialize();
	void begin_frame(double frame_step) {
		frame++;
		delta = frame_step;
	}

	void prepare_for_blitting_render_targets();
	void blit_render_targets_to_screen(DisplayServer::WindowID p_screen, const BlitToScreen *p_render_targets, int p_amount);

	void end_frame(bool p_swap_buffers);

	void finalize() {}

	static Rasterizer *_create_current() {
		return memnew(RasterizerGLES2);
	}

	static void make_current() {
		_create_func = _create_current;
	}

	virtual bool is_low_end() const { return true; }
	uint64_t get_frame_number() const { return frame; }
	float get_frame_delta_time() const { return delta; }

	RasterizerGLES2();
	~RasterizerGLES2() {}
};
