#pragma once

#include "rasterizer_storage_bgfx.h"

//class CanvasShaderBGFX : public RasterizerStorageBGFX::Shader {
class CanvasShaderBGFX {
	friend class BGFXDrawRect;

	static void _camera_matrix_to_mat16(const CameraMatrix &cm, float *mat);
	static void _transform_to_mat16(const Transform &tr, float *mat);
	static void _transform2D_to_mat16(const Transform2D &tr, float *mat);

public:
	static void _set_view_transform_2D(bgfx::ViewId p_view_id, const Transform2D &p_view_matrix, const CameraMatrix &p_projection_matrix, const Transform2D &p_extra_matrix);

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
		bgfx::VertexBufferHandle vbh = BGFX_INVALID_HANDLE;
		bgfx::IndexBufferHandle ibh = BGFX_INVALID_HANDLE;

		bgfx::ShaderHandle vsh = BGFX_INVALID_HANDLE;
		bgfx::ShaderHandle fsh = BGFX_INVALID_HANDLE;
		bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
	} data;

	bool bind() { return true; }
	void set_custom_shader(int id) {}
	void use_material(void *p_material) {}

	void set_uniform(Uniforms p_uniform, const Transform2D &p_transform) {
		//		if (get_uniform(p_uniform) < 0)
		//			return;

		//		const Transform2D &tr = p_transform;

		//		GLfloat matrix[16] = { /* build a 16x16 matrix */
		//			tr.elements[0][0],
		//			tr.elements[0][1],
		//			0,
		//			0,
		//			tr.elements[1][0],
		//			tr.elements[1][1],
		//			0,
		//			0,
		//			0,
		//			0,
		//			1,
		//			0,
		//			tr.elements[2][0],
		//			tr.elements[2][1],
		//			0,
		//			1
		//		};

		//		glUniformMatrix4fv(get_uniform(p_uniform), 1, false, matrix);
	}

	void set_uniform(Uniforms p_uniform, const CameraMatrix &p_matrix) {
		//		if (get_uniform(p_uniform)<0) return;

		//		GLfloat matrix[16];

		//		for (int i=0;i<4;i++) {
		//			for (int j=0;j<4;j++) {

		//				matrix[i*4+j]=p_matrix.matrix[i][j];
		//			}
		//		}

		//		glUniformMatrix4fv(get_uniform(p_uniform),1,false,matrix);
	}

	void create();
	void destroy();
	CanvasShaderBGFX() { create(); }
	~CanvasShaderBGFX() { destroy(); }
};
