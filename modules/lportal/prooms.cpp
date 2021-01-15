#include "prooms.h"
#include "pdebug.h"


using namespace Lawn;

PRooms::PRooms()
{
	_roomlist_node = nullptr;
}


void PRooms::set_roomlist_node(Spatial * p_roomlist)
{
	if (p_roomlist)
	{
		LPRINT(2, "\tset_roomlist_node");
	}
	else
	{
		LPRINT(2, "\tset_roomlist_node NULL");
	}

	_roomlist_node = nullptr;
	_roomlist_godot_ID = 0;

	if (p_roomlist)
	{
		_roomlist_node = p_roomlist;
		_roomlist_godot_ID = _roomlist_node->get_instance_id();
		LPRINT(2, "\t\troomlist was godot ID " + itos(_roomlist_godot_ID));
	}

}
