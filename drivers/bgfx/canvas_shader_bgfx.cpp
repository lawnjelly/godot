#include "canvas_shader_bgfx.h"
#include "bgfx_funcs.h"

void CanvasShaderBGFX::_camera_matrix_to_mat16(const CameraMatrix &cm, float *mat) {
	for (int n = 0; n < 4; n++) {
		for (int m = 0; m < 4; m++) {
			mat[(n * 4) + m] = cm.matrix[n][m];
		}
	}
}

void CanvasShaderBGFX::_transform_to_mat16(const Transform &tr, float *mat) {
	mat[0] = tr.basis.elements[0].x;
	mat[1] = tr.basis.elements[0].y;
	mat[2] = tr.basis.elements[0].z;
	mat[3] = tr.origin.x;

	mat[4] = tr.basis.elements[1].x;
	mat[5] = tr.basis.elements[1].y;
	mat[6] = tr.basis.elements[1].z;
	mat[7] = tr.origin.y;

	mat[8] = tr.basis.elements[2].x;
	mat[9] = tr.basis.elements[2].y;
	mat[10] = tr.basis.elements[2].z;
	mat[11] = tr.origin.z;

	mat[12] = 0;
	mat[13] = 0;
	mat[14] = 0;
	mat[15] = 1;
}

void CanvasShaderBGFX::_transform2D_to_mat16(const Transform2D &tr, float *mat) {
	/*
	mat[0] = tr.basis.elements[0].x;
	mat[1] = tr.basis.elements[0].y;
	mat[2] = tr.basis.elements[0].z;
	mat[3] = tr.origin.x;

	mat[4] = tr.basis.elements[1].x;
	mat[5] = tr.basis.elements[1].y;
	mat[6] = tr.basis.elements[1].z;
	mat[7] = tr.origin.y;

	mat[8] = tr.basis.elements[2].x;
	mat[9] = tr.basis.elements[2].y;
	mat[10] = tr.basis.elements[2].z;
	mat[11] = tr.origin.z;

	mat[12] = 0;
	mat[13] = 0;
	mat[14] = 0;
	mat[15] = 1;
	*/
}

void CanvasShaderBGFX::_set_view_transform_2D(bgfx::ViewId p_view_id, const Transform2D &p_view_matrix, const CameraMatrix &p_projection_matrix, const Transform2D &p_extra_matrix) {
	float view[16];
	float proj[16];
	_transform2D_to_mat16(p_view_matrix, view);
	_camera_matrix_to_mat16(p_projection_matrix, proj);

	//	bgfx::setViewTransform(p_view_id, view, proj);
}

void CanvasShaderBGFX::create() {
	//	data.vsh = BGFX::loadShader("v_canvas.bin");
	//	data.fsh = BGFX::loadShader("f_canvas.bin");
	//	data.program = bgfx::createProgram(data.vsh, data.fsh, true);
}

void CanvasShaderBGFX::destroy() {
	BGFX_DESTROY(data.vsh);
	BGFX_DESTROY(data.fsh);
	BGFX_DESTROY(data.program);
}
