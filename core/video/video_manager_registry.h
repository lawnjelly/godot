#pragma once

#include "core/templates/vector.h"

class VideoManager;

class VideoManagerRegistry
{
public:
	// registering
	void register_video_manager(VideoManager * p_manager);
	
	// access	
	int get_num_managers() const {return managers.size();}
	VideoManager * get_manager(int p_id) {return managers[p_id];}
	
private:
	Vector<VideoManager *> managers;
};
