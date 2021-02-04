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
#include "core/engine.h"
#include "core/math/quick_hull.h"
#include "portal.h"
#include "room.h"
#include "scene/3d/camera.h"
#include "scene/3d/light.h"

RoomManager::RoomManager() {
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
			if (!is_inside_world())
				return;

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
					}
					// check planes
					if (!changed) {
						if (planes.size() != _godot_camera_planes.size())
							changed = true;
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
						VisualServer::get_singleton()->rooms_override_camera(get_world()->get_scenario(), true, camera_pos, &planes);
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

	ClassDB::bind_method(D_METHOD("set_pvs_filename", "pvs_filename"), &RoomManager::set_pvs_filename);
	ClassDB::bind_method(D_METHOD("get_pvs_filename"), &RoomManager::get_pvs_filename);

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
	ADD_PROPERTY(PropertyInfo(Variant::INT, "pvs_mode", PROPERTY_HINT_ENUM, "Disabled,Generate,Generate and Cull"), "set_pvs_mode", "get_pvs_mode");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "pvs_filename", PROPERTY_HINT_FILE, "*.pvs"), "set_pvs_filename", "get_pvs_filename");

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

void RoomManager::set_simplify_dist(float p_dist) {
	_plane_simplify_dist = p_dist;
}
float RoomManager::get_simplify_dist() const {
	return _plane_simplify_dist;
}

void RoomManager::set_simplify_dot(float p_dot) {
	_plane_simplify_dot = p_dot;
}
float RoomManager::get_simplify_dot() const {
	return _plane_simplify_dot;
}

void RoomManager::set_default_portal_margin(float p_dist) {
	_default_portal_margin = p_dist;

	// send to portals
	Spatial *roomlist = _resolve_path<Spatial>(_settings_path_roomlist);
	if (!roomlist) {
		return;
	}

	_update_portal_margins(roomlist, _default_portal_margin);
}

void RoomManager::_update_portal_margins(Spatial *p_node, float p_margin) {
	Portal *portal = Object::cast_to<Portal>(p_node);

	if (portal) {
		portal->_default_margin = p_margin;
	}

	// recurse
	for (int n = 0; n < p_node->get_child_count(); n++) {
		Spatial *child = Object::cast_to<Spatial>(p_node->get_child(n));

		if (child) {
			_update_portal_margins(child, p_margin);
		}
	}
}

float RoomManager::get_default_portal_margin() const {
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

	for (int n = 0; n < roomlist->get_child_count(); n++) {
		Room *child = Object::cast_to<Room>(roomlist->get_child(n));

		if (child) {
			child->set_show_debug(p_show);
		}
	}
}

bool RoomManager::get_show_debug() const {
	return _show_debug;
}

void RoomManager::set_debug_sprawl(bool p_active) {
	if (is_inside_world() && get_world().is_valid()) {
		VisualServer::get_singleton()->rooms_set_debug_feature(get_world()->get_scenario(), VisualServer::ROOMS_DEBUG_SPRAWL, p_active);
		_debug_sprawl = p_active;
	}
}

