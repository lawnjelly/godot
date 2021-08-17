/*************************************************************************/
/*  occluder_shape_poly.cpp                                              */
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

#include "occluder_shape_poly.h"

#include "servers/visual_server.h"

#ifdef TOOLS_ENABLED
void OccluderShapePoly::_update_aabb() {
	_aabb_local = AABB();

	if (_poly_pts_local.size()) {
		Vector3 begin = _vec2to3(_poly_pts_local[0]);
		Vector3 end = begin;

		for (int n = 1; n < _poly_pts_local.size(); n++) {
			Vector3 pt = _vec2to3(_poly_pts_local[n]);

			if (pt.x < begin.x) {
				begin.x = pt.x;
			}
			if (pt.y < begin.y) {
				begin.y = pt.y;
			}
			if (pt.z < begin.z) {
				begin.z = pt.z;
			}

			if (pt.x > end.x) {
				end.x = pt.x;
			}
			if (pt.y > end.y) {
				end.y = pt.y;
			}
			if (pt.z > end.z) {
				end.z = pt.z;
			}
		}

		for (int n = 0; n < _hole_pts_local.size(); n++) {
			Vector3 pt = _vec2to3(_hole_pts_local[n]);
			if (pt.x < begin.x) {
				begin.x = pt.x;
			}
			if (pt.y < begin.y) {
				begin.y = pt.y;
			}
			if (pt.z < begin.z) {
				begin.z = pt.z;
			}

			if (pt.x > end.x) {
				end.x = pt.x;
			}
			if (pt.y > end.y) {
				end.y = pt.y;
			}
			if (pt.z > end.z) {
				end.z = pt.z;
			}
		}

		_aabb_local.position = begin;
		_aabb_local.size = end - begin;
	}
}

AABB OccluderShapePoly::get_fallback_gizmo_aabb() const {
	return _aabb_local;
}

#endif

void OccluderShapePoly::_sanitize_points_internal(const PoolVector<Vector2> &p_from, Vector<Vector2> &r_to) {
	// remove duplicates? NYI maybe not necessary
	Vector<Vector2> raw;
	raw.resize(p_from.size());
	for (int n = 0; n < p_from.size(); n++) {
		raw.set(n, p_from[n]);
	}

	// this function may get rid of some concave points due to user editing ..
	// may not be necessary, no idea how fast it is
	r_to = Geometry::convex_hull_2d(raw);

	// some peculiarity of convex_hull_2d function, it duplicates the last point for some reason
	if (r_to.size() > 1) {
		r_to.resize(r_to.size() - 1);
	}

	// sort winding, the system expects counter clockwise polys
	Geometry::sort_polygon_winding(r_to, false);
}

void OccluderShapePoly::_sanitize_points() {
	_sanitize_points_internal(_poly_pts_local_raw, _poly_pts_local);
	_sanitize_points_internal(_hole_pts_local_raw, _hole_pts_local);

#ifdef TOOLS_ENABLED
	_update_aabb();
#endif
}

void OccluderShapePoly::set_poly_point(int p_idx, const Vector2 &p_point) {
	if (p_idx >= _poly_pts_local_raw.size()) {
		return;
	}

	_poly_pts_local_raw.set(p_idx, p_point);
	_sanitize_points();
	//update_gizmo();
	notify_change_to_owners();
}

void OccluderShapePoly::set_hole_point(int p_idx, const Vector2 &p_point) {
	if (p_idx >= _hole_pts_local_raw.size()) {
		return;
	}

	_hole_pts_local_raw.set(p_idx, p_point);
	_sanitize_points();
	notify_change_to_owners();
}

void OccluderShapePoly::set_poly_points(const PoolVector<Vector2> &p_points) {
	_poly_pts_local_raw = p_points;
	_sanitize_points();

	//	if (is_inside_tree()) {
	//		update_gizmo();
	//	}
	notify_change_to_owners();
}

void OccluderShapePoly::set_hole_points(const PoolVector<Vector2> &p_points) {
	_hole_pts_local_raw = p_points;
	_sanitize_points();
	notify_change_to_owners();
}

