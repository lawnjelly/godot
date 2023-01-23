#pragma once

#include "core/color.h"
#include "core/math/camera_matrix.h"
#include "mvp.h"
#include "thirdparty/bgfx/bgfx/include/bgfx/bgfx.h"
#include "thirdparty/bgfx/bgfx/include/bgfx/platform.h"

#define BGFX_DESTROY(A)          \
	if (bgfx::isValid(A)) {      \
		bgfx::destroy(A);        \
		A = BGFX_INVALID_HANDLE; \
	}

namespace BGFX {

bgfx::ShaderHandle loadShader(const char *FILENAME);
bgfx::ShaderHandle loadShaderOld(const char *FILENAME);

const bgfx::Memory *loadFile(String p_filename);

#pragma pack(push, 1)
struct PosColorVertex {
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

struct PosUVNormVertex {
	float x = 0;
	float y = 0;
	float z = 0;
	float u = 0;
	float v = 0;
	uint8_t nx = 0;
	uint8_t ny = 0;
	uint8_t nz = 0;
	uint8_t na = 0;
	static bgfx::VertexLayout layout;
	static void init() {
		layout.begin()
				.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
				.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
				.add(bgfx::Attrib::Normal, 4, bgfx::AttribType::Uint8, true)
				.end();
	}
};

#pragma pack(pop)

class Scene {
	friend class IBL;
	//	bgfx::ShaderHandle scene_vertex_shader = BGFX_INVALID_HANDLE;
	//	bgfx::ShaderHandle scene_fragment_shader = BGFX_INVALID_HANDLE;
	bgfx::ProgramHandle scene_program = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle scene_uniform_sampler_tex = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle scene_uniform_modulate = BGFX_INVALID_HANDLE;
	bgfx::TextureHandle scene_current_texture = BGFX_INVALID_HANDLE;
	bgfx::ViewId scene_view_id = UINT16_MAX;

	MVP _mvp;
	Color _modulate;
	bool _modulate_dirty = true;

	void _refresh_modulate();

public:
	void draw(const Transform &p_model_xform, bgfx::VertexBufferHandle p_vb, bgfx::IndexBufferHandle p_ib, int p_primitive_type);
	void set_texture(bgfx::TextureHandle p_tex_handle);
	void set_modulate(const Color &p_color);
	void set_view_transform(const CameraMatrix &p_projection, const Transform &p_camera_view);
	void prepare(bgfx::ViewId p_view_id);
	void prepare_scene(int p_viewport_width, int p_viewport_height);

	void create();
	void destroy();
};

extern Scene scene;
extern bool reassociate_framebuffers;

void transpose_mat16(float *m);
void camera_matrix_to_mat16(const CameraMatrix &cm, float *mat);
void transform_to_mat16(const Transform tr, float *mat);

} //namespace BGFX
