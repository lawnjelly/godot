/*************************************************************************/
/*  occluder_shape_mesh.cpp                                              */
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

#include "occluder_shape_mesh.h"

#include "core/debug_image.h"
#include "scene/3d/occluder.h"
#include "scene/3d/spatial.h"
#include "scene/3d/visual_instance.h"
#include "scene/resources/surface_tool.h"
#include "servers/visual/portals/portal_defines.h"
#include "servers/visual_server.h"

#include <iostream>

/*
Vector2 OccluderShapeMesh::Keil::Line::lineInt(const Line &l1, const Line &l2) {
	Vector2 i;
	real_t a1, b1, c1, a2, b2, c2, det;
	a1 = l1.second.y - l1.first.y;
	b1 = l1.first.x - l1.second.x;
	c1 = a1 * l1.first.x + b1 * l1.first.y;
	a2 = l2.second.y - l2.first.y;
	b2 = l2.first.x - l2.second.x;
	c2 = a2 * l2.first.x + b2 * l2.first.y;
	det = a1 * b2 - a2 * b1;
	if (!eq(det, 0)) { // lines are not parallel
		i.x = (b2 * c1 - b1 * c2) / det;
		i.y = (a1 * c2 - a2 * c1) / det;
	}
	return i;
}

OccluderShapeMesh::IndexedPoint &OccluderShapeMesh::Keil::Polygon::operator[](const int &i) {
	return v[i];
}

OccluderShapeMesh::IndexedPoint &OccluderShapeMesh::Keil::Polygon::at(const int &i) {
	const int s = v.size();
	return v[i < 0 ? i % s + s : i % s];
}

OccluderShapeMesh::IndexedPoint &OccluderShapeMesh::Keil::Polygon::first() {
	return v[0];
}

OccluderShapeMesh::IndexedPoint &OccluderShapeMesh::Keil::Polygon::last() {
	return v[v.size() - 1];
}

int OccluderShapeMesh::Keil::Polygon::size() const {
	return v.size();
}

OccluderShapeMesh::Keil::EdgeList OccluderShapeMesh::Keil::Polygon::decomp(int depth) {
	EdgeList min, tmp1, tmp2;
	int nDiags = INT32_MAX;

	for (int i = 0; i < v.size(); ++i) {
		if (isReflex(i)) {
			for (int j = 0; j < v.size(); ++j) {
				if (canSee(i, j)) {
					//					String sz;
					//					for (int n=0; n<depth; n++)
					//					{
					//						sz += "\t";
					//					}
					//					print_line(sz + "i: " + itos(i) + ", j: " + itos(j));

					tmp1 = copy(i, j).decomp(depth + 1);
					tmp2 = copy(j, i).decomp(depth + 1);

					for (int n = 0; n < tmp2.size(); n++) {
						tmp1.push_back(tmp2[n]);
					}

					if (tmp1.size() < nDiags) {
						min = tmp1;
						nDiags = tmp1.size();
						min.push_back(Edge(at(i), at(j)));
					}
				}
			}
		}
	}

	return min;
}

bool OccluderShapeMesh::Keil::Polygon::isReflex(const int &i) {
	return Point::right(at(i - 1), at(i), at(i + 1));
}

bool OccluderShapeMesh::Keil::Polygon::canSee(const int &a, const int &b) {
	Vector2 p;
	real_t dist;

	if (Point::leftOn(at(a + 1), at(a), at(b)) && Point::rightOn(at(a - 1), at(a), at(b))) {
		return false;
	}
	dist = Point::sqdist(at(a), at(b));
	for (int i = 0; i < v.size(); ++i) { // for each edge
		if ((i + 1) % v.size() == a || i == a) // ignore incident edges
			continue;
		if (Point::leftOn(at(a), at(b), at(i + 1)) && Point::rightOn(at(a), at(b), at(i))) { // if diag intersects an edge
			p = Line::lineInt(Line(at(a), at(b)), Line(at(i), at(i + 1)));
			if ((at(a).pos - p).length_squared() < dist) { // if edge is blocking visibility to b
				return false;
			}
		}
	}

	return true;
}

void OccluderShapeMesh::Keil::Polygon::push(const IndexedPoint &p) {
	v.push_back(p);
}

void OccluderShapeMesh::Keil::Polygon::clear() {
	v.clear();
	//	tp.clear();
	//	tl.clear();
}
void OccluderShapeMesh::Keil::Polygon::makeCCW() {
	int br = 0;

	// find bottom right point
	for (int i = 1; i < v.size(); ++i) {
		if (v[i].pos.y < v[br].pos.y || (v[i].pos.y == v[br].pos.y && v[i].pos.x > v[br].pos.x)) {
			br = i;
		}
	}

	// reverse poly if clockwise
	if (!Point::left(at(br - 1), at(br), at(br + 1))) {
		reverse();
	}
}
void OccluderShapeMesh::Keil::Polygon::reverse() {
	v.invert();
}
OccluderShapeMesh::Keil::Polygon OccluderShapeMesh::Keil::Polygon::copy(const int &i, const int &j) {
	Polygon p;
	if (i < j) {
		int s = j - i;
		p.v.resize(s);
		for (int n = 0; n < s; n++) {
			p.v[n] = v[i + n];
		}
	} else {
		int s = v.size() - (i - j);
		p.v.resize(s);

		// end
		for (int n = 0; n < size() - i; n++) {
			p.v[n] = v[n + i];
		}

		// start
		int start = size() - i;
		for (int n = 0; n < j; n++) {
			p.v[n + start] = v[n];
		}
	}

	CRASH_COND(p.size() > v.size());
	return p;
}

void OccluderShapeMesh::Keil::decompose(const Vector<IndexedPoint> &p_input, List<Vector<IndexedPoint>> &r_results) {
	Polygon p;

	for (int n = 0; n < p_input.size(); n++) {
		IndexedPoint ip = p_input[n];
		ip.idx = n;
		p.push(ip);
	}

	EdgeList el = p.decomp();

	for (int n = 0; n < el.size(); n++) {
		const Edge &e = el[n];
		String sz = "edge " + itos(n) + ": ";
		sz += itos(e.a.idx) + " to " + itos(e.b.idx);
		print_line(sz);
	}
}
*/

////////////////////////////////////////////////////

void OccluderShapeMesh::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_bake_path", "path"), &OccluderShapeMesh::set_bake_path);
	ClassDB::bind_method(D_METHOD("get_bake_path"), &OccluderShapeMesh::get_bake_path);

	ClassDB::bind_method(D_METHOD("set_threshold_input_size", "size"), &OccluderShapeMesh::set_threshold_input_size);
	ClassDB::bind_method(D_METHOD("get_threshold_input_size"), &OccluderShapeMesh::get_threshold_input_size);

	ClassDB::bind_method(D_METHOD("set_threshold_output_size", "size"), &OccluderShapeMesh::set_threshold_output_size);
	ClassDB::bind_method(D_METHOD("get_threshold_output_size"), &OccluderShapeMesh::get_threshold_output_size);

	ClassDB::bind_method(D_METHOD("set_simplify", "simplify"), &OccluderShapeMesh::set_simplify);
	ClassDB::bind_method(D_METHOD("get_simplify"), &OccluderShapeMesh::get_simplify);

	ClassDB::bind_method(D_METHOD("set_plane_simplify_angle", "angle"), &OccluderShapeMesh::set_plane_simplify_angle);
	ClassDB::bind_method(D_METHOD("get_plane_simplify_angle"), &OccluderShapeMesh::get_plane_simplify_angle);

	ClassDB::bind_method(D_METHOD("set_debug_face_id", "id"), &OccluderShapeMesh::set_debug_face_id);
	ClassDB::bind_method(D_METHOD("get_debug_face_id"), &OccluderShapeMesh::get_debug_face_id);

	ClassDB::bind_method(D_METHOD("set_remove_floor", "angle"), &OccluderShapeMesh::set_remove_floor);
	ClassDB::bind_method(D_METHOD("get_remove_floor"), &OccluderShapeMesh::get_remove_floor);

	ClassDB::bind_method(D_METHOD("set_bake_mask", "mask"), &OccluderShapeMesh::set_bake_mask);
	ClassDB::bind_method(D_METHOD("get_bake_mask"), &OccluderShapeMesh::get_bake_mask);
	ClassDB::bind_method(D_METHOD("set_bake_mask_value", "layer_number", "value"), &OccluderShapeMesh::set_bake_mask_value);
	ClassDB::bind_method(D_METHOD("get_bake_mask_value", "layer_number"), &OccluderShapeMesh::get_bake_mask_value);

	ClassDB::bind_method(D_METHOD("_set_data"), &OccluderShapeMesh::_set_data);
	ClassDB::bind_method(D_METHOD("_get_data"), &OccluderShapeMesh::_get_data);

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "bake_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Spatial"), "set_bake_path", "get_bake_path");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "bake_mask", PROPERTY_HINT_LAYERS_3D_RENDER), "set_bake_mask", "get_bake_mask");

	ADD_PROPERTY(PropertyInfo(Variant::REAL, "threshold_input_size", PROPERTY_HINT_RANGE, "0.0,10.0,0.1"), "set_threshold_input_size", "get_threshold_input_size");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "threshold_output_size", PROPERTY_HINT_RANGE, "0.0,10.0,0.1"), "set_threshold_output_size", "get_threshold_output_size");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "plane_angle", PROPERTY_HINT_RANGE, "0.0,45.0,0.1"), "set_plane_simplify_angle", "get_plane_simplify_angle");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "simplify", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_simplify", "get_simplify");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_floor", PROPERTY_HINT_RANGE, "0, 90, 1"), "set_remove_floor", "get_remove_floor");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "debug_face_id", PROPERTY_HINT_RANGE, "0, 1000, 1"), "set_debug_face_id", "get_debug_face_id");

	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "_set_data", "_get_data");
}

