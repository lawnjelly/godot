#include "context_bgfx_x11.h"
#include "core/math/camera_matrix.h"

//#define BX_CONFIG_DEBUG
//#include "thirdparty/bgfx/bx/include/bx/math.h"

//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h>

void _camera_matrix_to_mat16(CameraMatrix &cm, float *mat) {
	for (int n = 0; n < 4; n++) {
		for (int m = 0; m < 4; m++) {
			mat[(n * 4) + m] = cm.matrix[n][m];
		}
	}
}

void _transform_to_mat16(Transform &tr, float *mat) {
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

struct PosColorVertex {
	float x;
	float y;
	float z;
	uint32_t abgr;
};

static PosColorVertex cubeVertices[] = {
	{ -1.0f, 1.0f, 1.0f, 0xff000000 },
	{ 1.0f, 1.0f, 1.0f, 0xff0000ff },
	{ -1.0f, -1.0f, 1.0f, 0xff00ff00 },
	{ 1.0f, -1.0f, 1.0f, 0xff00ffff },
	{ -1.0f, 1.0f, -1.0f, 0xffff0000 },
	{ 1.0f, 1.0f, -1.0f, 0xffff00ff },
	{ -1.0f, -1.0f, -1.0f, 0xffffff00 },
	{ 1.0f, -1.0f, -1.0f, 0xffffffff },
};

static const uint16_t cubeTriList[] = {
	0,
	1,
	2,
	1,
	3,
	2,
	4,
	6,
	5,
	5,
	6,
	7,
	0,
	2,
	4,
	4,
	2,
	6,
	1,
	5,
	3,
	5,
	7,
	3,
	0,
	4,
	1,
	4,
	5,
	1,
	2,
	3,
	6,
	6,
	3,
	7,
};

bgfx::ShaderHandle loadShader(const char *FILENAME) {
	//const char *shaderPath = "???";

	String filename = "/media/baron/Blue/Home/Pel/Godot/BGFX/bgfx/examples/runtime/";

	switch (bgfx::getRendererType()) {
		case bgfx::RendererType::Noop:
		case bgfx::RendererType::Direct3D9:
			filename += "shaders/dx9/";
			break;
		case bgfx::RendererType::Direct3D11:
		case bgfx::RendererType::Direct3D12:
			filename += "shaders/dx11/";
			break;
		case bgfx::RendererType::Gnm:
			filename += "shaders/pssl/";
			break;
		case bgfx::RendererType::Metal:
			filename += "shaders/metal/";
			break;
		case bgfx::RendererType::OpenGL:
			filename += "shaders/glsl/";
			break;
		case bgfx::RendererType::OpenGLES:
			filename += "shaders/essl/";
			break;
		case bgfx::RendererType::Vulkan:
			filename += "shaders/spirv/";
			break;
		default:
			break;
	}

	filename += FILENAME;

	//	size_t shaderLen = strlen(shaderPath);
	//	size_t fileLen = strlen(FILENAME);
	//	char *filePath = (char *)malloc(shaderLen + fileLen + 1);
	//	memcpy(filePath, shaderPath, shaderLen);
	//	memcpy(&filePath[shaderLen], FILENAME, fileLen);
	//	filePath[shaderLen + fileLen] = 0;
	//	const char * filePath = filename;

	//	FILE *file = fopen(filePath, "rb");
	//	fseek(file, 0, SEEK_END);
	//	long fileSize = ftell(file);
	//	fseek(file, 0, SEEK_SET);

	//	const bgfx::Memory *mem = bgfx::alloc(fileSize + 1);
	//	fread(mem->data, 1, fileSize, file);
	//	mem->data[mem->size - 1] = '\0';
	//	fclose(file);

	FileAccess *file = FileAccess::open(filename, FileAccess::READ);
	CRASH_COND(!file);
	uint64_t len = file->get_len();
	const bgfx::Memory *mem = bgfx::alloc(len + 1);
	file->get_buffer((uint8_t *)mem->data, len);
	memdelete(file);

	return bgfx::createShader(mem);
}

static bool ctxErrorOccurred = false;
static int ctxErrorHandler(Display *dpy, XErrorEvent *ev) {
	ctxErrorOccurred = true;
	return 0;
}

void ContextBGFX_X11::release_current() {
	//	glXMakeCurrent(x11_display, None, nullptr);
}

void ContextBGFX_X11::make_current() {
	//	glXMakeCurrent(x11_display, *x11_window, p->glx_context);
}

bool ContextBGFX_X11::is_offscreen_available() const {
	//	return p->glx_context_offscreen;
	return false;
}

void ContextBGFX_X11::make_offscreen_current() {
	//	glXMakeCurrent(x11_display, *x11_window, p->glx_context_offscreen);
}

void ContextBGFX_X11::release_offscreen_current() {
	//	glXMakeCurrent(x11_display, None, NULL);
}

//static bool ctxErrorOccurred = false;
//static int ctxErrorHandler(Display *dpy, XErrorEvent *ev) {
//	ctxErrorOccurred = true;
//	return 0;
//}

//static void set_class_hint(Display *p_display, Window p_window) {
//	XClassHint *classHint;

//	/* set the name and class hints for the window manager to use */
//	classHint = XAllocClassHint();
//	if (classHint) {
//		classHint->res_name = (char *)"Godot_Engine";
//		classHint->res_class = (char *)"Godot";
//	}
//	XSetClassHint(p_display, p_window, classHint);
//	XFree(classHint);
//}

void ContextBGFX_X11::test(int width, int height) {
	Display *display = x11_display;
	Window &window = *x11_window;
	XSetWindowAttributes attributes;
	//	XGCValues gr_values;
	//	XFontStruct *fontinfo;
	//	GC gr_context;
	Visual *visual;
	int depth;
	int screen;
	//	XEvent event;
	//	XColor color, dummy;

	//display = XOpenDisplay(NULL);
	screen = DefaultScreen(display);
	visual = DefaultVisual(display, screen);
	depth = DefaultDepth(display, screen);
	//attributes.background_pixel = XWhitePixel(display, screen);

	//	window = XCreateWindow(display, XRootWindow(display, screen),
	//			200, 200, width, height, 5, depth, InputOutput,
	//			visual, CWBackPixel, &attributes);

	window = XCreateWindow(display, XRootWindow(display, screen),
			200, 200, width, height, 5, depth, InputOutput,
			visual, 0, &attributes);

	//	XSelectInput(display, window, ExposureMask | KeyPressMask);
	//	fontinfo = XLoadQueryFont(display, "6x10");

	//	XAllocNamedColor(display, DefaultColormap(display, screen), "red",
	//			&color, &dummy);

	//	gr_values.font = fontinfo->fid;
	//	gr_values.foreground = color.pixel;
	//	gr_context = XCreateGC(display, window, GCFont + GCForeground, &gr_values);
	XFlush(display);
	XMapWindow(display, window);
	XFlush(display);

	//	while (1) {
	//		XNextEvent(display, &event);

	//		switch (event.type) {
	//			case Expose:
	//				XDrawLine(display, window, gr_context, 0, 0, 100, 100);
	//				XDrawString(display, window, gr_context, 100, 100, "hello", 5);
	//				break;
	//			case KeyPress:
	//				XCloseDisplay(display);
	//				exit(0);
	//		}
	//	}
}

Error ContextBGFX_X11::initialize() {
	//int fbcount;
	//GLXFBConfig fbconfig = nullptr;
	XVisualInfo *vi = nullptr;

	XSetWindowAttributes swa;
	swa.event_mask = StructureNotifyMask;
	swa.border_pixel = 0;
	//unsigned long valuemask = CWBorderPixel | CWColormap | CWEventMask;
	/*

	if (OS::get_singleton()->is_layered_allowed()) {
		GLXFBConfig *fbc = glXChooseFBConfig(x11_display, DefaultScreen(x11_display), visual_attribs_layered, &fbcount);
		ERR_FAIL_COND_V(!fbc, ERR_UNCONFIGURED);

		for (int i = 0; i < fbcount; i++) {
			vi = (XVisualInfo *)glXGetVisualFromFBConfig(x11_display, fbc[i]);
			if (!vi) {
				continue;
			}

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
*/
	int (*oldHandler)(Display *, XErrorEvent *) = XSetErrorHandler(&ctxErrorHandler);

	//	vi = XGetVisualInfo(x11_display,

	/*
		switch (context_type) {
			case OLDSTYLE: {
				p->glx_context = glXCreateContext(x11_display, vi, nullptr, GL_TRUE);
				ERR_FAIL_COND_V(!p->glx_context, ERR_UNCONFIGURED);
			} break;
			case GLES_2_0_COMPATIBLE: {
				p->glx_context = glXCreateNewContext(x11_display, fbconfig, GLX_RGBA_TYPE, nullptr, true);
				ERR_FAIL_COND_V(!p->glx_context, ERR_UNCONFIGURED);
			} break;
			case GLES_3_0_COMPATIBLE: {
				static int context_attribs[] = {
					GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
					GLX_CONTEXT_MINOR_VERSION_ARB, 3,
					GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
					GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB , //|GLX_CONTEXT_DEBUG_BIT_ARB
					None
				};

				p->glx_context = glXCreateContextAttribsARB(x11_display, fbconfig, nullptr, true, context_attribs);
				ERR_FAIL_COND_V(ctxErrorOccurred || !p->glx_context, ERR_UNCONFIGURED);
				p->glx_context_offscreen = glXCreateContextAttribsARB(x11_display, fbconfig, nullptr, true, context_attribs);
			} break;
		}
	*/
	/*
	swa.colormap = XCreateColormap(x11_display, RootWindow(x11_display, vi->screen), vi->visual, AllocNone);
	*x11_window = XCreateWindow(x11_display, RootWindow(x11_display, vi->screen), 0, 0, OS::get_singleton()->get_video_mode().width, OS::get_singleton()->get_video_mode().height, 0, vi->depth, InputOutput, vi->visual, valuemask, &swa);
	XStoreName(x11_display, *x11_window, "Godot Engine");

	ERR_FAIL_COND_V(!x11_window, ERR_UNCONFIGURED);
	set_class_hint(x11_display, *x11_window);

	if (!OS::get_singleton()->is_no_window_mode_enabled()) {
		XMapWindow(x11_display, *x11_window);
	}
	*/
	int width = _width;
	int height = _height;

	test(width, height);

	XSync(x11_display, False);
	XSetErrorHandler(oldHandler);

	//glXMakeCurrent(x11_display, *x11_window, p->glx_context);

	if (vi)
		XFree(vi);

	//	bgfx::PlatformData pd;
	//	pd.ndt = x11_display;
	//	pd.nwh = x11_window;

	//	// Tell bgfx about the platform and window
	//	bgfx::setPlatformData(pd);

	// Render an empty frame
	bgfx::renderFrame();

	bgfx::Init bgfxInit;
	//bgfxInit.type = bgfx::RendererType::Count; // Automatically choose a renderer.
	bgfxInit.type = bgfx::RendererType::OpenGL; // Automatically choose a renderer.
	bgfxInit.resolution.width = width;
	bgfxInit.resolution.height = height;
	bgfxInit.resolution.reset = BGFX_RESET_VSYNC;
	bgfxInit.platformData.ndt = x11_display;
	bgfxInit.platformData.nwh = (void *)*x11_window;
	//	bgfxInit.platformData.nwh = (void *)x11_window;

	bgfx::init(bgfxInit);

	// Reset window
	//bgfx::reset(width, height, BGFX_RESET_VSYNC);

	// Enable debug text.
	//bgfx::setDebug(BGFX_DEBUG_TEXT /*| BGFX_DEBUG_STATS*/);

	// Set view rectangle for 0th view
	bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));

	// Clear the view rect
	bgfx::setViewClear(0,
			BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
			0x443355FF, 1.0f, 0);

	bgfx::touch(0);
	bgfx::frame();

	bgfx::VertexLayout pcvDecl;
	pcvDecl.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.end();
	data.vbh = bgfx::createVertexBuffer(bgfx::makeRef(cubeVertices, sizeof(cubeVertices)), pcvDecl);
	data.ibh = bgfx::createIndexBuffer(bgfx::makeRef(cubeTriList, sizeof(cubeTriList)));

	//unsigned int counter = 0;

	data.vsh = loadShader("vs_cubes.bin");
	data.fsh = loadShader("fs_cubes.bin");
	data.program = bgfx::createProgram(data.vsh, data.fsh, true);

	return OK;
}

