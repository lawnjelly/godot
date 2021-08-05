#include "occluder_shape_poly.h"

#include "scene/3d/spatial.h"
#include "scene/3d/visual_instance.h"
#include "servers/visual_server.h"

void OccluderShapePoly::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_bake_path", "p_path"), &OccluderShapePoly::set_bake_path);
	ClassDB::bind_method(D_METHOD("get_bake_path"), &OccluderShapePoly::get_bake_path);

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "bake_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Spatial"), "set_bake_path", "get_bake_path");

	//	ClassDB::bind_method(D_METHOD("set_spheres", "spheres"), &OccluderShapeSphere::set_spheres);
	//	ClassDB::bind_method(D_METHOD("get_spheres"), &OccluderShapeSphere::get_spheres);

	//	ClassDB::bind_method(D_METHOD("set_sphere_position", "index", "position"), &OccluderShapeSphere::set_sphere_position);
	//	ClassDB::bind_method(D_METHOD("set_sphere_radius", "index", "radius"), &OccluderShapeSphere::set_sphere_radius);

	//	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "spheres", PROPERTY_HINT_NONE, itos(Variant::PLANE) + ":"), "set_spheres", "get_spheres");
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

//void OccluderShapePoly::set_spheres(const Vector<Plane> &p_spheres) {
//	_spheres = p_spheres;

//	// sanitize radii
//	for (int n = 0; n < _spheres.size(); n++) {
//		if (_spheres[n].d < 0.0) {
//			Plane p = _spheres[n];
//			p.d = 0.0;
//			_spheres.set(n, p);
//		}
//	}

//	notify_change_to_owners();
//}

//void OccluderShapePoly::set_sphere_position(int p_idx, const Vector3 &p_position) {
//	if ((p_idx >= 0) && (p_idx < _spheres.size())) {
//		Plane p = _spheres[p_idx];
//		p.normal = p_position;
//		_spheres.set(p_idx, p);
//		notify_change_to_owners();
//	}
//}

//void OccluderShapePoly::set_sphere_radius(int p_idx, real_t p_radius) {
//	if ((p_idx >= 0) && (p_idx < _spheres.size())) {
//		Plane p = _spheres[p_idx];
//		p.d = MAX(p_radius, 0.0);
//		_spheres.set(p_idx, p);
//		notify_change_to_owners();
//	}
//}

void OccluderShapePoly::clear() {
	_mesh_data.vertices.clear();
	_mesh_data.faces.clear();
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
		real_t s = 10.0;
		Face3 f;
		f.vertex[0] = Vector3(0, 0, 0);
		f.vertex[1] = Vector3(s, s, 0);
		f.vertex[2] = Vector3(s, 0, 0);
		//_try_bake_face(f);
	}

	update_shape_to_visual_server();
}

void OccluderShapePoly::_try_bake_face(const Face3 &p_face) {
	real_t area = p_face.get_twice_area_squared();
	//	if ((area < 1000) || (area > 3000)) {
	if (area < 1000) {
		return;
	}

	const real_t threshold = 1.0 * 1.0;

	Vector3 e[3];
	for (int n = 0; n < 3; n++) {
		const Vector3 &v0 = p_face.vertex[n];
		const Vector3 &v1 = p_face.vertex[(n + 1) % 3];

		e[n] = v1 - v0;
		if (e[n].length_squared() < threshold) {
			return;
		}
	}

	// face is big enough to use as occluder
	Geometry::MeshData::Face face;
	face.plane = Plane(p_face.vertex[0], p_face.vertex[1], p_face.vertex[2]);

	// first attempt to join to an existing face
	for (int f = 0; f < _mesh_data.faces.size(); f++) {
		const Geometry::MeshData::Face &tface = _mesh_data.faces[f];

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
			return;
		}

		if (matching_verts != 2) {
			continue;
		}

		print_line("match found");

		int face_new_index = 3 - face_matched_index[0] + face_matched_index[1];

		// add the new vertex
		int new_vertex_index = _mesh_data.vertices.size();
		_mesh_data.vertices.push_back(p_face.vertex[face_new_index]);

		// add the index to the test face
		Geometry::MeshData::Face new_face = tface;
		new_face.indices.push_back(new_vertex_index);

		_sort_face_winding(new_face);

		// set the new face into the mesh data
		_mesh_data.faces.set(f, new_face);
		return;
	}

	int i = _mesh_data.vertices.size();
	face.indices.push_back(i);
	face.indices.push_back(i + 1);
	face.indices.push_back(i + 2);

	_mesh_data.faces.push_back(face);

	for (int n = 0; n < 3; n++) {
		_mesh_data.vertices.push_back(p_face.vertex[n]);
	}

	print_line("baking face size " + String(Variant(area)));
}

void OccluderShapePoly::_sort_face_winding(Geometry::MeshData::Face &r_face) {
}

void OccluderShapePoly::_bake_recursive(Spatial *p_node) {
	VisualInstance *vi = Object::cast_to<VisualInstance>(p_node);

	// check the flags, visible, static etc
	if (vi && vi->is_visible_in_tree() && (vi->get_portal_mode() == CullInstance::PORTAL_MODE_STATIC)) {
		Transform tr = vi->get_global_transform();

		PoolVector<Face3> faces = vi->get_faces(VisualInstance::FACES_SOLID);

		if (faces.size()) {
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