void OccluderShapeMesh::_set_data(const Dictionary &p_d) {
	ERR_FAIL_COND(!p_d.has("version"));

	int version = p_d["version"];
	ERR_FAIL_COND(version != 100);

	ERR_FAIL_COND(!p_d.has("verts"));
	ERR_FAIL_COND(!p_d.has("planes"));
	ERR_FAIL_COND(!p_d.has("indices"));
	ERR_FAIL_COND(!p_d.has("first_indices"));
	ERR_FAIL_COND(!p_d.has("num_indices"));

	_mesh_data.vertices = p_d["verts"];

	Vector<Plane> planes = p_d["planes"];
	Vector<int> indices = p_d["indices"];
	Vector<int> face_first_indices = p_d["first_indices"];
	Vector<int> face_num_indices = p_d["num_indices"];

	ERR_FAIL_COND(planes.size() != face_first_indices.size());
	ERR_FAIL_COND(planes.size() != face_num_indices.size());

	_mesh_data.faces.resize(planes.size());
	for (int n = 0; n < planes.size(); n++) {
		Geometry::MeshData::Face f;
		f.plane = planes[n];
		f.indices.resize(face_num_indices[n]);

		for (int i = 0; i < face_num_indices[n]; i++) {
			int ind_id = i + face_first_indices[n];

			// check for invalid
			if (ind_id > indices.size()) {
				_mesh_data.vertices.clear();
				_mesh_data.faces.clear();
				ERR_FAIL_COND(ind_id > indices.size());
			}

			int ind = indices[ind_id];
			f.indices.set(i, ind);
		}

		_mesh_data.faces.set(n, f);
	}
}

Dictionary OccluderShapeMesh::_get_data() const {
	Dictionary d;
	d["version"] = 100;
	d["verts"] = _mesh_data.vertices;

	Vector<Plane> planes;
	Vector<int> indices;
	Vector<int> face_first_indices;
	Vector<int> face_num_indices;
	for (int n = 0; n < _mesh_data.faces.size(); n++) {
		const Geometry::MeshData::Face &f = _mesh_data.faces[n];
		planes.push_back(f.plane);
		face_first_indices.push_back(indices.size());

		for (int i = 0; i < f.indices.size(); i++) {
			indices.push_back(f.indices[i]);
		}
		face_num_indices.push_back(f.indices.size());
	}

	d["planes"] = planes;
	d["indices"] = indices;
	d["first_indices"] = face_first_indices;
	d["num_indices"] = face_num_indices;
	return d;
}

void OccluderShapeMesh::set_remove_floor(int p_angle) {
	_settings_remove_floor_angle = p_angle;
	_settings_remove_floor_dot = Math::cos(Math::deg2rad((real_t)p_angle));
}

void OccluderShapeMesh::update_transform_to_visual_server(const Transform &p_global_xform) {
	VisualServer::get_singleton()->occluder_set_transform(get_shape(), p_global_xform);
}

void OccluderShapeMesh::update_shape_to_visual_server() {
	VisualServer::get_singleton()->occluder_mesh_update(get_shape(), _mesh_data.faces, _mesh_data.vertices);
}

Transform OccluderShapeMesh::center_node(const Transform &p_global_xform, const Transform &p_parent_xform, real_t p_snap) {
	return Transform();
}

void OccluderShapeMesh::set_bake_mask(uint32_t p_mask) {
	_settings_bake_mask = p_mask;
	//update_configuration_warnings();
}

uint32_t OccluderShapeMesh::get_bake_mask() const {
	return _settings_bake_mask;
}

void OccluderShapeMesh::set_bake_mask_value(int p_layer_number, bool p_value) {
	ERR_FAIL_COND_MSG(p_layer_number < 1, "Render layer number must be between 1 and 20 inclusive.");
	ERR_FAIL_COND_MSG(p_layer_number > 20, "Render layer number must be between 1 and 20 inclusive.");
	uint32_t mask = get_bake_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	} else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_bake_mask(mask);
}

bool OccluderShapeMesh::get_bake_mask_value(int p_layer_number) const {
	ERR_FAIL_COND_V_MSG(p_layer_number < 1, false, "Render layer number must be between 1 and 20 inclusive.");
	ERR_FAIL_COND_V_MSG(p_layer_number > 20, false, "Render layer number must be between 1 and 20 inclusive.");
	return _settings_bake_mask & (1 << (p_layer_number - 1));
}

void OccluderShapeMesh::notification_enter_world(RID p_scenario) {
	VisualServer::get_singleton()->occluder_set_scenario(get_shape(), p_scenario, VisualServer::OCCLUDER_TYPE_MESH);
}

void OccluderShapeMesh::clear() {
	_mesh_data.vertices.clear();
	_mesh_data.faces.clear();
	_bd.clear();
}

void OccluderShapeMesh::_log(String p_string) {
	print_line(p_string);
}

void OccluderShapeMesh::bake(Node *owner) {
	clear();

	// make sure precalced values are correct
	_settings_remove_floor_dot = Math::cos(Math::deg2rad((real_t)_settings_remove_floor_angle));

	// test debug image
	//	DebugImage im;
	//	im.create(320, 240);
	//	im.line(5, 5, 200, 220);
	//	im.save_png("test_png.png");

	// the owner must be the occluder
	Occluder *occ = Object::cast_to<Occluder>(owner);
	ERR_FAIL_NULL_MSG(occ, "bake argument must be the Occluder");

	// We want the points in local space to the occluder ...
	// so that if the occluder moves, the polys can also move with it.
	// First we bake in world space though, as the epsilons etc are set up for world space.
	Transform xform_inv = occ->get_global_transform().affine_inverse();

	// one off conversion
	_settings_plane_simplify_dot = Math::cos(Math::deg2rad(_settings_plane_simplify_degrees));

	// try to get the nodepath
	if (owner->has_node(_settings_bake_path)) {
		Node *node = owner->get_node(_settings_bake_path);

		Spatial *branch = Object::cast_to<Spatial>(node);
		if (!branch) {
			return;
		}

		_bake_recursive(branch);

		_bake_quantize_float_faces();
	}

	_simplify_triangles();

	_verify_verts();
	_find_neighbour_face_ids();
	_process_islands();

	uint32_t process_tick = 1;
	//	while (_make_faces(process_tick++)) {
	while (_make_faces_new(process_tick++)) {
		;
	}

	print_line("num process ticks : " + itos(process_tick));

	_process_out_faces();
	//_verify_verts();
	_finalize_faces();

	// clear intermediate data
	_bd.clear();

	// if not an identity matrix
	if (xform_inv != Transform()) {
		// transform all the verts etc from world space to local space
		for (int n = 0; n < _mesh_data.vertices.size(); n++) {
			const Vector3 &pt = _mesh_data.vertices[n];
			_mesh_data.vertices.set(n, xform_inv.xform(pt));
		}

		// planes
		for (int n = 0; n < _mesh_data.faces.size(); n++) {
			Geometry::MeshData::Face face = _mesh_data.faces[n];
			face.plane = xform_inv.xform(face.plane);
			_mesh_data.faces.set(n, face);
		}
	}

	update_shape_to_visual_server();
}

