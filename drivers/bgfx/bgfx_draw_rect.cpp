#include "bgfx_draw_rect.h"
#include "canvas_shader_bgfx.h"

#pragma pack(push, 1)
struct CanvasVertex {
	float x;
	float y;
	float z;
	float u;
	float v;
	uint32_t abgr;
};
#pragma pack(pop)

static_assert(sizeof(CanvasVertex) == 24, "Vertex size incorrect");

//static CanvasVertex cubeVertices[] = {
//	{ -1.0f, 1.0f, 0.0f, 0xff000000, 0.0f, 0.0f },
//	{ 1.0f, 1.0f, 0.0f, 0xff0000ff, 0.0f, 0.0f },
//	{ -1.0f, -1.0f, 0.0f, 0xff00ff00, 0.0f, 0.0f },
//	{ 1.0f, -1.0f, 0.0f, 0xff00ffff, 0.0f, 0.0f },
//};

static CanvasVertex cubeVertices[] = {
	{ -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0xff000000 },
	{ 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0xff0000ff },
	{ -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0xff00ff00 },
	{ 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0xff00ffff },
};

static const uint16_t cubeTriList[] = {
	0,
	1,
	2,
	3,
};

void BGFXDrawRect::create() {
	bgfx::VertexLayout layout;
	layout.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.end();
	//	.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	//	data.vbh = bgfx::createVertexBuffer(bgfx::makeRef(cubeVertices, sizeof(cubeVertices)), pcvDecl);
	data.vbh = bgfx::createDynamicVertexBuffer(bgfx::makeRef(cubeVertices, sizeof(cubeVertices)), layout);
	data.ibh = bgfx::createIndexBuffer(bgfx::makeRef(cubeTriList, sizeof(cubeTriList)));
}

void BGFXDrawRect::destroy() {
}

void BGFXDrawRect::prepare() {
	return;
	Transform look_at;
	look_at.origin = Vector3(0, 0, -5);
	look_at = look_at.looking_at(Vector3(0, 0, 0), Vector3(0, 1, 0));

	CameraMatrix cm;
	cm.set_orthogonal(5, 1.0, 0, 100);

	float view[16];
	float proj[16];
	CanvasShaderBGFX::_transform_to_mat16(look_at, view);
	CanvasShaderBGFX::_camera_matrix_to_mat16(cm, proj);

	bgfx::setViewTransform(0, view, proj);
}

void BGFXDrawRect::draw_rect(CanvasShaderBGFX *p_shader, const Vector2 *p_points, const Vector2 *p_uvs, RasterizerStorageBGFX::Texture *p_texture) {
	return;
	//	static PosColorVertex cubeVertices[] = {
	//		{ -1.0f, 1.0f, 0.0f, 0xff000000 },

	for (int n = 0; n < 4; n++) {
		cubeVertices[n].x = p_points[n].x;
		cubeVertices[n].y = p_points[n].y;
	}

	//	if (p_uvs) {
	//		for (int n = 0; n < 4; n++) {
	//			cubeVertices[n].u = p_uvs[n].x;
	//			cubeVertices[n].v = p_uvs[n].y;
	//		}
	//	}

	bgfx::update(data.vbh, 0, bgfx::makeRef(cubeVertices, sizeof(cubeVertices)));

	/*
	Transform look_at;
	look_at.origin = Vector3(0, 0, -5);
	look_at = look_at.looking_at(Vector3(0, 0, 0), Vector3(0, 1, 0));

	CameraMatrix cm;
	cm.set_orthogonal(5, 1.0, 0, 100);

	float view[16];
	float proj[16];
	_transform_to_mat16(look_at, view);
	_camera_matrix_to_mat16(cm, proj);

	bgfx::setViewTransform(0, view, proj);
	*/

	bgfx::setVertexBuffer(0, data.vbh);
	bgfx::setIndexBuffer(data.ibh);

	uint64_t state = 0
			//		| (m_r ? BGFX_STATE_WRITE_R : 0)
			//		| (m_g ? BGFX_STATE_WRITE_G : 0)
			//		| (m_b ? BGFX_STATE_WRITE_B : 0)
			//		| (m_a ? BGFX_STATE_WRITE_A : 0)
			| BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW | BGFX_STATE_MSAA | BGFX_STATE_PT_TRISTRIP;

	// Set render states.
	bgfx::setState(state);

	bgfx::submit(data.current_view_id, p_shader->data.program);
	//	bgfx::submit(0, p_shader->data.program);

	//bgfx::frame();
}
