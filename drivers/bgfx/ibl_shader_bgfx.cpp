#include "ibl_shader_bgfx.h"
#include "bgfx_funcs.h"

namespace BGFX {

float IBL::s_texelHalf = 0.0f;
IBL ibl;

void IBL::prepare() {
}

void IBL::create() {
	bgfx::ShaderHandle scene_vertex_shader = loadShader("v_scene_ibl_mesh.bin");
	bgfx::ShaderHandle scene_fragment_shader = loadShader("f_scene_ibl_mesh.bin");
	data.program = bgfx::createProgram(scene_vertex_shader, scene_fragment_shader, true);
	data.uniform_params = bgfx::createUniform("u_params", bgfx::UniformType::Vec4, NUM_VEC_4);
	//data.uniform_sampler_tex = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
	//_uniform_modulate = bgfx::createUniform("u_modulate", bgfx::UniformType::Vec4);

	//			bgfx::setUniform(u_params, m_params, NumVec4);

	data.uniform_mtx = bgfx::createUniform("u_mtx", bgfx::UniformType::Mat4);
	data.uniform_params = bgfx::createUniform("u_params", bgfx::UniformType::Vec4);
	data.uniform_flags = bgfx::createUniform("u_flags", bgfx::UniformType::Vec4);
	data.uniform_camPos = bgfx::createUniform("u_camPos", bgfx::UniformType::Vec4);
	data.uniform_sampler_texCube = bgfx::createUniform("s_texCube", bgfx::UniformType::Sampler);
	data.uniform_sampler_texCubeIrr = bgfx::createUniform("s_texCubeIrr", bgfx::UniformType::Sampler);
}

void IBL::destroy() {
	BGFX_DESTROY(data.program);
	BGFX_DESTROY(data.uniform_params);
}

} //namespace BGFX