void OccluderShapeMesh::_simplify_triangles() {
	// noop
	if (!_mesh_data.vertices.size()) {
		return;
	}

	// clear hash table to reuse
	_bd.hash_verts.clear();
	_bd.hash_triangles._table.clear();

	// get the data into a format that mesh optimizer can deal with
	LocalVector<uint32_t> inds_in;
	for (int n = 0; n < _bd.faces.size(); n++) {
		const BakeFace &face = _bd.faces[n];
		CRASH_COND(face.indices.size() != 3);

		inds_in.push_back(face.indices[0]);
		inds_in.push_back(face.indices[1]);
		inds_in.push_back(face.indices[2]);
	}

	LocalVector<uint32_t> inds_out;
	inds_out.resize(inds_in.size());

	struct Vec3f {
		void set(real_t xx, real_t yy, real_t zz) {
			x = xx;
			y = yy;
			z = zz;
		}
		float x, y, z;
	};

	// prevent a screwup if real_t is double,
	// as the mesh optimizer lib expects floats
	LocalVector<Vec3f> verts;
	verts.resize(_mesh_data.vertices.size());
	for (int n = 0; n < _mesh_data.vertices.size(); n++) {
		const Vector3 &p = _mesh_data.vertices[n];
		verts[n].set(p.x, p.y, p.z);
	}

	//	typedef size_t (*SimplifyFunc)(unsigned int *destination, const unsigned int *indices, size_t index_count, const float *vertex_positions, size_t vertex_count, size_t vertex_positions_stride, size_t target_index_count, float target_error, float *r_error);
	//real_t target_error = 0.001;
	//	real_t target_error = 0.01;

	size_t result = 0;
	if (_settings_simplify > 0.0) {
#define GODOT_OCCLUDER_POLY_USE_ERROR_METRIC
#ifdef GODOT_OCCLUDER_POLY_USE_ERROR_METRIC
		// by an error metric
		real_t target_error = (_settings_simplify * _settings_simplify * _settings_simplify);
		target_error *= 0.05;
		result = SurfaceTool::simplify_func(&inds_out[0], &inds_in[0], inds_in.size(), (const float *)&verts[0], verts.size(), sizeof(Vec3f), 1, target_error, nullptr);
#else
		// by a percentage of the indices
		size_t target_indices = inds_in.size() - (inds_in.size() * _settings_simplify);
		target_indices = CLAMP(target_indices, 3, inds_in.size());
		result = SurfaceTool::simplify_func(&inds_out[0], &inds_in[0], inds_in.size(), (const float *)&verts[0], verts.size(), sizeof(Vec3f), target_indices, 0.9, nullptr);
#endif

	} else {
		// no simplification, just copy
		result = inds_in.size();
		inds_out = inds_in;
	}

	// resave
	_bd.faces.clear();
	_bd.faces.resize(result / 3);

	int count = 0;
	for (int n = 0; n < _bd.faces.size(); n++) {
		BakeFace face;
		face.indices.resize(3);

		Face3 f3;

		for (int c = 0; c < 3; c++) {
			const Vector3 &pos = _mesh_data.vertices[inds_out[count++]];
			int new_ind = _bd.find_or_create_vert(pos, n);
			face.indices[c] = new_ind;
			f3.vertex[c] = pos;
		}

		face.plane = f3.get_plane();

		face.area = f3.get_area();
		_bd.faces[n] = face;
	}

	// delete the orig verts
	_mesh_data.vertices.clear();

	// clear hash table no longer need
	_bd.hash_verts.clear();

	// sort all the vertex faces by area
	for (int n = 0; n < _bd.verts.size(); n++) {
		_sort_vertex_faces_by_area(n);
	}

	_create_sort_faces();
}

void OccluderShapeMesh::_create_sort_faces() {
	_bd.sort_faces.resize(_bd.faces.size());
	for (int n = 0; n < _bd.sort_faces.size(); n++) {
		const BakeFace &face = _bd.faces[n];
		SortFace &sface = _bd.sort_faces[n];
		sface.id = n;
		sface.area = face.area;
	}

	_bd.sort_faces.sort();
}

void OccluderShapeMesh::_sort_vertex_faces_by_area(uint32_t p_vertex_id) {
	BakeVertex &vert = _bd.verts[p_vertex_id];
	int num_faces = vert.linked_faces.size();

	// fill sort list
	LocalVector<SortFace> faces;
	faces.resize(num_faces);

	for (int n = 0; n < num_faces; n++) {
		uint32_t id = vert.linked_faces[n];
		faces[n].id = id;
		faces[n].area = _bd.faces[id].area;
	}

	faces.sort();

	for (int n = 0; n < num_faces; n++) {
		vert.linked_faces[n] = faces[n].id;
	}
}

void OccluderShapeMesh::_print_line(String p_sz) {
	//return;
	print_line(p_sz);
	std::cout << p_sz.c_str();
	std::cout.flush();
}

void OccluderShapeMesh::_debug_print_face(uint32_t p_face_id, String p_before_string) {
	return;

	BakeFace &face = _bd.faces[p_face_id];

	String sz;
	sz = p_before_string;
	sz += " face " + itos(p_face_id) + " " + itos(face.indices.size()) + " sides.";

	for (int n = 0; n < face.indices.size(); n++) {
		sz += "\n\tind " + itos(face.indices[n]) + ", neigh " + itos(face.neighbour_face_ids[n]);
	}

	_print_line(sz);
}

// MUST start from a face with at least one neighbour, and at least one free edge
// (not a central tri)
void OccluderShapeMesh::_find_face_zone_edges(const LocalVectori<uint32_t> &p_face_ids, LocalVectori<uint32_t> &r_edges) {
	// first find the first non neighbour edge
	uint32_t first_face_id = p_face_ids[0];
	BakeFace &face = _bd.faces[first_face_id];

	_debug_print_face(first_face_id, "first");

	int num_sides = face.neighbour_face_ids.size();

	// must be zero if none of the other edges have neighbours
	// (providing there are any neighbours at all .. this should be checked earlier)
	int first_edge = 0;
	for (int c = 1; c < num_sides; c++) {
		uint32_t id = face.neighbour_face_ids[c];
		if (id == UINT32_MAX) {
			first_edge = c;
			break;
		}
	}
	// if 1 is the first edge, it actually could be zero (because edge 0 could have no neighbour)
	if ((first_edge == 1) && (face.neighbour_face_ids[0] == UINT32_MAX)) {
		first_edge = 0;
	}

	// first add the edges from the first poly to the list
	uint32_t face_id;
	uint32_t join_vert_id = 0;

	for (int c = 0; c < num_sides; c++) {
		int e = (c + first_edge) % num_sides;
		face_id = face.neighbour_face_ids[e];

		if (face_id == UINT32_MAX) {
			r_edges.push_back(face.indices[e]);
		} else {
			join_vert_id = face.indices[e];
			break;
		}
	}

	//	uint32_t first_join_vert_id = join_vert_id;

	// face id is now the neighbour face we are traversing to
	//	while (face_id != first_face_id) {
	while (true) {
		face_id = _trace_zone_edge(face_id, join_vert_id, r_edges);
		if (join_vert_id == r_edges[0])
		//		if (join_vert_id == first_join_vert_id)
		{
			break;
		}
	}
	// add the last point? (from the first triangle)
	//_trace_zone_edge(face_id, join_vert_id, r_edges);

	// print the edge list
	String sz = "edge list : ";
	for (int n = 0; n < r_edges.size(); n++) {
		sz += itos(r_edges[n]) + ", ";
	}
	_print_line(sz);
}

uint32_t OccluderShapeMesh::_trace_zone_edge(uint32_t p_face_id, uint32_t &r_join_vert_id, LocalVectori<uint32_t> &r_edges) {
	BakeFace &face = _bd.faces[p_face_id];

	_debug_print_face(p_face_id, "trace");

	int num_sides = face.indices.size();

	int first_edge = -1;
	for (int c = 0; c < num_sides; c++) {
		// we want to join from the previous vertex
		if (face.indices[c] == r_join_vert_id) {
			first_edge = c;
			break;
		}
	}
	CRASH_COND(first_edge == -1);

	// add all edges till we find the next poly to traverse to
	uint32_t face_id_next;
	for (int c = 0; c < num_sides; c++) {
		int e = (c + first_edge) % num_sides;
		face_id_next = face.neighbour_face_ids[e];

		if (face_id_next == UINT32_MAX) {
			r_edges.push_back(face.indices[e]);
		} else {
			r_join_vert_id = face.indices[e];
			break;
		}
	}

	// face_id_next is now the neighbour face we are traversing to
	return face_id_next;
}

bool OccluderShapeMesh::_merge_face_zone(const LocalVectori<uint32_t> &p_face_ids) {
	// first we want to identify a list of edges around the the zone.
	LocalVectori<uint32_t> edges;
	_find_face_zone_edges(p_face_ids, edges);

	Plane average_plane;
	real_t total_area = 0.0;
	for (int n = 0; n < p_face_ids.size(); n++) {
		const BakeFace &face = _bd.faces[p_face_ids[n]];
		//		average_plane.normal += face.plane.normal * face.area;
		average_plane.normal += face.plane.normal;
		average_plane.d += face.plane.d * face.area;
		total_area += face.area;
	}

	// can't deal with this
	if (total_area == 0.0) {
		return false;
	}
	average_plane.normal.normalize();
	average_plane.d /= total_area;

	// once we have the edges, we can split this into convex chunks
	LocalVectori<uint32_t> convex;
	//_find_convex_chunk(edges, convex, average_plane);
	_make_convex_chunk(edges, average_plane, convex);

	// print the edge list
	String sz = "convex list : ";
	for (int n = 0; n < convex.size(); n++) {
		sz += itos(convex[n]) + ", ";
	}
	_print_line(sz);

	//	// create out face
	//	_bd.out_faces.resize(_bd.out_faces.size()+1);
	//	BakeFace &out = _bd.out_faces[_bd.out_faces.size()-1];

	//	out.indices = convex;
	//	//out.indices = edges;
	//	out.area = 100.0;

	return false;
}

