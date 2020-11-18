/*************************************************************************/
/*  context_gl_x11.h                                                     */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef CONTEXT_GL_X11_H
#define CONTEXT_GL_X11_H

#include "temp_gl_defines.h"

#ifdef X11_ENABLED

#if defined(OPENGL_ENABLED)

#include "core/os/os.h"
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include "servers/display_server.h"
#include "core/templates/local_vector.h"

struct ContextGL_X11_Private;

class ContextGL_X11 {
public:
	enum ContextType {
		GLES_2_0_COMPATIBLE,
	};

private:
	// any data specific to the window
	struct GLWindow
	{
		GLWindow() {in_use = false;}
		
		bool in_use;
		
		// the external ID
		DisplayServer::WindowID window_id;
		int width;
		int height;
		::Window window;
		::Window x11_window;
		int gldisplay_id;
	};
	
	struct GLDisplay
	{
		GLDisplay() {context = nullptr;}
		~GLDisplay();
		ContextGL_X11_Private *context;
		::Display *x11_display;
		XVisualInfo x_vi;
		XSetWindowAttributes x_swa;
		unsigned long x_valuemask;
	};
	
	LocalVector<GLWindow> _windows;
	LocalVector<GLDisplay> _displays;
	
	GLWindow &get_window(unsigned int id) {return _windows[id];}
	const GLWindow &get_window(unsigned int id) const {return _windows[id];}
	
	//ContextGL_X11_Private *p;
    //OS::VideoMode default_video_mode;
//	::Display *x11_display;
//	::Window &x11_window;
	bool double_buffer;
	bool direct_render;
	int glx_minor, glx_major;
	bool use_vsync;
	ContextType context_type;
	
private:
	int _find_or_create_display(Display *p_x11_display);
	Error _create_context(GLDisplay &gl_display);
public:
	Error window_create(DisplayServer::WindowID p_window_id, ::Window p_window, Display *p_display, int p_width, int p_height);
	void window_resize(DisplayServer::WindowID p_window_id, int p_width, int p_height);
	int window_get_width(DisplayServer::WindowID p_window = 0);
	int window_get_height(DisplayServer::WindowID p_window = 0);
	void window_destroy(DisplayServer::WindowID p_window_id);

	void release_current();
	void make_current();
	void swap_buffers();
	int get_window_width();
	int get_window_height();

	Error initialize();

	void set_use_vsync(bool p_use);
	bool is_using_vsync() const;

//	ContextGL_X11(::Display *p_x11_display, ::Window &p_x11_window, const OS::VideoMode &p_default_video_mode, ContextType p_context_type);
//	ContextGL_X11(::Display *p_x11_display, const Vector2i &p_size, ContextType p_context_type);
	ContextGL_X11(const Vector2i &p_size, ContextType p_context_type);
	~ContextGL_X11();
};

#endif

#endif
#endif
