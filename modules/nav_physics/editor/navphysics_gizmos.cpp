#include "navphysics_gizmos.h"
#include "modules/nav_physics/godot/np_mesh.h"
#include "modules/nav_physics/godot/np_region.h"

NavPhysicsMeshSpatialGizmoPlugin::NavPhysicsMeshSpatialGizmoPlugin() {
	create_material("navigation_edge_material", EDITOR_DEF("editors/3d_gizmos/gizmo_colors/navigation_edge", Color(0.5, 1, 1)));
	create_material("navigation_edge_material_disabled", EDITOR_DEF("editors/3d_gizmos/gizmo_colors/navigation_edge_disabled", Color(0.7, 0.7, 0.7)));
	create_material("navigation_solid_material", EDITOR_DEF("editors/3d_gizmos/gizmo_colors/navigation_solid", Color(0.5, 1, 1, 0.4)));
	create_material("navigation_solid_material_disabled", EDITOR_DEF("editors/3d_gizmos/gizmo_colors/navigation_solid_disabled", Color(0.7, 0.7, 0.7, 0.4)));
}

bool NavPhysicsMeshSpatialGizmoPlugin::has_gizmo(Spatial *p_spatial) {
	return Object::cast_to<NPRegion>(p_spatial) != nullptr;
}

String NavPhysicsMeshSpatialGizmoPlugin::get_name() const {
	return "NPRegion";
}

int NavPhysicsMeshSpatialGizmoPlugin::get_priority() const {
	return -1;
}

void NavPhysicsMeshSpatialGizmoPlugin::redraw(EditorSpatialGizmo *p_gizmo) {
	p_gizmo->clear();
	NPRegion *region = Object::cast_to<NPRegion>(p_gizmo->get_spatial_node());

	Ref<NPMesh> mesh = region->get_mesh();
	if (mesh.is_null()) {
		return;
	}

	Vector<Vector3> verts = mesh->get_vertices();
	Vector<int> inds = mesh->get_indices();
	Vector<NPMesh::Poly> polys = mesh->get_polys();

	PoolVector<Vector3> tmeshfaces;
	tmeshfaces.resize(inds.size());

	{
		PoolVector<Vector3>::Write tw = tmeshfaces.write();
		for (int n = 0; n < inds.size(); n++) {
			tw[n] = verts[inds[n]];
		}
	}

	Vector<Vector3> lines;
	for (int n = 0; n < polys.size(); n++) {
		const NPMesh::Poly &p = polys[n];
		for (int e = 0; e < p.num_indices; e++) {
			int e2 = (e + 1) % p.num_indices;

			int i0 = e + p.first_index;
			int i1 = e2 + p.first_index;

			lines.push_back(verts[inds[i0]]);
			lines.push_back(verts[inds[i1]]);
		}
	}

	Ref<TriangleMesh> tmesh = memnew(TriangleMesh);
	tmesh->create(tmeshfaces);

	Ref<Material> edge_material = get_material("navigation_edge_material", p_gizmo);
	Ref<Material> solid_material = get_material("navigation_solid_material", p_gizmo);
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

	if (lines.size()) {
		//p_gizmo->add_lines(lines, navmesh->is_enabled() ? edge_material : edge_material_disabled);
		p_gizmo->add_lines(lines, edge_material);
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