bool OccluderShapeMesh::_any_further_points_within(const Vector<IndexedPoint> &p_pts, int p_test_pt) const {
	//#define GODOT_OCCLUDER_SHAPE_MESH_DEBUG_POINTS_WITHIN

	// as well as the edge going backward, we also want to test further points
	// in case they form a concave polygon. If so, the point is disallowed
	for (int t = p_test_pt + 1; t < p_pts.size(); t++) {
		const Vec2i &pt_test = p_pts[t].pos;

		bool within = true;

#ifdef GODOT_OCCLUDER_SHAPE_MESH_DEBUG_POINTS_WITHIN
		print_line("\ttestpoint " + itos(t) + " : " + String(Variant(pt_test)));
#endif

		// test this point against all the edges .. if it is inside all, then it forms concave, and is not allowed
		for (int e = 0; e <= p_test_pt; e++) {
			const Vec2i &a = p_pts[e].pos;
			const Vec2i &b = p_pts[(e + 1) % p_test_pt].pos; // loop back to first point

			// test against the edge
			int cross = a.cross(b, pt_test);

#ifdef GODOT_OCCLUDER_SHAPE_MESH_DEBUG_POINTS_WITHIN
			print_line("\t\tedge " + itos(e) + " a " + String(Variant(a)) + "b " + String(Variant(b)) + " cross: " + rtos(cross));
#endif

			if (cross < 0) {
				within = false;
				break;
			}
		}

		if (within) {
#ifdef GODOT_OCCLUDER_SHAPE_MESH_DEBUG_POINTS_WITHIN
			print_line("\t\t\tpoints within TRUE");
#endif
			return true;
		}
	}

#ifdef GODOT_OCCLUDER_SHAPE_MESH_DEBUG_POINTS_WITHIN
	print_line("\t\t\tpoints within FALSE");
#endif
	return false;
}

bool OccluderShapeMesh::_can_see(const Vector<IndexedPoint> &p_points, int p_test_point) const {
	// there must be a clear line between test point and zero, not crossed by any other edge
	const Vec2i &a_from = p_points[0].pos;
	const Vec2i &a_to = p_points[p_test_point].pos;

	for (int n = p_test_point + 2; n < p_points.size(); n++) {
		const Vec2i &b_from = p_points[n - 1].pos;
		const Vec2i &b_to = p_points[n].pos;

		//		if (Geometry::segment_intersects_segment_2d(a_from, a_to, b_from, b_to, nullptr)) {
		if (Vec2i::intersect_test_lines(a_from, a_to, b_from, b_to)) {
			return false;
		}
	}

	//	static bool segment_intersects_segment_2d(const Vector2 &p_from_a, const Vector2 &p_to_a, const Vector2 &p_from_b, const Vector2 &p_to_b, Vector2 *r_result) {

	return true;
}

void OccluderShapeMesh::_debug_draw(const Vector<IndexedPoint> &p_points, String p_filename) {
#define GODOT_OCC_SHAPE_MESH_DEBUG_DRAW
#ifdef GODOT_OCC_SHAPE_MESH_DEBUG_DRAW
	if (!p_points.size()) {
		return;
	}

	DebugImage im;
	im.create(256, 256);
	Vector2 p_start = p_points[0].pos.vec2();
	p_start.x = -p_start.x;
	im.l_move(p_start);
	for (int n = 0; n < p_points.size(); n++) {
		Vector2 p = p_points[n].pos.vec2();
		p.x = -p.x;
		im.l_line_to(p);
	}
	im.l_line_to(p_start);
	im.l_flush();
	im.save_png(p_filename);
#endif
}

bool OccluderShapeMesh::_make_convex_chunk(const LocalVectori<uint32_t> &p_edge_verts, const Plane &p_poly_plane, LocalVectori<uint32_t> &r_convex_inds) {
	// cannot sort less than 3 verts
	if (p_edge_verts.size() < 3) {
		return false;
	}

	// simplify the problem to 2d
	Vector3 center(0, 0, 0);
	for (int n = 0; n < p_edge_verts.size(); n++) {
		center += _bd.verts[p_edge_verts[n]].posf;
	}
	center /= p_edge_verts.size();

	// transform to match the plane and center of the poly
	Transform tr;

	// prevent warnings when poly normal matches the up vector
	Vector3 up(0, 1, 0);
	if (Math::abs(p_poly_plane.normal.dot(up)) > 0.9) {
		up = Vector3(1, 0, 0);
	}

	tr.set_look_at(Vector3(0, 0, 0), p_poly_plane.normal, up);
	tr.origin = center;
	Transform tr_inv = tr.affine_inverse();

	// two passes, first calculate bound
	Rect2 bound;
	for (int n = 0; n < p_edge_verts.size(); n++) {
		const Vector3 &orig_pt = _bd.verts[p_edge_verts[n]].posf;
		Vector3 pt = tr_inv.xform(orig_pt);

		if (n == 0) {
			bound.position = Vector2(pt.x, pt.y);
		} else {
			bound.expand_to(Vector2(pt.x, pt.y));
		}
	}
	real_t longest_axis = MAX(bound.size.x, bound.size.y);
	real_t bound_mult = QUANTIZE_RES_2D / longest_axis;

	Vector<IndexedPoint> pts_orig;
	for (int n = 0; n < p_edge_verts.size(); n++) {
		const Vector3 &orig_pt = _bd.verts[p_edge_verts[n]].posf;
		Vector3 pt = tr_inv.xform(orig_pt);

		IndexedPoint ip;
		ip.pos.x = (pt.x - bound.position.x) * bound_mult;
		ip.pos.y = (pt.y - bound.position.y) * bound_mult;

		ip.idx = p_edge_verts[n];

		pts_orig.push_back(ip);
	}

	//#ifdef GODOT_OCC_SHAPE_MESH_DEBUG_DRAW
	//	_debug_draw(pts_orig, "output/facein.png");
	//#endif

#if 0
	// try out keil method
	Keil keil;
	List<Vector<IndexedPoint>> keil_results;
	keil.decompose(pts_orig, keil_results);

	// output keil
	List<Vector<IndexedPoint>>::Element *ele;
	ele = keil_results.front();

	while (ele) {
		const Vector<IndexedPoint> &P = ele->get();
		// output (possibly invert and lose the last one if a duplicate)
		r_convex_inds.resize(P.size());
		for (int n = 0; n < r_convex_inds.size(); n++) {
			r_convex_inds[n] = P[n].idx;
		}

		// create out face
		_bd.out_faces.resize(_bd.out_faces.size() + 1);
		BakeFace &out = _bd.out_faces[_bd.out_faces.size() - 1];

		out.indices = r_convex_inds;
		//out.indices = edges;
		out.area = 100.0;
		out.plane = p_poly_plane;
		_finalize_out_face(out);
		// calculate area

		ele = ele->next();
	}
	return true;
#endif

	////////////////////////

	// debugging
	//	Vector<IndexedPoint> P_orig;
	//	P_orig = P;

	// points that are split out because concave, are processed again to form a new face
	List<Vector<IndexedPoint>> remaining;
	remaining.push_back(pts_orig);

	int panic_count = 0;

	while (!remaining.empty()) {
		//		P_concave.clear();

		List<Vector<IndexedPoint>>::Element *curr_ele = remaining.front();
		Vector<IndexedPoint> &P = curr_ele->get();
		CRASH_COND(P.size() < 3);

		// always start from the left most point, this is to ensure that
		// the first edge is part of the convex hull
		Vec2i leftmost_pt = Vec2i(INT32_MAX, INT32_MAX);
		int leftmost = -1;

		for (int n = 0; n < P.size(); n++) {
			const IndexedPoint &ip = P[n];
			if (ip.pos < leftmost_pt) {
				leftmost_pt = ip.pos;
				leftmost = n;
			}
		}

		// rejig the list P so that the leftmost is first
		if (leftmost != 0) {
			Vector<IndexedPoint> temp;
			temp.resize(P.size());
			for (int n = 0; n < P.size(); n++) {
				temp.set(n, P[(leftmost + n) % P.size()]);
			}

			P = temp;
		}

#ifdef GODOT_OCC_SHAPE_MESH_DEBUG_DRAW
		_debug_draw(P, "input/facein" + itos(panic_count) + ".png");
#endif

		List<Vector<IndexedPoint>>::Element *extra_ele = nullptr;
		Vector<IndexedPoint> *extra = nullptr;

		// load first
		Vec2i prev2 = P[0].pos;
		Vec2i prev = P[1].pos;

		//		const real_t epsilon = 0.001;

		// only do containment tests once the poly forms an area
		// (i.e. not during colinear points at the start)
		bool poly_has_area = false;

		for (int n = 2; n < P.size(); n++) {
			// point to test
			const Vec2i &curr = P[n].pos;

			// new check against all previous edges
			bool allow = true;
			//real_t cross;
			int32_t cross;
			for (int c = 1; c < n; c++) {
				//cross = Geometry::vec2_cross(P[c - 1].pos, P[c].pos, curr);
				cross = P[c - 1].pos.cross(P[c].pos, curr);
				//				if (cross < -epsilon) {
				if (cross <= 0) { // <= prevents colinear
					allow = false;
					break;
				}
			}

			// disallow if the point is already on the edge list
			if (allow) {
				for (int c = 0; c < n; c++) {
					if (P[c].idx == P[n].idx) {
						allow = false;
						break;
					}
				}
			}

			// don't  allow if any later points are within the convex hull formed by former points and a line from n to 0
			// possibly use epsilon for the cross check?
			//			if (allow && (poly_has_area || cross > 0.0)) {
			if (allow && (poly_has_area || cross > 0)) {
				for (int t = n + 1; t < P.size(); t++) {
					bool inside = true;
					// test against all planes of the hull
					for (int c = 1; c < n + 1; c++) {
						//						real_t cross2 = Geometry::vec2_cross(P[c - 1].pos, P[c].pos, P[t].pos);
						//						if (cross2 < epsilon) {
						int32_t cross2 = P[c - 1].pos.cross(P[c].pos, P[t].pos);
						if (cross2 < 0) {
							inside = false;
							break;
						}
					} // for edge c

					// test the last edge
					//					real_t cross2 = Geometry::vec2_cross(P[n].pos, P[0].pos, P[t].pos);
					//					if (cross2 < epsilon) {
					int32_t cross2 = P[n].pos.cross(P[0].pos, P[t].pos);
					if (cross2 < 0) {
						inside = false;
					}

					if (inside) {
						allow = false;
						break;
					}

				} // for test point t
			}

			// each new point must be able to see the start point
			if (allow) {
				if (!_can_see(P, n)) {
					allow = false;
				}
			}

			// don't allow any further points AHEAD of n to 0, but less than the distance of this line
			if (allow) {
				int64_t last_edge_dist = (P[n].pos - P[0].pos).length_squared();
				for (int t = n + 1; t < P.size(); t++) {
					// test the last edge
					//					real_t cross2 = Geometry::vec2_cross(P[n].pos, P[0].pos, P[t].pos);
					//					if (cross2 > epsilon) {
					int32_t cross2 = P[n].pos.cross(P[0].pos, P[t].pos);
					if (cross2 > 0) {
						int64_t dist = (P[t].pos - P[0].pos).length_squared();
						if (dist < last_edge_dist) {
							allow = false;
							break;
						}
					}
				}
			}

			//			if (allow) {
			//				// don't allow any further points BEHIND the new edge
			//				for (int t = n + 1; t < P.size(); t++) {
			//					real_t cross2 = Geometry::vec2_cross(P[n - 1].pos, P[n].pos, P[t].pos);
			//					if (cross2 < 0.0) {
			//						allow = false;
			//						break;
			//					}
			//				}
			//			}

			//real_t cross = Geometry::vec2_cross(prev2, prev, curr);
			//print_line("point " + itos(n) + " cross: " + rtos(cross));

			// as well as the edge going backward, we also want to test further points
			// in case they form a concave polygon. If so, the point is disallowed
			//bool points_within = _any_further_points_within(P, n);
			//bool points_within = false;

			//			if ((cross < -epsilon) || points_within) {
			if (!allow) {
				// add for further processing
				// is a current extra open?
				if (!extra) {
					Vector<IndexedPoint> dummy;
					remaining.push_back(dummy);
					extra_ele = remaining.back();
					extra = &extra_ele->get();

					// push the previous vert
					extra->push_back(P[n - 1]);
				}
				extra->push_back(P[n]);

				// can't add this to the convex chunk
				P.remove(n);
				n--;
			} else {
				// special case .. if the last cross was very small, we can remove the previous point
				// as they are almost on the same line!
				if (cross < 0) {
					P.remove(n - 1);
					n--;
					prev = curr;
				} else {
					// added to the chunk, move along
					prev2 = prev;
					prev = curr;
					poly_has_area = true;
				}

				// close any open extra chunks
				if (extra) {
					extra->push_back(P[n]);
					extra = nullptr;
					extra_ele = nullptr;
				}

			} // if allowed
		}

		// if extra is still open, close it
		if (extra) {
			extra->push_back(P[0]);

			// special case .. if P is 2, then we have a triangle
			// facing the wrong way continuously being added to the remaining queue..
			// we need to prevent infinite loop
			if (P.size() < 3) {
				remaining.erase(extra_ele);
			}

			extra = nullptr;
			extra_ele = nullptr;
		}

#ifdef GODOT_OCC_SHAPE_MESH_DEBUG_DRAW
		_debug_draw(P, "output/faceout" + itos(panic_count) + ".png");
#endif

		// output (possibly invert and lose the last one if a duplicate)
		r_convex_inds.resize(P.size());
		for (int n = 0; n < r_convex_inds.size(); n++) {
			r_convex_inds[n] = P[n].idx;
		}

		// create out face
		_bd.out_faces.resize(_bd.out_faces.size() + 1);
		BakeFace &out = _bd.out_faces[_bd.out_faces.size() - 1];

		out.indices = r_convex_inds;
		//out.indices = edges;
		out.area = 100.0;
		out.plane = p_poly_plane;
		_finalize_out_face(out);
		// calculate area

		//P = P_concave;

		// delete the current from remaining
		remaining.erase(curr_ele);

		panic_count++;
		if (panic_count >= 64) {
			WARN_PRINT_ONCE("OcclusionShapeMesh detected infinite loop");
			break;
		}
	} // while there are chunks remaining

	return true;
}

