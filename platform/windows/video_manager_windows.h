#pragma once

#include "core/video/video_manager.h"


#ifdef WINDOWS_ENABLED
#include <windows.h>


class VideoManager_Windows : public VideoManager
{
public:
	virtual Error window_create(DisplayServer::WindowID p_window_id, HWND p_hwnd, HINSTANCE p_hinstance, int p_width, int p_height) = 0;
};


#endif
