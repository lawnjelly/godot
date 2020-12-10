#pragma once

#include "servers/display_server.h"

// Base class for providing a video manager,
// which can control one or more video drivers.
// Video managers are 'plugins' which register themselves
// at startup, and allow third parties to more easily create new video
// drivers.

// Derive from the platform specific video managers you wish to support,
// and register these at startup.

class VideoManager
{
public:
	virtual void window_destroy(DisplayServer::WindowID p_window_id) = 0;
	virtual void window_resize(DisplayServer::WindowID p_window_id, int p_width, int p_height) = 0;
	
	virtual void window_make_current(DisplayServer::WindowID p_window_id) = 0;
	
//	virtual int window_get_width(DisplayServer::WindowID p_window_id) = 0;
//	virtual int window_get_height(DisplayServer::WindowID p_window_id) = 0;
	
	virtual void swap_buffers() = 0;
	
	// a video manager can support 1 or more drivers (e.g. GLES2 and GLES3 in one manager)
	virtual int get_num_drivers() = 0;
	virtual String get_driver_name(int p_driver_id) = 0;
	
	virtual Error initialize(int p_driver_id) = 0;
};


