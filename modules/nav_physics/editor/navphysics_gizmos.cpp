#include "navphysics_gizmos.h"
#include "modules/nav_physics/godot/np_mesh.h"
#include "modules/nav_physics/godot/np_mesh_instance.h"

NavPhysicsMeshSpatialGizmoPlugin::NavPhysicsMeshSpatialGizmoPlugin() {
	create_material("nav_physics_edge_material", EDITOR_DEF("editors/3d_gizmos/gizmo_colors/nav_physics_edge", Color(0.5, 1, 0)));
	create_material("nav_physics_connection_material", EDITOR_DEF("editors/3d_gizmos/gizmo_colors/nav_physics_connection", Color(1, 0, 0)));
	//create_material("navigation_edge_material_disabled", EDITOR_DEF("editors/3d_gizmos/gizmo_colors/navigation_edge_disabled", Color(0.7, 0.7, 0.7)));
	create_material("nav_physics_solid_material", EDITOR_DEF("editors/3d_gizmos/gizmo_colors/nav_physics_solid", Color(0.5, 1, 1, 0.4)));
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

	Vector<int> wall_connection_inds = mesh->get_wall_connection_indices();

	PoolVector<Vector3> tmeshfaces;
	//tmeshfaces.resize(inds.size());

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

				tmeshfaces.push_back(verts[ind0]);
				tmeshfaces.push_back(verts[ind1]);
				tmeshfaces.push_back(verts[ind2]);
			}
		}
	}

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

			for (u32 w = 0; w < wall_connection_inds.size(); w += 2) {
				u32 w0 = wall_connection_inds[w];
				u32 w1 = wall_connection_inds[w + 1];

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
				lines.push_back(verts[ind0]);
				lines.push_back(verts[ind1]);
			}
		}
	}

	Vector<Vector3> wall_connection_lines;
	for (int n = 0; n < wall_connection_inds.size(); n++) {
		int ind = wall_connection_inds[n];

		if (ind > verts.size()) {
			continue;
		}
		wall_connection_lines.push_back(verts[ind]);
	}
	if ((wall_connection_lines.size() % 2) != 0) {
		WARN_PRINT_ONCE("wall_connection_lines not a multiple of two.");
		wall_connection_lines.clear();
	}

	Ref<Material> edge_material = get_material("nav_physics_edge_material", p_gizmo);
	Ref<Material> connection_material = get_material("nav_physics_connection_material", p_gizmo);
#if 1
	Ref<TriangleMesh> tmesh = memnew(TriangleMesh);
	tmesh->create(tmeshfaces);

	Ref<Material> solid_material = get_material("nav_physics_solid_material", p_gizmo);
	//Ref<Material> solid_material_disabled = get_material("navigation_solid_material_disabled", p_gizmo);

	p_gizmo->add_collision_triangles(tmesh);
	Ref<ArrayMesh> m = memnew(ArrayMesh);
	Array a;
	a.resize(Mesh::ARRAY_MAX);
	a[0] = tmeshfaces;
	m->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, a);
	//m->surface_set_material(0, navmesh->is_enabled() ? solid_material : solid_material_disabled);
	m->surface_set_material(0, solid_material);
	p_gizmo->add_mesh(m);
#endif

	if (lines.size()) {
		//p_gizmo->add_lines(lines, navmesh->is_enabled() ? edge_material : edge_material_disabled);
		p_gizmo->add_lines(lines, edge_material);
	}
	if (wall_connection_lines.size()) {
		p_gizmo->add_lines(wall_connection_lines, connection_material);
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
