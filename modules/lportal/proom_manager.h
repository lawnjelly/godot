#pragma once

#include "scene/3d/spatial.h"
#include "prooms.h"
#include "pstatics.h"

class LRoomManager;

namespace Lawn {

class PRoomManager
{
	friend class ::LRoomManager;
private:
	// accessible from UI
	NodePath _settings_path_roomlist;

	// internal
	PRooms _rooms;
	PStatics _statics;

	// funcs
	bool resolve_roomlist_path(Node * p_anynode); // needs a node in the scene graph to use get_node

};

} // namespace
