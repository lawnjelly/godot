/*************************************************************************/
/*  context_gl_x11.cpp                                                   */
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

#include "context_gl_x11.h"

#ifdef X11_ENABLED
#if defined(OPENGL_ENABLED)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>
#include <GL/glxext.h>

#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092

typedef GLXContext (*GLXCREATECONTEXTATTRIBSARBPROC)(Display *, GLXFBConfig, GLXContext, Bool, const int *);

struct ContextGL_X11_Private {
	::GLXContext glx_context;
};


ContextGL_X11::GLDisplay::~GLDisplay()
{
	if (context)
	{
		//release_current();
		glXDestroyContext(x11_display, context->glx_context);
		memdelete(context);
		context = nullptr;
	}
}


static bool ctxErrorOccurred = false;
static int ctxErrorHandler(Display *dpy, XErrorEvent *ev) {
	ctxErrorOccurred = true;
	return 0;
}

static void set_class_hint(Display *p_display, Window p_window) {
	XClassHint *classHint;
	
	/* set the name and class hints for the window manager to use */
	classHint = XAllocClassHint();
	if (classHint) {
		classHint->res_name = (char *)"Godot_Engine";
		classHint->res_class = (char *)"Godot";
	}
	XSetClassHint(p_display, p_window, classHint);
	XFree(classHint);
}


int ContextGL_X11::_find_or_create_display(Display *p_x11_display)
{
	for (unsigned int n=0; n<_displays.size(); n++)
	{
		const GLDisplay &d = _displays[n];
		if (d.x11_display == p_x11_display)
			return n;
	}
	
	// create
	GLDisplay d_temp;
	d_temp.x11_display = p_x11_display;
	_displays.push_back(d_temp);
	int new_display_id = _displays.size()-1;
	
	// create context
	GLDisplay &d = _displays[new_display_id];
	
	d.context = memnew(ContextGL_X11_Private);;
	d.context->glx_context = 0;
	
	Error err = _create_context(d);
	return new_display_id;
}

