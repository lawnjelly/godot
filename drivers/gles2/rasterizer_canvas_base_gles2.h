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


};
