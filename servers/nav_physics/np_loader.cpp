#include "np_loader.h"
//#include "modules/navigation/nav_map.h"
//#include "modules/navigation/nav_mesh.h"
#include "np_map.h"
#include "np_mesh.h"
#include "scene/resources/navigation_mesh.h"

namespace NavPhysics {

//bool Loader::load(const NavMap &p_navmap, NavPhysics::Map &r_map) {
//	// make sure any current data is cleared out
//	r_map.clear();

//	for (uint32_t n = 0; n < p_navmap.meshs.size(); n++) {
//		if (p_navmap.meshs[n]) {
//			load_mesh(p_navmap, *p_navmap.meshs[n], r_map);
//		}
//	}

//	return true;
//}

/*
bool Loader::unload_mesh(NavPhysics::Map &r_map, uint32_t p_navphysics_mesh_id) {
	if (p_navphysics_mesh_id == UINT32_MAX) {
		return false;
	}

	Mesh *mesh = r_map._meshes[p_navphysics_mesh_id];
	memdelete(mesh);
	r_map._meshes[p_navphysics_mesh_id] = nullptr;
	r_map._meshes.free(p_navphysics_mesh_id);
	return true;

//		for (uint32_t n = 0; n < r_map._meshes.active_size(); n++) {
//			Mesh *mesh = r_map._meshes.get_active(n);
//			if (mesh && mesh->_source_nav_mesh == &p_mesh) {
//				// todo .. use the id directly instead of search through, once we are sure this routine works
//				DEV_ASSERT(r_map._meshes.get_active_id(n) == p_navphysics_mesh_id);
//				r_map.unload_mesh_callback(mesh->_mesh_id);
//				memdelete(mesh);

//				uint32_t id = r_map._meshes.get_active_id(n);
//				r_map._meshes.get_active(n) = nullptr;
//				r_map._meshes.free(id);
//				return true;
//			}
//		}

//		return false;
}
*/

bool Loader::load_mesh(Ref<NavigationMesh> p_navmesh, NavPhysics::Mesh &r_mesh) {
	// is the mesh already loaded?
	//	if (p_mesh.get_navphysics_id() != UINT32_MAX) {
	//		return p_mesh.get_navphysics_id();
	//	}

	r_mesh.clear();

	// ignore if no polys yet
	if (!p_navmesh->get_polygon_count()) {
		return false;
	}

	//Mesh *r = r_map._meshes[p_navphysics_mesh_id];
	//ERR_FAIL_NULL_V(r, false);

	// load polygons and verts
	//Mesh *r = memnew(Mesh);
	//r->_source_nav_mesh = &p_mesh;

	//uint32_t id = 0;
	//Mesh **pp;

	if (!load_polys(p_navmesh, r_mesh)) {
		goto failed;
	}

	find_index_nexts(r_mesh);
	load_fixed_point_verts(r_mesh);
	find_links(r_mesh);
	find_walls(r_mesh);

	print_line("NavPhysics loaded mesh:");
	print_line("\tpolys: " + itos(r_mesh.get_num_polys()));
	print_line("\tverts: " + itos(r_mesh.get_num_verts()));
	print_line("\tinds: " + itos(r_mesh.get_num_inds()));

	//pp = r_map._meshes.request(id);
	//*pp = r;

	//r->_mesh_id = id;

	//r_map._meshes.push_back(r);
	//return id;
	return true;

failed:
	r_mesh.clear();
	return false;
	//memdelete(r);
	//return UINT32_MAX;
}

// returns linked wall or -1
uint32_t Loader::find_linked_poly(NavPhysics::Mesh &r_dest, uint32_t p_poly_from, uint32_t p_ind_a, uint32_t p_ind_b, uint32_t &r_linked_poly) const {
	for (uint32_t pb = p_poly_from + 1; pb < r_dest.get_num_polys(); pb++) {
		const Poly &poly_b = r_dest.get_poly(pb);

		if (poly_b.num_inds < 3) {
			continue;
		}

		for (uint32_t wb = 0; wb < poly_b.num_inds; wb++) {
			uint32_t wall_id_c = poly_b.first_ind + wb;
			uint32_t wall_id_d = poly_b.first_ind + ((wb + 1) % poly_b.num_inds);

			uint32_t ind_c = r_dest.get_ind(wall_id_c);
			uint32_t ind_d = r_dest.get_ind(wall_id_d);

			// find links with a and b
			//var link_poly_id : int = -1

			bool is_link = false;

			if ((p_ind_a == ind_c) && (p_ind_b == ind_d)) {
				is_link = true;
			}
			if ((p_ind_a == ind_d) && (p_ind_b == ind_c)) {
				is_link = true;
			}

			if (is_link) {
				//_dmap._links.set(wall_id, poly_from)
				// we have found a link poly!
				//print("link from " + str(poly_from) + " to " + str(pb))
				//return [wall_id_c, pb]
				r_linked_poly = pb;
				return wall_id_c;
			}

			// swap
			//ind_c = ind_d
		} // for wb
	} // for pb

	// not found, no link poly, edge of navmesh
	return UINT32_MAX;
}

void Loader::find_index_nexts(NavPhysics::Mesh &r_dest) {
	for (uint32_t p = 0; p < r_dest.get_num_polys(); p++) {
		const Poly &poly = r_dest.get_poly(p);

		uint32_t num_poly_inds = poly.num_inds;

		for (uint32_t i = 0; i < num_poly_inds; i++) {
			uint32_t next = (i + 1) % num_poly_inds;
			next += poly.first_ind;

			next = r_dest.get_ind(next);
			r_dest._inds_next.push_back(next);
		} // for i
	} // for p
}

void Loader::find_walls(NavPhysics::Mesh &r_dest) {
	uint32_t num_walls = r_dest.get_num_links();

	r_dest._walls.resize(num_walls);
	for (uint32_t w = 0; w < num_walls; w++) {
		if (!r_dest.is_hard_wall(w)) {
			continue;
		}

		uint32_t ind_a = r_dest.get_ind(w);
		uint32_t ind_b = r_dest.get_ind_next(w);

		Wall &wall = r_dest._walls[w];
		wall.vert_a = ind_a;
		wall.vert_b = ind_b;

		vec2 wa = r_dest.get_vert(ind_a);
		vec2 wb = r_dest.get_vert(ind_b);

		wall.wall_vec = wb - wa;
		wall.normal.x = -wall.wall_vec.y;
		wall.normal.y = wall.wall_vec.x;
		wall.normal.normalize();

		// swap but NOT the normal
		if (ind_a > ind_b) {
			SWAP(wall.vert_a, wall.vert_b);
			wall.wall_vec = -wall.wall_vec;
		}
	}

	// Now assign a poly to each wall
	for (uint32_t p = 0; p < r_dest.get_num_polys(); p++) {
		const Poly &poly = r_dest.get_poly(p);
		ERR_CONTINUE(poly.num_inds < 3);

		for (uint32_t i = 0; i < poly.num_inds; i++) {
			uint32_t wall_id = poly.first_ind + i;
			if (!r_dest.is_hard_wall(wall_id)) {
				continue;
			}

			r_dest._walls[wall_id].poly_id = p;
		}
	}

	// Now find previous and next walls
	for (uint32_t wa = 0; wa < num_walls; wa++) {
		if (!r_dest.is_hard_wall(wa)) {
			continue;
		}

		const Wall &wall_a = r_dest.get_wall(wa);
		for (uint32_t wb = wa + 1; wb < num_walls; wb++) {
			if (!r_dest.is_hard_wall(wb)) {
				continue;
			}

			if (wa == wb) {
				continue;
			}

			const Wall &wall_b = r_dest.get_wall(wb);

			if (wall_b.has_vert(wall_a.vert_a) || wall_b.has_vert(wall_a.vert_b)) {
				wall_add_neighbour_wall(r_dest, wa, wb);
			}
		} // for wb
	} // for wa
}

void Loader::wall_add_neighbour_wall(NavPhysics::Mesh &r_dest, uint32_t p_a, uint32_t p_b) {
	Wall &wall_a = r_dest._walls[p_a];
	Wall &wall_b = r_dest._walls[p_b];

	// Special case to take care of.
	// If two sections are joining on a single vertex,
	// we need to either always prevent movement across,
	// or always allow movement across.
	if (wall_a.vert_b == wall_b.vert_a) {
		if ((wall_a.next_wall != UINT32_MAX) || (wall_b.prev_wall != UINT32_MAX)) {
			if (wall_a.poly_id == wall_b.poly_id) {
				return;
			}
		}
		wall_a.next_wall = p_b;
		wall_b.prev_wall = p_a;
		return;
	}

	if (wall_a.vert_b == wall_b.vert_b) {
		if ((wall_a.next_wall != UINT32_MAX) || (wall_b.next_wall != UINT32_MAX)) {
			if (wall_a.poly_id == wall_b.poly_id) {
				return;
			}
		}
		wall_a.next_wall = p_b;
		wall_b.next_wall = p_a;
		return;
	}

	if (wall_a.vert_a == wall_b.vert_a) {
		if ((wall_a.prev_wall != UINT32_MAX) || (wall_b.prev_wall != UINT32_MAX)) {
			if (wall_a.poly_id == wall_b.poly_id) {
				return;
			}
		}
		wall_a.prev_wall = p_b;
		wall_b.prev_wall = p_a;
		return;
	}

	if (wall_a.vert_a == wall_b.vert_b) {
		if ((wall_a.prev_wall != UINT32_MAX) || (wall_b.next_wall != UINT32_MAX)) {
			if (wall_a.poly_id == wall_b.poly_id) {
				return;
			}
		}
		wall_a.prev_wall = p_b;
		wall_b.next_wall = p_a;
		return;
	}

	// this should never happen
	DEV_ASSERT(false);
}

void Loader::find_links(NavPhysics::Mesh &r_dest) {
	LocalVector<uint32_t> &links = r_dest._links;
	links.resize(r_dest.get_num_inds());
	links.fill(UINT32_MAX);

	for (uint32_t pa = 0; pa < r_dest.get_num_polys(); pa++) {
		const Poly &poly_a = r_dest.get_poly(pa);

		if (poly_a.num_inds < 3) {
			// This isn't really a poly, and we can't find links
			continue;
		}

		for (uint32_t wa = 0; wa < poly_a.num_inds; wa++) {
			uint32_t wall_id_a = poly_a.first_ind + wa;
			uint32_t wall_id_b = poly_a.first_ind + ((wa + 1) % poly_a.num_inds);

			uint32_t ind_a = r_dest.get_ind(wall_id_a);
			uint32_t ind_b = r_dest.get_ind(wall_id_b);

			// find links with a and b
			uint32_t linked_poly = 0;
			uint32_t linked_wall = find_linked_poly(r_dest, pa, ind_a, ind_b, linked_poly);
			//var link_poly_id : int = find_linked_poly(pa, ind_a, ind_b)
			if (linked_wall != UINT32_MAX) {
				links[wall_id_a] = linked_poly;
				links[linked_wall] = pa;
				// links.set(wall_id_a, ret[1])
				// links.set(ret[0], pa)
			}

			// swap
			//ind_a = ind_b
		}
	}
}

Rect2 Loader::calculate_mesh_bound(const NavPhysics::Mesh &p_mesh) const {
	// bottom left, top right
	vec2 bl{ 0, 0 };
	vec2 tr{ vec2::FP_RANGE, vec2::FP_RANGE };

	Vector2 bl2 = p_mesh.vec2_to_float(bl);
	Vector2 tr2 = p_mesh.vec2_to_float(tr);

	Rect2 res;
	res.position = bl2;
	res.expand_to(tr2);

	return res;
}

void Loader::load_fixed_point_verts(NavPhysics::Mesh &r_dest) {
	const LocalVector<Vector2> &sverts = r_dest._fverts;

	// first find the offset and scale
	uint32_t num_verts = sverts.size();

	if (!num_verts) {
		return;
	}

	Rect2 rect;
	rect.position = sverts[0];

	for (uint32_t i = 1; i < num_verts; i++) {
		rect.expand_to(sverts[i]);
	}

	print_line("Internal Map AABB is " + String(Variant(rect.position)) + ", " + String(Variant(rect.size)));

	r_dest._float_to_fp_scale = Vector2(vec2::FP_RANGE / rect.size.x, vec2::FP_RANGE / rect.size.y);
	r_dest._float_to_fp_offset = rect.position;

	r_dest._fp_to_float_offset = rect.position;
	r_dest._fp_to_float_scale = Vector2(rect.size.x / vec2::FP_RANGE, rect.size.y / vec2::FP_RANGE);

	for (uint32_t i = 0; i < num_verts; i++) {
		const Vector2 &v = sverts[i];
		vec2 vfp = r_dest.float_to_vec2(v);
		// print("vert " + str(i) + " : " + vfp.sz())
		r_dest._verts.push_back(vfp);

		// var verify = _dmap._fpvec2_to_float(vfp)
		// var l = (verify- v).length()
		// print("length " + str(l))
	}

	// find the poly fixed point centers
	LocalVector<Poly> &dpolys = r_dest._polys;

	for (uint32_t n = 0; n < dpolys.size(); n++) {
		Poly &dpoly = dpolys[n];
		dpoly.center = r_dest.float_to_vec2(Vector2(dpoly.center3.x, dpoly.center3.z));
	}
}

// Note: use Geometry version of this when merged.
void Loader::plane_from_poly_newell(NavPhysics::Mesh &r_dest, NavPhysics::Poly &r_poly) const {
	int num_points = r_poly.num_inds;

	if (num_points < 3) {
		r_poly.plane = Plane();
		return;
	}

	Vector3 normal;
	Vector3 center;

	for (int i = 0; i < num_points; i++) {
		int j = (i + 1) % num_points;

		uint32_t ind_i = r_dest._inds[r_poly.first_ind + i];
		uint32_t ind_j = r_dest._inds[r_poly.first_ind + j];

		const Vector3 &pi = r_dest._fverts3[ind_i];
		const Vector3 &pj = r_dest._fverts3[ind_j];

		center += pi;

		normal.x += (((pi.z) + (pj.z)) * ((pj.y) - (pi.y)));
		normal.y += (((pi.x) + (pj.x)) * ((pj.z) - (pi.z)));
		normal.z += (((pi.y) + (pj.y)) * ((pj.x) - (pi.x)));
	}

	normal.normalize();
	center /= num_points;

	// point and normal
	r_poly.plane = Plane(center, normal);
}

bool Loader::load_polys(Ref<NavigationMesh> p_navmesh, NavPhysics::Mesh &r_dest) {
	//Ref<NavigationMesh> mesh = p_source.mesh;
	if (p_navmesh.is_null()) {
		return false;
	}

	PoolVector<Vector3> vertices = p_navmesh->get_vertices();
	int len = vertices.size();
	if (len == 0) {
		return false;
	}

	r_dest._polys.resize(p_navmesh->get_polygon_count());

	for (uint32_t n = 0; n < r_dest._polys.size(); n++) {
		Vector<int> source_poly = p_navmesh->get_polygon(n);
		Poly &dpoly = r_dest._polys[n];

		dpoly.first_ind = r_dest._inds.size();
		dpoly.center3 = Vector3();

		uint32_t num_poly_verts = source_poly.size();
		for (uint32_t v = 0; v < num_poly_verts; v++) {
			const Vector3 &pt = vertices[source_poly[v]];
			r_dest._inds.push_back(find_or_create_vert(r_dest, pt));
			dpoly.center3 += pt;
		}
		dpoly.num_inds = source_poly.size();

		if (num_poly_verts) {
			dpoly.center3 /= num_poly_verts;
		}

		// clockwise flag not yet dealt with...
		plane_from_poly_newell(r_dest, dpoly);
	}

	return true;
}

uint32_t Loader::find_or_create_vert(NavPhysics::Mesh &r_dest, const Vector3 &p_pt) {
	LocalVector<Vector3> &dverts = r_dest._fverts3;

	// slow for now
	for (uint32_t n = 0; n < dverts.size(); n++) {
		if (dverts[n].is_equal_approx(p_pt)) {
			return n;
		}
	}

	// not found, add
	dverts.push_back(p_pt);
	r_dest._fverts.push_back(Vector2(p_pt.x, p_pt.z));

	return dverts.size() - 1;
}

} //namespace NavPhysics
