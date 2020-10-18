#include "prooms.h"
#include "pdebug.h"


using namespace Lawn;

PRooms::PRooms()
{
	m_pRoomList = nullptr;
}


void PRooms::SetRoomListNode(Spatial * pRoomList)
{
	if (pRoomList)
	{
		LPRINT(2, "\tSetRoomListNode");
	}
	else
	{
		LPRINT(2, "\tSetRoomListNode NULL");
	}

	m_ID_RoomList = 0;
	m_pRoomList = 0;

	if (pRoomList)
	{
		m_ID_RoomList = pRoomList->get_instance_id();
		m_pRoomList = pRoomList;
		LPRINT(2, "\t\tRoomlist was Godot ID " + itos(m_ID_RoomList));
	}

}
