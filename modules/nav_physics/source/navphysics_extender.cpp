#include "navphysics_extender.h"

namespace NavPhysics {

void MeshExtender::llog(String p_sz) {
	// log(p_sz);
}

void MeshExtender::push_explicit_internal_wall_pairs(Mesh &r_mesh) {
	for (u32 n = 0; n < r_mesh.data.internal_wall_ids.size(); n++) {
		u32 wall_id = r_mesh.data.internal_wall_ids[n];

		// If lipped, we delay the adding of the edge until it has been extended.
		if (r_mesh.data.lipped_wall_ids.contains(wall_id)) {
			continue;
		}

		const Wall &wall = r_mesh.get_wall(wall_id);

		Mesh::WallPair pair;
		pair.vert_ids[0] = wall.vert_a;
		pair.vert_ids[1] = wall.vert_b;
		r_mesh.data.internal_wall_pairs.push_back(pair);
	}
}

void MeshExtender::push_extended_connection_new(Mesh &r_mesh, const EdgeWall &p_edge_wall, u32 p_ind_a, u32 p_ind_b) {
	Poly poly;
	poly.init();
	poly.first_ind = r_mesh.get_num_inds();

	bool folded = false;

	TVector<u32> &inds = r_mesh.floor.inds;

	if (p_ind_a == p_ind_b) {
		folded = true;
		inds.push_back(p_edge_wall.ind_a);
		inds.push_back(p_ind_a);
		inds.push_back(p_edge_wall.ind_b);

		poly.num_inds = 3;
	} else {
		inds.push_back(p_edge_wall.ind_a);
		inds.push_back(p_ind_a);
		inds.push_back(p_ind_b);
		inds.push_back(p_edge_wall.ind_b);

		poly.num_inds = 4;
	}

	fill_poly(r_mesh, r_mesh.floor, poly);

	PolyExtra poly_extra;

	r_mesh.floor.polys.push_back(poly);
	r_mesh._polys_extra.push_back(poly_extra);
	r_mesh.floor.poly_bounds.push_back(IRect2());

	// This must be done AFTER adding the poly and poly bounds to the lists.
	calculate_poly_bound(r_mesh.floor, r_mesh.get_num_polys() - 1);

	// external wall pair for later finding external walls...
	if (!folded) {
		Mesh::WallPair pair;
		pair.vert_ids[0] = p_ind_a;
		pair.vert_ids[1] = p_ind_b;

		if (p_edge_wall.external) {
			r_mesh.data.external_wall_pairs.push_back(pair);
			NP_LOG(String("Pushing external pair ") + pair.vert_ids[0] + ", " + pair.vert_ids[1]);
		} else {
			r_mesh.data.internal_wall_pairs.push_back(pair);
			NP_LOG(String("Pushing internal pair ") + pair.vert_ids[0] + ", " + pair.vert_ids[1]);
		}
	}
}

#if 0
const Wall &MeshExtender::get_wall(const Mesh &p_mesh, const Edge &p_edge, u32 p_which) const {
	u32 conn_wall_id = p_edge.cwalls[p_which];
	return p_mesh.get_wall(conn_wall_id);
}

const IPoint2 &MeshExtender::get_wall_normal(const Mesh &p_mesh, const Edge &p_edge, u32 p_which) const {
	const Wall &conn_wall = get_wall(p_mesh, p_edge, p_which);
	return conn_wall.normal;
}
#endif

bool MeshExtender::calculate_edge_wall(Edge &r_edge, u32 p_wall_id, const Mesh &p_mesh) {
	EdgeWall &wall = r_edge.walls[p_wall_id];

	u32 extension_dist = p_mesh.extension_data.agent_radius + p_mesh.extension_data.agent_lip;
	IPoint2 normal = wall.normal;

	u32 normal_merge_range = r_edge.loop ? r_edge.walls.size() : (r_edge.walls.size() - 1);

	if (p_wall_id < normal_merge_range) {
		EdgeWall &next_wall = r_edge.walls[(p_wall_id + 1) % r_edge.walls.size()];
		normal += next_wall.normal;
	}

	normal.normalize_to_scale(extension_dist);

	IPoint2 test_pt = wall.b + normal;

	if (p_wall_id > 0) {
		EdgeWall &prev_wall = r_edge.walls[p_wall_id - 1];
		const IPoint2 &prev_pt = _edge_pts[prev_wall.ind_c].pos;

		// detect going back on itself.
		// Detect quads that fold in on themselves.
		IPoint2 test_vec[2];
		test_vec[0] = prev_pt - wall.a;
		test_vec[1] = test_pt - prev_pt;

		i64 cross = test_vec[0].cross(test_vec[1]);
		//i64 cross = 1;
		NP_LLOG(String("cross ") + cross);

		// If folds in or same spot, we only want one average vert in the centre.
		//bool folded = false;

		if (cross <= 0) {
			NP_LLOG("\tfolded");
			// duplicate previous
			wall.ind_c = prev_wall.ind_c;
			return true;
		}
	} else {
		// Check the leading point.
		const IPoint2 &prev_pt = _edge_pts[r_edge.leading_pt].pos;

		// detect going back on itself.
		// Detect quads that fold in on themselves.
		IPoint2 test_vec[2];
		test_vec[0] = test_pt - prev_pt;
		test_vec[1] = wall.b - test_pt;

		i64 cross = test_vec[0].cross(test_vec[1]);
		NP_LLOG(String("leading cross ") + cross);

		if (cross <= 0) {
			NP_LLOG("leading folded");
			r_edge.merge_leading_to_c = true;
		}
	}

	// Point is ok, add.
	wall.ind_c = _edge_pts.size();
	_add_edge_point(p_mesh, test_pt);

	return true;
}

//u32 MeshExtender::create_final_mesh_vert(const EdgeWall &p_wall, Mesh &r_mesh, const IPoint2 &p_pos) {
u32 MeshExtender::create_final_mesh_vert(const EdgeWall &p_wall, Mesh &r_mesh, u32 p_edge_point) {
	EdgePoint &ept = _edge_pts[p_edge_point];
	if (ept.final_vert_id != UINT32_MAX) {
		return ept.final_vert_id;
	}
	const IPoint2 &pos = ept.pos;

	float height = r_mesh.find_height_on_poly_plane(p_wall.poly_id, pos);

	// Create the final vertex.
	u32 ind_final = r_mesh.get_num_verts();
	ept.final_vert_id = ind_final;

	NP_LLOG(String("extender creating final mesh vert from edge point ") + p_edge_point + " to final vert " + ind_final + " on poly " + p_wall.poly_id);
	r_mesh.floor.verts.push_back(pos);

	// reconstruct vec3 versions
	FPoint2 c2 = r_mesh.fixed_point_to_float_2(pos);
	FPoint3 c3 = FPoint3::make(c2.x, height, c2.y);

	r_mesh.floor.fverts3.push_back(c3);

	return ind_final;
}

void MeshExtender::finalize_edge_wall(Edge &r_edge, u32 p_wall_id, Mesh &r_mesh) {
	EdgeWall &wall = r_edge.walls[p_wall_id];

	NP_DEV_ASSERT(wall.ind_c != UINT32_MAX);
	//const IPoint2 &c = _edge_pts[wall.ind_c];

	// Create the final vertex.
	wall.ind_c_final = create_final_mesh_vert(wall, r_mesh, wall.ind_c);
	r_mesh._check_for_duplicate_verts();

	// Need a previous index to make a quad.
	u32 ind_prev = wall.ind_c_final;

	if (p_wall_id > 0) {
		EdgeWall &prev_wall = r_edge.walls[p_wall_id - 1];
		ind_prev = prev_wall.ind_c_final;
	} else {
		// Create one from the original leading vertex.
		if (r_edge.merge_leading_to_c) {
			// do nothing, this is the default for ind_prev
		} else {
			// We need to create index previous.
			//const IPoint2 &d = _edge_pts[r_edge.leading_pt];
			ind_prev = create_final_mesh_vert(wall, r_mesh, r_edge.leading_pt);
			r_mesh._check_for_duplicate_verts();
		}
	}

	//const Wall &conn_wall = r_mesh.get_wall(wall.connecting_wall_id);

	push_extended_connection_new(r_mesh, wall, ind_prev, wall.ind_c_final);
}

void MeshExtender::_add_edge_point(const Mesh &p_mesh, const IPoint2 &p_pt) {
#ifdef NP_DEV_ENABLED
	/*
	// Check not already in mesh.
	if (p_mesh._verts.find(p_pt) != -1) {
		log("_add_edge_point duplicate vert in p_mesh");
	}

	// Check not already in edge points.
	if (_edge_pts.find(p_pt) != -1) {
		log("_add_edge_point duplicate vert in _edge_pts");
	}
*/

#endif
	EdgePoint ept;
	ept.pos = p_pt;
	_edge_pts.push_back(ept);
}

void MeshExtender::extend_edge(Mesh &r_mesh, u32 p_edge_id) {
	Edge &edge = _edges[p_edge_id];
	_edge_pts.clear();

	// For now we set leading point to a simple version.
	u32 extension_dist = r_mesh.extension_data.agent_radius + r_mesh.extension_data.agent_lip;
	IPoint2 normal = edge.walls[0].normal;
	normal.normalize_to_scale(extension_dist);
	//edge.leading_pt = edge.walls[0].a + normal;
	_add_edge_point(r_mesh, edge.walls[0].a + normal);
	edge.leading_pt = 0;

	// Repeatedly try and calculate points until we get to the end.
	bool restart = true;

	while (restart) {
		restart = false;

		for (u32 w = 0; w < edge.walls.size(); w++) {
			if (!calculate_edge_wall(edge, w, r_mesh)) {
				restart = true;
				break;
			}
		}
	}

	// Special case for edge loop, replace the leading pt with the last c from the edge.
	// Note, this could cause folds potentially? NYI.
	if (edge.loop) {
		edge.leading_pt = edge.walls.get_last()->ind_c;

		// disallow merging lead point.
		edge.merge_leading_to_c = false;
	}

//#define NAVPHYSICS_EXTENDER_EXPORT_SVG
#ifdef NAVPHYSICS_EXTENDER_EXPORT_SVG
	u32 svg_edge_poly_begin = r_mesh.get_num_polys();
#endif

	// Finalize.
	for (u32 w = 0; w < edge.walls.size(); w++) {
		finalize_edge_wall(edge, w, r_mesh);
	}

#ifdef NAVPHYSICS_EXTENDER_EXPORT_SVG
	r_mesh.svg_export("../test_edges.svg", svg_edge_poly_begin);
#endif

#if 0
	u32 extension_dist = r_mesh.extension_data.agent_radius + r_mesh.extension_data.agent_lip;

	u32 num_walls = edge.cwalls.size();
	u32 num_points = num_walls;

	if (!edge.loop)
		num_points++;

	Vector<IPoint2, u32, false> vecs_out;
	Vector<IPoint2, u32, false> positions;
	Vector<freal> heights;
	vecs_out.resize(num_points);
	positions.resize(num_points);
	heights.resize(num_points);
	heights.fill(0);

	//u32 point_count = 0;

	// First calculate the positions (fixed point)
	for (u32 n = 0; n < num_points; n++) {
		IPoint2 vec_out;

		u32 wall_a = (n + num_walls - 1) % num_walls;
		u32 wall_b = n % num_walls;

		bool swap = false;

		if (!edge.loop) {
			if (n == 0) {
				wall_a = wall_b;
			}
			if (n == (num_points - 1)) {
				wall_b = wall_a;

				// Also swap vertices if the last point on a non-loop.
				swap = !swap;
			}
		}
		vec_out = get_wall_normal(r_mesh, edge, wall_a);
		vec_out += get_wall_normal(r_mesh, edge, wall_b);

		vec_out.normalize_to_scale(extension_dist);
		vecs_out[n] = vec_out;

		//const Wall &wa = get_wall(r_mesh, edge, wall_a);
		const Wall &wb = get_wall(r_mesh, edge, wall_b);

		IPoint2 va = r_mesh.get_vert(wb.vert_a);
		IPoint2 vb = r_mesh.get_vert(wb.vert_b);

		if (wb.verts_swapped) {
			swap = !swap;
		}

		if (swap) {
			log(String("SWAPPING for n ") + n);
			SWAP(va, vb);
		}

		if (n == (num_points - 1)) {
			//va = r_mesh.get_vert(wb.vert_b) - vec_out;
		}

		log(String("\t[ ") + n + " ] pos " + va + ", vec_out " + vec_out);
		va -= vec_out;

		positions[n] = va;
		heights[n] = r_mesh.find_height_on_poly(wb.poly_id, va);
	}


	Vector<u32> inds;
	inds.resize(num_points);
	inds.fill(0);

	for (u32 n = 0; n < num_points; n++) {
		inds[n] = r_mesh._verts.size();

		IPoint2 va = positions[n];
		r_mesh._verts.push_back(va);

		// reconstruct vec3 versions
		FPoint2 fa = r_mesh.fixed_point_to_float_2(va);
		FPoint3 fa3 = FPoint3::make(fa.x, heights[n], fa.y);

		r_mesh._fverts3.push_back(fa3);
	}

	// Keep a running tally of the last index merged
	// on the previous edge (if any, else leave at -1).
	u32 merged_index = UINT32_MAX;

	//for (u32 n = 0; n < 1; n++) {
	for (u32 n = 0; n < (num_points - 1); n++) {
		u32 ind_a = inds[n];
		u32 ind_b = inds[n + 1];
		u32 conn_wall_id = edge.cwalls[n];

		const Wall &wall = r_mesh.get_wall(conn_wall_id);

		if (wall.verts_swapped) {
			SWAP(ind_a, ind_b);
		}

		push_extended_connection_detail(r_mesh, ind_a, ind_b, conn_wall_id, merged_index);
	}

	// Loop? Add an extra wall.
	if (edge.loop) {
		u32 ind_a = inds[num_points - 1];
		u32 ind_b = inds[0];
		u32 conn_wall_id = edge.cwalls[edge.cwalls.size() - 1];

		const Wall &wall = r_mesh.get_wall(conn_wall_id);

		if (wall.verts_swapped) {
			SWAP(ind_a, ind_b);
		}

		push_extended_connection_detail(r_mesh, ind_a, ind_b, conn_wall_id, merged_index);
	}
#endif
}

void MeshExtender::calculate_poly_bound(Mesh::SubMesh &r_submesh, u32 p_poly_id) {
	const Poly &p = r_submesh.polys[p_poly_id];

	NP_DEV_ASSERT(p.num_inds);

	IRect2 &bound = r_submesh.poly_bounds[p_poly_id];
	bound.zero();

	u32 first_ind = r_submesh.inds[p.first_ind];
	bound.position = r_submesh.verts[first_ind];

	for (u32 i = 1; i < p.num_inds; i++) {
		u32 ind = r_submesh.inds[p.first_ind + i];
		bound.expand_to_fast(r_submesh.verts[ind]);
	}
	bound.increment_size();

	NP_DEV_ASSERT(bound.size.x >= 0);
	NP_DEV_ASSERT(bound.size.y >= 0);
}

bool MeshExtender::fill_poly(const Mesh &p_mesh, const Mesh::SubMesh &p_submesh, Poly &r_poly) {
	r_poly.center3.zero();

	for (u32 n = 0; n < r_poly.num_inds; n++) {
		u32 ind = p_submesh.inds[r_poly.first_ind + n];
		NP_DEV_ASSERT(ind < p_submesh.fverts3.size());
		const FPoint3 &pt = p_submesh.fverts3[ind];
		r_poly.center3 += pt;
	}

	if (r_poly.num_inds) {
		r_poly.center3 /= (float)r_poly.num_inds;
		r_poly.center = p_mesh.float_to_fixed_point_2(FPoint2::make(r_poly.center3.x, r_poly.center3.z));
	}

	// Clockwise flag not yet dealt with...
	return plane_from_poly_newell(p_submesh.fverts3, p_submesh.inds, r_poly);
}

//void MeshExtender::fill_ceiling_poly(Mesh &r_mesh, Poly &r_poly) {
//	r_poly.center3.zero();

//	for (u32 n = 0; n < r_poly.num_inds; n++) {
//		u32 ind = r_mesh.ceiling.inds[r_poly.first_ind + n];
//		NP_DEV_ASSERT(ind < r_mesh.ceiling.fverts3.size());
//		const FPoint3 &pt = r_mesh.ceiling.fverts3[ind];
//		r_poly.center3 += pt;
//	}

//	if (r_poly.num_inds) {
//		r_poly.center3 /= (float)r_poly.num_inds;
//		r_poly.center = r_mesh.float_to_fixed_point_2(FPoint2::make(r_poly.center3.x, r_poly.center3.z));
//	}

//	// Clockwise flag not yet dealt with...
//	plane_from_poly_newell(r_mesh.ceiling.fverts3, r_mesh.ceiling.inds, r_poly);
//}

bool MeshExtender::plane_from_poly_newell(const TVector<FPoint3> &p_verts, const TVector<u32> &p_inds, Poly &r_poly) {
	int num_points = r_poly.num_inds;

	if (num_points < 3) {
		NP_WARN_PRINT_ONCE("MeshExtender::plane_from_poly_newell poly has less than 3 points.");
		r_poly.plane.zero();
		return false;
	}

	FPoint3 normal;
	FPoint3 center;

	for (int i = 0; i < num_points; i++) {
		int j = (i + 1) % num_points;

		u32 ind_i = p_inds[r_poly.first_ind + i];
		u32 ind_j = p_inds[r_poly.first_ind + j];

		const FPoint3 &pi = p_verts[ind_i];
		const FPoint3 &pj = p_verts[ind_j];

		center += pi;

		normal.x += (((pi.z) + (pj.z)) * ((pj.y) - (pi.y)));
		normal.y += (((pi.x) + (pj.x)) * ((pj.z) - (pi.z)));
		normal.z += (((pi.y) + (pj.y)) * ((pj.x) - (pi.x)));
	}

	if ((normal.length_squared() == 0) || !normal.isfinite()) {
		log("MeshExtender::plane_from_poly_newell : poly with invalid normal detected.");
		for (int i = 0; i < num_points; i++) {
			u32 ind_i = p_inds[r_poly.first_ind + i];
			const FPoint3 &pi = p_verts[ind_i];
			log(String("\t") + i + " : " + pi);
		}
		NP_WARN_PRINT_ONCE("MeshExtender::plane_from_poly_newell : poly with invalid normal detected.");
		r_poly.plane.zero();
		return false;
	}

	normal.normalize();
	center /= num_points;

	NP_DEV_ASSERT(normal.isfinite());

	// point and normal
	r_poly.plane.set(center, normal);
	return true;
}

bool MeshExtender::extend_mesh(Mesh &r_mesh) {
	// First save original stats.
	Mesh::ExtensionData &ex = r_mesh.extension_data;
	ex.orig_num_inds = r_mesh.get_num_inds();
	ex.orig_num_verts = r_mesh.get_num_verts();
	ex.orig_num_polys = r_mesh.get_num_polys();

	NP_DEV_ASSERT(!r_mesh.data.external_wall_pairs.size());
	NP_DEV_ASSERT(!r_mesh.data.internal_wall_pairs.size());

	push_explicit_internal_wall_pairs(r_mesh);

	find_edges(r_mesh);

	// Go through each edge and extend.
	for (u32 n = 0; n < _edges.size(); n++) {
		extend_edge(r_mesh, n);
	}

	// Go through each connection and extend.
	//	for (u32 n = 0; n < r_mesh.data.wall_connections.size(); n++) {
	//		extend_connection(r_mesh, n);
	//	}

	r_mesh.clear(true);
	return true;
}

bool MeshExtender::unextend_mesh(Mesh &r_mesh) {
	// Restore orig sizes.
	const Mesh::ExtensionData &ex = r_mesh.extension_data;
	r_mesh.floor.inds.resize(ex.orig_num_inds);
	r_mesh.floor.verts.resize(ex.orig_num_verts);
	r_mesh.floor.fverts3.resize(ex.orig_num_verts);
	r_mesh.floor.polys.resize(ex.orig_num_polys);
	r_mesh._polys_extra.resize(ex.orig_num_polys);
	r_mesh.floor.poly_bounds.resize(ex.orig_num_polys);

	r_mesh.clear(true);
	return true;
}

void MeshExtender::find_edges(const Mesh &p_mesh) {
	_edges.clear();

	const TVector<u32> &cwalls = p_mesh.data.lipped_wall_ids;
	u32 num_cwalls = cwalls.size();

	for (u32 n = 0; n < num_cwalls; n++) {
		add_cwall_to_edges(cwalls[n], p_mesh);
	}

	// At this point, we may end up with separate strands of edge,
	// which can be joined again to form a smaller number of edges.
	for (u32 e = 0; e < _edges.size(); e++) {
		try_join_edge(e, p_mesh);
	}

	identify_loop_edges(p_mesh);
	//orientate_edges(p_mesh);

	NP_LLOG("Final edges:");
	for (u32 n = 0; n < _edges.size(); n++) {
		const Edge &edge = _edges[n];
		String sz = "\t";
		for (u32 i = 0; i < edge.walls.size(); i++) {
			const EdgeWall &ew = edge.walls[i];
			sz += String(ew.connecting_wall_id) + " ( " + ew.ind_a + ", " + ew.ind_b + " ), ";
		}

		NP_LLOG(sz);
	}

	// Keep a list of the pairs left to do.
	// When it is empty, we are done.
	//	Vector<u32> remaining_pairs;
	//	remaining_pairs.resize(p_mesh.data.external_wall_pairs.size());
	//	for (u32 n=0; n<remaining_pairs.size(); n++)
	//	{
	//		remaining_pairs[n] = n;
	//	}
}

void MeshExtender::identify_loop_edges(const Mesh &p_mesh) {
	for (u32 e = 0; e < _edges.size(); e++) {
		Edge &edge = _edges[e];
		if (edge.walls.size() > 2) {
			if (can_join_walls(*edge.walls.get_first(), *edge.walls.get_last()) == -1) {
				edge.loop = true;
			}
		}
	}
}

void MeshExtender::try_join_edge(u32 p_edge_id, const Mesh &p_mesh) {
	// Beware of vector resizing, keep the pointer refreshed.
	Edge *edge = &_edges[p_edge_id];

	for (u32 e = p_edge_id + 1; e < _edges.size(); e++) {
		const Edge &other = _edges[e];

		if (can_join_walls(*edge->walls.get_last(), *other.walls.get_first()) == 1) {
			// Copy to back of existing edge.
			edge->walls.insert_multiple(&other.walls[0], other.walls.size(), edge->walls.size());

			// Now remove the other edge.
			_edges.remove(e);

			// Repeat this edge ID as a new one will have replaced it.
			e--;

			continue;
		}

		if (can_join_walls(*edge->walls.get_first(), *other.walls.get_last())) {
			// Copy to front of the existing edge.
			edge->walls.insert_multiple(&other.walls[0], other.walls.size(), 0);

			// Now remove the other edge.
			_edges.remove(e);

			// Repeat this edge ID as a new one will have replaced it.
			e--;

			continue;
		}
	}
}

//void MeshExtender::orientate_edges(const Mesh &p_mesh) {
//	for (u32 e = 0; e < _edges.size(); e++) {
//		Edge &edge = _edges[e];
//		if (edge.cwalls.size() >= 2) {
//			// Always use 0 and 1 by convention.
//			const Wall &wa = p_mesh.get_wall(edge.cwalls[0]);
//			const Wall &wb = p_mesh.get_wall(edge.cwalls[1]);

//			if (wa.get_swapped_vert_a() == wb.get_swapped_vert_b()) {
//				edge.cwalls.invert();
//			}
//		}
//	}
//}

i32 MeshExtender::can_join_walls(const EdgeWall &p_wall_a, const EdgeWall &p_wall_b) const {
	// If they have a common vertex.
	if (p_wall_a.ind_b == p_wall_b.ind_a)
		return 1;
	if (p_wall_b.ind_b == p_wall_a.ind_a)
		return -1;

	return 0;
}

void MeshExtender::add_cwall_to_edges(u32 p_cwall_id, const Mesh &p_mesh) {
	// First check existing edges...
	const Wall &wall = p_mesh.get_wall(p_cwall_id);

	// First fill out the EdgeWall
	EdgeWall ew;
	ew.ind_a = wall.get_swapped_vert_a();
	ew.ind_b = wall.get_swapped_vert_b();
	ew.a = p_mesh.get_vert(ew.ind_a);
	ew.b = p_mesh.get_vert(ew.ind_b);
	ew.connecting_wall_id = p_cwall_id;
	ew.normal = -wall.normal; // We want to push outward from the edge, rather than inward.
	ew.poly_id = wall.poly_id;
	ew.external = !p_mesh.data.internal_wall_ids.contains(p_cwall_id);

	for (u32 n = 0; n < _edges.size(); n++) {
		Edge &edge = _edges[n];

		if (can_join_walls(*edge.walls.get_last(), ew) == 1) {
			// Add to back of edge
			edge.walls.push_back(ew);
			return;
		}

		if (can_join_walls(ew, *edge.walls.get_first()) == 1) {
			//		if (cwall.has_vert(first.vert_a) || cwall.has_vert(first.vert_b)) {
			// Add to back of edge
			edge.walls.insert(0, ew);
			return;
		}
	}

	// If not found, create new edge.
	Edge &new_edge = _edges.request();
	new_edge.walls.push_back(ew);
}

} //namespace NavPhysics