void OccluderShapeMesh::_finalize_out_face(BakeFace &r_face) {
	LocalVector<Vector3> pts;
	for (int n = 0; n < r_face.indices.size(); n++) {
		pts.push_back(_bd.verts[r_face.indices[n]].posf);
	}
	r_face.area = Geometry::find_polygon_area(&pts[0], pts.size());
}

void OccluderShapeMesh::_find_convex_chunk(const LocalVectori<uint32_t> &p_edge_verts, LocalVectori<uint32_t> &r_convex, const Plane &p_poly_plane) {
	// first find average normal of the entire zone using newell method
	//	Vector3 normal = _normal_from_edge_verts_newell(p_edge_verts);

	//	// compare the normal to the first tri
	//	real_t dot = normal.dot(p_poly_plane.normal);
	//	if (dot < 0.0) {
	//		normal = -normal;
	//	}
	//	normal = p_first_tri_normal;

	// get all the points in 2d
	//LocalVectori<uint32_t> relative_inds;
	_make_convex_chunk(p_edge_verts, p_poly_plane, r_convex);
}

Vector3 OccluderShapeMesh::_normal_from_edge_verts_newell(const LocalVectori<uint32_t> &p_edge_verts) const {
	int num_points = p_edge_verts.size();

	// should not happen hopefully
	if (num_points < 3) {
		return Vector3(1, 0, 0);
	}

	Vector3 normal;

	for (int i = 0; i < num_points; i++) {
		int j = (i + 1) % num_points;

		const Vector3 &pi = _bd.verts[i].posf;
		const Vector3 &pj = _bd.verts[j].posf;

		normal.x += (((pi.z) + (pj.z)) * ((pj.y) - (pi.y)));
		normal.y += (((pi.x) + (pj.x)) * ((pj.z) - (pi.z)));
		normal.z += (((pi.y) + (pj.y)) * ((pj.x) - (pi.x)));
	}

	normal.normalize();
	return normal;
}

