#pragma once

#include "core/video/video_manager.h"
#include <X11/Xlib.h>

class VideoManager_x11 : public VideoManager
{
public:
	virtual Error window_create(DisplayServer::WindowID p_window_id, ::Window p_window, Display *p_display, int p_width, int p_height) = 0;
};
