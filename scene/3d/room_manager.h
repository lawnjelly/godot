/*************************************************************************/
/*  room_manager.h                                                       */
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

#pragma once

#include "core/local_vector.h"
#include "spatial.h"

class Portal;
class Room;
class MeshInstance;

class RoomManager : public Spatial {
	GDCLASS(RoomManager, Spatial);

public:
	enum PVSMode {
		PVS_MODE_DISABLED,
		PVS_MODE_GENERATE,
		PVS_MODE_GENERATE_AND_CULL,
	};

	void set_rooms_path(const NodePath &p_path) {
		_settings_path_roomlist = p_path;
	}
	NodePath get_rooms_path() const {
		return _settings_path_roomlist;
	}
	void set_camera_path(const NodePath &p_path);

	NodePath get_camera_path() const {
		return _settings_path_camera;
	}

	void rooms_set_active(bool p_active);
	bool rooms_get_active() const;

	void set_show_debug(bool p_show);
	bool get_show_debug() const;

	void set_debug_sprawl(bool p_active);
	bool get_debug_sprawl() const;

	void set_portal_plane_convention(bool p_reversed);
	bool get_portal_plane_convention() const;

	void set_simplify_dist(float p_dist);
	float get_simplify_dist() const;

	void set_simplify_dot(float p_dot);
	float get_simplify_dot() const;

	void set_default_portal_margin(float p_dist);
	float get_default_portal_margin() const;

	void set_pvs_mode(PVSMode p_mode);
	PVSMode get_pvs_mode() const;

	void set_pvs_filename(String p_filename);
	String get_pvs_filename() const;

	void rooms_convert();
	void rooms_clear();

	// for internal use in the editor..
	// either we can clear the rooms and unload,
	// or reconvert.
	void _rooms_changed();

	RoomManager();

private:
	// funcs
	bool resolve_camera_path();

	// conversion
	void _convert_rooms_recursive(Spatial *p_node, LocalVector<Portal *> &portals);
	void _convert_room(Spatial *p_node, LocalVector<Portal *> &portals);
	void _convert_portal(Room *p_room, MeshInstance *pMI, LocalVector<Portal *> &portals);
	void _second_pass_portals(Spatial *p_roomlist, LocalVector<Portal *> &r_portals);
	void _second_pass_rooms();
	void _second_pass_room(Room *p_room);
	bool _convert_manual_bound(Room *p_room, Spatial *p_node);
	void _add_portal_points(Spatial *p_node, Vector<Vector3> &r_room_pts);

	void _find_statics_recursive(Room *p_room, Spatial *p_node, Vector<Vector3> &r_room_pts);
	bool _bound_findpoints(MeshInstance *p_mi, Vector<Vector3> &r_room_pts, AABB &r_aabb);
	bool _convert_room_hull(Room *p_room, const Vector<Vector3> &p_room_pts);
	void _add_plane_if_unique(LocalVector<Plane, int32_t> &p_planes, const Plane &p);
	void _update_portal_margins(Spatial *p_node, float p_margin);

	// helper funcs
	bool _name_starts_with(const Node *p_node, String p_search_string) const;
	template <class NODE_TYPE>
	NODE_TYPE *_resolve_path(NodePath p_path) const;
	template <class NODE_TYPE>
	bool _node_is_type(Spatial *p_node) const;
	template <class T>
	T *_change_node_type(Spatial *p_node, String p_prefix, bool p_delete = true);

	// output strings during conversion process
	void convert_log(String p_string) { debug_print_line(p_string); }

	// only prints when user has set 'debug' in the room manager inspector
	// also does not show in non editor builds
	void debug_print_line(String p_string);

public:
	static String _find_name_after(Node *p_node, String p_string_start);

private:
	// accessible from UI
	NodePath _settings_path_roomlist;
	NodePath _settings_path_camera;

	bool _active = true;

	// portals, room hulls etc
	bool _show_debug = true;
	bool _debug_sprawl = false;

	// pvs
	PVSMode _pvs_mode = PVS_MODE_DISABLED;
	String _pvs_filename;

	int _conversion_tick = 0;

	// just used during conversion, could be invalidated
	// later by user deleting rooms etc.
	LocalVector<Room *, int32_t> _rooms;

	// advanced params
	float _default_portal_margin = 1.0f;
	float _plane_simplify_dist = 0.08f;
	float _plane_simplify_dot = 0.98f;

	// debug override camera
	ObjectID _godot_camera_ID = -1;
	// local version of the godot camera frustum,
	// to prevent updating the visual server (and causing
	// a screen refresh) where not necessary.
	Vector3 _godot_camera_pos;
	Vector<Plane> _godot_camera_planes;

protected:
	static void _bind_methods();
	void _notification(int p_what);
};

VARIANT_ENUM_CAST(RoomManager::PVSMode);
