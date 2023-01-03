#include "context.h"

void Context::release_current() {
}

void Context::make_current() {
}

void Context::swap_buffers() {
}

int Context::get_window_width() {
}

int Context::get_window_height() {
}

void *Context::get_glx_context() {
}

bool Context::is_offscreen_available() const {
}

void Context::make_offscreen_current() {
}

void Context::release_offscreen_current() {
}

Error Context::initialize() {
}

void Context::set_use_vsync(bool p_use) {
}

bool Context::is_using_vsync() const {
}

Context::Context(::Display *p_x11_display, ::Window &p_x11_window, const OS::VideoMode &p_default_video_mode, ContextType p_context_type) {
}

Context::~Context() {
}
