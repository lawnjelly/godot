/*************************************************************************/
/*  room_manager.cpp                                                     */
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

#include "room_manager.h"
#include "core/bitfield_dynamic.h"
#include "core/engine.h"
#include "core/math/quick_hull.h"
#include "portal.h"
#include "room.h"
#include "room_group.h"
#include "scene/3d/camera.h"
#include "scene/3d/light.h"

RoomManager::RoomManager() {
}

void RoomManager::rooms_update_gameplay_ex(const Vector<Vector3> &p_camera_positions, bool p_use_secondary_pvs) {
	if (!is_inside_world())
		return;

	RID scenario = get_world()->get_scenario();

	VisualServer::get_singleton()->rooms_update_gameplay_monitor(scenario, p_camera_positions, p_use_secondary_pvs);
}

void RoomManager::rooms_update_gameplay(const Vector3 &p_camera_pos, bool p_use_secondary_pvs) {
	Vector<Vector3> positions;
	positions.push_back(p_camera_pos);

	rooms_update_gameplay_ex(positions, p_use_secondary_pvs);
}

void RoomManager::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			if (Engine::get_singleton()->is_editor_hint()) {
				set_process_internal(_godot_camera_ID != (ObjectID)-1);
			}
		} break;
		case NOTIFICATION_INTERNAL_PROCESS: {
			// can't call visual server if not inside world
			if (!is_inside_world()) {
				return;
			}

			RID scenario = get_world()->get_scenario();

			if (_godot_camera_ID != (ObjectID)-1) {
				Camera *cam = Object::cast_to<Camera>(ObjectDB::get_instance(_godot_camera_ID));
				if (!cam) {
					_godot_camera_ID = (ObjectID)-1;
				} else {
					// get camera position and direction
					Vector3 camera_pos = cam->get_global_transform().origin;
					Vector<Plane> planes = cam->get_frustum();

					// only update the visual server when there is a change.. as it will request a screen redraw
					// this is kinda silly, but the other way would be keeping track of the override camera in visual server
					// and tracking the camera deletes, which might be more error prone for a debug feature...
					bool changed = false;
					if (camera_pos != _godot_camera_pos) {
						changed = true;

						// update gameplay monitor
						Vector<Vector3> camera_positions;
						camera_positions.push_back(camera_pos);
						VisualServer::get_singleton()->rooms_update_gameplay_monitor(scenario, camera_positions, false);
					}
					// check planes
					if (!changed) {
						if (planes.size() != _godot_camera_planes.size()) {
							changed = true;
						}
					}

					if (!changed) {
						// num of planes must be identical
						for (int n = 0; n < planes.size(); n++) {
							if (planes[n] != _godot_camera_planes[n]) {
								changed = true;
								break;
							}
						}
					}

					if (changed) {
						_godot_camera_pos = camera_pos;
						_godot_camera_planes = planes;
						VisualServer::get_singleton()->rooms_override_camera(scenario, true, camera_pos, &planes);
					}
				}
			}
		} break;
	}
}

