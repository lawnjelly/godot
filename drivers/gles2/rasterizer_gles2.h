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
	
	void _blit_render_target_to_screen(RID p_render_target, const Rect2 &p_screen_rect, int p_screen = 0);
	
public:
	RasterizerStorage *get_storage() {return &storage;}
	RasterizerCanvas *get_canvas() {return &canvas;}
	RasterizerScene *get_scene() {return &scene;}
	
	void set_boot_image(const Ref<Image> &p_image, const Color &p_color, bool p_scale, bool p_use_filter = true) {}
	
	void initialize();// {}
	void begin_frame(double frame_step)
	{
		frame++;
		delta = frame_step;
	}
	
	void prepare_for_blitting_render_targets();
	void blit_render_targets_to_screen(int p_screen, const BlitToScreen *p_render_targets, int p_amount);
	
	void end_frame(bool p_swap_buffers);
//	{
//		if (p_swap_buffers) {
//			DisplayServer::get_singleton()->swap_buffers();
//		}
//	}
	
	void finalize() {}
	
	static Rasterizer *_create_current() {
		return memnew(RasterizerGLES2);
	}
	
	static void make_current() {
		_create_func = _create_current;
	}
	
	virtual bool is_low_end() const {return true;}
	uint64_t get_frame_number() const {return frame;}
	float get_frame_delta_time() const {return delta;}
	
	RasterizerGLES2();
	~RasterizerGLES2() {}
};


#ifdef GODOT_3

class RasterizerGLES2 : public Rasterizer {

	static Rasterizer *_create_current();

	RasterizerStorageGLES2 *storage;
	RasterizerCanvasGLES2 *canvas;
	RasterizerSceneGLES2 *scene;

	double time_total;
	float time_scale;

public:
	virtual RasterizerStorage *get_storage();
	virtual RasterizerCanvas *get_canvas();
	virtual RasterizerScene *get_scene();

	virtual void set_boot_image(const Ref<Image> &p_image, const Color &p_color, bool p_scale, bool p_use_filter = true);
	virtual void set_shader_time_scale(float p_scale);

	virtual void initialize();
	virtual void begin_frame(double frame_step);
	virtual void set_current_render_target(RID p_render_target);
	virtual void restore_render_target(bool p_3d_was_drawn);
	virtual void clear_render_target(const Color &p_color);
	virtual void blit_render_target_to_screen(RID p_render_target, const Rect2 &p_screen_rect, int p_screen = 0);
	virtual void output_lens_distorted_to_screen(RID p_render_target, const Rect2 &p_screen_rect, float p_k1, float p_k2, const Vector2 &p_eye_center, float p_oversample);
	virtual void end_frame(bool p_swap_buffers);
	virtual void finalize();

	static Error is_viable();
	static void make_current();
	static void register_config();

	virtual bool is_low_end() const { return true; }

	virtual const char *gl_check_for_error(bool p_print_error = true);

	RasterizerGLES2();
	~RasterizerGLES2();
};

#endif // godot 3

