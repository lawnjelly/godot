#pragma once

#include "scene/3d/mesh_instance.h"
#include "lvector.h"

class LPortal : public MeshInstance
{
	GDCLASS(LPortal, MeshInstance);
public:
	// ui interface
	void set_linked_room(const NodePath &link_path);
	NodePath get_linked_room() const;
	void set_portal_active(bool p_active);
	bool get_portal_active() const;


	// create the cached data from mesh instance, sort winding etc
	void UpdateFromMesh();

	// normal determined by winding order
	Vector<Vector3> m_ptsWorld;
	Vector3 m_ptCentre; // world
	Plane m_Plane;

	LPortal();

private:
	NodePath m_Settings_path_LinkedRoom;
	bool m_Settings_Active;

	ObjectID m_LinkedRoom_ObjectID;

protected:
	static void _bind_methods();
};