void RoomManager::_bind_methods() {
	BIND_ENUM_CONSTANT(RoomManager::PVS_MODE_DISABLED);
	BIND_ENUM_CONSTANT(RoomManager::PVS_MODE_GENERATE);
	BIND_ENUM_CONSTANT(RoomManager::PVS_MODE_GENERATE_AND_CULL);

	// main functions
	ClassDB::bind_method(D_METHOD("rooms_convert"), &RoomManager::rooms_convert);
	ClassDB::bind_method(D_METHOD("rooms_clear"), &RoomManager::rooms_clear);

	ClassDB::bind_method(D_METHOD("set_pvs_mode", "pvs_mode"), &RoomManager::set_pvs_mode);
	ClassDB::bind_method(D_METHOD("get_pvs_mode"), &RoomManager::get_pvs_mode);

	// ClassDB::bind_method(D_METHOD("set_pvs_filename", "pvs_filename"), &RoomManager::set_pvs_filename);
	// ClassDB::bind_method(D_METHOD("get_pvs_filename"), &RoomManager::get_pvs_filename);

	ClassDB::bind_method(D_METHOD("rooms_update_gameplay", "camera_pos", "use_secondary_pvs"), &RoomManager::rooms_update_gameplay);
	ClassDB::bind_method(D_METHOD("rooms_update_gameplay_ex", "camera_positions", "use_secondary_pvs"), &RoomManager::rooms_update_gameplay);

	// just some macros to make setting inspector values easier
#define LPORTAL_STRINGIFY(x) #x
#define LPORTAL_TOSTRING(x) LPORTAL_STRINGIFY(x)

#define LIMPL_PROPERTY(P_TYPE, P_NAME, P_SET, P_GET)                                                        \
	ClassDB::bind_method(D_METHOD(LPORTAL_TOSTRING(P_SET), LPORTAL_TOSTRING(P_NAME)), &RoomManager::P_SET); \
	ClassDB::bind_method(D_METHOD(LPORTAL_TOSTRING(P_GET)), &RoomManager::P_GET);                           \
	ADD_PROPERTY(PropertyInfo(P_TYPE, LPORTAL_TOSTRING(P_NAME)), LPORTAL_TOSTRING(P_SET), LPORTAL_TOSTRING(P_GET));

#define LIMPL_PROPERTY_RANGE(P_TYPE, P_NAME, P_SET, P_GET, P_RANGE_STRING)                                  \
	ClassDB::bind_method(D_METHOD(LPORTAL_TOSTRING(P_SET), LPORTAL_TOSTRING(P_NAME)), &RoomManager::P_SET); \
	ClassDB::bind_method(D_METHOD(LPORTAL_TOSTRING(P_GET)), &RoomManager::P_GET);                           \
	ADD_PROPERTY(PropertyInfo(P_TYPE, LPORTAL_TOSTRING(P_NAME), PROPERTY_HINT_RANGE, P_RANGE_STRING), LPORTAL_TOSTRING(P_SET), LPORTAL_TOSTRING(P_GET));

	ADD_GROUP("Main", "");
	LIMPL_PROPERTY(Variant::BOOL, active, rooms_set_active, rooms_get_active);
	LIMPL_PROPERTY(Variant::BOOL, reverse_portals, set_portal_plane_convention, get_portal_plane_convention);
	LIMPL_PROPERTY(Variant::BOOL, show_debug, set_show_debug, get_show_debug);
	LIMPL_PROPERTY(Variant::BOOL, debug_sprawl, set_debug_sprawl, get_debug_sprawl);

	ADD_GROUP("Paths", "");
	LIMPL_PROPERTY(Variant::NODE_PATH, rooms, set_rooms_path, get_rooms_path);
	LIMPL_PROPERTY(Variant::NODE_PATH, camera, set_camera_path, get_camera_path);

	ADD_GROUP("PVS", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "pvs_mode", PROPERTY_HINT_ENUM, "Disabled,Partial,Full"), "set_pvs_mode", "get_pvs_mode");
	// ADD_PROPERTY(PropertyInfo(Variant::STRING, "pvs_filename", PROPERTY_HINT_FILE, "*.pvs"), "set_pvs_filename", "get_pvs_filename");

	ADD_GROUP("Optimize", "");
	LIMPL_PROPERTY(Variant::BOOL, merge_meshes, set_merge_meshes, get_merge_meshes);
	LIMPL_PROPERTY(Variant::BOOL, remove_danglers, set_remove_danglers, get_remove_danglers);

	ADD_GROUP("Advanced", "");
	LIMPL_PROPERTY_RANGE(Variant::REAL, plane_simplify_dist, set_simplify_dist, get_simplify_dist, "0.0,0.5,0.01");
	LIMPL_PROPERTY_RANGE(Variant::REAL, plane_simplify_dot, set_simplify_dot, get_simplify_dot, "0.9,1.0,0.001");
	LIMPL_PROPERTY_RANGE(Variant::REAL, default_portal_margin, set_default_portal_margin, get_default_portal_margin, "0.0, 10.0, 0.01");

#undef LIMPL_PROPERTY
#undef LIMPL_PROPERTY_RANGE
#undef LPORTAL_STRINGIFY
#undef LPORTAL_TOSTRING
}

void RoomManager::set_camera_path(const NodePath &p_path) {
	_settings_path_camera = p_path;

	resolve_camera_path();

	bool camera_on = _godot_camera_ID != (ObjectID)-1;

	// make sure the cached camera planes are invalid, this will
	// force an update to the visual server on the next internal_process
	_godot_camera_planes.clear();

	// if in the editor, turn processing on or off
	// according to whether the camera is overridden
	if (Engine::get_singleton()->is_editor_hint()) {
		if (is_inside_tree()) {
			set_process_internal(camera_on);
		}
	}

	// if we are turning camera override off, must inform visual server
	if (!camera_on && is_inside_world() && get_world().is_valid() && get_world()->get_scenario().is_valid()) {
		VisualServer::get_singleton()->rooms_override_camera(get_world()->get_scenario(), false, Vector3(), nullptr);
	}

	// we couldn't resolve the path, let's set it to null
	if (!camera_on) {
		_settings_path_camera = NodePath();
	}
}

void RoomManager::set_portal_plane_convention(bool p_reversed) {
	Portal::_portal_plane_convention = p_reversed;
}

bool RoomManager::get_portal_plane_convention() const {
	return Portal::_portal_plane_convention;
}

void RoomManager::set_simplify_dist(real_t p_dist) {
	_plane_simplify_dist = p_dist;
}
real_t RoomManager::get_simplify_dist() const {
	return _plane_simplify_dist;
}

