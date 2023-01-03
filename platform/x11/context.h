#pragma once

#include "core/os/os.h"
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

class Context {
public:
	enum ContextType {
		OLDSTYLE,
		GLES_2_0_COMPATIBLE,
		GLES_3_0_COMPATIBLE
	};

private:
	OS::VideoMode default_video_mode;
	::Display *x11_display;
	::Window &x11_window;
	bool double_buffer;
	bool direct_render;
	bool use_vsync;
	ContextType context_type;

public:
	void release_current();
	void make_current();
	void swap_buffers();
	int get_window_width();
	int get_window_height();
	void *get_glx_context();

	bool is_offscreen_available() const;
	void make_offscreen_current();
	void release_offscreen_current();

	Error initialize();

	void set_use_vsync(bool p_use);
	bool is_using_vsync() const;

	Context(::Display *p_x11_display, ::Window &p_x11_window, const OS::VideoMode &p_default_video_mode, ContextType p_context_type);
	~Context();
};
