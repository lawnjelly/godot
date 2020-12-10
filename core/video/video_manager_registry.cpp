#include "video_manager_registry.h"
#include "core/video/video_manager.h"


void VideoManagerRegistry::register_video_manager(VideoManager * p_manager)
{
	managers.push_back(p_manager);
}

VideoManager * VideoManagerRegistry::initialize_driver(String p_requested_driver_name)
{
	// generic video manager
	// go through each video manager, and each video driver in the registry looking for a match
	for (int n=0; n<get_num_managers(); n++)
	{
		VideoManager * vm = get_manager(n);
		
		for (int d=0; d<vm->get_num_drivers(); d++)
		{
			String driver_name = vm->get_driver_name(d);
			
			// match?
			if (p_requested_driver_name == driver_name)
			{
				// attempt to create the driver
				if (vm->initialize(d) != OK)
				{
					return nullptr;
					// no good .. fallback? NYI
				}
				return vm;
			}
		}
		
	}
	
	return nullptr;
}

