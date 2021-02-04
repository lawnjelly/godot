/*************************************************************************/
/*  portal.h                                                             */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef PORTAL_H
#define PORTAL_H

#include "core/rid.h"
#include "mesh_instance.h"

class RoomManager;

class Portal : public MeshInstance {
	GDCLASS(Portal, MeshInstance);

	RID _portal_rid;

	friend class RoomManager;
	friend class RoomManagerGizmoPlugin;

public:
	// ui interface .. will have no effect after room conversion
	void set_linked_room(const NodePath &link_path);
	NodePath get_linked_room() const;

	// open and close doors
	void set_portal_active(bool p_active);
	bool get_portal_active() const;

	// we can optionally include the portal as part of room convex hull
	void set_include_in_bound_enabled(bool p_enabled);
	bool get_include_in_bound_enabled() const;

	void clear();

	// whether to use the room manager default
	void set_use_default_margin(bool p_use);
	bool get_use_default_margin() const;

	// custom portal margin (per portal) .. only valid if use_default_margin is off
	void set_portal_margin(real_t p_margin);
	real_t get_portal_margin() const;

	// either the default margin or the custom portal margin, depending on the setting
	real_t get_active_portal_margin() const;

	// if the user wants a moving portal they can do it by moving the portal node, then calling either
	// full update (if the mesh has changed) or transform update (if only transform has changed).
	// Note that while this will work in most circumstances, static sprawling is calculated as a one off
	// at room conversion, so it won't work correctly with sprawling statics.
	void portal_update_full();
	void portal_update_transform();

	Portal();
	~Portal();

	// whether the convention is that the normal of the portal points outward (false) or inward (true)
	// normally I'd recommend portal normal faces outward. But you may make a booboo, so this can work
	// with either convention.
	static bool _portal_plane_convention;

private:
	void set_linked_room_internal(const NodePath &link_path);
	bool try_set_unique_name(const String &p_name);

	void sort_verts_clockwise(bool portal_plane_convention);
	void reverse_winding_order();
	void plane_from_points();
	void debug_check_plane_validity(const Plane &p) const;
	void resolve_links(const RID &p_from_room_rid);
	void _changed();

	// nodepath to the room this outgoing portal leads to
	NodePath _settings_path_linkedroom;

	// portal can be turned on and off at runtime, for e.g.
	// opening and closing a door
	bool _settings_active;

	// user can choose not to include the portal in the convex hull of the room
	// during conversion
	bool _settings_include_in_bound;

	// room from and to, ID in the room manager
	int _linkedroom_ID[2];

	// normal determined by winding order
	Vector<Vector3> _pts_world;

	// centre of the world points
	Vector3 _pt_centre_world;

	// model space (for moving portals
	Vector<Vector3> _pts_model;

	// default convention is for the plane normal to point out from the source room
	Plane _plane;

	// extension margin
	real_t _margin;
	real_t _default_margin;
	bool _use_default_margin;

	// for editing
#ifdef TOOLS_ENABLED
	ObjectID _room_manager_godot_ID;
	RoomManager *_find_room_manager();
#endif

public:
	// makes sure portals are not converted more than once per
	// call to rooms_convert
	int _conversion_tick = -1;

protected:
	static void _bind_methods();
	void _notification(int p_what);
};

#endif
