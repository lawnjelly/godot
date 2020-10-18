#pragma once

#include "scene/3d/spatial.h"


namespace Lawn {

class PRooms
{
public:
	PRooms();
	void SetRoomListNode(Spatial * pRoomList);

	Spatial * GetRoomList() const {return m_pRoomList;}

private:
	Spatial * m_pRoomList;
	ObjectID m_ID_RoomList;

};

} // namespace
