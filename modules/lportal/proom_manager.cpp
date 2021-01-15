#include "proom_manager.h"
#include "pdebug.h"

using namespace Lawn;

bool PRoomManager::resolve_roomlist_path(Node * p_anynode)
{
	LPRINT(2, "resolve_roomlist_path " + _settings_path_roomlist);

	if (p_anynode->has_node(_settings_path_roomlist))
	{
		LPRINT(2, "has_node");
		Spatial * node = Object::cast_to<Spatial>(p_anynode->get_node(_settings_path_roomlist));
		if (node)
		{
			_rooms.set_roomlist_node(node);
			return true;
		}
		else
		{
			WARN_PRINT("RoomList must be a Spatial");
		}
	}

	// default to setting to null
	_rooms.set_roomlist_node(nullptr);
	return false;
}
