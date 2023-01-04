
#pragma once

#include "core/math/camera_matrix.h"
#include "core/self_list.h"
#include "scene/resources/mesh.h"
#include "servers/visual/rasterizer.h"
#include "servers/visual_server.h"

#include "rasterizer_canvas_bgfx.h"
#include "rasterizer_scene_bgfx.h"
#include "rasterizer_storage_bgfx.h"

class RasterizerBGFX : public Rasterizer {
protected:
	RasterizerSceneBGFX scene;
	RasterizerCanvasBGFX canvas;
	RasterizerStorageBGFX storage;

	double time_total = 0.0;
	float time_scale = 0.0;

public:
	RasterizerStorage *get_storage() { return &storage; }
	RasterizerCanvas *get_canvas() { return &canvas; }
	RasterizerScene *get_scene() { return &scene; }

	void set_boot_image(const Ref<Image> &p_image, const Color &p_color, bool p_scale, bool p_use_filter = true) {}
	void set_shader_time_scale(float p_scale) {}

	void initialize();
	void begin_frame(double frame_step);
	void set_current_render_target(RID p_render_target);
	void restore_render_target(bool p_3d_was_drawn);
	void clear_render_target(const Color &p_color);
	void blit_render_target_to_screen(RID p_render_target, const Rect2 &p_screen_rect, int p_screen = 0) {}
	void output_lens_distorted_to_screen(RID p_render_target, const Rect2 &p_screen_rect, float p_k1, float p_k2, const Vector2 &p_eye_center, float p_oversample) {}
	void end_frame(bool p_swap_buffers);
	void finalize();

	static Error is_viable() {
		return OK;
	}

	static Rasterizer *_create_current() {
		return memnew(RasterizerBGFX);
	}

	static void make_current() {
		_create_func = _create_current;
	}

	virtual bool is_low_end() const { return true; }

	RasterizerBGFX();
	~RasterizerBGFX();
};
