/*************************************************************************/
/*  room_manager_editor_plugin.h                                         */
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

#include "editor/editor_node.h"
#include "editor/editor_plugin.h"
#include "editor/spatial_editor_gizmos.h"
#include "scene/3d/room_manager.h"
#include "scene/resources/material.h"

class RoomManagerGizmoPlugin : public EditorSpatialGizmoPlugin {
	GDCLASS(RoomManagerGizmoPlugin, EditorSpatialGizmoPlugin);

	bool _created = false;

protected:
	virtual bool has_gizmo(Spatial *p_spatial);

	String get_name() const;
	int get_priority() const;

	void redraw(EditorSpatialGizmo *p_gizmo);

public:
	RoomManagerGizmoPlugin();
};

class RoomManagerEditorPlugin : public EditorPlugin {
	GDCLASS(RoomManagerEditorPlugin, EditorPlugin);

	RoomManager *_room_manager;

	ToolButton *bake;
	EditorNode *editor;

	void _bake();

protected:
	static void _bind_methods();

public:
	virtual String get_name() const { return "RoomManager"; }
	bool has_main_screen() const { return false; }
	virtual void edit(Object *p_object);
	virtual bool handles(Object *p_object) const;
	virtual void make_visible(bool p_visible);

	RoomManagerEditorPlugin(EditorNode *p_node);
	~RoomManagerEditorPlugin();
};
