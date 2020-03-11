#pragma once

#include "rasterizer_array.h"
#include "rasterizer_storage_gles2.h"
#include "servers/visual/rasterizer.h"

#include "shaders/canvas.glsl.gen.h"
#include "shaders/lens_distorted.glsl.gen.h"

#include "shaders/canvas_shadow.glsl.gen.h"


class RasterizerCanvasBaseGLES2 : public RasterizerCanvas {
public:
	enum {
		INSTANCE_ATTRIB_BASE = 8,
	};

	struct Uniforms {
		Transform projection_matrix;

		Transform2D modelview_matrix;
		Transform2D extra_matrix;

		Color final_modulate;

		float time;
	};

	struct Data {
		GLuint canvas_quad_vertices;
		GLuint polygon_buffer;
		GLuint polygon_index_buffer;

		uint32_t polygon_buffer_size;
		uint32_t polygon_index_buffer_size;

		GLuint ninepatch_vertices;
		GLuint ninepatch_elements;
	} data;

	struct State {
		Uniforms uniforms;
		bool canvas_texscreen_used;
		CanvasShaderGLES2 canvas_shader;
		CanvasShadowShaderGLES2 canvas_shadow_shader;
		LensDistortedShaderGLES2 lens_shader;

		bool using_texture_rect;
		bool using_ninepatch;
		bool using_skeleton;

		Transform2D skeleton_transform;
		Transform2D skeleton_transform_inverse;
		Size2i skeleton_texture_size;

		RID current_tex;
		RID current_normal;
		RasterizerStorageGLES2::Texture *current_tex_ptr;

		Transform vp;
		Light *using_light;
		bool using_shadow;
		bool using_transparent_rt;

	} state;

	typedef void Texture;

	RasterizerSceneGLES2 *scene_render;

	RasterizerStorageGLES2 *storage;

	bool use_nvidia_rect_workaround;


	void _set_uniforms();


	virtual void canvas_begin();
	virtual void canvas_end();

	void _draw_gui_primitive(int p_points, const Vector2 *p_vertices, const Color *p_colors, const Vector2 *p_uvs);
	void _draw_polygon(const int *p_indices, int p_index_count, int p_vertex_count, const Vector2 *p_vertices, const Vector2 *p_uvs, const Color *p_colors, bool p_singlecolor, const float *p_weights = NULL, const int *p_bones = NULL);
	void _draw_generic(GLuint p_primitive, int p_vertex_count, const Vector2 *p_vertices, const Vector2 *p_uvs, const Color *p_colors, bool p_singlecolor);
	void _draw_generic_indices(GLuint p_primitive, const int *p_indices, int p_index_count, int p_vertex_count, const Vector2 *p_vertices, const Vector2 *p_uvs, const Color *p_colors, bool p_singlecolor);


	void _bind_quad_buffer();
	void _copy_texscreen(const Rect2 &p_rect);
	void _copy_screen(const Rect2 &p_rect);

	virtual void draw_window_margins(int *black_margin, RID *black_image);
	void draw_generic_textured_rect(const Rect2 &p_rect, const Rect2 &p_src);
	void draw_lens_distortion_rect(const Rect2 &p_rect, float p_k1, float p_k2, const Vector2 &p_eye_center, float p_oversample);

	virtual void reset_canvas();
	virtual void canvas_light_shadow_buffer_update(RID p_buffer, const Transform2D &p_light_xform, int p_light_mask, float p_near, float p_far, LightOccluderInstance *p_occluders, CameraMatrix *p_xform_cache);
	virtual void canvas_debug_viewport_shadows(Light *p_lights_with_shadow);

	RasterizerStorageGLES2::Texture *_bind_canvas_texture(const RID &p_texture, const RID &p_normal_map);

};
