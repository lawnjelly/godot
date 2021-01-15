#pragma once

#include "scene/3d/spatial.h"


namespace Lawn {

class PRooms
{
public:
	PRooms();
	void set_roomlist_node(Spatial * p_roomlist);

	Spatial * get_roomlist() const {return _roomlist_node;}

private:
	Spatial * _roomlist_node;
	ObjectID _roomlist_godot_ID;

};

} // namespace
