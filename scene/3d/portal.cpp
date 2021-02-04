/*************************************************************************/
/*  portal.cpp                                                           */
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

#include "portal.h"
#include "core/engine.h"
#include "room.h"
#include "room_manager.h"
#include "scene/main/viewport.h"
#include "servers/visual_server.h"

bool Portal::_portal_plane_convention = false;

Portal::Portal() {
	clear();

	// the visual server portal lifetime is linked to the lifetime of this object
	_portal_rid = VisualServer::get_singleton()->portal_create();

#ifdef TOOLS_ENABLED
	_room_manager_godot_ID = 0;
#endif
}

Portal::~Portal() {
	if (_portal_rid != RID()) {
		VisualServer::get_singleton()->free(_portal_rid);
	}
}

// extra editor links to the room manager to allow unloading
// on change, or re-converting
void Portal::_changed() {
#ifdef TOOLS_ENABLED
	RoomManager *rm = _find_room_manager();
	if (!rm) {
		return;
	}

	rm->_rooms_changed();
#endif
}

#ifdef TOOLS_ENABLED
RoomManager *Portal::_find_room_manager() {
	RoomManager *rm = Object::cast_to<RoomManager>(ObjectDB::get_instance(_room_manager_godot_ID));
	if (rm) {
		return rm;
	}

	// try and find it... this is slow
	Node *parent = get_parent();

	while (parent) {
		// check this
		rm = Object::cast_to<RoomManager>(parent);
		if (rm) {
			break;
		}

		// check children
		for (int n = 0; n < parent->get_child_count(); n++) {
			rm = Object::cast_to<RoomManager>(parent->get_child(n));
			if (rm) {
				break;
			}
		}

		if (rm) {
			break;
		}

		parent = parent->get_parent();
	}

	return rm;
}
#endif

void Portal::clear() {
	_settings_active = true;
	_settings_include_in_bound = false;
	_linkedroom_ID[0] = -1;
	_linkedroom_ID[1] = -1;
	_pts_world.clear();
	_pt_centre_world = Vector3();
	_plane = Plane();
	_margin = 1.0f;
	_default_margin = 1.0f;
	_use_default_margin = true;
}

void Portal::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_WORLD: {
			ERR_FAIL_COND(get_world().is_null());

			// defer full creation of the visual server portal to when the editor portal is in the scene tree
			VisualServer::get_singleton()->portal_set_scenario(_portal_rid, get_world()->get_scenario());
		} break;
		case NOTIFICATION_EXIT_WORLD: {
			// partially destroy  the visual server portal when the editor portal exits the scene tree
			VisualServer::get_singleton()->portal_set_scenario(_portal_rid, RID());
		} break;
		case NOTIFICATION_TRANSFORM_CHANGED: {
			// editor only, to help place portals.
			// explicitly call portal_update_transform() in release,
			// for more efficiency as NOTIFICATION_TRANSFORM_CHANGED could
			// be called in housekeeping.
#ifdef TOOLS_ENABLED
			if (Engine::get_singleton()->is_editor_hint()) {
				portal_update_transform();
			}
#endif
		} break;
	}
}

void Portal::set_portal_active(bool p_active) {
	_settings_active = p_active;
	VisualServer::get_singleton()->portal_set_active(_portal_rid, p_active);
}

bool Portal::get_portal_active() const {
	return _settings_active;
}

void Portal::set_include_in_bound_enabled(bool p_enabled) {
	if (_settings_include_in_bound == p_enabled) {
		return;
	}
	_settings_include_in_bound = p_enabled;
	_changed();
}

bool Portal::get_include_in_bound_enabled() const {
	return _settings_include_in_bound;
}

void Portal::set_use_default_margin(bool p_use) {
	_use_default_margin = p_use;
	update_gizmo();
}

bool Portal::get_use_default_margin() const {
	return _use_default_margin;
}

void Portal::set_portal_margin(real_t p_margin) {
	_margin = p_margin;

	if (!_use_default_margin) {
		// give visual feedback in the editor for the portal margin zone
		update_gizmo();
	}
}

real_t Portal::get_portal_margin() const {
	return _margin;
}

void Portal::resolve_links(const RID &p_from_room_rid) {
	Room *linkedroom = nullptr;
	if (has_node(_settings_path_linkedroom)) {
		linkedroom = Object::cast_to<Room>(get_node(_settings_path_linkedroom));
	}

	if (linkedroom) {
		//_linkedroom_godot_ID = linkedroom->get_instance_id();
		_linkedroom_ID[1] = linkedroom->_room_ID;

		// send to visual server
		VisualServer::get_singleton()->portal_link(_portal_rid, p_from_room_rid, linkedroom->_room_rid, true);
	} else {
		_linkedroom_ID[1] = -1;
	}
}

