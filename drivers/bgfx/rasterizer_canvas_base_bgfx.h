#pragma once

#include "bgfx_draw_rect.h"
#include "canvas_shader_bgfx.h"
#include "drivers/gles_common/rasterizer_array.h"
#include "drivers/gles_common/rasterizer_storage_common.h"
#include "rasterizer_storage_bgfx.h"
#include "servers/visual/rasterizer.h"

class RasterizerCanvasBaseBGFX : public RasterizerCanvas {
public:
	struct Uniforms {
		Transform projection_matrix;
		Transform2D modelview_matrix;
		Transform2D extra_matrix;
		Color final_modulate;
		float time;
	};

	struct State {
		Uniforms uniforms;
		bool canvas_texscreen_used = false;
		CanvasShaderBGFX canvas_shader;
		BGFXDrawRect draw_rect;

		bool using_texture_rect = false;

		bool using_light_angle = false;
		bool using_modulate = false;
		bool using_large_vertex = false;

		bool using_ninepatch = false;
		bool using_skeleton = false;

		Transform2D skeleton_transform;
		Transform2D skeleton_transform_inverse;
		Size2i skeleton_texture_size;

		RID current_tex;
		RID current_normal;
		RasterizerStorageBGFX::Texture *current_tex_ptr;

		Transform vp;
		Light *using_light;
		bool using_shadow = false;
		bool using_transparent_rt = false;

	} state;

	RasterizerStorageBGFX *storage = nullptr;

	void _set_uniforms(bool set_projection = false);

	virtual RID light_internal_create() { return RID(); }
	virtual void light_internal_update(RID p_rid, Light *p_light) {}
	virtual void light_internal_free(RID p_rid) {}

	virtual void canvas_begin();
	virtual void canvas_end();

	virtual void canvas_debug_viewport_shadows(Light *p_lights_with_shadow) {}

	virtual void canvas_light_shadow_buffer_update(RID p_buffer, const Transform2D &p_light_xform, int p_light_mask, float p_near, float p_far, LightOccluderInstance *p_occluders, CameraMatrix *p_xform_cache) {}

	virtual void reset_canvas() {}

	void _copy_texscreen(const Rect2 &p_rect);
	void _copy_screen(const Rect2 &p_rect);

	virtual void draw_window_margins(int *p_margins, RID *p_margin_textures) {}
	void _draw_gui_primitive(int p_points, const Vector2 *p_vertices, const Color *p_colors, const Vector2 *p_uvs, const float *p_light_angles = nullptr);
	void _draw_polygon(const int *p_indices, int p_index_count, int p_vertex_count, const Vector2 *p_vertices, const Vector2 *p_uvs, const Color *p_colors, bool p_singlecolor, const float *p_weights = nullptr, const int *p_bones = nullptr);

	RasterizerStorageBGFX::Texture *_bind_canvas_texture(const RID &p_texture, const RID &p_normal_map);
	void _set_texture_rect_mode(bool p_texture_rect, bool p_light_angle = false, bool p_modulate = false, bool p_large_vertex = false);

	RasterizerCanvasBaseBGFX();
	~RasterizerCanvasBaseBGFX();
};