void ContextBGFX_X11::swap_buffers() {
	//	glXSwapBuffers(x11_display, *x11_window);
	/*
  Put this inside the event loop of SDL, to render bgfx output
  */
	// Set view rectangle for 0th view
	bgfx::setViewRect(0, 0, 0, uint16_t(_width), uint16_t(_height));

	// Clear the view rect
	bgfx::setViewClear(0,
			BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
			0x443355FF, 1.0f, 0);

	// Set empty primitive on screen
	bgfx::touch(0);

	Transform look_at;
	look_at.origin = Vector3(0, 0, -5);
	look_at = look_at.looking_at(Vector3(0, 0, 0), Vector3(0, 1, 0));

	CameraMatrix cm;
	cm.set_orthogonal(5, 1.0, 0, 100);

	float view[16];
	float proj[16];
	_transform_to_mat16(look_at, view);
	_camera_matrix_to_mat16(cm, proj);

	bgfx::setViewTransform(0, view, proj);
	/*
		const bx::Vec3 at = { 0.0f, 0.0f, 0.0f };
		const bx::Vec3 eye = { 0.0f, 0.0f, -5.0f };
		float view[16];
		bx::mtxLookAt(view, eye, at);
		float proj[16];
		bx::mtxProj(proj, 60.0f, float(WNDW_WIDTH) / float(WNDW_HEIGHT), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
		bgfx::setViewTransform(0, view, proj);
		*/

	bgfx::setVertexBuffer(0, data.vbh);
	bgfx::setIndexBuffer(data.ibh);

	bgfx::submit(0, data.program);

	bgfx::frame();
}

