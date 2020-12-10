#pragma once

#include "core/templates/vector.h"

class VideoManager;

// see https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
class VideoManagerRegistry
{
	VideoManagerRegistry() {}
public:
	static VideoManagerRegistry &get_singleton()
	{
		static VideoManagerRegistry instance;
		return instance;
	}
	
	// singleton
	VideoManagerRegistry(VideoManagerRegistry const&) = delete;
	void operator=(VideoManagerRegistry const&) = delete;	
	
	// registering
	void register_video_manager(VideoManager * p_manager);
	
	// access	
	int get_num_managers() const {return managers.size();}
	VideoManager * get_manager(int p_id) {return managers[p_id];}
	
private:
	Vector<VideoManager *> managers;
};
