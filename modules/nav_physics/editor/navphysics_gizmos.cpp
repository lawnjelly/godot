#include "navphysics_gizmos.h"
#include "modules/nav_physics/godot/np_mesh.h"
#include "modules/nav_physics/godot/np_mesh_instance.h"

NavPhysicsMeshSpatialGizmoPlugin::NavPhysicsMeshSpatialGizmoPlugin() {
	create_material("nav_physics_edge_material", EDITOR_DEF("editors/3d_gizmos/gizmo_colors/nav_physics_edge", Color(0.5, 1, 0)));
	create_material("nav_physics_external_connection_material", EDITOR_DEF("editors/3d_gizmos/gizmo_colors/nav_physics_external_connection", Color(1, 0, 0)));
	create_material("nav_physics_internal_connection_material", EDITOR_DEF("editors/3d_gizmos/gizmo_colors/nav_physics_internal_connection", Color(0, 0, 1)));

	//create_material("navigation_edge_material_disabled", EDITOR_DEF("editors/3d_gizmos/gizmo_colors/navigation_edge_disabled", Color(0.7, 0.7, 0.7)));
	create_material("nav_physics_solid_material", EDITOR_DEF("editors/3d_gizmos/gizmo_colors/nav_physics_solid", Color(0.5, 1, 1, 0.4)));
	create_material("nav_physics_ceiling_material", EDITOR_DEF("editors/3d_gizmos/gizmo_colors/nav_physics_ceiling", Color(0.5, 0.5, 1, 0.4)));
	//create_material("navigation_solid_material_disabled", EDITOR_DEF("editors/3d_gizmos/gizmo_colors/navigation_solid_disabled", Color(0.7, 0.7, 0.7, 0.4)));
}

bool NavPhysicsMeshSpatialGizmoPlugin::has_gizmo(Spatial *p_spatial) {
	return Object::cast_to<NPMeshInstance>(p_spatial) != nullptr;
}

String NavPhysicsMeshSpatialGizmoPlugin::get_name() const {
	return "NPMeshInstance";
}

int NavPhysicsMeshSpatialGizmoPlugin::get_priority() const {
	return -1;
}

