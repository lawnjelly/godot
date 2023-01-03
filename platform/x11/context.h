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

protected:
	OS::VideoMode default_video_mode;
	::Display *x11_display = nullptr;
	::Window *x11_window = nullptr;
	bool double_buffer = false;
	bool direct_render = false;
	bool use_vsync = false;
	ContextType context_type = OLDSTYLE;

	int _width = 640;
	int _height = 480;

	void create(::Display *p_x11_display, ::Window &p_x11_window, const OS::VideoMode &p_default_video_mode, ContextType p_context_type);
	void destroy();

public:
	virtual void release_current();
	virtual void make_current();
	virtual void swap_buffers();
	int get_window_width();
	int get_window_height();
	virtual void *get_glx_context() { return nullptr; }

	virtual bool is_offscreen_available() const;
	virtual void make_offscreen_current();
	virtual void release_offscreen_current();

	virtual Error initialize();

	virtual void set_use_vsync(bool p_use);
	virtual bool is_using_vsync() const;

	Context();
	virtual ~Context();
};
