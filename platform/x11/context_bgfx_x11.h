#pragma once

#include "context.h"

#include "core/os/os.h"
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

#include "thirdparty/bgfx/bgfx/include/bgfx/bgfx.h"
#include "thirdparty/bgfx/bgfx/include/bgfx/platform.h"

class ContextBGFX_X11 : public Context {
private:
	struct Data {
		bgfx::VertexBufferHandle vbh = BGFX_INVALID_HANDLE;
		bgfx::IndexBufferHandle ibh = BGFX_INVALID_HANDLE;

		bgfx::ShaderHandle vsh = BGFX_INVALID_HANDLE;
		bgfx::ShaderHandle fsh = BGFX_INVALID_HANDLE;
		bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
	} data;

	//	float _angle = 0.0;

public:
	void release_current();
	void make_current();
	void swap_buffers();

	bool is_offscreen_available() const;
	void make_offscreen_current();
	void release_offscreen_current();

	Error initialize();
	void test(int width, int height);

	void set_use_vsync(bool p_use);

protected:
public:
	ContextBGFX_X11(::Display *p_x11_display, ::Window &p_x11_window, const OS::VideoMode &p_default_video_mode, ContextType p_context_type);
	~ContextBGFX_X11();
};