void RoomManager::set_simplify_dot(real_t p_dot) {
	_plane_simplify_dot = p_dot;
}
real_t RoomManager::get_simplify_dot() const {
	return _plane_simplify_dot;
}

void RoomManager::set_default_portal_margin(real_t p_dist) {
	_default_portal_margin = p_dist;

	// send to portals
	Spatial *roomlist = _resolve_path<Spatial>(_settings_path_roomlist);
	if (!roomlist) {
		return;
	}

	_update_portal_margins(roomlist, _default_portal_margin);
}

void RoomManager::_update_portal_margins(Spatial *p_node, real_t p_margin) {
	Portal *portal = Object::cast_to<Portal>(p_node);

	if (portal) {
		portal->_default_margin = p_margin;
		portal->update_gizmo();
	}

	// recurse
	for (int n = 0; n < p_node->get_child_count(); n++) {
		Spatial *child = Object::cast_to<Spatial>(p_node->get_child(n));

		if (child) {
			_update_portal_margins(child, p_margin);
		}
	}
}

real_t RoomManager::get_default_portal_margin() const {
	return _default_portal_margin;
}

void RoomManager::set_show_debug(bool p_show) {
	// force not to show when not in editor
	if (!Engine::get_singleton()->is_editor_hint())
		p_show = false;

	_show_debug = p_show;

	Spatial *roomlist = _resolve_path<Spatial>(_settings_path_roomlist);
	if (!roomlist) {
		return;
	}

	_set_show_debug_recursive(roomlist, p_show);
}

bool RoomManager::get_show_debug() const {
	return _show_debug;
}

void RoomManager::set_debug_sprawl(bool p_enable) {
	if (is_inside_world() && get_world().is_valid()) {
		VisualServer::get_singleton()->rooms_set_debug_feature(get_world()->get_scenario(), VisualServer::ROOMS_DEBUG_SPRAWL, p_enable);
		_debug_sprawl = p_enable;
	}
}

bool RoomManager::get_debug_sprawl() const {
	return _debug_sprawl;
}

void RoomManager::set_merge_meshes(bool p_enable) {
	_settings_merge_meshes = p_enable;
}

bool RoomManager::get_merge_meshes() const {
	return _settings_merge_meshes;
}

void RoomManager::set_remove_danglers(bool p_enable) {
	_settings_remove_danglers = p_enable;
}

bool RoomManager::get_remove_danglers() const {
	return _settings_remove_danglers;
}

void RoomManager::debug_print_line(String p_string) {
	if (_show_debug) {
		print_line(p_string);
	}
}

void RoomManager::rooms_set_active(bool p_active) {
	if (is_inside_world() && get_world().is_valid()) {
		VisualServer::get_singleton()->rooms_set_active(get_world()->get_scenario(), p_active);
		_active = p_active;
	}
}

bool RoomManager::rooms_get_active() const {
	return _active;
}

void RoomManager::set_pvs_mode(PVSMode p_mode) {
	_pvs_mode = p_mode;
}

RoomManager::PVSMode RoomManager::get_pvs_mode() const {
	return _pvs_mode;
}

void RoomManager::set_pvs_filename(String p_filename) {
	_pvs_filename = p_filename;
}

String RoomManager::get_pvs_filename() const {
	return _pvs_filename;
}

void RoomManager::_rooms_changed() {
	_rooms.clear();
	if (is_inside_world() && get_world().is_valid()) {
		VisualServer::get_singleton()->rooms_unload(get_world()->get_scenario());
	}
}

void RoomManager::rooms_clear() {
	_rooms.clear();
	if (is_inside_world() && get_world().is_valid()) {
		VisualServer::get_singleton()->rooms_and_portals_clear(get_world()->get_scenario());
	}
}

void RoomManager::rooms_convert() {
	Spatial *roomlist = _resolve_path<Spatial>(_settings_path_roomlist);
	if (!roomlist) {
		WARN_PRINT("Cannot resolve nodepath");
		return;
	}

	ERR_FAIL_COND(!is_inside_world() || !get_world().is_valid());

	// every time we run convert we increment this,
	// to prevent individual rooms / portals being converted
	// more than once in one run
	_conversion_tick++;

	rooms_clear();
	LocalVector<Portal *> portals;
	LocalVector<RoomGroup *> roomgroups;

	// find the rooms and portals
	_convert_rooms_recursive(roomlist, portals, roomgroups);

	// add portal links
	_second_pass_portals(roomlist, portals);

	// create the statics
	_second_pass_rooms(roomgroups);

	bool generate_pvs = false;
	bool pvs_cull = false;
	switch (_pvs_mode) {
		default: {
		} break;
		case PVS_MODE_GENERATE: {
			generate_pvs = true;
		} break;
		case PVS_MODE_GENERATE_AND_CULL: {
			generate_pvs = true;
			pvs_cull = true;
		} break;
	}

	VisualServer::get_singleton()->rooms_finalize(get_world()->get_scenario(), generate_pvs, pvs_cull, _pvs_filename);

	// refresh whether to show portals etc
	set_show_debug(_show_debug);
}

