#include "bgfx_funcs.h"
#include "core/engine.h"
#include "core/os/file_access.h"
#include "servers/visual_server.h"

namespace BGFX {

bgfx::VertexLayout PosColorVertex::layout;
Scene scene;
bool reassociate_framebuffers = false;

void transpose_mat16(float *m) {
	float t[16];
	t[0] = m[0];
	t[1] = m[4];
	t[2] = m[8];
	t[3] = m[12];
	t[4] = m[1];
	t[5] = m[5];
	t[6] = m[9];
	t[7] = m[13];
	t[8] = m[2];
	t[9] = m[6];
	t[10] = m[10];
	t[11] = m[14];
	t[12] = m[3];
	t[13] = m[7];
	t[14] = m[11];
	t[15] = m[15];
	memcpy(m, t, sizeof(float) * 16);
}

void camera_matrix_to_mat16(const CameraMatrix &cm, float *mat) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			mat[i * 4 + j] = cm.matrix[i][j];
		}
	}
}

void transform_to_mat16(const Transform tr, float *mat) {
	mat[0] = tr.basis.elements[0][0];
	mat[1] = tr.basis.elements[1][0];
	mat[2] = tr.basis.elements[2][0];
	mat[3] = 0;

	mat[4] = tr.basis.elements[0][1];
	mat[5] = tr.basis.elements[1][1];
	mat[6] = tr.basis.elements[2][1];
	mat[7] = 0;

	mat[8] = tr.basis.elements[0][2];
	mat[9] = tr.basis.elements[1][2];
	mat[10] = tr.basis.elements[2][2];
	mat[11] = 0;

	mat[12] = tr.origin.x;
	mat[13] = tr.origin.y;
	mat[14] = tr.origin.z;
	mat[15] = 1;
}

bgfx::ShaderHandle loadShader(const char *FILENAME) {
	//const char *shaderPath = "???";

	//	String filename = "/media/baron/Blue/Home/Pel/Godot/BGFX/bgfx/examples/runtime/shaders/";
	String filename = "/home/baron/Apps/Fork/shaders/compiled";

	switch (bgfx::getRendererType()) {
		case bgfx::RendererType::Noop:
		case bgfx::RendererType::Direct3D9:
			filename += "/dx9/";
			break;
		case bgfx::RendererType::Direct3D11:
		case bgfx::RendererType::Direct3D12:
			filename += "/dx11/";
			break;
		case bgfx::RendererType::Gnm:
			filename += "/pssl/";
			break;
		case bgfx::RendererType::Metal:
			filename += "/metal/";
			break;
		case bgfx::RendererType::OpenGL:
			filename += "/glsl/";
			break;
		case bgfx::RendererType::OpenGLES:
			filename += "/essl/";
			break;
		case bgfx::RendererType::Vulkan:
			filename += "/spirv/";
			break;
		default:
			break;
	}

	filename += FILENAME;

	//	size_t shaderLen = strlen(shaderPath);
	//	size_t fileLen = strlen(FILENAME);
	//	char *filePath = (char *)malloc(shaderLen + fileLen + 1);
	//	memcpy(filePath, shaderPath, shaderLen);
	//	memcpy(&filePath[shaderLen], FILENAME, fileLen);
	//	filePath[shaderLen + fileLen] = 0;
	//	const char * filePath = filename;

	//	FILE *file = fopen(filePath, "rb");
	//	fseek(file, 0, SEEK_END);
	//	long fileSize = ftell(file);
	//	fseek(file, 0, SEEK_SET);

	//	const bgfx::Memory *mem = bgfx::alloc(fileSize + 1);
	//	fread(mem->data, 1, fileSize, file);
	//	mem->data[mem->size - 1] = '\0';
	//	fclose(file);

	FileAccess *file = FileAccess::open(filename, FileAccess::READ);
	if (!file) {
		ERR_PRINT("Could not open " + filename);
	}
	CRASH_COND(!file);
	uint64_t len = file->get_len();
	const bgfx::Memory *mem = bgfx::alloc(len + 1);
	file->get_buffer((uint8_t *)mem->data, len);
	memdelete(file);

	return bgfx::createShader(mem);
}

