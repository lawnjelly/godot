#include "context.h"

void Context::release_current() {
}

void Context::make_current() {
}

void Context::swap_buffers() {
}

int Context::get_window_width() {
	XWindowAttributes xwa;
	XGetWindowAttributes(x11_display, *x11_window, &xwa);

	return xwa.width;
}

int Context::get_window_height() {
	XWindowAttributes xwa;
	XGetWindowAttributes(x11_display, *x11_window, &xwa);

	return xwa.height;
}

bool Context::is_offscreen_available() const {
	return false;
}

void Context::make_offscreen_current() {
}

void Context::release_offscreen_current() {
}

Error Context::initialize() {
	return OK;
}

void Context::set_use_vsync(bool p_use) {
	use_vsync = p_use;
}

bool Context::is_using_vsync() const {
	return use_vsync;
}

void Context::create(::Display *p_x11_display, ::Window &p_x11_window, const OS::VideoMode &p_default_video_mode, ContextType p_context_type) {
	x11_window = &p_x11_window;
	default_video_mode = p_default_video_mode;
	x11_display = p_x11_display;

	context_type = p_context_type;

	double_buffer = false;
	direct_render = false;
	use_vsync = false;
}

void Context::destroy() {
	release_current();
}

Context::Context() {
}

Context::~Context() {
}