void RoomManager::_second_pass_room(Room *p_room, const LocalVector<RoomGroup *> &p_roomgroups) {
	if (_settings_merge_meshes) {
		_merge_meshes_in_room(p_room);
	}

	// find statics and manual bound
	bool manual_bound_found = false;

	// points making up the room geometry, in world space, to create the convex hull
	Vector<Vector3> room_pts;

	for (int n = 0; n < p_room->get_child_count(); n++) {
		Spatial *child = Object::cast_to<Spatial>(p_room->get_child(n));

		if (child) {
			if (_name_starts_with(child, "GPortal_") || _node_is_type<Portal>(child)) {
				_add_portal_points(child, room_pts);
			} else if (_name_starts_with(child, "Bound_")) {
				manual_bound_found = _convert_manual_bound(p_room, child);
			} else {
				_find_statics_recursive(p_room, child, room_pts);
			}
		}
	}

	// create convex hull
	if (!manual_bound_found) {
		_convert_room_hull(p_room, room_pts);
	}

	// add the room to roomgroups
	for (int n = 0; n < p_room->_roomgroups.size(); n++) {
		int roomgroup_id = p_room->_roomgroups[n];
		p_roomgroups[roomgroup_id]->add_room(p_room);
	}

	p_room->update_gizmo();
}

void RoomManager::_second_pass_rooms(const LocalVector<RoomGroup *> &p_roomgroups) {
	for (int n = 0; n < _rooms.size(); n++) {
		_second_pass_room(_rooms[n], p_roomgroups);
	}
}

void RoomManager::_second_pass_portals(Spatial *p_roomlist, LocalVector<Portal *> &r_portals) {
	convert_log("_second_pass_portals");

	for (unsigned int n = 0; n < r_portals.size(); n++) {
		Portal *pPortal = r_portals[n];
		String szLinkRoom = _find_name_after(pPortal, "Portal_");
		szLinkRoom = "Room_" + szLinkRoom;

		Room *pLinkedRoom = Object::cast_to<Room>(p_roomlist->find_node(szLinkRoom, true, false));
		if (pLinkedRoom) {
			NodePath path = pPortal->get_path_to(pLinkedRoom);
			pPortal->set_linked_room_internal(path);
		} else {
			convert_log("\tlink_room : " + szLinkRoom + "\tnot found.");
		}

		// get the room we are linking from
		int room_from_id = pPortal->_linkedroom_ID[0];
		if (room_from_id != -1) {
			Room *room_from = _rooms[room_from_id];
			pPortal->resolve_links(room_from->_room_rid);
		}
	}
}

void RoomManager::_convert_rooms_recursive(Spatial *p_node, LocalVector<Portal *> &portals, LocalVector<RoomGroup *> &r_roomgroups, int p_roomgroup) {
	// is this a room?
	if (_name_starts_with(p_node, "Room_") || _node_is_type<Room>(p_node)) {
		_convert_room(p_node, portals, p_roomgroup);
	}

	// is this a roomgroup?
	if (_name_starts_with(p_node, "RoomGroup_") || _node_is_type<RoomGroup>(p_node)) {
		p_roomgroup = _convert_roomgroup(p_node, r_roomgroups);
	}

	// recurse through children
	for (int n = 0; n < p_node->get_child_count(); n++) {
		Spatial *child = Object::cast_to<Spatial>(p_node->get_child(n));

		if (child) {
			_convert_rooms_recursive(child, portals, r_roomgroups, p_roomgroup);
		}
	}
}

int RoomManager::_convert_roomgroup(Spatial *p_node, LocalVector<RoomGroup *> &r_roomgroups) {
	String szFullName = p_node->get_name();

	// is it already a roomgroup?
	RoomGroup *roomgroup = Object::cast_to<RoomGroup>(p_node);

	// if not already a RoomGroup, convert the node and move all children
	if (!roomgroup) {
		// create a RoomGroup
		roomgroup = _change_node_type<RoomGroup>(p_node, "G");
	} else {
		// already hit this tick?
		if (roomgroup->_conversion_tick == _conversion_tick) {
			return roomgroup->_roomgroup_ID;
		}
	}

	convert_log("convert_roomgroup : " + szFullName);

	// make sure the roomgroup is blank, especially if already created
	roomgroup->clear();

	// make sure the object ID is sent to the visual server
	VisualServer::get_singleton()->roomgroup_prepare(roomgroup->_room_group_rid, roomgroup->get_instance_id());

	// mark so as only to convert once
	roomgroup->_conversion_tick = _conversion_tick;

	roomgroup->_roomgroup_ID = r_roomgroups.size();

	r_roomgroups.push_back(roomgroup);

	return r_roomgroups.size() - 1;
}