bool RoomManager::get_debug_sprawl() const {
	return _debug_sprawl;
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

	// find the rooms and portals
	_convert_rooms_recursive(roomlist, portals);

	// add portal links
	_second_pass_portals(roomlist, portals);

	// create the statics
	_second_pass_rooms();

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

void RoomManager::_second_pass_room(Room *p_room) {
	// find statics and manual bound
	bool manual_bound_found = false;

	// points making up the room geometry, in world space, to create the convex hull
	Vector<Vector3> room_pts;

	for (int n = 0; n < p_room->get_child_count(); n++) {
		Spatial *child = Object::cast_to<Spatial>(p_room->get_child(n));

		if (child) {
			if (_name_starts_with(child, "gportal_") || _node_is_type<Portal>(child)) {
				_add_portal_points(child, room_pts);
			} else if (_name_starts_with(child, "bound_")) {
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
}

void RoomManager::_second_pass_rooms() {
	for (int n = 0; n < _rooms.size(); n++) {
		_second_pass_room(_rooms[n]);
	}
}

void RoomManager::_second_pass_portals(Spatial *p_roomlist, LocalVector<Portal *> &r_portals) {
	convert_log("_second_pass_portals");

	for (unsigned int n = 0; n < r_portals.size(); n++) {
		Portal *pPortal = r_portals[n];
		String szLinkRoom = _find_name_after(pPortal, "portal_");
		szLinkRoom = "room_" + szLinkRoom;

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

void RoomManager::_convert_rooms_recursive(Spatial *p_node, LocalVector<Portal *> &portals) {
	// is this a room?
	if (_name_starts_with(p_node, "room_") || _node_is_type<Room>(p_node)) {
		_convert_room(p_node, portals);
	}

	// recurse through children
	for (int n = 0; n < p_node->get_child_count(); n++) {
		Spatial *child = Object::cast_to<Spatial>(p_node->get_child(n));

		if (child) {
			_convert_rooms_recursive(child, portals);
		}
	}
}

void RoomManager::_convert_room(Spatial *p_node, LocalVector<Portal *> &portals) {
	String szFullName = p_node->get_name();

	//	String szRoom = find_name_after(p_node, "room_");

	// owner should normally be root
	Node *owner = p_node->get_owner();

	// is it already an lroom?
	Room *room = Object::cast_to<Room>(p_node);

	// if not already a Room, convert the node and move all children
	if (!room) {
		// create a Room
		room = _change_node_type<Room>(p_node, "g");

		// move each child
		while (p_node->get_child_count()) {
			Node *child = p_node->get_child(0);
			p_node->remove_child(child);

			// needs to set owner to appear in IDE
			room->add_child(child);
			child->set_owner(owner);
		}
	} else {
		// already hit this tick?
		if (room->_conversion_tick == _conversion_tick)
			return;
	}

	convert_log("convert_room : " + szFullName);

	// make sure the room is blank, especially if already created
	room->clear();

	// mark so as only to convert once
	room->_conversion_tick = _conversion_tick;

	// add to the list of rooms
	room->_room_ID = _rooms.size();
	_rooms.push_back(room);

	for (int n = 0; n < room->get_child_count(); n++) {
		MeshInstance *child = Object::cast_to<MeshInstance>(room->get_child(n));

		if (child) {
			if (_name_starts_with(child, "portal_") || _node_is_type<Portal>(child)) {
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
	if (!mi)
		return; // should not happen, but just in case

	AABB aabb;
	_bound_findpoints(mi, r_room_pts, aabb);
}

void RoomManager::_find_statics_recursive(Room *p_room, Spatial *p_node, Vector<Vector3> &r_room_pts) {

	bool ignore = false;
	VisualInstance *vi = Object::cast_to<VisualInstance>(p_node);

	// we are only interested in VIs with static or dynamic mode
	if (vi) {
		switch (vi->get_culling_portal_mode()) {
			default: {
				ignore = true;
			} break;
			case VisualInstance::PORTAL_MODE_DYNAMIC:
			case VisualInstance::PORTAL_MODE_STATIC:
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
	if (!mi)
		return false;

	Vector<Vector3> points;
	AABB aabb;
	if (!_bound_findpoints(mi, points, aabb))
		return false;

	mi->set_culling_portal_mode(VisualInstance::PORTAL_MODE_IGNORE);

	// hide bounds after conversion
	// set to portal mode ignore?
	mi->hide();

	return _convert_room_hull(p_room, points);
}

bool RoomManager::_convert_room_hull(Room *p_room, const Vector<Vector3> &p_room_pts) {
	if (p_room_pts.size() > 3) {
		Geometry::MeshData md;
		Error err = QuickHull::build(p_room_pts, md);
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
			VisualServer::get_singleton()->room_set_bound(p_room->_room_rid, p_room->_planes, p_room->_aabb);

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
		const float d = _plane_simplify_dist; // 0.08f

		if (fabs(p.d - o.d) > d)
			continue;

		float dot = p.normal.dot(o.normal);
		if (dot < _plane_simplify_dot) // 0.98f
			continue;

		// match!
		return;
	}

	// test
	//	Vector3 va(1, 0, 0);
	//	Vector3 vb(1, 0.2, 0);
	//	vb.normalize();
	//	float dot = va.dot(vb);
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
		portal = _change_node_type<Portal>(pMI, "g", false);

		// copy mesh
		portal->set_mesh(pMI->get_mesh());
		if (pMI->get_surface_material_count())
			portal->set_surface_material(0, pMI->get_surface_material(0));

		pMI->queue_delete();
	} else {
		// only allow converting once
		if (portal->_conversion_tick == _conversion_tick)
			return;
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

	// owner should normally be root
	Node *owner = p_node->get_owner();

	// change the name of the node to be deleted
	p_node->set_name(p_prefix + szFullName);

	// create the new class T object
	T *pNew = memnew(T);
	//	pNew->set_name(p_prefix + szFullName);
	pNew->set_name(szFullName);

	// needs to set owner to appear in IDE
	p_node->get_parent()->add_child(pNew);
	pNew->set_owner(owner);

	// new lroom should have same transform
	pNew->set_transform(p_node->get_transform());

	// delete old node
	if (p_delete) {
		p_node->queue_delete();
	}

	return pNew;
}

bool RoomManager::_name_starts_with(const Node *p_node, String p_search_string) const {
	int sl = p_search_string.length();

	String name = p_node->get_name();
	int l = name.length();

	if (l < sl)
		return false;

	String szStart = name.substr(0, sl);

	if (szStart == p_search_string)
		return true;

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