bool OccluderShapeMesh::_make_faces_new(uint32_t p_process_tick) {
//#define GODOT_OCCLUDER_SHAPE_MESH_SINGLE_FACE
// find a seed face
#ifdef GODOT_OCCLUDER_SHAPE_MESH_SINGLE_FACE
	for (int n = _settings_debug_face_id; n < _bd.faces.size(); n++) {
#else
	for (int n = 0; n < _bd.faces.size(); n++) {
#endif
		if (!_bd.faces[n].done) {
			BakeFace &face = _bd.faces[n];

			// is it suitable for starting from?
			// must have at least 1 neighbour face, and not have all neighbour faces
			int num_neighs = 0;

			for (int n = 0; n < face.neighbour_face_ids.size(); n++) {
				if (face.neighbour_face_ids[n] != UINT32_MAX) {
					num_neighs++;
				}
			}

			// if no neighbours, special case, mark as done, as it will always remain
			// a triangle (aww)
			if (!num_neighs) {
				face.done = true;

				// add to out faces

				// create out face
				_bd.out_faces.resize(_bd.out_faces.size() + 1);
				BakeFace &out = _bd.out_faces[_bd.out_faces.size() - 1];
				out = face;
				//				out.indices = r_convex_inds;
				//				out.area = 100.0;

				continue;
			}

			// XMAS must have 2 or more non-neighbours
			if (num_neighs > face.neighbour_face_ids.size() - 2) {
				continue;
			}

			// if ALL sides are neighbours, can't use it as a seed for a zone
			// FOLLOWS FROM ABOVE
			if (num_neighs == face.neighbour_face_ids.size()) {
				continue;
				// could mark as done to prevent checking multiple times?
			}

			LocalVectori<uint32_t> face_ids;
			_flood_fill_from_face(n, p_process_tick, face_ids);

			if (_merge_face_zone(face_ids)) {
				return true;
			}
		}

#ifdef GODOT_OCCLUDER_SHAPE_MESH_SINGLE_FACE
		break;
#endif
	}

	return false;
}

bool OccluderShapeMesh::_flood_fill_from_face(int p_face_id, uint32_t p_process_tick, LocalVectori<uint32_t> &r_face_ids) {
	BakeFace &face = _bd.faces[p_face_id];
	if (face.last_processed_tick == p_process_tick) {
		return false;
	}

	r_face_ids.push_back(p_face_id);

	// only hit once per flood
	face.last_processed_tick = p_process_tick;

	// always mark as done
	face.done = true;

	// recurse through neighbours
	for (int n = 0; n < face.neighbour_face_ids.size(); n++) {
		uint32_t id = face.neighbour_face_ids[n];
		if (id != UINT32_MAX) {
			_flood_fill_from_face(id, p_process_tick, r_face_ids);
		}
	}

	return false;
}

bool OccluderShapeMesh::_make_faces(uint32_t p_process_tick) {
	bool found_one = false;

	// for now just recreate this each time
	_create_sort_faces();

	// maintain a list of failed combos, so as not to keep trying the
	// same ones
	LocalVectori<LocalVectori<uint32_t>> disallowed;

	// go through from the largest faces to the smallest
	for (int sort_face_id = 0; sort_face_id < _bd.sort_faces.size(); sort_face_id++) {
		int face_id = _bd.sort_faces[sort_face_id].id;
		//_log("source_face_id " + itos(face_id));

		const BakeFace &source_face = _bd.faces[face_id];

		for (int which_vert = 0; which_vert < source_face.indices.size(); which_vert++) {
			int v = source_face.indices[which_vert];

			//for (int v = 0; v < _bd.verts.size(); v++) {
			BakeVertex &vert = _bd.verts[v];

			// has the vert been done already?
			if (vert.last_processed_tick == p_process_tick) {
				continue;
			}

			// mark as done
			vert.last_processed_tick = p_process_tick;

			if (!vert.is_active()) {
				continue;
			}

			// dirty?
			if (!vert.dirty) {
				continue;
			} else {
				vert.dirty = false;
			}

			int num_faces = vert.linked_faces.size();

			if (num_faces <= 1) {
				continue;
			}

			_log("vert_id " + itos(v));

			// find maximum number of matching faces
			LocalVectori<uint32_t> matching_faces;
			LocalVectori<uint32_t> test_matching_faces;
			LocalVectori<uint32_t> best_matching_faces;
			real_t best_matching_faces_area = 0.0;
			real_t fit = 0.0;

			// unused
			int matching_edge_a;
			int matching_edge_b;

			for (int a = 0; a < num_faces; a++) {
				matching_faces.clear();
				matching_faces.push_back(vert.linked_faces[a]);
				const BakeFace &fa = _bd.faces[vert.linked_faces[a]];

				real_t matching_faces_area = 0.0;

				for (int b = a + 1; b < num_faces; b++) {
					const BakeFace &fb = _bd.faces[vert.linked_faces[b]];

					if (_are_faces_coplanar_for_merging(fa, fb, fit)) {
						_log("\tfrom face_id " + itos(vert.linked_faces[a]) + " to face_id " + itos(vert.linked_faces[b]));

						// must be a neighbour of an existing face
						bool is_neighbour = false;
						for (int n = 0; n < matching_faces.size(); n++) {
							const BakeFace &neigh_face = _bd.faces[matching_faces[n]];
							if (_are_faces_neighbours(neigh_face, fb, matching_edge_a, matching_edge_b)) {
								is_neighbour = true;
								break;
							}
						}

						if (is_neighbour) {
							_log("\t\tis_neighbour");
							test_matching_faces.resize(1);
							test_matching_faces[0] = matching_faces[0];
							test_matching_faces.push_back(vert.linked_faces[b]);

							if (!_are_faces_disallowed(disallowed, test_matching_faces)) {
								//_log("\t\tarea : " + rtos(_bd.faces[matching_faces

								// better fit?
								real_t area = _find_matching_faces_total_area(test_matching_faces);
								if (area > matching_faces_area) {
									_log("\t\tarea > matching_faces_area");
									matching_faces_area = area;
									matching_faces = test_matching_faces;
								} else {
									_log("\t\tarea <= matching_faces_area");
								}
							} else {
								_log("\t\tDISALLOWED");
							}
						}
					}
				} // for b

				if (matching_faces.size() < 2)
					continue;

				// better than the best so far?
				real_t area = _find_matching_faces_total_area(matching_faces);

				//bool good_to_add = matching_faces.size() > best_matching_faces.size();
				bool good_to_add = area > best_matching_faces_area;

				if (good_to_add) {
					_log("\t\tadding to best_matching_faces");
					best_matching_faces = matching_faces;
					best_matching_faces_area = area;
				}

			} // for a

			// no good, can't match any faces from this vertex
			if (best_matching_faces.size() < 2) {
				disallowed.clear();
				continue;
			}

			_log("\t\t_merge_matching_faces " + itos(best_matching_faces[0]) + " to " + itos(best_matching_faces[1]));
			if (_merge_matching_faces(best_matching_faces)) {
				disallowed.clear();
				return true;
			} else {
				// don't allow this combo next time
				disallowed.push_back(best_matching_faces);

				// repeat for this vertex
				v--;
			}

		} // for vertex in sort face
	} // for sort face

	return found_one;
}

real_t OccluderShapeMesh::_find_matching_faces_total_area(const LocalVectori<uint32_t> &p_faces) const {
	real_t area = 0.0;
	for (int n = 0; n < p_faces.size(); n++) {
		area += _bd.faces[p_faces[n]].area;
	}
	return area;
}

bool OccluderShapeMesh::_are_faces_disallowed(const LocalVectori<LocalVectori<uint32_t>> &p_disallowed, const LocalVectori<uint32_t> &p_faces) const {
	for (int n = 0; n < p_disallowed.size(); n++) {
		const LocalVectori<uint32_t> &testlist = p_disallowed[n];

		if (testlist.size() != p_faces.size()) {
			continue;
		}

		bool match = true;
		for (int i = 0; i < testlist.size(); i++) {
			if (testlist[i] != p_faces[i]) {
				match = false;
				break;
			}
		}

		if (match) {
			return true;
		}
	}

	return false;
}

void OccluderShapeMesh::_process_islands() {
}

void OccluderShapeMesh::_find_neighbour_face_ids() {
	// make sure face edge neighbours array correct size
	for (int n = 0; n < _bd.faces.size(); n++) {
		BakeFace &face = _bd.faces[n];

		int num_sides = face.indices.size();
		face.neighbour_face_ids.resize(num_sides);
		face.adjacent_face_ids.resize(num_sides);

		// blank
		for (int i = 0; i < num_sides; i++) {
			face.neighbour_face_ids[i] = UINT32_MAX;
			face.adjacent_face_ids[i] = UINT32_MAX;
		}
	}

	// first find all adjacent faces
	for (int n = 0; n < _bd.verts.size(); n++) {
		const BakeVertex &bv = _bd.verts[n];

		for (int i = 0; i < bv.linked_faces.size(); i++) {
			uint32_t linked_face_a_id = bv.linked_faces[i];
			BakeFace &face_a = _bd.faces[linked_face_a_id];

			for (int j = i + 1; j < bv.linked_faces.size(); j++) {
				uint32_t linked_face_b_id = bv.linked_faces[j];
				BakeFace &face_b = _bd.faces[linked_face_b_id];

				int edge_a, edge_b;
				if (!_are_faces_neighbours(face_a, face_b, edge_a, edge_b)) {
					continue;
				}

				face_a.adjacent_face_ids[edge_a] = linked_face_b_id;
				face_b.adjacent_face_ids[edge_b] = linked_face_a_id;
			} // for j

		} // for i
	} // for n

	// flood fill from each face
	// make sure face edge neighbours array correct size
	LocalVector<uint32_t> face_stack;
	uint32_t island_id = 0;

	// dummy first island
	_bd.islands.resize(1);

	for (int n = 0; n < _bd.faces.size(); n++) {
		if (_bd.faces[n].island != 0) {
			continue;
		}
		face_stack.clear();
		face_stack.push_back(n);

		// start a new island
		island_id++;
		_bd.islands.resize(_bd.islands.size() + 1);
		BakeIsland &island = _bd.islands[_bd.islands.size() - 1];
		island.first_face_id = n;

		while (!face_stack.empty()) {
			// pop face
			uint32_t face_id_a = face_stack[face_stack.size() - 1];
			face_stack.resize(face_stack.size() - 1);
			BakeFace &face_a = _bd.faces[face_id_a];

			// done already
			if (face_a.island)
				continue;

			// mark which island
			face_a.island = island_id;

			// traverse to neighbours
			for (int n = 0; n < face_a.num_sides(); n++) {
				uint32_t face_id_b = face_a.adjacent_face_ids[n];
				if (face_id_b != UINT32_MAX) {
					BakeFace &face_b = _bd.faces[face_id_b];

					// only consider them if they are coplanar
					real_t fit;
					if (!_are_faces_coplanar_for_merging(face_a, face_b, fit)) {
						continue;
					}

					int edge_a, edge_b;
					if (!_are_faces_neighbours(face_a, face_b, edge_a, edge_b)) {
						continue;
					}

					face_a.neighbour_face_ids[edge_a] = face_id_b;
					face_b.neighbour_face_ids[edge_b] = face_id_a;

					// add the neighbour to the stack
					face_stack.push_back(face_id_b);
				}
			} // for n
		} // while stack not empty
	}

	print_line("num islands " + itos(island_id));

	// join together all faces in the same island
	for (uint32_t id_a = 0; id_a < _bd.faces.size(); id_a++) {
		BakeFace &face_a = _bd.faces[id_a];

		for (int n = 0; n < face_a.num_sides(); n++) {
			uint32_t id_b = face_a.adjacent_face_ids[n];
			if ((face_a.neighbour_face_ids[n] == UINT32_MAX) && (id_b != UINT32_MAX)) {
				BakeFace &face_b = _bd.faces[id_b];
				if (face_a.island != face_b.island)
					continue;

				// they are the same island
				int edge_a, edge_b;
				if (!_are_faces_neighbours(face_a, face_b, edge_a, edge_b)) {
					continue;
				}

				face_a.neighbour_face_ids[edge_a] = id_b;
				face_b.neighbour_face_ids[edge_b] = id_a;
			}
		} // for n
	}

	//	struct SeedNormal
	//	{
	//		SeedNormal() {done = false;}
	//		bool done;
	//		Vector3 normal;
	//	};
	/*
	LocalVector<SeedNormal> seed_normals;
	seed_normals.resize(_bd.faces.size());

	for (int n = 0; n < _bd.verts.size(); n++) {
		const BakeVertex &bv = _bd.verts[n];

		for (int i = 0; i < bv.linked_faces.size(); i++) {
			uint32_t linked_face_a_id = bv.linked_faces[i];
			BakeFace &face_a = _bd.faces[linked_face_a_id];

			for (int j = i + 1; j < bv.linked_faces.size(); j++) {
				uint32_t linked_face_b_id = bv.linked_faces[j];
				BakeFace &face_b = _bd.faces[linked_face_b_id];

				int edge_a, edge_b;
				if (!_are_faces_neighbours(face_a, face_b, edge_a, edge_b)) {
					continue;
				}

//				SeedNormal &sn_a = seed_normals[linked_face_a_id];
//				SeedNormal &sn_b = seed_normals[linked_face_b_id];


				// only consider them if they are coplanar
				real_t fit;
				if (!_are_faces_coplanar_for_merging(face_a, face_b, fit)) {
					continue;
				}

//				bool _are_faces_coplanar_for_merging(const BakeFace &p_a, const BakeFace &p_b, real_t &r_fit) const {
//					r_fit = p_a.plane.normal.dot(p_b.plane.normal);
//					return r_fit >= _settings_plane_simplify_dot;
//				}


//				int edge_a, edge_b;
//				if (_are_faces_neighbours(face_a, face_b, edge_a, edge_b)) {
					face_a.neighbour_face_ids[edge_a] = linked_face_b_id;
					face_b.neighbour_face_ids[edge_b] = linked_face_a_id;
//				}
			} // for j

		} // for i
	} // for n
	*/
	// do another pass to join all of the same island together as neighbours
}

void OccluderShapeMesh::_verify_verts() {
	return;
#ifdef TOOLS_ENABLED
	for (int n = 0; n < _bd.verts.size(); n++) {
		const BakeVertex &bv = _bd.verts[n];

		for (int i = 0; i < bv.linked_faces.size(); i++) {
			uint32_t linked_face_id = bv.linked_faces[i];
			const BakeFace &face = _bd.faces[linked_face_id];

			int found = face.indices.find(n);
			CRASH_COND(found == -1);
		}
	}
#endif
}

bool OccluderShapeMesh::_are_faces_neighbours(const BakeFace &p_a, const BakeFace &p_b, int &r_edge_a, int &r_edge_b) const {
	for (int n = 0; n < p_a.indices.size(); n++) {
		int a0 = p_a.indices[n];
		int a1 = p_a.indices[(n + 1) % p_a.indices.size()];

		if (a1 < a0) {
			SWAP(a0, a1);
		}

		for (int m = 0; m < p_b.indices.size(); m++) {
			int b0 = p_b.indices[m];
			int b1 = p_b.indices[(m + 1) % p_b.indices.size()];

			if (b1 < b0) {
				SWAP(b0, b1);
			}

			if ((a0 == b0) && (a1 == b1)) {
				// return which edges are neighbours
				r_edge_a = n;
				r_edge_b = m;
				return true;
			}
		}
	}

	return false;
}

bool OccluderShapeMesh::_merge_matching_faces(const LocalVectori<uint32_t> &p_faces) {
	// count indices
	int count = 0;
	for (int n = 0; n < p_faces.size(); n++) {
		int face_id = p_faces[n];
		const BakeFace &fa = _bd.faces[face_id];
		count += fa.indices.size();
	}

	// add ALL the verts
	BakeFace new_face;
	new_face.indices.resize(count);
	count = 0;

	new_face.plane.normal = Vector3(0, 0, 0);
	new_face.plane.d = 0;
	real_t old_area = 0.0;

	for (int n = 0; n < p_faces.size(); n++) {
		int face_id = p_faces[n];
		const BakeFace &fa = _bd.faces[face_id];

		real_t area = fa.area;

		// average this
		new_face.plane.normal += fa.plane.normal * area;
		new_face.plane.d += fa.plane.d * area;

		old_area += area;

		for (int i = 0; i < fa.indices.size(); i++) {
			new_face.indices[count++] = fa.indices[i];
		}
	}

	// average the new plane
	if (old_area > 0.001) {
		new_face.plane.normal /= old_area;
		new_face.plane.d /= old_area;
	} else {
		// else face too small .. can't merge these,
		// we'd end up getting float error...
		return false;
	}

	// now remove the central, and any duplicates
	_tri_face_remove_central_and_duplicates(new_face, -1);

	BakeFace new_face_before_simplify = new_face;

	// return false if not convex
	real_t new_total_area = 0.0;

	if (!_create_merged_convex_face(new_face, old_area, new_total_area)) {
		return false; // not convex etc
	}

	// Impose a maximum number of verts for a face
	// (this is applied in the visual server anyway, so we want to avoid
	// merging such faces).
	// The reason for this maximum is that runtime speed depends on the number of edges.
	if (new_face.indices.size() > PortalDefines::OCCLUSION_POLY_MAX_VERTS) {
		return false;
	}

	// resave
	int new_face_id = _bd.faces.size();
	new_face.area = new_total_area;
	_bd.faces.push_back(new_face);

	// we need to remove the old face links from all the old vertices BEFORE
	// simplification (as some verts may have been removed)
	for (int n = 0; n < new_face_before_simplify.indices.size(); n++) {
		int idx = new_face_before_simplify.indices[n];
		BakeVertex &bv = _bd.verts[idx];

		for (int i = 0; i < p_faces.size(); i++) {
			uint32_t old_linked_face = p_faces[i];
			bv.remove_linked_face(old_linked_face);
		}
	}

	// store the new linked face in the vertices AFTER simplification
	for (int n = 0; n < new_face.indices.size(); n++) {
		int idx = new_face.indices[n];
		BakeVertex &bv = _bd.verts[idx];

		// add the new face
		bv.add_linked_face(new_face_id);
	}

	// remove source faces
	BakeFace face_blank;
	for (int n = 0; n < p_faces.size(); n++) {
		_bd.faces[p_faces[n]] = face_blank;
	}

	return true;
}

bool OccluderShapeMesh::_face_replace_index(Geometry::MeshData::Face &r_face, uint32_t p_face_id, int p_del_index, int p_replace_index, bool p_fix_vertex_linked_faces) {
	int64_t found = r_face.indices.find(p_del_index);
	if (found != -1) {
		r_face.indices.set(found, p_replace_index);

		if (p_fix_vertex_linked_faces) {
			_bd.verts[p_del_index].remove_linked_face(p_face_id);
			_bd.verts[p_replace_index].add_linked_face(p_face_id);
		}

		return true;
	}
	return false;
}

void OccluderShapeMesh::_tri_face_remove_central_and_duplicates(BakeFace &p_face, uint32_t p_central_idx) const {
	for (int n = 0; n < p_face.indices.size(); n++) {
		uint32_t idx = p_face.indices[n];
		if (idx == p_central_idx) {
			// remove this index, and repeat the element
			p_face.indices.remove(n);
			n--;
		} else {
			// check for duplicates
			for (int i = n + 1; i < p_face.indices.size(); i++) {
				uint32_t idx2 = p_face.indices[i];

				// if we find a duplicate, remove the first occurrence
				if (idx == idx2) {
					// remove this index, and repeat the element
					p_face.indices.remove(n);
					n--;
					break;
				}
			}
		}
	}
}

void OccluderShapeMesh::_process_out_faces() {
	_bd.faces = _bd.out_faces;
}

void OccluderShapeMesh::_finalize_faces() {
	// save into the geometry::mesh
	// first delete blank faces in the mesh
	for (int n = 0; n < _bd.faces.size(); n++) {
		const BakeFace &face = _bd.faces[n];

		// delete the face if no indices, or the size below the threshold
		if (!face.indices.size() || (face.area < _settings_threshold_output_size)) {
			// face can be deleted
			_bd.faces.remove(n);

			// repeat this element
			n--;
		}
	}

	LocalVector<uint32_t> vertex_remaps;
	vertex_remaps.resize(_bd.verts.size());
	for (uint32_t n = 0; n < vertex_remaps.size(); n++) {
		vertex_remaps[n] = UINT32_MAX;
	}

	for (int n = 0; n < _bd.faces.size(); n++) {
		BakeFace face = _bd.faces[n];

		// remap the indices
		for (int c = 0; c < face.indices.size(); c++) {
			int index_before = face.indices[c];
			uint32_t &index_after = vertex_remaps[index_before];

			// vertex not done yet
			if (index_after == UINT32_MAX) {
				_mesh_data.vertices.push_back(_bd.verts[index_before].posf);
				index_after = _mesh_data.vertices.size() - 1;
			}

			// save the after index in the face
			face.indices[c] = index_after;

			// resave the face
			_bd.faces[n] = face;
		}
	}

	// copy the bake faces to the mesh faces
	_mesh_data.faces.resize(_bd.faces.size());
	for (int n = 0; n < _mesh_data.faces.size(); n++) {
		const BakeFace &src = _bd.faces[n];
		Geometry::MeshData::Face dest;
		dest.plane = src.plane;

		dest.indices.resize(src.indices.size());
		for (int i = 0; i < src.indices.size(); i++) {
			dest.indices.set(i, src.indices[i]);
		}

		_mesh_data.faces.set(n, dest);
	}

	// display some stats
	print_line("OccluderShapeMesh faces : " + itos(_mesh_data.faces.size()) + ", verts : " + itos(_mesh_data.vertices.size()));
}

String OccluderShapeMesh::_vec3_to_string(const Vector3 &p_pt) const {
	String str;
	str = "(";
	str += itos(p_pt.x);
	str += ", ";
	str += itos(p_pt.y);
	str += ", ";
	str += itos(p_pt.z);
	str += ") ";
	return str;
}

void OccluderShapeMesh::_bake_quantize_float_faces() {
	_bd.input_aabb = AABB();

	if (!_bd.float_input_faces.size()) {
		return;
	}

	_bd.input_aabb.position = _bd.float_input_faces[0].vertex[0];

	for (int n = 0; n < _bd.float_input_faces.size(); n++) {
		const Face3 &face = _bd.float_input_faces[n];

		for (int i = 0; i < 3; i++) {
			const Vector3 &v = face.vertex[i];
			_bd.input_aabb.expand_to(v);
		}
	}

	// calculate multiplier to go from world space to integers and vice verse
	real_t world_size = _bd.input_aabb.get_longest_axis_size();
	_bd.world_to_int_multiplier = 0.0;
	_bd.int_to_world_multiplier = 0.0;
	if (world_size > 0.001) {
		_bd.world_to_int_multiplier = (real_t)QUANTIZE_RES / world_size;
		_bd.int_to_world_multiplier = world_size / (real_t)QUANTIZE_RES;
	}

	// now add the faces
	for (int n = 0; n < _bd.float_input_faces.size(); n++) {
		const Face3 &face = _bd.float_input_faces[n];
		_bake_input_face(face);
	}
}

void OccluderShapeMesh::_bake_input_face(const Face3 &p_face) {
	_log("_bake_input_face " + _vec3_to_string(p_face.vertex[0]) + _vec3_to_string(p_face.vertex[1]) + _vec3_to_string(p_face.vertex[2]));

	BakeFace face;
	face.plane = Plane(p_face.vertex[0], p_face.vertex[1], p_face.vertex[2]);

	uint32_t inds[3];
	face.indices.resize(3);
	for (int c = 0; c < 3; c++) {
		face.indices[c] = _find_or_create_vert(p_face.vertex[c]);
		inds[c] = face.indices[c];
	}

	// already baked this face? a duplicate so ignore
	uint32_t stored_face_id = _bd.hash_triangles.find(inds);
	if (stored_face_id != UINT32_MAX)
		return;

	_bd.hash_triangles.add(inds, _bd.faces.size());
	face.area = p_face.get_area();
	_bd.faces.push_back(face);

	_log("\tbaked initial face");
}

bool OccluderShapeMesh::_try_bake_face(const Face3 &p_face) {
	real_t area = p_face.get_twice_area_squared() * 0.5;
	if (area < _settings_threshold_input_size_squared) {
		return false;
	}

	// face is big enough to use as occluder
	BakeFace face;
	face.plane = Plane(p_face.vertex[0], p_face.vertex[1], p_face.vertex[2]);

	// reject floors
	if ((_settings_remove_floor_angle) && (Math::abs(face.plane.normal.y) > (_settings_remove_floor_dot))) {
		return false;
	}

	_bd.float_input_faces.push_back(p_face);
	return true;
}

real_t OccluderShapeMesh::_find_face_area(const Geometry::MeshData::Face &p_face) const {
	int num_inds = p_face.indices.size();

	Vector<Vector3> pts;
	for (int n = 0; n < num_inds; n++) {
		pts.push_back(_bd.verts[p_face.indices[n]].posf);
	}

	return Geometry::find_polygon_area(pts.ptr(), pts.size());
}

// return false if not convex
bool OccluderShapeMesh::_create_merged_convex_face(BakeFace &r_face, real_t p_old_face_area, real_t &r_new_total_area) {
	int num_inds = r_face.indices.size();

	Vector<Vector3> pts;
	pts.resize(num_inds);
	for (int n = 0; n < num_inds; n++) {
		pts.set(n, _bd.verts[r_face.indices[n]].posf);
	}
	Vector<Vector3> pts_changed = pts;

	// make the new face convex, remove any redundant verts, find the area and compare to the old
	// poly area plus the new face, to detect invalid joins
	if (!Geometry::make_polygon_convex(pts_changed, -r_face.plane.normal)) {
		return false;
	}
	CRASH_COND(pts_changed.size() > num_inds);

	// reverse order, as we want counter clockwise polys
	//pts_changed.invert();

	r_new_total_area = Geometry::find_polygon_area(pts_changed.ptr(), pts_changed.size());

	// conditions to disallow invalid joins
	real_t diff = p_old_face_area - r_new_total_area;

	// this epsilon here is crucial for tuning to prevent
	// artifact 'overlap'. Can this be automatically determined from
	// scale? NYI
	if (Math::abs(diff) > 0.01) { // 0.01
		return false;
	}

	LocalVectori<uint32_t> new_inds;

	for (int n = 0; n < pts_changed.size(); n++) {
		for (int i = 0; i < num_inds; i++) {
			if (pts_changed[n] == pts[i]) {
				new_inds.push_back(r_face.indices[i]);
				break;
			}
		}
	}

	// now should have the new indices
	r_face.indices = new_inds;

	return true;
}

bool OccluderShapeMesh::_bake_material_check(Ref<Material> p_material) {
	SpatialMaterial *spat = Object::cast_to<SpatialMaterial>(p_material.ptr());
	if (spat && spat->get_feature(SpatialMaterial::FEATURE_TRANSPARENT)) {
		return false;
	}

	return true;
}

void OccluderShapeMesh::_bake_recursive(Spatial *p_node) {
	VisualInstance *vi = Object::cast_to<VisualInstance>(p_node);

	// check the flags, visible, static etc
	if (vi && vi->is_visible_in_tree() && (vi->get_portal_mode() == CullInstance::PORTAL_MODE_STATIC)) {
		bool valid = true;

		// material check for transparency
		GeometryInstance *gi = Object::cast_to<GeometryInstance>(p_node);
		if (gi && valid && !_bake_material_check(gi->get_material_override())) {
			valid = false;
		}

		if ((vi->get_layer_mask() & _settings_bake_mask) == 0) {
			valid = false;
		}

		if (valid) {
			Transform tr = vi->get_global_transform();

			PoolVector<Face3> faces = vi->get_faces(VisualInstance::FACES_SOLID);

			if (faces.size()) {
				_log("baking : " + p_node->get_name());

				for (int n = 0; n < faces.size(); n++) {
					Face3 face = faces[n];

					// transform to world space
					for (int c = 0; c < 3; c++) {
						face.vertex[c] = tr.xform(face.vertex[c]);
					}

					_try_bake_face(face);
				}
			} // if there were faces

		} // if valid
	}

	for (int n = 0; n < p_node->get_child_count(); n++) {
		Spatial *child = Object::cast_to<Spatial>(p_node->get_child(n));
		if (child) {
			_bake_recursive(child);
		}
	}
}

OccluderShapeMesh::OccluderShapeMesh() :
		OccluderShape(VisualServer::get_singleton()->occluder_create()) {
}
