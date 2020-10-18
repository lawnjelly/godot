#pragma once

#include "scene/3d/spatial.h"
#include "proom_manager.h"

class LRoomManager : public Spatial
{
	GDCLASS(LRoomManager, Spatial);
protected:
	Lawn::PRoomManager m_RM;

public:
	void set_rooms_path(const NodePath &path);
	NodePath get_rooms_path() const;

	void rooms_convert();

protected:


	static void _bind_methods();
	//void _notification(int p_what);

public:
	LRoomManager();
};
