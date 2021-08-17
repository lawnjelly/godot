#include "occluder_shape_poly.h"

#include "scene/3d/spatial.h"
#include "scene/3d/visual_instance.h"
#include "servers/visual_server.h"

void OccluderShapePoly::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_bake_path", "path"), &OccluderShapePoly::set_bake_path);
	ClassDB::bind_method(D_METHOD("get_bake_path"), &OccluderShapePoly::get_bake_path);

	ClassDB::bind_method(D_METHOD("set_threshold_size", "size"), &OccluderShapePoly::set_threshold_size);
	ClassDB::bind_method(D_METHOD("get_threshold_size"), &OccluderShapePoly::get_threshold_size);

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "bake_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Spatial"), "set_bake_path", "get_bake_path");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "threshold_size", PROPERTY_HINT_RANGE, "0.0,10.0,0.1"), "set_threshold_size", "get_threshold_size");
}

void OccluderShapePoly::update_transform_to_visual_server(const Transform &p_global_xform) {
	VisualServer::get_singleton()->occluder_set_transform(get_shape(), p_global_xform);
}

void OccluderShapePoly::update_shape_to_visual_server() {
	VisualServer::get_singleton()->occluder_polys_update(get_shape(), _mesh_data.faces, _mesh_data.vertices);
}

Transform OccluderShapePoly::center_node(const Transform &p_global_xform, const Transform &p_parent_xform, real_t p_snap) {
	return Transform();
}

void OccluderShapePoly::notification_enter_world(RID p_scenario) {
	VisualServer::get_singleton()->occluder_set_scenario(get_shape(), p_scenario, VisualServer::OCCLUDER_TYPE_POLY);
}

void OccluderShapePoly::clear() {
	_mesh_data.vertices.clear();
	_mesh_data.faces.clear();
	_faces_areas.clear();
}

void OccluderShapePoly::_log(String p_string) {
	print_line(p_string);
}

void OccluderShapePoly::bake(Node *owner) {
	clear();

	// try to get the nodepath
	if (owner->has_node(_bake_path)) {
		Node *node = owner->get_node(_bake_path);

		Spatial *branch = Object::cast_to<Spatial>(node);
		if (!branch) {
			return;
		}

		_bake_recursive(branch);

		// hard code
		//		real_t s = 10.0;
		//		Face3 f;
		//		f.vertex[0] = Vector3(0, 0, 0);
		//		f.vertex[1] = Vector3(s, s, 0);
		//		f.vertex[2] = Vector3(s, 0, 0);
		//_try_bake_face(f);
	}

	// try again iteratively to merge faces
	bool merged_any = true;

	while (merged_any) {
		merged_any = false;
		for (int n = 0; n < _mesh_data.faces.size(); n++) {
			const Geometry::MeshData::Face &md_face = _mesh_data.faces[n];

			if (md_face.indices.size() == 3) {
				Face3 face;
				for (int c = 0; c < 3; c++) {
					face.vertex[c] = _mesh_data.vertices[md_face.indices[c]];
				}

				if (_try_merge_face(face, n)) {
					// remove the original face
					_mesh_data.faces.remove(n);

					// repeat this face id
					n--;

					merged_any = true;
				}
			}
		}
	} // while loop

	update_shape_to_visual_server();
}

bool OccluderShapePoly::_try_merge_face(const Face3 &p_face, int p_ignore_face) {
	// we need this to detect invalid joins
	real_t new_face_area = p_face.get_area();
	_log("\tnew_face_area : " + rtos(new_face_area));

	// face is big enough to use as occluder
	Geometry::MeshData::Face face;
	face.plane = Plane(p_face.vertex[0], p_face.vertex[1], p_face.vertex[2]);

	// first attempt to join to an existing face
	for (int f = 0; f < _mesh_data.faces.size(); f++) {
		if (f == p_ignore_face) {
			continue;
		}

		const Geometry::MeshData::Face &tface = _mesh_data.faces[f];

		//		if (tface.indices.size() > 3) {
		//			continue;
		//		}

		// is the face plane matching?
		if (!Math::is_equal_approx(tface.plane.d, face.plane.d, (real_t)0.005)) {
			continue;
		}
		real_t dot = tface.plane.normal.dot(face.plane.normal);
		if (dot < 0.99) {
			continue;
		}

		int matching_verts = 0;

		int test_face_matched_index[3];
		int face_matched_index[3];

		for (int i = 0; i < tface.indices.size(); i++) {
			const Vector3 &pt = _mesh_data.vertices[tface.indices[i]];

			for (int c = 0; c < 3; c++) {
				if (pt.is_equal_approx(p_face.vertex[c])) {
					test_face_matched_index[matching_verts] = i;
					face_matched_index[matching_verts] = c;

					matching_verts++;
				}
			}
		}

		if (matching_verts == 3) {
			// duplicate face, do not add
			return true;
		}

		if (matching_verts != 2) {
			continue;
		}

		//print_line("match found");

		int face_new_index = 3 - (face_matched_index[0] + face_matched_index[1]);

		// add the new vertex
		int new_vertex_index = _mesh_data.vertices.size();
		_mesh_data.vertices.push_back(p_face.vertex[face_new_index]);

		// add the index to the test face
		Geometry::MeshData::Face new_face = tface;
		new_face.indices.push_back(new_vertex_index);

		// return false if not convex
		real_t new_total_area = 0.0;

		if (!_sort_face_winding(new_face, new_face_area, _faces_areas[f], new_total_area)) {
			continue;
		}

		// set the new face into the mesh data
		_mesh_data.faces.set(f, new_face);
		_faces_areas[f] = new_total_area;
		return true;
	}

	return false;
}