void RoomManager::_convert_room(Spatial *p_node, LocalVector<Portal *> &portals, int p_roomgroup) {
	String szFullName = p_node->get_name();

	//	String szRoom = find_name_after(p_node, "room_");

	// is it already an lroom?
	Room *room = Object::cast_to<Room>(p_node);

	// if not already a Room, convert the node and move all children
	if (!room) {
		// create a Room
		room = _change_node_type<Room>(p_node, "G");
	} else {
		// already hit this tick?
		if (room->_conversion_tick == _conversion_tick) {
			return;
		}
	}

	convert_log("convert_room : " + szFullName);

	// make sure the room is blank, especially if already created
	room->clear();

	// mark so as only to convert once
	room->_conversion_tick = _conversion_tick;

	// set roomgroup
	if (p_roomgroup != -1) {
		room->_roomgroups.push_back(p_roomgroup);
	}

	// add to the list of rooms
	room->_room_ID = _rooms.size();
	_rooms.push_back(room);

	for (int n = 0; n < room->get_child_count(); n++) {
		MeshInstance *child = Object::cast_to<MeshInstance>(room->get_child(n));

		if (child) {
			if (_name_starts_with(child, "Portal_") || _node_is_type<Portal>(child)) {
				_convert_portal(room, child, portals);
			}
		}
	}
}

void RoomManager::_add_portal_points(Spatial *p_node, Vector<Vector3> &r_room_pts) {
	// if it is already a portal, it could have conversion to bound switched off
	Portal *portal = Object::cast_to<Portal>(p_node);

	// only pre-converted portals that are marked to include are turned on for this currently.
	// todo - some name hack to support loading this info from blender etc.
	if (!(portal && portal->get_include_in_bound_enabled())) {
		return;
	}

	MeshInstance *mi = Object::cast_to<MeshInstance>(p_node);
	if (!mi) {
		return; // should not happen, but just in case
	}

	AABB aabb;
	_bound_findpoints(mi, r_room_pts, aabb);
}

void RoomManager::_find_statics_recursive(Room *p_room, Spatial *p_node, Vector<Vector3> &r_room_pts) {
	bool ignore = false;
	VisualInstance *vi = Object::cast_to<VisualInstance>(p_node);

	// we are only interested in VIs with static or dynamic mode
	if (vi) {
		switch (vi->get_portal_mode()) {
			default: {
				ignore = true;
			} break;
			case CullInstance::PORTAL_MODE_DYNAMIC:
			case CullInstance::PORTAL_MODE_STATIC:
				break;
		}
	}

	if (!ignore) {
		// lights
		Light *light = Object::cast_to<Light>(p_node);
		if (light) {
			convert_log("\tfound light " + light->get_name());

			Vector<Vector3> dummy_pts;
			VisualServer::get_singleton()->room_add_instance(p_room->_room_rid, light->get_instance(), light->get_transformed_aabb(), dummy_pts);
		}

		MeshInstance *mi = Object::cast_to<MeshInstance>(p_node);
		if (mi) {
			convert_log("\tfound mesh " + mi->get_name());

			// static
			Vector<Vector3> object_pts;

			AABB aabb;
			// get the object points and don't immediately add to the room
			// points, as we want to use these points for sprawling algorithm in
			// the visual server.
			if (_bound_findpoints(mi, object_pts, aabb)) {
				// need to keep track of room bound
				r_room_pts.append_array(object_pts);

				VisualServer::get_singleton()->room_add_instance(p_room->_room_rid, mi->get_instance(), mi->get_transformed_aabb(), object_pts);
			} // if bound found points
		}
	} // if not ignore

	for (int n = 0; n < p_node->get_child_count(); n++) {
		Spatial *child = Object::cast_to<Spatial>(p_node->get_child(n));

		_find_statics_recursive(p_room, child, r_room_pts);
	}
}

bool RoomManager::_convert_manual_bound(Room *p_room, Spatial *p_node) {
	MeshInstance *mi = Object::cast_to<MeshInstance>(p_node);
	if (!mi) {
		return false;
	}

	Vector<Vector3> points;
	AABB aabb;
	if (!_bound_findpoints(mi, points, aabb)) {
		return false;
	}

	mi->set_portal_mode(CullInstance::PORTAL_MODE_IGNORE);

	// hide bounds after conversion
	// set to portal mode ignore?
	mi->hide();

	return _convert_room_hull(p_room, points);
}

