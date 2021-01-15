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
	void update_from_mesh();

	// normal determined by winding order
	Vector<Vector3> _pts_world;
	Vector3 _pt_centre; // world
	Plane _plane;

	LPortal();

private:
	NodePath _settings_path_linkedroom;
	bool _settings_active;

	ObjectID _linkedroom_godot_ID;

protected:
	static void _bind_methods();
};


