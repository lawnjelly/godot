/*************************************************************************/
/*  room.cpp                                                             */
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

#include "room.h"
#include "portal.h"
#include "servers/visual_server.h"

void Room::clear() {
	_room_ID = -1;
	_planes.clear();
	_roomgroups.clear();
	_bound_mesh_data.edges.clear();
	_bound_mesh_data.faces.clear();
	_bound_mesh_data.vertices.clear();
	_aabb = AABB();
}

Room::Room() {
	_room_rid = VisualServer::get_singleton()->room_create();
}

Room::~Room() {
	if (_room_rid != RID()) {
		VisualServer::get_singleton()->free(_room_rid);
	}
}

void Room::set_show_debug(bool p_show) {
	for (int n = 0; n < get_child_count(); n++) {
		Portal *child = Object::cast_to<Portal>(get_child(n));

		if (child) {
			child->set_visible(p_show);

			// as well as just making them visible / invisible we have to make this work if portal rendering is active
			if (p_show) {
				child->set_culling_portal_mode(VisualInstance::PortalMode::PORTAL_MODE_GLOBAL);
			} else {
				child->set_culling_portal_mode(VisualInstance::PortalMode::PORTAL_MODE_IGNORE);
			}
		}
	}
}

void Room::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_WORLD: {
			ERR_FAIL_COND(get_world().is_null());
			VisualServer::get_singleton()->room_set_scenario(_room_rid, get_world()->get_scenario());
		} break;
		case NOTIFICATION_EXIT_WORLD: {
			VisualServer::get_singleton()->room_set_scenario(_room_rid, RID());
		} break;
	}
}