bgfx::ShaderHandle loadShaderOld(const char *FILENAME) {
	//const char *shaderPath = "???";

	String filename = "/media/baron/Blue/Home/Pel/Godot/BGFX/bgfx/examples/runtime/";

	switch (bgfx::getRendererType()) {
		case bgfx::RendererType::Noop:
		case bgfx::RendererType::Direct3D9:
			filename += "shaders/dx9/";
			break;
		case bgfx::RendererType::Direct3D11:
		case bgfx::RendererType::Direct3D12:
			filename += "shaders/dx11/";
			break;
		case bgfx::RendererType::Gnm:
			filename += "shaders/pssl/";
			break;
		case bgfx::RendererType::Metal:
			filename += "shaders/metal/";
			break;
		case bgfx::RendererType::OpenGL:
			filename += "shaders/glsl/";
			break;
		case bgfx::RendererType::OpenGLES:
			filename += "shaders/essl/";
			break;
		case bgfx::RendererType::Vulkan:
			filename += "shaders/spirv/";
			break;
		default:
			break;
	}

	filename += FILENAME;

	//	size_t shaderLen = strlen(shaderPath);
	//	size_t fileLen = strlen(FILENAME);
	//	char *filePath = (char *)malloc(shaderLen + fileLen + 1);
	//	memcpy(filePath, shaderPath, shaderLen);
	//	memcpy(&filePath[shaderLen], FILENAME, fileLen);
	//	filePath[shaderLen + fileLen] = 0;
	//	const char * filePath = filename;

	//	FILE *file = fopen(filePath, "rb");
	//	fseek(file, 0, SEEK_END);
	//	long fileSize = ftell(file);
	//	fseek(file, 0, SEEK_SET);

	//	const bgfx::Memory *mem = bgfx::alloc(fileSize + 1);
	//	fread(mem->data, 1, fileSize, file);
	//	mem->data[mem->size - 1] = '\0';
	//	fclose(file);

	FileAccess *file = FileAccess::open(filename, FileAccess::READ);
	CRASH_COND(!file);
	uint64_t len = file->get_len();
	const bgfx::Memory *mem = bgfx::alloc(len + 1);
	file->get_buffer((uint8_t *)mem->data, len);
	memdelete(file);

	return bgfx::createShader(mem);
}

void Scene::set_texture(bgfx::TextureHandle p_tex_handle) {
	if (bgfx::isValid(p_tex_handle)) {
		scene_current_texture = p_tex_handle;
	}
}

void Scene::set_view_transform(const CameraMatrix &p_projection, const Transform &p_camera_view) {
	_mvp.projection = p_projection;
	_mvp.view = p_camera_view;
	_mvp.calc_view_proj();
}

void Scene::prepare(bgfx::ViewId p_view_id) {
	//	if ((Engine::get_singleton()->get_frames_drawn() % 2) == 1)
	//		p_view_id = 0;

	scene_view_id = p_view_id;
	//	bgfx::resetView(p_view_id);
}

void Scene::prepare_scene(int p_viewport_width, int p_viewport_height) {
	DEV_ASSERT(scene_view_id != UINT16_MAX);
	// Set view rectangle for 0th view
	bgfx::setViewRect(scene_view_id, 0, 0, uint16_t(p_viewport_width), uint16_t(p_viewport_height));

	// Clear the view rect
	//	bgfx::setViewClear(scene_view_id,
	//			BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
	//			0x00a000FF, 1.0f, 0);
}

void Scene::draw(const Transform &p_model_xform, bgfx::VertexBufferHandle p_vb, bgfx::IndexBufferHandle p_ib, int p_primitive_type) {
	DEV_ASSERT(scene_view_id != UINT16_MAX);

	if (!bgfx::isValid(p_vb) || !bgfx::isValid(p_ib)) {
		return;
	}

	//int frame = Engine::get_singleton()->get_frames_drawn();
	//print_line(itos(frame) + " tr " + p_view);
	_mvp.model = p_model_xform;
	_mvp.calc_model_view_proj();

	bgfx::setTransform(_mvp.model_view_proj16);

	bgfx::setVertexBuffer(0, p_vb);
	bgfx::setIndexBuffer(p_ib);

	switch (p_primitive_type) {
		case VS::PRIMITIVE_TRIANGLES: {
		} break;
		default: {
		} break;
	}

	if (bgfx::isValid(scene_current_texture)) {
		bgfx::setTexture(0, scene_uniform_sampler_tex, scene_current_texture);
	}
	bgfx::setState(0 | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CCW);
	//	bgfx::setState(0 | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);

	bgfx::submit(scene_view_id, scene_program);
	//bgfx::submit(scene_view_id, scene_program, BGFX_DISCARD_NONE);
}

} //namespace BGFX
