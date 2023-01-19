#include "mvp.h"
#include "bgfx_funcs.h"
#include "thirdparty/bgfx/bgfx/include/bgfx/bgfx.h"

namespace BGFX {

void MVP::calc_view_proj() {
	view_proj = projection * CameraMatrix(view);
	//camera_matrix_to_mat16(view_proj, view_proj16);
}

void MVP::calc_model_view_proj() {
	model_view_proj = view_proj * CameraMatrix(model);
	camera_matrix_to_mat16(model_view_proj, model_view_proj16);
}

} //namespace BGFX