bool RoomManager::_convert_room_hull(Room *p_room, const Vector<Vector3> &p_room_pts) {
	if (p_room_pts.size() > 3) {
		Geometry::MeshData md;

		Error err = OK;

		// if there are too many room points, quickhull will fail or freeze etc, so we will revert
		// to a bounding rect and send an error message
		if (p_room_pts.size() > 100000) {
			WARN_PRINT(String(p_room->get_name()) + " contains too many vertices to find convex hull, use a manual bound instead.");

			AABB aabb;
			aabb.create_from_points(p_room_pts);

			LocalVector<Vector3> pts;
			Vector3 mins = aabb.position;
			Vector3 maxs = mins + aabb.size;

			pts.push_back(Vector3(mins.x, mins.y, mins.z));
			pts.push_back(Vector3(mins.x, maxs.y, mins.z));
			pts.push_back(Vector3(maxs.x, maxs.y, mins.z));
			pts.push_back(Vector3(maxs.x, mins.y, mins.z));
			pts.push_back(Vector3(mins.x, mins.y, maxs.z));
			pts.push_back(Vector3(mins.x, maxs.y, maxs.z));
			pts.push_back(Vector3(maxs.x, maxs.y, maxs.z));
			pts.push_back(Vector3(maxs.x, mins.y, maxs.z));

			err = QuickHull::build(pts, md);
		} else {
			err = QuickHull::build(p_room_pts, md);
		}

		if (err == OK) {
			// get the planes
			for (int n = 0; n < md.faces.size(); n++) {
				const Plane &p = md.faces[n].plane;
				_add_plane_if_unique(p_room->_planes, p);
			}

			convert_log("\t\t\tcontained " + itos(p_room->_planes.size()) + " planes.");

			// make a copy of the mesh data for debugging
			// note this could be avoided in release builds? NYI
			p_room->_bound_mesh_data = md;

			// aabb (should actually include portals too, NYI)
			if (p_room_pts.size()) {
				p_room->_aabb.position = p_room_pts[0];
				p_room->_aabb.size = Vector3(0, 0, 0);
			}

			for (int n = 1; n < p_room_pts.size(); n++) {
				p_room->_aabb.expand_to(p_room_pts[n]);
			}

			// send bound to visual server
			VisualServer::get_singleton()->room_set_bound(p_room->_room_rid, p_room->get_instance_id(), p_room->_planes, p_room->_aabb);

			return true;
		}
	}

	return false;
}

void RoomManager::_add_plane_if_unique(LocalVector<Plane, int32_t> &p_planes, const Plane &p) {
	for (int n = 0; n < p_planes.size(); n++) {
		const Plane &o = p_planes[n];

		// this is a fudge factor for how close planes can be to be considered the same ...
		// to prevent ridiculous amounts of planes
		const real_t d = _plane_simplify_dist; // 0.08f

		if (Math::abs(p.d - o.d) > d) {
			continue;
		}

		real_t dot = p.normal.dot(o.normal);
		if (dot < _plane_simplify_dot) // 0.98f
		{
			continue;
		}

		// match!
		return;
	}

	// test
	//	Vector3 va(1, 0, 0);
	//	Vector3 vb(1, 0.2, 0);
	//	vb.normalize();
	//	real_t dot = va.dot(vb);
	//	print("va dot vb is " + String(Variant(dot)));

	// is unique
	//	print("\t\t\t\tAdding bound plane : " + p);

	p_planes.push_back(p);
}

void RoomManager::_convert_portal(Room *p_room, MeshInstance *pMI, LocalVector<Portal *> &portals) {
	String szFullName = pMI->get_name();
	convert_log("convert_portal : " + szFullName);

	Portal *portal = Object::cast_to<Portal>(pMI);

	// if not a gportal already, convert the node type
	if (!portal) {
		portal = _change_node_type<Portal>(pMI, "G", false);

		// copy mesh
		portal->set_mesh(pMI->get_mesh());
		if (pMI->get_surface_material_count()) {
			portal->set_surface_material(0, pMI->get_surface_material(0));
		}

		pMI->queue_delete();
	} else {
		// only allow converting once
		if (portal->_conversion_tick == _conversion_tick) {
			return;
		}
	}

	// create from the ID of the room the portal leads from
	//portal->create(p_room->_room_ID);

	// mark so as only to convert once
	portal->_conversion_tick = _conversion_tick;

	// link rooms
	portal->portal_update_full();

	// keep a list of portals for second pass
	portals.push_back(portal);

	// the portal  is linking from this first room it is added to
	portal->_linkedroom_ID[0] = p_room->_room_ID;
}

