#include "lportal.h"
#include "lroom.h"

void LPortal::_bind_methods()
{
	// main functions
	//ClassDB::bind_method(D_METHOD("rooms_convert", "verbose", "delete lights"), &LRoomManager::rooms_convert);
	ClassDB::bind_method(D_METHOD("set_portal_active", "p_active"), &LPortal::set_portal_active);
	ClassDB::bind_method(D_METHOD("get_portal_active"), &LPortal::get_portal_active);

	ClassDB::bind_method(D_METHOD("set_linked_room", "p_room"), &LPortal::set_linked_room);
	ClassDB::bind_method(D_METHOD("get_linked_room"), &LPortal::get_linked_room);


	ADD_GROUP("Portal", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "portal_active"), "set_portal_active", "get_portal_active");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "linked_room"), "set_linked_room", "get_linked_room");
}

void LPortal::set_portal_active(bool p_active)
{
	m_Settings_Active = p_active;
}

bool LPortal::get_portal_active() const
{
	return m_Settings_Active;
}

void LPortal::set_linked_room(const NodePath &link_path)
{
	m_Settings_path_LinkedRoom = link_path;

	LRoom * pLinkedRoom = Object::cast_to<LRoom>(get_node(link_path));

	if (pLinkedRoom)
	{
		m_LinkedRoom_ObjectID = pLinkedRoom->get_instance_id();
	}
	else
	{
		m_LinkedRoom_ObjectID = 0;
	}
}

NodePath LPortal::get_linked_room() const
{
	return m_Settings_path_LinkedRoom;
}


LPortal::LPortal()
{
	m_Settings_Active = true;
}

// create the cached data from mesh instance, sort winding etc
void LPortal::UpdateFromMesh()
{

}




