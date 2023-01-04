#pragma once

#include "core/math/camera_matrix.h"
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
#pragma pack(pop)

class Scene {
public:
	//	bgfx::ShaderHandle scene_vertex_shader = BGFX_INVALID_HANDLE;
	//	bgfx::ShaderHandle scene_fragment_shader = BGFX_INVALID_HANDLE;
	bgfx::ProgramHandle scene_program = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle scene_uniform_sampler_tex = BGFX_INVALID_HANDLE;
	bgfx::TextureHandle scene_current_texture = BGFX_INVALID_HANDLE;

	void create() {
		PosColorVertex::init();
		//		bgfx::ShaderHandle scene_vertex_shader = loadShaderOld("vs_cubes.bin");
		//		bgfx::ShaderHandle scene_fragment_shader = loadShaderOld("fs_cubes.bin");
		bgfx::ShaderHandle scene_vertex_shader = loadShader("v_cubes.bin");
		bgfx::ShaderHandle scene_fragment_shader = loadShader("f_cubes.bin");
		scene_program = bgfx::createProgram(scene_vertex_shader, scene_fragment_shader, true);
		scene_uniform_sampler_tex = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
	}

	void draw(const Transform &p_model_xform, bgfx::VertexBufferHandle p_vb, bgfx::IndexBufferHandle p_ib, int p_primitive_type);
	void set_texture(bgfx::TextureHandle p_tex_handle);
	void set_view_transform(const CameraMatrix &p_projection, const Transform &p_camera_view);

	void destroy() {
		//		BGFX_DESTROY(scene_vertex_shader);
		//		BGFX_DESTROY(scene_fragment_shader);
		BGFX_DESTROY(scene_program);
	}
};

extern Scene scene;

void transpose_mat16(float *m);
void camera_matrix_to_mat16(CameraMatrix &cm, float *mat);
void transform_to_mat16(Transform tr, float *mat);

} //namespace BGFX
