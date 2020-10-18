#pragma once

#include "scene/3d/spatial.h"
#include "prooms.h"

class LRoomManager;

namespace Lawn {

class PRoomManager
{
	friend class ::LRoomManager;
private:
	// accessible from UI
	NodePath m_Settings_path_RoomList;


	// internal
	PRooms m_Rooms;

	// funcs
	bool ResolveRoomListPath(Node * pAnyNode); // needs a node in the scene graph to use get_node

};

} // namespace