Error ContextGL_X11::_create_context(GLDisplay &gl_display)
{
	// some aliases
	::Display *x11_display = gl_display.x11_display;
	
	const char *extensions = glXQueryExtensionsString(x11_display, DefaultScreen(x11_display));
	
	GLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = (GLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress((const GLubyte *)"glXCreateContextAttribsARB");
	
	ERR_FAIL_COND_V(!glXCreateContextAttribsARB, ERR_UNCONFIGURED);
	
	static int visual_attribs[] = {
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_DOUBLEBUFFER, true,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_DEPTH_SIZE, 24,
		None
	};
	
	static int visual_attribs_layered[] = {
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_DOUBLEBUFFER, true,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		GLX_ALPHA_SIZE, 8,
		GLX_DEPTH_SIZE, 24,
		None
	};
	
	int fbcount;
	GLXFBConfig fbconfig = 0;
	XVisualInfo *vi = nullptr;
	
//	XSetWindowAttributes swa;
	gl_display.x_swa.event_mask = StructureNotifyMask;
	gl_display.x_swa.border_pixel = 0;
	gl_display.x_valuemask = CWBorderPixel | CWColormap | CWEventMask;
	
	if (OS::get_singleton()->is_layered_allowed()) {
		GLXFBConfig *fbc = glXChooseFBConfig(x11_display, DefaultScreen(x11_display), visual_attribs_layered, &fbcount);
		ERR_FAIL_COND_V(!fbc, ERR_UNCONFIGURED);
		
		for (int i = 0; i < fbcount; i++) {
			vi = (XVisualInfo *)glXGetVisualFromFBConfig(x11_display, fbc[i]);
			if (!vi)
				continue;
			
			XRenderPictFormat *pict_format = XRenderFindVisualFormat(x11_display, vi->visual);
			if (!pict_format) {
				XFree(vi);
				vi = nullptr;
				continue;
			}
			
			fbconfig = fbc[i];
			if (pict_format->direct.alphaMask > 0) {
				break;
			}
		}
		XFree(fbc);
		
		ERR_FAIL_COND_V(!fbconfig, ERR_UNCONFIGURED);
		
		gl_display.x_swa.background_pixmap = None;
		gl_display.x_swa.background_pixel = 0;
		gl_display.x_swa.border_pixmap = None;
		gl_display.x_valuemask |= CWBackPixel;
		
	} else {
		GLXFBConfig *fbc = glXChooseFBConfig(x11_display, DefaultScreen(x11_display), visual_attribs, &fbcount);
		ERR_FAIL_COND_V(!fbc, ERR_UNCONFIGURED);
		
		vi = glXGetVisualFromFBConfig(x11_display, fbc[0]);
		
		fbconfig = fbc[0];
		XFree(fbc);
	}
	
	int (*oldHandler)(Display *, XErrorEvent *) = XSetErrorHandler(&ctxErrorHandler);
	
	switch (context_type) {
		case GLES_2_0_COMPATIBLE: {
			gl_display.context->glx_context = glXCreateNewContext(gl_display.x11_display, fbconfig, GLX_RGBA_TYPE, 0, true);
			ERR_FAIL_COND_V(!gl_display.context->glx_context, ERR_UNCONFIGURED);
		} break;
	}
	
	gl_display.x_swa.colormap = XCreateColormap(x11_display, RootWindow(x11_display, vi->screen), vi->visual, AllocNone);
	
	/*
	//	x11_window = XCreateWindow(x11_display, RootWindow(x11_display, vi->screen), 0, 0, OS::get_singleton()->get_video_mode().width, OS::get_singleton()->get_video_mode().height, 0, vi->depth, InputOutput, vi->visual, valuemask, &swa);
	x11_window = XCreateWindow(x11_display, RootWindow(x11_display, vi->screen), 0, 0, 320, 240, 0, vi->depth, InputOutput, vi->visual, valuemask, &swa);
	XStoreName(x11_display, x11_window, "Godot Engine");
	
	ERR_FAIL_COND_V(!x11_window, ERR_UNCONFIGURED);
	set_class_hint(x11_display, x11_window);
	XMapWindow(x11_display, x11_window);
	
	XSync(x11_display, False);
	XSetErrorHandler(oldHandler);
	
	glXMakeCurrent(x11_display, x11_window, p->glx_context);
*/
	
	XSync(x11_display, False);
	XSetErrorHandler(oldHandler);
	
	
	// make our own copy of the vi data
	// for later creating windows using this display
	if (vi)
	{
		gl_display.x_vi = *vi;
	}
	
	XFree(vi);
	
	return OK;
}



Error ContextGL_X11::window_create(DisplayServer::WindowID p_window_id, ::Window p_window, Display *p_display, int p_width, int p_height)
{
	// make sure vector is big enough...
	// we can mirror the external vector, it is simpler
	// to keep the IDs identical for fast lookup
	if (p_window_id >= (int) _windows.size())
	{
		_windows.resize(p_window_id+1);
	}

	GLWindow &win = _windows[p_window_id];
	win.in_use = true;
	win.window_id = p_window_id;
	win.width = p_width;
	win.height = p_height;
	win.window = p_window;
	win.gldisplay_id = _find_or_create_display(p_display);
	
	// the display could be invalid .. check NYI
	GLDisplay &gl_display = _displays[win.gldisplay_id];
	const XVisualInfo &vi = gl_display.x_vi;
	XSetWindowAttributes &swa = gl_display.x_swa;
	::Display * x11_display = gl_display.x11_display;
	::Window &x11_window = win.x11_window;
	
	// set our error handler	
	int (*oldHandler)(Display *, XErrorEvent *) = XSetErrorHandler(&ctxErrorHandler);
	
	x11_window = XCreateWindow(x11_display, RootWindow(x11_display, vi.screen), 0, 0, p_width, p_height, 0, vi.depth, InputOutput, vi.visual, gl_display.x_valuemask, &swa);
	XStoreName(x11_display, x11_window, "Godot Engine");
	
	ERR_FAIL_COND_V(!x11_window, ERR_UNCONFIGURED);
	set_class_hint(x11_display, x11_window);
	XMapWindow(x11_display, x11_window);
	
	XSync(x11_display, False);
	XSetErrorHandler(oldHandler);
	
	glXMakeCurrent(x11_display, x11_window, gl_display.context->glx_context);
	
	return OK;
}

void ContextGL_X11::window_resize(DisplayServer::WindowID p_window_id, int p_width, int p_height)
{

}

int ContextGL_X11::window_get_width(DisplayServer::WindowID p_window)
{
	return get_window(p_window).width;	
}

int ContextGL_X11::window_get_height(DisplayServer::WindowID p_window)
{
	return get_window(p_window).height;
}

void ContextGL_X11::window_destroy(DisplayServer::WindowID p_window_id)
{
	GLWindow &win = get_window(p_window_id);
	win.in_use = false;
	
	

}

void ContextGL_X11::release_current() {
//	glXMakeCurrent(x11_display, None, nullptr);
}

void ContextGL_X11::make_current() {
//	glXMakeCurrent(x11_display, x11_window, p->glx_context);
}

void ContextGL_X11::swap_buffers() {
//	glXSwapBuffers(x11_display, x11_window);
}


Error ContextGL_X11::initialize() {
	
	/*
	const char *extensions = glXQueryExtensionsString(x11_display, DefaultScreen(x11_display));

	GLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = (GLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress((const GLubyte *)"glXCreateContextAttribsARB");

	ERR_FAIL_COND_V(!glXCreateContextAttribsARB, ERR_UNCONFIGURED);

	static int visual_attribs[] = {
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_DOUBLEBUFFER, true,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_DEPTH_SIZE, 24,
		None
	};

	static int visual_attribs_layered[] = {
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_DOUBLEBUFFER, true,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		GLX_ALPHA_SIZE, 8,
		GLX_DEPTH_SIZE, 24,
		None
	};

	int fbcount;
	GLXFBConfig fbconfig = 0;
	XVisualInfo *vi = nullptr;

	XSetWindowAttributes swa;
	swa.event_mask = StructureNotifyMask;
	swa.border_pixel = 0;
	unsigned long valuemask = CWBorderPixel | CWColormap | CWEventMask;

	if (OS::get_singleton()->is_layered_allowed()) {
		GLXFBConfig *fbc = glXChooseFBConfig(x11_display, DefaultScreen(x11_display), visual_attribs_layered, &fbcount);
		ERR_FAIL_COND_V(!fbc, ERR_UNCONFIGURED);

		for (int i = 0; i < fbcount; i++) {
			vi = (XVisualInfo *)glXGetVisualFromFBConfig(x11_display, fbc[i]);
			if (!vi)
				continue;

			XRenderPictFormat *pict_format = XRenderFindVisualFormat(x11_display, vi->visual);
			if (!pict_format) {
				XFree(vi);
				vi = nullptr;
				continue;
			}

			fbconfig = fbc[i];
			if (pict_format->direct.alphaMask > 0) {
				break;
			}
		}
		XFree(fbc);
		ERR_FAIL_COND_V(!fbconfig, ERR_UNCONFIGURED);

		swa.background_pixmap = None;
		swa.background_pixel = 0;
		swa.border_pixmap = None;
		valuemask |= CWBackPixel;

	} else {
		GLXFBConfig *fbc = glXChooseFBConfig(x11_display, DefaultScreen(x11_display), visual_attribs, &fbcount);
		ERR_FAIL_COND_V(!fbc, ERR_UNCONFIGURED);

		vi = glXGetVisualFromFBConfig(x11_display, fbc[0]);

		fbconfig = fbc[0];
		XFree(fbc);
	}

	int (*oldHandler)(Display *, XErrorEvent *) = XSetErrorHandler(&ctxErrorHandler);

	switch (context_type) {
		case GLES_2_0_COMPATIBLE: {
			p->glx_context = glXCreateNewContext(x11_display, fbconfig, GLX_RGBA_TYPE, 0, true);
			ERR_FAIL_COND_V(!p->glx_context, ERR_UNCONFIGURED);
		} break;
	}

	swa.colormap = XCreateColormap(x11_display, RootWindow(x11_display, vi->screen), vi->visual, AllocNone);
//	x11_window = XCreateWindow(x11_display, RootWindow(x11_display, vi->screen), 0, 0, OS::get_singleton()->get_video_mode().width, OS::get_singleton()->get_video_mode().height, 0, vi->depth, InputOutput, vi->visual, valuemask, &swa);
	x11_window = XCreateWindow(x11_display, RootWindow(x11_display, vi->screen), 0, 0, 320, 240, 0, vi->depth, InputOutput, vi->visual, valuemask, &swa);
	XStoreName(x11_display, x11_window, "Godot Engine");

	ERR_FAIL_COND_V(!x11_window, ERR_UNCONFIGURED);
	set_class_hint(x11_display, x11_window);
	XMapWindow(x11_display, x11_window);

	XSync(x11_display, False);
	XSetErrorHandler(oldHandler);

	glXMakeCurrent(x11_display, x11_window, p->glx_context);

	XFree(vi);
*/
	return OK;
}

int ContextGL_X11::get_window_width() {
//	XWindowAttributes xwa;
//	XGetWindowAttributes(x11_display, x11_window, &xwa);

//	return xwa.width;
	return 0;
}

int ContextGL_X11::get_window_height() {
//	XWindowAttributes xwa;
//	XGetWindowAttributes(x11_display, x11_window, &xwa);

//	return xwa.height;
	return 0;
}

void ContextGL_X11::set_use_vsync(bool p_use) {
	/*
	static bool setup = false;
	static PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = nullptr;
	static PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalMESA = nullptr;
	static PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI = nullptr;

	if (!setup) {
		setup = true;
		String extensions = glXQueryExtensionsString(x11_display, DefaultScreen(x11_display));
		if (extensions.find("GLX_EXT_swap_control") != -1)
			glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddressARB((const GLubyte *)"glXSwapIntervalEXT");
		if (extensions.find("GLX_MESA_swap_control") != -1)
			glXSwapIntervalMESA = (PFNGLXSWAPINTERVALSGIPROC)glXGetProcAddressARB((const GLubyte *)"glXSwapIntervalMESA");
		if (extensions.find("GLX_SGI_swap_control") != -1)
			glXSwapIntervalSGI = (PFNGLXSWAPINTERVALSGIPROC)glXGetProcAddressARB((const GLubyte *)"glXSwapIntervalSGI");
	}
	int val = p_use ? 1 : 0;
	if (glXSwapIntervalMESA) {
		glXSwapIntervalMESA(val);
	} else if (glXSwapIntervalSGI) {
		glXSwapIntervalSGI(val);
	} else if (glXSwapIntervalEXT) {
		GLXDrawable drawable = glXGetCurrentDrawable();
		glXSwapIntervalEXT(x11_display, drawable, val);
	} else
		return;
	use_vsync = p_use;
	*/
}

bool ContextGL_X11::is_using_vsync() const {
	return use_vsync;
}

//ContextGL_X11::ContextGL_X11(::Display *p_x11_display, ::Window &p_x11_window, const OS::VideoMode &p_default_video_mode, ContextType p_context_type) :
//		x11_window(p_x11_window) {
//	ContextGL_X11::ContextGL_X11(::Display *p_x11_display, const Vector2i &p_size, ContextType p_context_type)  {
	ContextGL_X11::ContextGL_X11(const Vector2i &p_size, ContextType p_context_type)  {
	//default_video_mode = p_default_video_mode;
	//x11_display = p_x11_display;

	context_type = p_context_type;

	double_buffer = false;
	direct_render = false;
	glx_minor = glx_major = 0;
	//p = memnew(ContextGL_X11_Private);
	//p->glx_context = 0;
	use_vsync = false;
}

ContextGL_X11::~ContextGL_X11() {
	release_current();
//	glXDestroyContext(x11_display, p->glx_context);
//	memdelete(p);
}

#endif
#endif