bool RoomManager::_bound_findpoints(MeshInstance *p_mi, Vector<Vector3> &r_room_pts, AABB &r_aabb) {
	// max opposite extents .. note AABB storing size is rubbish in this aspect
	// it can fail once mesh min is larger than FLT_MAX / 2.
	r_aabb.position = Vector3(FLT_MAX / 2, FLT_MAX / 2, FLT_MAX / 2);
	r_aabb.size = Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	// some godot jiggery pokery to get the mesh verts in local space
	Ref<Mesh> rmesh = p_mi->get_mesh();

	ERR_FAIL_COND_V(!rmesh.is_valid(), false);

	if (rmesh->get_surface_count() == 0) {
		String sz;
		sz = "MeshInstance '" + p_mi->get_name() + "' has no surfaces, ignoring";
		WARN_PRINT(sz);
		return false;
	}

	bool success = false;

	// for converting meshes to world space
	Transform trans = p_mi->get_global_transform();

	for (int surf = 0; surf < rmesh->get_surface_count(); surf++) {
		Array arrays = rmesh->surface_get_arrays(surf);

		// possible to have a meshinstance with no geometry .. don't want to crash
		if (!arrays.size()) {
			WARN_PRINT_ONCE("PConverter::bound_findpoints MeshInstance surface with no mesh, ignoring");
			continue;
		}

		success = true;

		PoolVector<Vector3> vertices = arrays[VS::ARRAY_VERTEX];

		// convert to world space
		for (int n = 0; n < vertices.size(); n++) {
			Vector3 ptWorld = trans.xform(vertices[n]);
			r_room_pts.push_back(ptWorld);

			// keep the bound up to date
			r_aabb.expand_to(ptWorld);
		}

	} // for through the surfaces

	return success;
}

bool RoomManager::resolve_camera_path() {
	Camera *camera = _resolve_path<Camera>(_settings_path_camera);

	if (camera) {
		_godot_camera_ID = camera->get_instance_id();
		return true;
	}
	_godot_camera_ID = -1;
	return false;
}

template <class NODE_TYPE>
NODE_TYPE *RoomManager::_resolve_path(NodePath p_path) const {
	if (has_node(p_path)) {
		NODE_TYPE *node = Object::cast_to<NODE_TYPE>(get_node(p_path));
		if (node) {
			return node;
		} else {
			WARN_PRINT("node is incorrect type");
		}
	}

	return nullptr;
}

template <class NODE_TYPE>
bool RoomManager::_node_is_type(Spatial *p_node) const {
	NODE_TYPE *node = Object::cast_to<NODE_TYPE>(p_node);
	return node != nullptr;
}

template <class T>
T *RoomManager::_change_node_type(Spatial *p_node, String p_prefix, bool p_delete) {
	String szFullName = p_node->get_name();

	Node *parent = p_node->get_parent();
	if (!parent) {
		return nullptr;
	}

	// owner should normally be root
	Node *owner = p_node->get_owner();

	// change the name of the node to be deleted
	p_node->set_name(p_prefix + szFullName);

	// create the new class T object
	T *pNew = memnew(T);
	pNew->set_name(szFullName);

	// add the child at the same position as the old node
	// (this is more convenient for users)
	parent->add_child_below_node(p_node, pNew);

	// new lroom should have same transform
	pNew->set_transform(p_node->get_transform());

	// move each child
	while (p_node->get_child_count()) {
		Node *child = p_node->get_child(0);
		p_node->remove_child(child);

		// needs to set owner to appear in IDE
		pNew->add_child(child);
	}

	// needs to set owner to appear in IDE
	_set_owner_recursive(pNew, owner);

	// delete old node
	if (p_delete) {
		p_node->queue_delete();
	}

	return pNew;
}

void RoomManager::_set_show_debug_recursive(Node *p_node, bool p_show) {
	Room *child = Object::cast_to<Room>(p_node);

	if (child) {
		child->set_show_debug(p_show);
	}

	for (int n = 0; n < p_node->get_child_count(); n++) {
		_set_show_debug_recursive(p_node->get_child(n), p_show);
	}
}

void RoomManager::_set_owner_recursive(Node *p_node, Node *p_owner) {
	if (p_node != p_owner) {
		p_node->set_owner(p_owner);
	}
	for (int n = 0; n < p_node->get_child_count(); n++) {
		_set_owner_recursive(p_node->get_child(n), p_owner);
	}
}

bool RoomManager::_name_starts_with(const Node *p_node, String p_search_string) const {
	int sl = p_search_string.length();

	String name = p_node->get_name();
	int l = name.length();

	if (l < sl) {
		return false;
	}

	String szStart = name.substr(0, sl);

	if (szStart == p_search_string) {
		return true;
	}

	return false;
}

String RoomManager::_find_name_after(Node *p_node, String p_string_start) {
	String szRes;
	String name = p_node->get_name();
	szRes = name.substr(p_string_start.length());

	// because godot doesn't support multiple nodes with the same name, we will strip e.g. a number
	// after an @ on the end of the name...
	// e.g. portal_kitchen@2
	for (int c = 0; c < szRes.length(); c++) {
		if (szRes[c] == '*') {
			// remove everything after and including this character
			szRes = szRes.substr(0, c);
			break;
		}
	}

	return szRes;
}