void Portal::set_linked_room_internal(const NodePath &link_path) {
	_settings_path_linkedroom = link_path;
}

bool Portal::try_set_unique_name(const String &p_name) {
	SceneTree *scene_tree = get_tree();
	if (!scene_tree) {
		// should not happen in the editor
		return false;
	}

	Viewport *root = scene_tree->get_root();
	if (!root) {
		return false;
	}

	Node *found = root->find_node(p_name, true, false);

	// if the name does not already exist in the scene tree, we can use it
	if (!found) {
		set_name(p_name);
		return true;
	}

	// we are trying to set the same name this node already has...
	if (found == this) {
		// noop
		return true;
	}

	return false;
}

void Portal::set_linked_room(const NodePath &link_path) {
	// change the name of the portal as well, if the link looks legit
	Room *linkedroom = nullptr;
	if (has_node(link_path)) {
		linkedroom = Object::cast_to<Room>(get_node(link_path));

		if (linkedroom) {
			if (linkedroom != get_parent()) {
				_settings_path_linkedroom = link_path;

				// change the portal name
				String szLinkRoom = RoomManager::_find_name_after(linkedroom, "room_");

				// we need a unique name for the portal
				String szNameBase = "portal_" + szLinkRoom;
				if (!try_set_unique_name(szNameBase)) {
					bool success = false;
					for (int n = 0; n < 128; n++) {
						String szName = szNameBase + "*" + itos(n);
						if (try_set_unique_name(szName)) {
							success = true;
							_changed();
							break;
						}
					}

					if (!success) {
						WARN_PRINT("Could not set portal name, set name manually instead.");
					}
				} else {
					_changed();
				}
			} else {
				WARN_PRINT("Linked room cannot be portal's parent room, ignoring.");
			}
		} else {
			WARN_PRINT("Linked room path is not a room, ignoring.");
		}
	} else {
		WARN_PRINT("Linked room path not found.");
	}
}

NodePath Portal::get_linked_room() const {
	return _settings_path_linkedroom;
}

void Portal::portal_update_full() {
	_pts_model.clear();

	// some godot jiggery pokery to get the mesh verts in local space
	Ref<Mesh> rmesh = get_mesh();

	ERR_FAIL_COND(!rmesh.is_valid());

	if (rmesh->get_surface_count() == 0) {
		String sz;
		sz = "Portal '" + get_name() + "' has no surfaces, ignoring";
		WARN_PRINT(sz);
		return;
	}

	Array arrays = rmesh->surface_get_arrays(0);
	PoolVector<Vector3> vertices = arrays[VS::ARRAY_VERTEX];

	// get the model space verts and find centre
	int nPoints = vertices.size();
	ERR_FAIL_COND(nPoints < 3);

	for (int n = 0; n < nPoints; n++) {
		const Vector3 &pt = vertices[n];

		// test for duplicates.
		// Some geometry may contain duplicate verts in portals
		// which will muck up the winding etc...
		bool bDuplicate = false;
		for (int m = 0; m < _pts_model.size(); m++) {
			Vector3 ptDiff = pt - _pts_model[m];
			// hopefully this epsilon will do in nearly all cases
			if (ptDiff.length() < 0.001f) {
				bDuplicate = true;
				break;
			}
		}

		if (!bDuplicate) {
			_pts_model.push_back(pt);
		}
	}

	portal_update_transform();
}

void Portal::portal_update_transform() {
	_pts_world.clear();
	_pt_centre_world = Vector3();
	_plane = Plane();

	// world space
	int nPoints = _pts_model.size();
	ERR_FAIL_COND(nPoints < 3);

	Transform trans = get_global_transform();
	for (int n = 0; n < nPoints; n++) {
		Vector3 ptWorld = trans.xform(_pts_model[n]);
		_pts_world.push_back(ptWorld);
	}

	sort_verts_clockwise(_portal_plane_convention);
	plane_from_points();

	// a bit of a bodge, but a small epsilon pulling in the portal edges towards the centre
	// can hide walls in the opposite room that abutt the portal (due to real_ting point error)
	const real_t pull_in = 0.0001;

	for (int n = 0; n < _pts_world.size(); n++) {
		Vector3 offset = _pts_world[n] - _pt_centre_world;
		real_t l = offset.length();

		// don't apply the pull in for tiny holes
		if (l > (pull_in * 2.0f)) {
			real_t fract = (l - pull_in) / l;
			offset *= fract;
			_pts_world.set(n, _pt_centre_world + offset);
		}
	}

	// extension margin to prevent objects too easily sprawling
	real_t margin = get_active_portal_margin();
	VisualServer::get_singleton()->portal_set_geometry(_portal_rid, _pts_world, margin);
}

real_t Portal::get_active_portal_margin() const {
	if (_use_default_margin) {
		return _default_margin;
	}
	return _margin;
}

