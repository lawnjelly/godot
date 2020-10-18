#include "proom_manager.h"
#include "pdebug.h"

using namespace Lawn;


bool PRoomManager::ResolveRoomListPath(Node * pAnyNode)
{
	LPRINT(2, "ResolveRoomListPath " + m_Settings_path_RoomList);

	if (pAnyNode->has_node(m_Settings_path_RoomList))
	{
		LPRINT(2, "has_node");
		Spatial * pNode = Object::cast_to<Spatial>(pAnyNode->get_node(m_Settings_path_RoomList));
		if (pNode)
		{
			m_Rooms.SetRoomListNode(pNode);
			return true;
		}
		else
		{
			WARN_PRINT("RoomList must be a Spatial");
		}
	}

	// default to setting to null
	m_Rooms.SetRoomListNode(nullptr);
	return false;
}
