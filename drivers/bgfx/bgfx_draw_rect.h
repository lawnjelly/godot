#pragma once

#include "thirdparty/bgfx/bgfx/include/bgfx/bgfx.h"

#include "core/math/camera_matrix.h"
#include "core/math/transform_2d.h"
#include "rasterizer_storage_bgfx.h"

class CanvasShaderBGFX;
//class Texture;

class BGFXDrawRect {
	struct Data {
		bgfx::DynamicVertexBufferHandle vbh;
		bgfx::IndexBufferHandle ibh;

		//		bgfx::ShaderHandle vsh;
		//		bgfx::ShaderHandle fsh;
		//		bgfx::ProgramHandle program;
		bgfx::ViewId current_view_id = 0;
	} data;

public:
	void create();
	void destroy();

	void prepare();
	void draw_rect(CanvasShaderBGFX *p_shader, const Vector2 *p_points, const Vector2 *p_uvs, RasterizerStorageBGFX::Texture *p_texture);
	void set_current_view_id(bgfx::ViewId p_current_view_id) { data.current_view_id = p_current_view_id; }
};
