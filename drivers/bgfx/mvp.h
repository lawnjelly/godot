#pragma once

#include "core/math/camera_matrix.h"

namespace BGFX {

// Model View Projection matrices,
// and combined to send as one uniform.
// This is done manually rather than relying on
// bgfx::setViewTransform, because BGFX can only
// handle one transform per view, and we need at least two
// (for canvas and scene).
struct MVP {
	// view before inverse
	Transform camera;

	CameraMatrix projection;
	Transform view;
	Transform model;

	// concatenated
	CameraMatrix view_proj;
	//float view_proj16[16];

	void calc_view_proj();
	void calc_model_view_proj();

	// concatenated
	CameraMatrix model_view_proj;
	float model_view_proj16[16];
};

} //namespace BGFX