void NavPhysicsMeshSpatialGizmoPlugin::redraw(EditorSpatialGizmo *p_gizmo) {
	// MutexLock guard(_mutex);

	p_gizmo->clear();
	NPMeshInstance *region = Object::cast_to<NPMeshInstance>(p_gizmo->get_spatial_node());

	Ref<NPMesh> mesh = region->get_mesh();
	if (mesh.is_null()) {
		return;
	}

	Vector<Vector3> verts = mesh->get_vertices();
	if (!verts.size()) {
		return;
	}
	Vector<int> inds = mesh->get_indices();
	if (!inds.size()) {
		return;
	}
	Vector<NPMesh::Poly> polys = mesh->get_polys();
	if (!polys.size()) {
		return;
	}

	Vector<Vector3> ceil_verts = mesh->get_vertices(true);
	Vector<int> ceil_inds = mesh->get_indices(true);
	Vector<NPMesh::Poly> ceil_polys = mesh->get_polys(true);

	Vector<int> external_wall_connection_inds = mesh->get_external_wall_connection_indices();
	Vector<int> internal_wall_connection_inds = mesh->get_internal_wall_connection_indices();

	PoolVector<Vector3> tmeshfaces;
	//tmeshfaces.resize(inds.size());

	// Display offset.
	Vector3 off(0, 0.25, 0);
	//Vector3 off(0, 0.25, 0);
	Vector3 ceil_off(0, 0, 0);

	{
		//		PoolVector<Vector3>::Write tw = tmeshfaces.write();
		//		for (int n = 0; n < inds.size(); n++) {
		//			tw[n] = verts[inds[n]];
		//		}

		for (int n = 0; n < polys.size(); n++) {
			const NPMesh::Poly &p = polys[n];

			int i0 = p.first_index;
			if (i0 >= inds.size())
				continue;
			int ind0 = inds[i0];
			if (ind0 >= verts.size()) {
				continue;
			}

			for (int e = 2; e < p.num_indices; e++) {
				int i1 = e - 1 + p.first_index;
				int i2 = e + p.first_index;

				if (i1 >= inds.size())
					continue;
				if (i2 >= inds.size())
					continue;

				int ind1 = inds[i1];
				int ind2 = inds[i2];

				if ((ind1 >= verts.size()) || (ind2 >= verts.size())) {
					continue;
				}

				tmeshfaces.push_back(verts[ind0] + off);
				tmeshfaces.push_back(verts[ind1] + off);
				tmeshfaces.push_back(verts[ind2] + off);
			}
		}
	}

	/////
	PoolVector<Vector3> ceil_tmeshfaces;

	{
		for (int n = 0; n < ceil_polys.size(); n++) {
			const NPMesh::Poly &p = ceil_polys[n];
			int i0 = p.first_index;
			if (i0 >= ceil_inds.size())
				continue;
			int ind0 = ceil_inds[i0];
			if (ind0 >= ceil_verts.size()) {
				continue;
			}

			for (int e = 2; e < p.num_indices; e++) {
				int i1 = e - 1 + p.first_index;
				int i2 = e + p.first_index;

				if (i1 >= ceil_inds.size())
					continue;
				if (i2 >= ceil_inds.size())
					continue;

				int ind1 = ceil_inds[i1];
				int ind2 = ceil_inds[i2];

				if ((ind1 >= ceil_verts.size()) || (ind2 >= ceil_verts.size())) {
					continue;
				}

				// Reverse so facing groundward...
				ceil_tmeshfaces.push_back(ceil_verts[ind0] - ceil_off);
				ceil_tmeshfaces.push_back(ceil_verts[ind2] - ceil_off);
				ceil_tmeshfaces.push_back(ceil_verts[ind1] - ceil_off);
			}
		}
	}

	////

	Vector<Vector3> lines;
	for (int n = 0; n < polys.size(); n++) {
		const NPMesh::Poly &p = polys[n];
		for (int e = 0; e < p.num_indices; e++) {
			int e2 = (e + 1) % p.num_indices;

			int i0 = e + p.first_index;
			int i1 = e2 + p.first_index;

			if (i0 >= inds.size())
				continue;
			if (i1 >= inds.size())
				continue;

			int ind0 = inds[i0];
			int ind1 = inds[i1];

			if ((ind0 >= verts.size()) || (ind1 >= verts.size())) {
				continue;
			}

			// If the line is on the connections, don't render.
			// This is slow, could be done better.
			bool add = true;

			for (u32 w = 0; w < external_wall_connection_inds.size(); w += 2) {
				u32 w0 = external_wall_connection_inds[w];
				u32 w1 = external_wall_connection_inds[w + 1];

				if ((ind0 == w0) && (ind1 == w1)) {
					add = false;
					break;
				}
				if ((ind0 == w1) && (ind1 == w0)) {
					add = false;
					break;
				}
			}

			if (add) {
				for (u32 w = 0; w < internal_wall_connection_inds.size(); w += 2) {
					u32 w0 = internal_wall_connection_inds[w];
					u32 w1 = internal_wall_connection_inds[w + 1];

					if ((ind0 == w0) && (ind1 == w1)) {
						add = false;
						break;
					}
					if ((ind0 == w1) && (ind1 == w0)) {
						add = false;
						break;
					}
				}
			}

			if (add) {
				lines.push_back(verts[ind0] + off);
				lines.push_back(verts[ind1] + off);
			}
		}
	}

	Vector<Vector3> external_wall_connection_lines;

	// Internal lines have priority.
	for (int n = 0; n < external_wall_connection_inds.size() / 2; n++) {
		int ind_a = external_wall_connection_inds[n * 2];
		int ind_b = external_wall_connection_inds[(n * 2) + 1];

		if (ind_a > verts.size()) {
			continue;
		}
		if (ind_b > verts.size()) {
			continue;
		}

		// Is this line on the internal list too?
		bool ignore = false;

		for (int i = 0; i < internal_wall_connection_inds.size() / 2; i++) {
			int ind_c = internal_wall_connection_inds[i * 2];
			int ind_d = internal_wall_connection_inds[(i * 2) + 1];

			if ((ind_a == ind_c) && (ind_b == ind_d)) {
				ignore = true;
				break;
			}
		}

		if (!ignore) {
			external_wall_connection_lines.push_back(verts[ind_a] + off);
			external_wall_connection_lines.push_back(verts[ind_b] + off);
		}
	}
	if ((external_wall_connection_lines.size() % 2) != 0) {
		WARN_PRINT_ONCE("external_wall_connection_lines not a multiple of two.");
		external_wall_connection_lines.clear();
	}

	Vector<Vector3> internal_wall_connection_lines;
	for (int n = 0; n < internal_wall_connection_inds.size(); n++) {
		int ind = internal_wall_connection_inds[n];

		if (ind > verts.size()) {
			continue;
		}
		internal_wall_connection_lines.push_back(verts[ind] + off);
	}
	if ((internal_wall_connection_lines.size() % 2) != 0) {
		WARN_PRINT_ONCE("internal_wall_connection_lines not a multiple of two.");
		internal_wall_connection_lines.clear();
	}

	Ref<Material> edge_material = get_material("nav_physics_edge_material", p_gizmo);
	Ref<Material> external_connection_material = get_material("nav_physics_external_connection_material", p_gizmo);
	Ref<Material> internal_connection_material = get_material("nav_physics_internal_connection_material", p_gizmo);
#if 1
	Ref<TriangleMesh> tmesh = memnew(TriangleMesh);

	Ref<Material> solid_material = get_material("nav_physics_solid_material", p_gizmo);
	Ref<Material> ceiling_material = get_material("nav_physics_ceiling_material", p_gizmo);
	//Ref<Material> solid_material_disabled = get_material("navigation_solid_material_disabled", p_gizmo);

	if (tmeshfaces.size()) {
		tmesh->create(tmeshfaces);
		p_gizmo->add_collision_triangles(tmesh);
	}

	Ref<ArrayMesh> m = memnew(ArrayMesh);
	Array a;
	a.resize(Mesh::ARRAY_MAX);
	a[0] = tmeshfaces;
	m->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, a);
	//m->surface_set_material(0, navmesh->is_enabled() ? solid_material : solid_material_disabled);
	m->surface_set_material(0, solid_material);
	p_gizmo->add_mesh(m);

	if (ceil_tmeshfaces.size()) {
		Ref<ArrayMesh> ceil_m = memnew(ArrayMesh);
		Array ceil_a;
		ceil_a.resize(Mesh::ARRAY_MAX);
		ceil_a[0] = ceil_tmeshfaces;
		ceil_m->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, ceil_a);
		//m->surface_set_material(0, navmesh->is_enabled() ? solid_material : solid_material_disabled);
		ceil_m->surface_set_material(0, ceiling_material);
		p_gizmo->add_mesh(ceil_m);
	}