String OccluderShapePoly::_vec3_to_string(const Vector3 &p_pt) const {
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

void OccluderShapePoly::_try_bake_face(const Face3 &p_face) {
	//	real_t area = p_face.get_twice_area_squared();
	//	if (area < 1000) {
	//		return;
	//	}

	//const real_t threshold = 1.0 * 1.0;

	Vector3 e[3];
	for (int n = 0; n < 3; n++) {
		const Vector3 &v0 = p_face.vertex[n];
		const Vector3 &v1 = p_face.vertex[(n + 1) % 3];

		e[n] = v1 - v0;
		if (e[n].length_squared() < _threshold_size_squared) {
			return;
		}
	}

	// face is big enough to use as occluder
	Geometry::MeshData::Face face;
	face.plane = Plane(p_face.vertex[0], p_face.vertex[1], p_face.vertex[2]);

	_log("try_bake_face " + _vec3_to_string(p_face.vertex[0]) + _vec3_to_string(p_face.vertex[1]) + _vec3_to_string(p_face.vertex[2]));

	// if successfully merged
	if (_try_merge_face(p_face, -1)) {
		return;
	}

	int i = _mesh_data.vertices.size();
	face.indices.push_back(i);
	face.indices.push_back(i + 1);
	face.indices.push_back(i + 2);

	_mesh_data.faces.push_back(face);

	// keep track of the areas to detect non convex joins
	_faces_areas.push_back(p_face.get_area());

	for (int n = 0; n < 3; n++) {
		_mesh_data.vertices.push_back(p_face.vertex[n]);
	}

	_log("\tbaked initial face");
}

// return false if not convex
bool OccluderShapePoly::_sort_face_winding(Geometry::MeshData::Face &r_face, real_t p_new_triangle_area, real_t p_old_face_area, real_t &r_new_total_area) {
	int num_inds = r_face.indices.size();

	Vector<Vector3> pts;
	for (int n = 0; n < num_inds; n++) {
		pts.push_back(_mesh_data.vertices[r_face.indices[n]]);
	}
	Vector<Vector3> pts_changed = pts;

	Geometry::sort_polygon_winding(pts_changed, r_face.plane.normal);
	CRASH_COND(pts_changed.size() > num_inds);

	// make the new face convex, remove any redundant verts, find the area and compare to the old
	// poly area plus the new face, to detect invalid joins
	if (!Geometry::make_polygon_convex(pts_changed, r_face.plane.normal)) {
		return false;
	}
	CRASH_COND(pts_changed.size() > num_inds);

	r_new_total_area = Geometry::find_polygon_area(pts_changed.ptr(), pts_changed.size());

	// conditions to disallow invalid joins
	real_t diff = r_new_total_area - (p_old_face_area + p_new_triangle_area);
	if (Math::abs(diff) > 0.1) {
		return false;
	}

	Vector<int> new_inds;

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

void OccluderShapePoly::_bake_recursive(Spatial *p_node) {
	VisualInstance *vi = Object::cast_to<VisualInstance>(p_node);

	// check the flags, visible, static etc
	if (vi && vi->is_visible_in_tree() && (vi->get_portal_mode() == CullInstance::PORTAL_MODE_STATIC)) {
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
		}
	}

	for (int n = 0; n < p_node->get_child_count(); n++) {
		Spatial *child = Object::cast_to<Spatial>(p_node->get_child(n));
		if (child) {
			_bake_recursive(child);
		}
	}
}

OccluderShapePoly::OccluderShapePoly() :
		OccluderShape(VisualServer::get_singleton()->occluder_create()) {
}
