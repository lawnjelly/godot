#pragma once

#include "mvp.h"
#include "rasterizer_storage_bgfx.h"

//class CanvasShaderBGFX : public RasterizerStorageBGFX::Shader {
class CanvasShaderBGFX {
	friend class BGFXDrawRect;

	bgfx::ViewId get_view_id() const { return data.current_view_id; }
	void _refresh_modulate();
	void _refresh_state();
	void _refresh_scissor();

public:
#pragma pack(push, 1)
	struct CanvasVertex {
		float x = 0;
		float y = 0;
		float z = 0;
		uint32_t abgr = UINT32_MAX;
		float u = 0;
		float v = 0;

		static bgfx::VertexLayout layout;
		static void init() {
			layout.begin()
					.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
					.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
					.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
					.end();
		}
	};
#pragma pack(pop)

	enum Uniforms {
		PROJECTION_MATRIX,
		MODELVIEW_MATRIX,
		EXTRA_MATRIX,
		SKELETON_TEXTURE_SIZE,
		SKELETON_TRANSFORM,
		SKELETON_TRANSFORM_INVERSE,
		FINAL_MODULATE,
		COLOR_TEXPIXEL_SIZE,
		DST_RECT,
		SRC_RECT,
		TIME,
		LIGHT_MATRIX,
		LIGHT_MATRIX_INVERSE,
		LIGHT_LOCAL_MATRIX,
		SHADOW_MATRIX,
		LIGHT_COLOR,
		LIGHT_SHADOW_COLOR,
		LIGHT_POS,
		SHADOWPIXEL_SIZE,
		SHADOW_GRADIENT,
		LIGHT_HEIGHT,
		LIGHT_OUTSIDE_ALPHA,
		SHADOW_DISTANCE_MULT,
		SCREEN_PIXEL_SIZE,
		USE_DEFAULT_NORMAL,
	};

	struct Data {
		//bgfx::DynamicVertexBufferHandle vbh = BGFX_INVALID_HANDLE;
		bgfx::IndexBufferHandle ibh = BGFX_INVALID_HANDLE;

		//		bgfx::ShaderHandle vsh = BGFX_INVALID_HANDLE;
		//		bgfx::ShaderHandle fsh = BGFX_INVALID_HANDLE;
		bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;

		bgfx::UniformHandle uniform_sampler_tex = BGFX_INVALID_HANDLE;
		bgfx::UniformHandle uniform_modulate = BGFX_INVALID_HANDLE;
		bgfx::TextureHandle current_texture = BGFX_INVALID_HANDLE;
		bgfx::TextureHandle white_texture = BGFX_INVALID_HANDLE;
		bool white_texture_bound = false;
		RasterizerStorageBGFX::Texture *current_storage_texture = nullptr;
		bgfx::ViewId current_view_id = UINT16_MAX;

		bool modulate_dirty = false;
		Color modulate;
		uint64_t state = 0;

		int viewport_width = 0;
		int viewport_height = 0;

		BGFX::MVP mvp;

		bool scissor_active = false;
		bool scissor_dirty = true;
		int scissor_x = 0;
		int scissor_y = 0;
		int scissor_width = 0;
		int scissor_height = 0;
	} data;

	bool bind() { return true; }
	void set_custom_shader(int id) {}
	void use_material(void *p_material) {}

	void set_uniform(Uniforms p_uniform, const Transform2D &p_transform) {}
	void set_uniform(Uniforms p_uniform, const CameraMatrix &p_matrix) {}

	//	void draw(const Transform2D &p_model_xform, bgfx::VertexBufferHandle p_vb, bgfx::IndexBufferHandle p_ib, int p_primitive_type);
	//	void set_texture(bgfx::TextureHandle p_tex_handle);
	//	void set_view_transform(const CameraMatrix &p_projection, const Transform &p_camera_view);
	void draw_rect(const Vector2 *p_points, const Vector2 *p_uvs, RasterizerStorageBGFX::Texture *p_texture, const Color *p_colors);
	void draw_polygon(const int16_t *p_indices, uint32_t p_index_count, const Vector2 *p_vertices, uint32_t p_vertex_count, const Color *p_colors, const Color *p_single_color);

	void set_view_transform(const CameraMatrix &p_projection_matrix);
	void set_model_transform(const Transform2D &p_view_matrix, const Transform2D &p_extra_matrix);
	void set_modulate(const Color &p_color);
	void set_blend_state(uint64_t p_state);
	void set_scissor(int p_x, int p_y, int p_width, int p_height);
	void set_scissor_disable();

	void prepare(bgfx::ViewId p_view_id, int p_viewport_width, int p_viewport_height);

	void create();
	void destroy();
	CanvasShaderBGFX() { create(); }
	~CanvasShaderBGFX() { destroy(); }
};
