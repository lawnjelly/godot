/*************************************************************************/
/*  room.h                                                               */
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

#ifndef ROOM_H
#define ROOM_H

#include "core/local_vector.h"
#include "core/rid.h"
#include "spatial.h"

class Portal;

class Room : public Spatial {
	GDCLASS(Room, Spatial);

	friend class RoomManager;
	friend class RoomGroup;
	friend class Portal;
	friend class RoomManagerGizmoPlugin;

	RID _room_rid;

public:
	Room();
	~Room();

	// just used for showing / hiding portals for now
	void set_show_debug(bool p_show);

private:
	void clear();

	// planes forming convex hull of room
	LocalVector<Plane, int32_t> _planes;
	Geometry::MeshData _bound_mesh_data;
	AABB _aabb;

	// makes sure lrooms are not converted more than once per
	// call to rooms_convert
	int _conversion_tick = -1;

	// room ID during conversion, used for matching portals links to rooms
	int _room_ID;

	// a room may be in one or several roomgroups
	LocalVector<int, int32_t> _roomgroups;

protected:
	void _notification(int p_what);
};

#endif
