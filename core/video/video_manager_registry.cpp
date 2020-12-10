#include "video_manager_registry.h"


void VideoManagerRegistry::register_video_manager(VideoManager * p_manager)
{
	managers.push_back(p_manager);
}