PoolVector<Vector2> OccluderShapePoly::get_poly_points() const {
	return _poly_pts_local_raw;
}

PoolVector<Vector2> OccluderShapePoly::get_hole_points() const {
	return _hole_pts_local_raw;
}

void OccluderShapePoly::notification_enter_world(RID p_scenario) {
	VisualServer::get_singleton()->occluder_set_scenario(get_shape(), p_scenario, VisualServer::OCCLUDER_TYPE_MESH);
}

void OccluderShapePoly::update_shape_to_visual_server() {
	if (_poly_pts_local.size() < 3)
		return;

	Geometry::OccluderMeshData md;
	md.faces.resize(1);

	Geometry::OccluderMeshData::Face &face = md.faces[0];

	md.vertices.resize(_poly_pts_local.size() + _hole_pts_local.size());
	face.indices.resize(_poly_pts_local.size());

	for (int n = 0; n < _poly_pts_local.size(); n++) {
		md.vertices[n] = _vec2to3(_poly_pts_local[n]);
		face.indices[n] = n;
	}

	// hole points
	if (_hole_pts_local.size()) {
		face.holes.resize(1);
		Geometry::OccluderMeshData::Hole &hole = face.holes[0];
		hole.indices.resize(_hole_pts_local.size());

		for (int n = 0; n < _hole_pts_local.size(); n++) {
			int dest_idx = n + _poly_pts_local.size();

			hole.indices[n] = dest_idx;
			md.vertices[dest_idx] = _vec2to3(_hole_pts_local[n]);
		}
	}

	face.plane = Plane(Vector3(0, 0, 0), Vector3(0, 0, -1));

	VisualServer::get_singleton()->occluder_mesh_update(get_shape(), md);
}

void OccluderShapePoly::set_two_way(bool p_two_way) {
	_settings_two_way = p_two_way;
}

Transform OccluderShapePoly::center_node(const Transform &p_global_xform, const Transform &p_parent_xform, real_t p_snap) {
	return Transform();
}

void OccluderShapePoly::clear() {
	_poly_pts_local.clear();
	_poly_pts_local_raw.resize(0);
	_hole_pts_local.clear();
	_hole_pts_local_raw.resize(0);
#ifdef TOOLS_ENABLED
	_aabb_local = AABB();
#endif
}

void OccluderShapePoly::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_two_way", "two_way"), &OccluderShapePoly::set_two_way);
	ClassDB::bind_method(D_METHOD("is_two_way"), &OccluderShapePoly::is_two_way);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "two_way"), "set_two_way", "is_two_way");

	ClassDB::bind_method(D_METHOD("set_poly_points", "points"), &OccluderShapePoly::set_poly_points);
	ClassDB::bind_method(D_METHOD("get_poly_points"), &OccluderShapePoly::get_poly_points);

	ClassDB::bind_method(D_METHOD("set_poly_point", "index", "position"), &OccluderShapePoly::set_poly_point);

	ADD_PROPERTY(PropertyInfo(Variant::POOL_VECTOR2_ARRAY, "poly_points"), "set_poly_points", "get_poly_points");

	ClassDB::bind_method(D_METHOD("set_hole_points", "points"), &OccluderShapePoly::set_hole_points);
	ClassDB::bind_method(D_METHOD("get_hole_points"), &OccluderShapePoly::get_hole_points);
	ClassDB::bind_method(D_METHOD("set_hole_point", "index", "position"), &OccluderShapePoly::set_hole_point);

	ADD_PROPERTY(PropertyInfo(Variant::POOL_VECTOR2_ARRAY, "hole_points"), "set_hole_points", "get_hole_points");
}

OccluderShapePoly::OccluderShapePoly() :
		OccluderShape(RID_PRIME(VisualServer::get_singleton()->occluder_create())) {
	clear();

	PoolVector<Vector2> points;
	points.resize(4);
	points.set(0, Vector2(1, -1));
	points.set(1, Vector2(1, 1));
	points.set(2, Vector2(-1, 1));
	points.set(3, Vector2(-1, -1));

	set_poly_points(points); // default shape
}