void Portal::sort_verts_clockwise(bool portal_plane_convention) {
	Vector<Vector3> &verts = _pts_world;

	// We first assumed first 3 determine the desired normal

	// find normal
	Plane plane;
	if (portal_plane_convention) {
		plane = Plane(verts[0], verts[2], verts[1]);
	} else {
		plane = Plane(verts[0], verts[1], verts[2]);
	}

	Vector3 ptNormal = plane.normal;

	// find centroid
	int nPoints = verts.size();

	_pt_centre_world = Vector3(0, 0, 0);

	for (int n = 0; n < nPoints; n++) {
		_pt_centre_world += verts[n];
	}
	_pt_centre_world /= nPoints;

	// now algorithm
	for (int n = 0; n < nPoints - 2; n++) {
		Vector3 a = verts[n] - _pt_centre_world;
		a.normalize();

		Plane p = Plane(verts[n], _pt_centre_world, _pt_centre_world + ptNormal);

		double SmallestAngle = -1;
		int Smallest = -1;

		for (int m = n + 1; m < nPoints; m++) {
			if (p.distance_to(verts[m]) > 0.0f) {
				Vector3 b = verts[m] - _pt_centre_world;
				b.normalize();

				double Angle = a.dot(b);

				if (Angle > SmallestAngle) {
					SmallestAngle = Angle;
					Smallest = m;
				}
			} // which side

		} // for m

		// swap smallest and n+1 vert
		if (Smallest != -1) {
			Vector3 temp = verts[Smallest];
			verts.set(Smallest, verts[n + 1]);
			verts.set(n + 1, temp);
		}
	} // for n

	// the vertices are now sorted, but may be in the opposite order to that wanted.
	// we detect this by calculating the normal of the poly, then flipping the order if the normal is pointing
	// the wrong way.
	plane = Plane(verts[0], verts[1], verts[2]);

	if (ptNormal.dot(plane.normal) < 0.0f) {
		// reverse order of verts
		reverse_winding_order();
	}
}

void Portal::reverse_winding_order() {
	Vector<Vector3> &verts = _pts_world;
	Vector<Vector3> copy = verts;

	for (int n = 0; n < verts.size(); n++) {
		verts.set(n, copy[verts.size() - n - 1]);
	}
}

void Portal::plane_from_points() {
	ERR_FAIL_COND_MSG((_pts_world.size() < 3), "Portal must have at least 3 vertices");

	// create plane from points
	_plane = Plane(_pts_world[0], _pts_world[1], _pts_world[2]);
}

void Portal::debug_check_plane_validity(const Plane &p) const {
	// portal planes should point inward? by convention
	ERR_FAIL_COND(p.distance_to(_pt_centre_world) >= 0.0f);
}

void Portal::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_portal_active", "p_active"), &Portal::set_portal_active);
	ClassDB::bind_method(D_METHOD("get_portal_active"), &Portal::get_portal_active);

	ClassDB::bind_method(D_METHOD("set_include_in_bound_enabled", "p_enable"), &Portal::set_include_in_bound_enabled);
	ClassDB::bind_method(D_METHOD("get_include_in_bound_enabled"), &Portal::get_include_in_bound_enabled);

	ClassDB::bind_method(D_METHOD("set_use_default_margin", "p_use"), &Portal::set_use_default_margin);
	ClassDB::bind_method(D_METHOD("get_use_default_margin"), &Portal::get_use_default_margin);

	ClassDB::bind_method(D_METHOD("set_portal_margin", "p_margin"), &Portal::set_portal_margin);
	ClassDB::bind_method(D_METHOD("get_portal_margin"), &Portal::get_portal_margin);

	ClassDB::bind_method(D_METHOD("set_linked_room", "p_room"), &Portal::set_linked_room);
	ClassDB::bind_method(D_METHOD("get_linked_room"), &Portal::get_linked_room);

	// use for moving portal dynamically. Note this won't affect sprawling, but will work in most situations
	// use if mesh geometry has changed, no need to call portal_update_transform as it is called implicitly
	ClassDB::bind_method(D_METHOD("portal_update_full"), &Portal::portal_update_full);
	// use if only the transform has changed
	ClassDB::bind_method(D_METHOD("portal_update_transform"), &Portal::portal_update_transform);

	ADD_GROUP("Portal", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "portal_active"), "set_portal_active", "get_portal_active");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "linked_room"), "set_linked_room", "get_linked_room");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_default_margin"), "set_use_default_margin", "get_use_default_margin");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "portal_margin", PROPERTY_HINT_RANGE, "0.0,10.0,0.01"), "set_portal_margin", "get_portal_margin");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "include_in_bound"), "set_include_in_bound_enabled", "get_include_in_bound_enabled");
}
