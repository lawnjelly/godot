/*************************************************************************/
/*  room_manager_editor_plugin.cpp                                       */
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

#include "room_manager_editor_plugin.h"
#include "scene/3d/portal.h"
#include "scene/3d/room.h"

RoomManagerGizmoPlugin::RoomManagerGizmoPlugin() {
	create_icon_material("portal_icon", SpatialEditor::get_singleton()->get_icon("GizmoPortal", "EditorIcons"));
}

bool RoomManagerGizmoPlugin::has_gizmo(Spatial *p_spatial) {
	Room *room = Object::cast_to<Room>(p_spatial);

	if (room) {
		// print_line("gizmo found room");
		return true;
	}

	Portal *portal = Object::cast_to<Portal>(p_spatial);

	if (portal) {
		// print_line("gizmo found portal");
		return true;
	}

	return false;
}

String RoomManagerGizmoPlugin::get_name() const {
	return "Rooms and Portals";
}

int RoomManagerGizmoPlugin::get_priority() const {
	//	return -1;
	return 0; // needs to override the meshinstance gizmo
}

void RoomManagerGizmoPlugin::redraw(EditorSpatialGizmo *p_gizmo) {
	if (!_created) {
		create_material("room", Color(0.5, 1.0, 0.0), false, true, false);
		create_material("portal", Color(1.0, 0.2, 0.2, 0.5f), false, false, false);
		create_material("portal_edge", Color(1.0, 1.0, 1.0, 1.0f), false, false, false);
		//create_material("portal_edge", Color(0.984, 0.28, 0.77, 1.0f), false, false, false);
		_created = true;
	}

	p_gizmo->clear();

	Room *room = Object::cast_to<Room>(p_gizmo->get_spatial_node());

	// as a mesh
	if (room) {
		const Geometry::MeshData &md = room->_bound_mesh_data;
		if (!md.edges.size())
			return;

		Vector<Vector3> lines;
		Transform tr = room->get_global_transform();
		tr.affine_invert();

		Ref<Material> material = get_material("room", p_gizmo);
		Color color(1, 1, 1, 1);

		for (int n = 0; n < md.edges.size(); n++) {
			Vector3 a = md.vertices[md.edges[n].a];
			Vector3 b = md.vertices[md.edges[n].b];

			// xform
			a = tr.xform(a);
			b = tr.xform(b);

			lines.push_back(a);
			lines.push_back(b);
		}

		p_gizmo->add_lines(lines, material, false, color);

	} else {
		// if not a room we are also wanting to show portals
		Portal *portal = Object::cast_to<Portal>(p_gizmo->get_spatial_node());

		if (portal) {
			// icon switched off as it is a bit distracting and not sure if necessary
			//Ref<Material> icon = get_material("portal_icon", p_gizmo);
			//p_gizmo->add_unscaled_billboard(icon, 0.05);

			Vector<Vector3> lines;
			Transform tr = portal->get_global_transform();
			tr.affine_invert();

			Ref<Material> material = get_material("portal", p_gizmo);
			Ref<Material> material_edge = get_material("portal_edge", p_gizmo);
			Color color(1, 1, 1, 1);

			PoolVector<Vector3> pts;
			Vector<Vector3> edge_pts;

			real_t margin = portal->get_active_portal_margin();

			if (margin > 0.05f) {
				// make sure world points are up to date
				portal->portal_update_full();

				int num_points = portal->_pts_world.size();

				// prevent compiler warnings later on
				if (num_points < 3) {
					return;
				}

				Vector3 portal_normal_world_space = portal->_plane.normal;
				portal_normal_world_space *= margin;

				// this may not be necessary, dealing with non uniform scales,
				// possible the affine_invert dealt with this earlier .. but it's just for
				// the editor so not performance critical
				Basis normal_basis = tr.basis;
				normal_basis.invert();
				normal_basis.transpose();

				Vector3 portal_normal = normal_basis.xform(portal_normal_world_space);

				for (int n = 0; n < num_points; n++) {
					Vector3 pt = portal->_pts_world[n];
					pt = tr.xform(pt);

					// CI for visual studio can't seem to get around the possibility
					// that this could cause a divide by zero, so using a local to preclude the
					// possibility of aliasing from another thread
					int m = (n + 1) % num_points;
					Vector3 pt_next = portal->_pts_world[m];
					pt_next = tr.xform(pt_next);

					Vector3 pt0 = pt - portal_normal;
					Vector3 pt1 = pt + portal_normal;
					Vector3 pt2 = pt_next - portal_normal;
					Vector3 pt3 = pt_next + portal_normal;

					pts.push_back(pt0);
					pts.push_back(pt2);
					pts.push_back(pt1);

					pts.push_back(pt2);
					pts.push_back(pt3);
					pts.push_back(pt1);

					edge_pts.push_back(pt0);
					edge_pts.push_back(pt2);
					edge_pts.push_back(pt1);
					edge_pts.push_back(pt3);
				}

				// faces
				Ref<ArrayMesh> mesh = memnew(ArrayMesh);
				Array array;
				array.resize(Mesh::ARRAY_MAX);
				array[Mesh::ARRAY_VERTEX] = pts;
				mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, array);

				p_gizmo->add_mesh(mesh, false, Ref<SkinReference>(), material);

				// lines around the outside of mesh
				p_gizmo->add_lines(edge_pts, material_edge, false, color);

			} // only if the margin is sufficient to be worth drawing
		} // was portal

	} // not room
}

//////////////////////////////////////////////////
void RoomManagerEditorPlugin::_bake() {
	if (_room_manager) {
		_room_manager->rooms_convert();
	}
}

void RoomManagerEditorPlugin::edit(Object *p_object) {
	RoomManager *s = Object::cast_to<RoomManager>(p_object);
	if (!s) {
		return;
	}

	_room_manager = s;
}

bool RoomManagerEditorPlugin::handles(Object *p_object) const {
	return p_object->is_class("RoomManager");
}

void RoomManagerEditorPlugin::make_visible(bool p_visible) {
	if (p_visible) {
		bake->show();
	} else {
		bake->hide();
	}
}

void RoomManagerEditorPlugin::_bind_methods() {
	ClassDB::bind_method("_bake", &RoomManagerEditorPlugin::_bake);
}

RoomManagerEditorPlugin::RoomManagerEditorPlugin(EditorNode *p_node) {
	editor = p_node;
	bake = memnew(ToolButton);
	bake->set_icon(editor->get_gui_base()->get_icon("Bake", "EditorIcons"));
	bake->set_text(TTR("Convert Rooms"));
	bake->hide();
	bake->connect("pressed", this, "_bake");
	add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, bake);
	_room_manager = NULL;

	Ref<RoomManagerGizmoPlugin> gizmo_plugin = Ref<RoomManagerGizmoPlugin>(memnew(RoomManagerGizmoPlugin));
	SpatialEditor::get_singleton()->add_gizmo_plugin(gizmo_plugin);
}

RoomManagerEditorPlugin::~RoomManagerEditorPlugin() {
}
