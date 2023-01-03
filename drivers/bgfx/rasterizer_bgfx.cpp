#include "rasterizer_bgfx.h"

#include "core/os/os.h"

#include "thirdparty/bgfx/bgfx/include/bgfx/bgfx.h"
#include "thirdparty/bgfx/bgfx/include/bgfx/platform.h"

void RasterizerBGFX::initialize() {
}

void RasterizerBGFX::finalize() {
}

void RasterizerBGFX::end_frame(bool p_swap_buffers) {
	if (p_swap_buffers) {
		OS::get_singleton()->swap_buffers();
	}
}