void RoomManager::_merge_meshes_in_room(Room *p_room) {
	// only do in running game so as not to lose data
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}

	_merge_log("merging room " + p_room->get_name());

	// list of meshes suitable
	LocalVector<MeshInstance *, int32_t> source_meshes;
	_list_mergeable_mesh_instances(p_room, source_meshes);

	// none suitable
	if (!source_meshes.size()) {
		return;
	}

	_merge_log("\t" + itos(source_meshes.size()) + " source meshes");

	BitFieldDynamic bf;
	bf.create(source_meshes.size(), true);

	for (int n = 0; n < source_meshes.size(); n++) {
		LocalVector<MeshInstance *, int32_t> merge_list;

		// find similar meshes
		MeshInstance *a = source_meshes[n];
		merge_list.push_back(a);

		// may not be necessary
		bf.set_bit(n, true);

		for (int c = n + 1; c < source_meshes.size(); c++) {
			// if not merged already
			if (!bf.get_bit(c)) {
				MeshInstance *b = source_meshes[c];

				//				if (_are_meshes_mergeable(a, b)) {
				if (a->is_mergeable_with(*b)) {
					merge_list.push_back(b);
					bf.set_bit(c, true);
				}
			} // if not merged already
		} // for c through secondary mesh

		// only merge if more than 1
		if (merge_list.size() > 1) {
			// we can merge!
			// create a new holder mesh

			MeshInstance *merged = memnew(MeshInstance);
			merged->set_name("MergedMesh");

			_merge_log("\t\t" + merged->get_name());

			if (merged->create_by_merging(merge_list)) {
				// set all the source meshes to portal mode ignore so not shown
				for (int i = 0; i < merge_list.size(); i++) {
					merge_list[i]->set_portal_mode(CullInstance::PORTAL_MODE_IGNORE);
				}

				// and set the new merged mesh to static
				merged->set_portal_mode(CullInstance::PORTAL_MODE_STATIC);

				// attach to scene tree
				p_room->add_child(merged);
				merged->set_owner(p_room->get_owner());

				// compensate for room transform, as the verts are now in world space
				Transform tr = p_room->get_global_transform();
				tr.affine_invert();
				merged->set_transform(tr);

				// delete originals?
				// note this isn't perfect, it may still end up with dangling spatials, but they can be
				// deleted later.
				for (int i = 0; i < merge_list.size(); i++) {
					MeshInstance *mi = merge_list[i];
					if (!mi->get_child_count()) {
						mi->queue_delete();
					} else {
						Node *parent = mi->get_parent();
						if (parent) {
							// if there are children, we don't want to delete it, but we do want to
							// remove the mesh drawing, e.g. by replacing it with a spatial
							String name = mi->get_name();
							mi->set_name("DeleteMe"); // can be anything, just to avoid name conflict with replacement node
							Spatial *replacement = memnew(Spatial);
							replacement->set_name(name);

							parent->add_child(replacement);

							// make the transform and owner match
							replacement->set_owner(mi->get_owner());
							replacement->set_transform(mi->get_transform());

							// move all children from the mesh instance to the replacement
							while (mi->get_child_count()) {
								Node *child = mi->get_child(0);
								mi->remove_child(child);
								replacement->add_child(child);
							}

						} // if the mesh instance has a parent (should hopefully be always the case?)
					}
				}

			} else {
				// no success
				memdelete(merged);
			}
		}

	} // for n through primary mesh

	if (_settings_remove_danglers) {
		_remove_redundant_dangling_nodes(p_room);
	}
}

bool RoomManager::_remove_redundant_dangling_nodes(Spatial *p_node) {
	int non_queue_delete_children = 0;

	// do the children first
	for (int n = 0; n < p_node->get_child_count(); n++) {
		Node *node_child = p_node->get_child(n);

		Spatial *child = Object::cast_to<Spatial>(node_child);
		if (child) {
			_remove_redundant_dangling_nodes(child);
		}

		if (node_child && !node_child->is_queued_for_deletion()) {
			non_queue_delete_children++;
		}
	}

	if (!non_queue_delete_children) {
		// only remove true spatials, not derived classes
		if (p_node->get_class_name() == "Spatial") {
			p_node->queue_delete();
			return true;
		}
	}

	return false;
}

void RoomManager::_list_mergeable_mesh_instances(Spatial *p_node, LocalVector<MeshInstance *, int32_t> &r_list) {
	MeshInstance *mi = Object::cast_to<MeshInstance>(p_node);

	if (mi) {
		// only interested in static portal mode meshes
		VisualInstance *vi = Object::cast_to<VisualInstance>(mi);

		// we are only interested in VIs with static or dynamic mode
		if (vi && vi->get_portal_mode() == CullInstance::PORTAL_MODE_STATIC) {
			// disallow for portals or bounds
			if (!_node_is_type<Portal>(mi) && !_name_starts_with(mi, "Bound_")) {
				// only merge if visible
				if (mi->is_visible_in_tree()) {
					r_list.push_back(mi);
				}
			}
		}
	}

	for (int n = 0; n < p_node->get_child_count(); n++) {
		Spatial *child = Object::cast_to<Spatial>(p_node->get_child(n));
		if (child) {
			_list_mergeable_mesh_instances(child, r_list);
		}
	}
}
