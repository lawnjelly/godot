#include "lroom_manager.h"
#include "pconverter.h"
#include "pdebug.h"

LRoomManager::LRoomManager()
{
}

void LRoomManager::_bind_methods()
{
	// main functions
	//ClassDB::bind_method(D_METHOD("rooms_convert", "verbose", "delete lights"), &LRoomManager::rooms_convert);

#define LPORTAL_STRINGIFY(x) #x
#define LPORTAL_TOSTRING(x) LPORTAL_STRINGIFY(x)



#define LIMPL_PROPERTY(P_TYPE, P_NAME, P_SET, P_GET) ClassDB::bind_method(D_METHOD(LPORTAL_TOSTRING(P_SET), LPORTAL_TOSTRING(P_NAME)), &LRoomManager::P_SET);\
ClassDB::bind_method(D_METHOD(LPORTAL_TOSTRING(P_GET)), &LRoomManager::P_GET);\
ADD_PROPERTY(PropertyInfo(P_TYPE, LPORTAL_TOSTRING(P_NAME)), LPORTAL_TOSTRING(P_SET), LPORTAL_TOSTRING(P_GET));

#define LIMPL_PROPERTY_RANGE(P_TYPE, P_NAME, P_SET, P_GET, P_RANGE_STRING) ClassDB::bind_method(D_METHOD(LPORTAL_TOSTRING(P_SET), LPORTAL_TOSTRING(P_NAME)), &LRoomManager::P_SET);\
ClassDB::bind_method(D_METHOD(LPORTAL_TOSTRING(P_GET)), &LRoomManager::P_GET);\
ADD_PROPERTY(PropertyInfo(P_TYPE, LPORTAL_TOSTRING(P_NAME), PROPERTY_HINT_RANGE, P_RANGE_STRING), LPORTAL_TOSTRING(P_SET), LPORTAL_TOSTRING(P_GET));


	ADD_GROUP("Main", "");
//	LIMPL_PROPERTY(Variant::BOOL, active, rooms_set_active, rooms_get_active);

	ADD_GROUP("Paths", "");
	LIMPL_PROPERTY(Variant::NODE_PATH, rooms, set_rooms_path, get_rooms_path);

#undef LIMPL_PROPERTY
#undef LIMPL_PROPERTY_RANGE
}

void LRoomManager::set_rooms_path(const NodePath &path)
{
	m_RM.m_Settings_path_RoomList = path;
}

NodePath LRoomManager::get_rooms_path() const
{
	return m_RM.m_Settings_path_RoomList;
}

void LRoomManager::rooms_convert()
{
	if (!m_RM.ResolveRoomListPath(this))
	{
		LWARN(0, "Cannot resolve nodepath");
		return;
	}

	Lawn::PConverter converter;
	converter.Convert(m_RM.m_Rooms);
}