void ContextBGFX_X11::set_use_vsync(bool p_use) {
	/*
	static bool setup = false;
	static PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = nullptr;
	static PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalMESA = nullptr;
	static PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI = nullptr;

	if (!setup) {
		setup = true;
		String extensions = glXQueryExtensionsString(x11_display, DefaultScreen(x11_display));
		if (extensions.find("GLX_EXT_swap_control") != -1) {
			glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddressARB((const GLubyte *)"glXSwapIntervalEXT");
		}
		if (extensions.find("GLX_MESA_swap_control") != -1) {
			glXSwapIntervalMESA = (PFNGLXSWAPINTERVALSGIPROC)glXGetProcAddressARB((const GLubyte *)"glXSwapIntervalMESA");
		}
		if (extensions.find("GLX_SGI_swap_control") != -1) {
			glXSwapIntervalSGI = (PFNGLXSWAPINTERVALSGIPROC)glXGetProcAddressARB((const GLubyte *)"glXSwapIntervalSGI");
		}
	}
	int val = p_use ? 1 : 0;
	if (glXSwapIntervalMESA) {
		glXSwapIntervalMESA(val);
	} else if (glXSwapIntervalSGI) {
		glXSwapIntervalSGI(val);
	} else if (glXSwapIntervalEXT) {
		GLXDrawable drawable = glXGetCurrentDrawable();
		glXSwapIntervalEXT(x11_display, drawable, val);
	} else {
		return;
	}
	*/
	Context::set_use_vsync(p_use);
}

ContextBGFX_X11::ContextBGFX_X11(::Display *p_x11_display, ::Window &p_x11_window, const OS::VideoMode &p_default_video_mode, ContextType p_context_type) {
	create(p_x11_display, p_x11_window, p_default_video_mode, p_context_type);
}

ContextBGFX_X11::~ContextBGFX_X11() {
	destroy();
	/*
  And put this just before SDL_Quit()
  */
	bgfx::shutdown();
}
