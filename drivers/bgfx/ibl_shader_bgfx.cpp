#include "ibl_shader_bgfx.h"
#include "bgfx_funcs.h"
#include "core/image.h"
#include "core/io/resource_loader.h"

namespace BGFX {

float IBL::s_texelHalf = 0.0f;
IBL ibl;

void IBL::draw(const Transform &p_model_xform, bgfx::VertexBufferHandle p_vb, bgfx::IndexBufferHandle p_ib, int p_primitive_type) {
	if (!bgfx::isValid(p_vb) || !bgfx::isValid(p_ib)) {
		return;
	}

	ensure_loaded();

	//int frame = Engine::get_singleton()->get_frames_drawn();
	//print_line(itos(frame) + " tr " + p_view);
	scene._mvp.model = p_model_xform;
	scene._mvp.calc_model_view_proj();

	// try setting view and model separately?
#if 1
	float proj16[16];
	float view16[16];
	float model16[16];
	//	scene._mvp.view_proj
	BGFX::camera_matrix_to_mat16(scene._mvp.projection, proj16);
	BGFX::transform_to_mat16(scene._mvp.view, view16);
	BGFX::transform_to_mat16(scene._mvp.model, model16);
	bgfx::setViewTransform(scene.scene_view_id, view16, proj16);

	bgfx::setTransform(model16);

#else
	bgfx::setTransform(scene._mvp.model_view_proj16);
#endif

	bgfx::setVertexBuffer(0, p_vb);
	bgfx::setIndexBuffer(p_ib);

	if (bgfx::isValid(scene.scene_current_texture)) {
		//		bgfx::setTexture(0, scene_uniform_sampler_tex, scene.scene_current_texture);
	}

	bgfx::setState(0 | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CCW);

	moving_value += 0.01;
	if (moving_value > 1.0) {
		moving_value = 0.0;
	}

	_uniforms.m_glossiness = _settings.m_glossiness;
	_uniforms.m_glossiness = moving_value; //_settings.m_glossiness;
	_uniforms.m_reflectivity = _settings.m_reflectivity;
	//_uniforms.m_reflectivity = moving_value;// _settings.m_reflectivity;
	_uniforms.m_exposure = _settings.m_exposure;
	_uniforms.m_bgType = _settings.m_bgType;
	_uniforms.m_metalOrSpec = float(_settings.m_metalOrSpec);
	_uniforms.m_doDiffuse = float(_settings.m_doDiffuse);
	_uniforms.m_doSpecular = float(_settings.m_doSpecular);
	_uniforms.m_doDiffuseIbl = float(_settings.m_doDiffuseIbl);
	_uniforms.m_doSpecularIbl = float(_settings.m_doSpecularIbl);
	memcpy(_uniforms.m_rgbDiff, _settings.m_rgbDiff, 3 * sizeof(float));
	memcpy(_uniforms.m_rgbSpec, _settings.m_rgbSpec, 3 * sizeof(float));
	memcpy(_uniforms.m_lightDir, _settings.m_lightDir, 3 * sizeof(float));
	memcpy(_uniforms.m_lightCol, _settings.m_lightCol, 3 * sizeof(float));

	const Transform &cam = scene._mvp.camera;

	static_assert(sizeof(real_t) == 4, "Change this if real_t is 64 bit.");
	memcpy(_uniforms.m_cameraPos, &cam.origin, 3 * sizeof(float));

	// environmental rotation
	float env_rot16[16];
	Transform env_rot;
	env_rot.rotate(Vector3(0, 1, 0), env_angle);
	env_angle += 0.01f;
	BGFX::transform_to_mat16(env_rot, env_rot16);
	memcpy(_uniforms.m_mtx, env_rot16, 16 * sizeof(float)); // Used for IBL.

	// All uniforms as one
	bgfx::setUniform(data.uniform_params, _uniforms.m_params, NUM_VEC_4);

	// Cubemaps

	//	if (bgfx::isValid(scene.scene_current_texture)) {
	bgfx::setTexture(0, data.uniform_sampler_texCube, data.texture_radiance);
	bgfx::setTexture(1, data.uniform_sampler_texCubeIrr, data.texture_irradiance);
	//	}

	bgfx::submit(scene.scene_view_id, data.program);
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

	//Ref<Image> texture = ResourceLoader::load("res://Panoramas/bolonga_lod.dds");
	//Ref<Image> texture = ResourceLoader::load("res://Panoramas/bolonga_lod.dds");
}

void IBL::ensure_loaded() {
	if (!data.loaded) {
		data.loaded = true;

		const bgfx::Memory *rad_data = BGFX::loadFile("res://Panoramas/bolonga_lod.dds");
		if (rad_data) {
			data.texture_radiance = bgfx::createTexture(rad_data);
		} else {
			ERR_PRINT("Failed loading IBL radiance.");
		}
		const bgfx::Memory *irr_data = BGFX::loadFile("res://Panoramas/bolonga_irr.dds");
		if (irr_data) {
			data.texture_irradiance = bgfx::createTexture(irr_data);
		} else {
			ERR_PRINT("Failed loading IBL irradiance.");
		}

		//		Ref<Image> im = memnew(Image);
		//		Error err = im->load("res://Panoramas/rad_cubemap_bgr8.dds");

		//		if (err != OK) {
		//			print_line("failed loading");
		//		}
	}
}

void IBL::destroy() {
	BGFX_DESTROY(data.program);
	BGFX_DESTROY(data.uniform_params);
}

} //namespace BGFX