#endif

	if (lines.size()) {
		//p_gizmo->add_lines(lines, navmesh->is_enabled() ? edge_material : edge_material_disabled);
		p_gizmo->add_lines(lines, edge_material);
	}
	if (external_wall_connection_lines.size()) {
		p_gizmo->add_lines(external_wall_connection_lines, external_connection_material);
	}
	if (internal_wall_connection_lines.size()) {
		p_gizmo->add_lines(internal_wall_connection_lines, internal_connection_material);
	}

#if 0
	
	NavigationMeshInstance *navmesh = Object::cast_to<NavigationMeshInstance>(p_gizmo->get_spatial_node());
	
	Ref<Material> edge_material = get_material("navigation_edge_material", p_gizmo);
	Ref<Material> edge_material_disabled = get_material("navigation_edge_material_disabled", p_gizmo);
	
	p_gizmo->clear();
	Ref<NavigationMesh> navmeshie = navmesh->get_navigation_mesh();
	if (navmeshie.is_null()) {
		return;
	}
	
	PoolVector<Vector3> vertices = navmeshie->get_vertices();
	PoolVector<Vector3>::Read vr = vertices.read();
	List<Face3> faces;
	for (int i = 0; i < navmeshie->get_polygon_count(); i++) {
		Vector<int> p = navmeshie->get_polygon(i);
		
		for (int j = 2; j < p.size(); j++) {
			Face3 f;
			f.vertex[0] = vr[p[0]];
			f.vertex[1] = vr[p[j - 1]];
			f.vertex[2] = vr[p[j]];
			
			faces.push_back(f);
		}
	}
	
	if (faces.empty()) {
		return;
	}
	
	Map<_EdgeKey, bool> edge_map;
	PoolVector<Vector3> tmeshfaces;
	tmeshfaces.resize(faces.size() * 3);
	
	{
		PoolVector<Vector3>::Write tw = tmeshfaces.write();
		int tidx = 0;
		
		for (List<Face3>::Element *E = faces.front(); E; E = E->next()) {
			const Face3 &f = E->get();
			
			for (int j = 0; j < 3; j++) {
				tw[tidx++] = f.vertex[j];
				_EdgeKey ek;
				ek.from = f.vertex[j].snapped(Vector3(CMP_EPSILON, CMP_EPSILON, CMP_EPSILON));
				ek.to = f.vertex[(j + 1) % 3].snapped(Vector3(CMP_EPSILON, CMP_EPSILON, CMP_EPSILON));
				if (ek.from < ek.to) {
					SWAP(ek.from, ek.to);
				}
				
				Map<_EdgeKey, bool>::Element *F = edge_map.find(ek);
				
				if (F) {
					F->get() = false;
					
				} else {
					edge_map[ek] = true;
				}
			}
		}
	}
	Vector<Vector3> lines;
	
	for (Map<_EdgeKey, bool>::Element *E = edge_map.front(); E; E = E->next()) {
		if (E->get()) {
			lines.push_back(E->key().from);
			lines.push_back(E->key().to);
		}
	}
	
	Ref<TriangleMesh> tmesh = memnew(TriangleMesh);
	tmesh->create(tmeshfaces);
	
	if (lines.size()) {
		p_gizmo->add_lines(lines, navmesh->is_enabled() ? edge_material : edge_material_disabled);
	}
	p_gizmo->add_collision_triangles(tmesh);
	Ref<ArrayMesh> m = memnew(ArrayMesh);
	Array a;
	a.resize(Mesh::ARRAY_MAX);
	a[0] = tmeshfaces;
	m->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, a);
	m->surface_set_material(0, navmesh->is_enabled() ? solid_material : solid_material_disabled);
	p_gizmo->add_mesh(m);
	p_gizmo->add_collision_segments(lines);
#endif
}
