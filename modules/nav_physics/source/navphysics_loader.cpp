#include "navphysics_loader.h"
#include "navphysics_rect.h"

#include <cstring>

namespace NavPhysics {

bool RawLoader::read_u32(const u8 **pp_data, u32 &r_bytes_left, u32 &r_value) {
	if (r_bytes_left < 4) {
		log("RawLoader failed to read u32.");
		return false;
	}

	const u32 *pval = (const u32 *)*pp_data;
	// Big / little endian?
	r_value = *pval;
	*pp_data += 4;
	return true;
}

bool RawLoader::read_i32(const u8 **pp_data, u32 &r_bytes_left, i32 &r_value) {
	if (r_bytes_left < 4) {
		log("RawLoader failed to read i32.");
		return false;
	}
	const i32 *pval = (const i32 *)*pp_data;
	// Big / little endian?
	r_value = *pval;
	*pp_data += 4;
	return true;
}

bool RawLoader::read_f32(const u8 **pp_data, u32 &r_bytes_left, f32 &r_value) {
	if (r_bytes_left < 4) {
		log("RawLoader failed to read f32.");
		return false;
	}

	const f32 *pval = (const f32 *)*pp_data;
	// Big / little endian?
	r_value = *pval;
	*pp_data += 4;
	return true;
}

bool RawLoader::read_ipoint2(const u8 **pp_data, u32 &r_bytes_left, IPoint2 &r_point) {
	if (!read_i32(pp_data, r_bytes_left, r_point.x))
		return false;
	if (!read_i32(pp_data, r_bytes_left, r_point.y))
		return false;
	return true;
}

bool RawLoader::read_fpoint3(const u8 **pp_data, u32 &r_bytes_left, FPoint3 &r_point) {
	if (!read_f32(pp_data, r_bytes_left, r_point.x))
		return false;
	if (!read_f32(pp_data, r_bytes_left, r_point.y))
		return false;
	if (!read_f32(pp_data, r_bytes_left, r_point.z))
		return false;
	return true;
}

bool RawLoader::read_fpoint2(const u8 **pp_data, u32 &r_bytes_left, FPoint2 &r_point) {
	if (!read_f32(pp_data, r_bytes_left, r_point.x))
		return false;
	if (!read_f32(pp_data, r_bytes_left, r_point.y))
		return false;
	return true;
}

void RawLoader::write_u32(Vector<uint8_t> &r_data, u32 p_val) {
	uint8_t *p = (uint8_t *)&p_val;
	r_data.push_back(*p++);
	r_data.push_back(*p++);
	r_data.push_back(*p++);
	r_data.push_back(*p);
}

void RawLoader::write_i32(Vector<uint8_t> &r_data, i32 p_val) {
	uint8_t *p = (uint8_t *)&p_val;
	r_data.push_back(*p++);
	r_data.push_back(*p++);
	r_data.push_back(*p++);
	r_data.push_back(*p);
}

void RawLoader::write_f32(Vector<uint8_t> &r_data, f32 p_val) {
	uint8_t *p = (uint8_t *)&p_val;
	r_data.push_back(*p++);
	r_data.push_back(*p++);
	r_data.push_back(*p++);
	r_data.push_back(*p);
}

void RawLoader::write_ipoint2(Vector<uint8_t> &r_data, const IPoint2 &p_point) {
	write_i32(r_data, p_point.x);
	write_i32(r_data, p_point.y);
}

void RawLoader::write_fpoint3(Vector<uint8_t> &r_data, const FPoint3 &p_point) {
	write_f32(r_data, p_point.x);
	write_f32(r_data, p_point.y);
	write_f32(r_data, p_point.z);
}

void RawLoader::write_fpoint2(Vector<uint8_t> &r_data, const FPoint2 &p_point) {
	write_f32(r_data, p_point.x);
	write_f32(r_data, p_point.y);
}

////////////////////////////////////////////////////////

void Loader::llog(String p_sz) {
	//log(p_sz);
}
void Loader::log_load(String p_sz) {
	log(String("log_load:\t") + p_sz);
}

void Loader::find_index_nexts(Mesh &r_dest) {
	for (u32 p = 0; p < r_dest.get_num_polys(); p++) {
		const Poly &poly = r_dest.get_poly(p);

		u32 num_poly_inds = poly.num_inds;

		for (u32 i = 0; i < num_poly_inds; i++) {
			u32 next = (i + 1) % num_poly_inds;
			next += poly.first_ind;

			next = r_dest.get_ind(next);
			r_dest._inds_next.push_back(next);
		} // for i
	} // for p
}

u32 Loader::find_or_create_vert(Mesh &r_dest, const FPoint3 &p_pt) {
	Vector<FPoint3> &dverts = r_dest._fverts3;

	// Slow for now...
	for (u32 n = 0; n < dverts.size(); n++) {
		if (dverts[n].is_equal_approx(p_pt)) {
			return n;
		}
	}

	// Not found .. add.
	dverts.push_back(p_pt);
	//r_dest._fverts.push_back(FPoint2::make(p_pt.x, p_pt.z));
	return dverts.size() - 1;
}

void Loader::plane_from_poly_newell(Mesh &r_dest, Poly &r_poly) const {
	int num_points = r_poly.num_inds;

	if (num_points < 3) {
		r_poly.plane.zero();
		return;
	}

	FPoint3 normal;
	FPoint3 center;

	for (int i = 0; i < num_points; i++) {
		int j = (i + 1) % num_points;

		u32 ind_i = r_dest._inds[r_poly.first_ind + i];
		u32 ind_j = r_dest._inds[r_poly.first_ind + j];

		const FPoint3 &pi = r_dest._fverts3[ind_i];
		const FPoint3 &pj = r_dest._fverts3[ind_j];

		center += pi;

		normal.x += (((pi.z) + (pj.z)) * ((pj.y) - (pi.y)));
		normal.y += (((pi.x) + (pj.x)) * ((pj.z) - (pi.z)));
		normal.z += (((pi.y) + (pj.y)) * ((pj.x) - (pi.x)));
	}

	normal.normalize();
	center /= num_points;

	// point and normal
	r_poly.plane.set(center, normal);
}

bool Loader::_load_polys(u32 p_num_polys, const u32 *p_num_poly_inds, Mesh &r_mesh) {
	if (!p_num_polys)
		return false;
	if (!p_num_poly_inds)
		return false;
	if (!r_mesh._fverts3.size())
		return false;

	r_mesh._polys.resize(p_num_polys);
	r_mesh._polys_extra.resize(p_num_polys);

	u32 index_count = 0;

	for (u32 n = 0; n < p_num_polys; n++) {
		Poly &dpoly = r_mesh._polys[n];
		dpoly.init();

		dpoly.first_ind = index_count;
		dpoly.center3.zero();

		u32 num_inds = p_num_poly_inds[n];

		llog(String("Poly ") + n);

		for (u32 v = 0; v < num_inds; v++) {
			u32 ind = r_mesh._inds[index_count++];

			NP_DEV_ASSERT(ind < r_mesh._fverts3.size());
			const FPoint3 &pt = r_mesh.get_fvert3(ind);

			//u32 new_ind = find_or_create_vert(r_dest, pt);
			//llog(String("\told_ind ") + ind + ",\tnew_ind " + new_ind + ", :\t" + pt);
			//r_dest._inds.push_back(new_ind);
			dpoly.center3 += pt;
		}

		dpoly.num_inds = num_inds;
		if (num_inds) {
			dpoly.center3 /= (float)num_inds;
		}

		// Clockwise flag not yet dealt with...
		plane_from_poly_newell(r_mesh, dpoly);

		//index_count += num_inds;
	}

	return true;
}

bool Loader::load_polys(const SourceMeshData &p_mesh, Mesh &r_dest) {
	if (!p_mesh.num_verts)
		return false;
	if (!p_mesh.num_indices)
		return false;

	// Assuming polys are tris.
	//	u32 num_polys = p_mesh.num_indices / 3;
	u32 num_polys = p_mesh.num_polys;

	u32 index_count = 0;
	for (u32 n = 0; n < num_polys; n++) {
		u32 num_inds = p_mesh.poly_num_indices[n];

		llog(String("Poly ") + n);

		for (u32 v = 0; v < num_inds; v++) {
			u32 ind = p_mesh.indices[index_count++];

			NP_DEV_ASSERT(ind < p_mesh.num_verts);
			const FPoint3 &pt = p_mesh.verts[ind];

			u32 new_ind = find_or_create_vert(r_dest, pt);
			llog(String("\told_ind ") + ind + ",\tnew_ind " + new_ind + ", :\t" + pt);
			r_dest._inds.push_back(new_ind);
			//dpoly.center3 += pt;
		}
	}

	return _load_polys(p_mesh.num_polys, p_mesh.poly_num_indices, r_dest);
}

bool Loader::_load_polys_old(const SourceMeshData &p_mesh, Mesh &r_dest) {
	if (!p_mesh.num_verts)
		return false;
	if (!p_mesh.num_indices)
		return false;

	// Assuming polys are tris.
	u32 num_polys = p_mesh.num_indices / 3;

	r_dest._polys.resize(num_polys);
	r_dest._polys_extra.resize(num_polys);

	u32 index_count = 0;

	for (u32 n = 0; n < num_polys; n++) {
		Poly &dpoly = r_dest._polys[n];
		dpoly.init();

		dpoly.first_ind = r_dest._inds.size();
		dpoly.center3.zero();

		u32 num_inds = p_mesh.poly_num_indices[n];

		llog(String("Poly ") + n);

		for (u32 v = 0; v < num_inds; v++) {
			u32 ind = p_mesh.indices[index_count++];

			NP_DEV_ASSERT(ind < p_mesh.num_verts);
			const FPoint3 &pt = p_mesh.verts[ind];

			u32 new_ind = find_or_create_vert(r_dest, pt);
			llog(String("\told_ind ") + ind + ",\tnew_ind " + new_ind + ", :\t" + pt);
			r_dest._inds.push_back(new_ind);
			dpoly.center3 += pt;
		}

		dpoly.num_inds = num_inds;
		if (num_inds) {
			dpoly.center3 /= (float)num_inds;
		}

		// Clockwise flag not yet dealt with...
		plane_from_poly_newell(r_dest, dpoly);
	}

	return true;
}

void Loader::load_fixed_point_verts(Mesh &r_dest) {
	//const Vector<FPoint2> &sverts = r_dest._fverts;

	// first find the offset and scale
	u32 num_verts = r_dest._fverts3.size();

	if (!num_verts) {
		return;
	}

	Rect2 rect;
	rect.position = r_dest.get_fvert(0);

	for (u32 i = 1; i < num_verts; i++) {
		rect.expand_to(r_dest.get_fvert(i));
	}

	llog(String("Internal Map AABB is ") + String(rect.position) + ", " + String(rect.size));

	r_dest._f32_to_fp_scale = FPoint2::make(FPoint2::FP_RANGE / rect.size.x, FPoint2::FP_RANGE / rect.size.y);
	r_dest._f32_to_fp_offset = -rect.position;

	r_dest._fp_to_f32_offset = rect.position;
	r_dest._fp_to_f32_scale = FPoint2::make(rect.size.x / FPoint2::FP_RANGE, rect.size.y / FPoint2::FP_RANGE);

	llog(String("_f32_to_fp_scale ") + r_dest._f32_to_fp_scale);
	llog(String("_f32_to_fp_offset ") + r_dest._f32_to_fp_offset);

	llog(String("_fp_to_f32_offset ") + r_dest._fp_to_f32_offset);
	llog(String("_fp_to_f32_scale ") + r_dest._fp_to_f32_scale);

	for (u32 i = 0; i < num_verts; i++) {
		FPoint2 v = r_dest.get_fvert(i);
		IPoint2 vfp = r_dest.float_to_fixed_point_2(v);
		// print("vert " + str(i) + " : " + vfp.sz())
		r_dest._verts.push_back(vfp);

		// var verify = _dmap._fpvec2_to_float(vfp)
		// var l = (verify- v).length()
		// print("length " + str(l))
	}

	// find the poly fixed point centers
	Vector<Poly> &dpolys = r_dest._polys;

	for (u32 n = 0; n < dpolys.size(); n++) {
		Poly &dpoly = dpolys[n];
		dpoly.center = r_dest.float_to_fixed_point_2(FPoint2::make(dpoly.center3.x, dpoly.center3.z));
	}

	// Print verts
	llog("Verts:");
	for (u32 n = 0; n < r_dest._verts.size(); n++) {
		llog(String("\t") + n + String(" :\t") + r_dest._verts[n]);
	}
}

// returns linked wall or -1
u32 Loader::find_linked_poly(Mesh &r_dest, u32 p_poly_from, u32 p_ind_a, u32 p_ind_b, u32 &r_linked_poly) const {
	for (u32 pb = p_poly_from + 1; pb < r_dest.get_num_polys(); pb++) {
		const Poly &poly_b = r_dest.get_poly(pb);
		if (poly_b.num_inds < 3) {
			continue;
		}
		for (u32 wb = 0; wb < poly_b.num_inds; wb++) {
			u32 wall_id_c = poly_b.first_ind + wb;
			u32 wall_id_d = poly_b.first_ind + ((wb + 1) % poly_b.num_inds);
			u32 ind_c = r_dest.get_ind(wall_id_c);
			u32 ind_d = r_dest.get_ind(wall_id_d);
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

void Loader::find_links(Mesh &r_dest) {
	Vector<uint32_t> &links = r_dest._links;
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

	// Print links
	llog("Links:");
	for (u32 n = 0; n < links.size(); n++) {
		llog(String("\t") + links[n]);
	}
}

void Loader::find_walls(Mesh &r_dest) {
	u32 num_walls = r_dest.get_num_links();
	r_dest._walls.resize(num_walls);
	r_dest._walls.fill(Wall());
	for (u32 w = 0; w < num_walls; w++) {
		// if (!r_dest.is_hard_wall(w)) {
		// continue;
		// }
		u32 ind_a = r_dest.get_ind(w);
		u32 ind_b = r_dest.get_ind_next(w);
		Wall &wall = r_dest._walls[w];
		wall.vert_a = ind_a;
		wall.vert_b = ind_b;
		IPoint2 wa = r_dest.get_vert(ind_a);
		IPoint2 wb = r_dest.get_vert(ind_b);
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
	for (u32 p = 0; p < r_dest.get_num_polys(); p++) {
		const Poly &poly = r_dest.get_poly(p);
		NP_ERR_CONTINUE(poly.num_inds < 3);
		for (u32 i = 0; i < poly.num_inds; i++) {
			u32 wall_id = poly.first_ind + i;
			if (!r_dest.is_hard_wall(wall_id)) {
				continue;
			}
			r_dest._walls[wall_id].poly_id = p;
		}
	}
	// Now find previous and next walls
	for (u32 wa = 0; wa < num_walls; wa++) {
		if (!r_dest.is_hard_wall(wa)) {
			continue;
		}
		const Wall &wall_a = r_dest.get_wall(wa);
		for (u32 wb = wa + 1; wb < num_walls; wb++) {
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

	// Print links
	llog("Walls:");
	for (u32 n = 0; n < r_dest._walls.size(); n++) {
		const Wall &wall = r_dest._walls[n];
		llog(String("\twall ") + n);
		llog(String("\t\tprev_wall ") + wall.prev_wall);
		llog(String("\t\tnext_wall ") + wall.next_wall);
		llog(String("\t\tnormal ") + wall.normal);
		llog(String("\t\tpoly_id ") + wall.poly_id);
		llog(String("\t\tvert_a ") + wall.vert_a);
		llog(String("\t\tvert_b ") + wall.vert_b);
		llog(String("\t\twall_vec ") + wall.wall_vec);
	}
}

void Loader::wall_add_neighbour_wall(Mesh &r_dest, u32 p_a, u32 p_b) {
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
	NP_DEV_ASSERT(false);
}

void Loader::find_bottlenecks(Mesh &r_dest) {
}

bool Loader::extract_working_data(WorkingMeshData &r_data, const Mesh &p_mesh) {
	r_data.num_verts = p_mesh.get_num_verts();
	r_data.verts = p_mesh._fverts3.ptr();
	r_data.iverts = p_mesh._verts.ptr();

	r_data.num_indices = p_mesh.get_num_inds();
	r_data.indices = p_mesh._inds.ptr();

	r_data.num_polys = p_mesh._polys.size();
	_poly_num_indices.resize(r_data.num_polys);
	for (u32 n = 0; n < r_data.num_polys; n++) {
		u32 num_inds = p_mesh.get_poly(n).num_inds;
		_poly_num_indices[n] = num_inds;

		//llog(String("poly ") + n + " has " + num_inds + " indices.");
	}
	r_data.poly_num_indices = _poly_num_indices.ptr();

	r_data.fixed_point_to_float_offset = p_mesh._fp_to_f32_offset;
	r_data.fixed_point_to_float_scale = p_mesh._fp_to_f32_scale;
	r_data.float_to_fixed_point_offset = p_mesh._f32_to_fp_offset;
	r_data.float_to_fixed_point_scale = p_mesh._f32_to_fp_scale;

	return true;
}

bool Loader::load_raw_data(const uint8_t *p_data, uint32_t p_num_bytes, Mesh &r_mesh) {
	// FOURCC?

	// Version
	u32 version;
	if (!RawLoader::read_u32(&p_data, p_num_bytes, version)) {
		return false;
	}

	if (version != 100) {
		log("NavMesh data version incorrect.");
		return false;
	}
	log_load("Version correct.");

	WorkingMeshData md;
	if (!RawLoader::read_u32(&p_data, p_num_bytes, md.num_verts))
		return false;

	if (!RawLoader::read_u32(&p_data, p_num_bytes, md.num_indices))
		return false;

	if (!RawLoader::read_u32(&p_data, p_num_bytes, md.num_polys))
		return false;
	log_load("Numbers read OK.");

	if (!md.num_verts)
		return false;
	if (!md.num_indices)
		return false;
	if (!md.num_polys)
		return false;

	Vector<FPoint3> verts;
	verts.resize(md.num_verts);

	Vector<IPoint2> iverts;
	iverts.resize(md.num_verts);

	Vector<u32> inds;
	inds.resize(md.num_indices);

	Vector<u32> poly_num_inds;
	poly_num_inds.resize(md.num_polys);

	for (u32 n = 0; n < md.num_verts; n++) {
		if (!RawLoader::read_fpoint3(&p_data, p_num_bytes, verts[n])) {
			log("Failed to read vert.");
			return false;
		}
	}
	log_load("Verts read OK.");
	for (u32 n = 0; n < md.num_verts; n++) {
		if (!RawLoader::read_ipoint2(&p_data, p_num_bytes, iverts[n])) {
			log("Failed to read ivert.");
			return false;
		}
	}
	log_load("IVerts read OK.");
	for (u32 n = 0; n < md.num_indices; n++) {
		if (!RawLoader::read_u32(&p_data, p_num_bytes, inds[n])) {
			log("Failed to read index.");
			return false;
		}
	}
	log_load("Indices read OK.");
	for (u32 n = 0; n < md.num_polys; n++) {
		if (!RawLoader::read_u32(&p_data, p_num_bytes, poly_num_inds[n])) {
			log("Failed to read poly.");
			return false;
		}
	}
	log_load("Polys read OK.");

	md.verts = verts.ptr();
	md.iverts = iverts.ptr();
	md.indices = inds.ptr();
	md.poly_num_indices = poly_num_inds.ptr();

	if (!RawLoader::read_fpoint2(&p_data, p_num_bytes, md.fixed_point_to_float_offset)) {
		log("Failed to read fixed_point_to_float_offset.");
		return false;
	}
	if (!RawLoader::read_fpoint2(&p_data, p_num_bytes, md.fixed_point_to_float_scale)) {
		log("Failed to read fixed_point_to_float_scale.");
		return false;
	}
	if (!RawLoader::read_fpoint2(&p_data, p_num_bytes, md.float_to_fixed_point_offset)) {
		log("Failed to read float_to_fixed_point_offset.");
		return false;
	}
	if (!RawLoader::read_fpoint2(&p_data, p_num_bytes, md.float_to_fixed_point_scale)) {
		log("Failed to read float_to_fixed_point_scale.");
		return false;
	}
	log_load("Offsets and scales read OK.");

	return load_working_data(md, r_mesh);
}

uint32_t Loader::prepare_raw_data(const Mesh &p_mesh) {
	_save_data.clear();

	// FOURCC?

	// Version
	RawLoader::write_u32(_save_data, 100);

	WorkingMeshData md;
	extract_working_data(md, p_mesh);

	RawLoader::write_u32(_save_data, md.num_verts);
	RawLoader::write_u32(_save_data, md.num_indices);
	RawLoader::write_u32(_save_data, md.num_polys);

	for (u32 n = 0; n < md.num_verts; n++) {
		RawLoader::write_fpoint3(_save_data, md.verts[n]);
	}

	for (u32 n = 0; n < md.num_verts; n++) {
		RawLoader::write_ipoint2(_save_data, md.iverts[n]);
	}
	for (u32 n = 0; n < md.num_indices; n++) {
		RawLoader::write_u32(_save_data, md.indices[n]);
	}
	for (u32 n = 0; n < md.num_polys; n++) {
		RawLoader::write_u32(_save_data, md.poly_num_indices[n]);
	}

	RawLoader::write_fpoint2(_save_data, md.fixed_point_to_float_offset);
	RawLoader::write_fpoint2(_save_data, md.fixed_point_to_float_scale);
	RawLoader::write_fpoint2(_save_data, md.float_to_fixed_point_offset);
	RawLoader::write_fpoint2(_save_data, md.float_to_fixed_point_scale);

	log(String("save_data size ") + _save_data.size() + " bytes.");

	return _save_data.size();
}

bool Loader::save_raw_data(uint8_t *r_data, uint32_t p_num_bytes) {
	if (p_num_bytes != _save_data.size())
		return false;

	memcpy(r_data, _save_data.ptr(), p_num_bytes);
	return true;
}

bool Loader::load_working_data(const WorkingMeshData &p_data, Mesh &r_mesh) {
	r_mesh.clear();

	if (!p_data.verts)
		return false;
	if (!p_data.indices)
		return false;
	if (!p_data.poly_num_indices)
		return false;
	if (!p_data.num_verts)
		return false;
	if (!p_data.num_indices)
		return false;
	if (!p_data.num_polys)
		return false;

	// Verts
	r_mesh._fverts3.resize(p_data.num_verts);
	r_mesh._verts.resize(p_data.num_verts);

	for (u32 n = 0; n < p_data.num_verts; n++) {
		r_mesh._fverts3[n] = p_data.verts[n];
		r_mesh._verts[n] = p_data.iverts[n];
	}

	// Inds
	r_mesh._inds.resize(p_data.num_indices);

	for (u32 n = 0; n < p_data.num_indices; n++) {
		r_mesh._inds[n] = p_data.indices[n];
	}

	// Polys
	r_mesh._polys.resize(p_data.num_polys);
	r_mesh._polys_extra.resize(p_data.num_polys);

	r_mesh._f32_to_fp_offset = p_data.float_to_fixed_point_offset;
	r_mesh._f32_to_fp_scale = p_data.float_to_fixed_point_scale;
	r_mesh._fp_to_f32_offset = p_data.fixed_point_to_float_offset;
	r_mesh._fp_to_f32_scale = p_data.fixed_point_to_float_scale;

	if (!_load_polys(p_data.num_polys, p_data.poly_num_indices, r_mesh))
		return false;

	_load(r_mesh);
	return true;
}

bool Loader::load_mesh(const SourceMeshData &p_source_mesh, Mesh &r_mesh) {
	r_mesh.clear();

	if (!p_source_mesh.verts)
		return false;
	if (!p_source_mesh.indices)
		return false;

	if (!load_polys(p_source_mesh, r_mesh)) {
		goto failed;
	}

	load_fixed_point_verts(r_mesh);

	_load(r_mesh);

	return true;

failed:
	r_mesh.clear();
	return false;
}

void Loader::_load(Mesh &r_mesh) {
	find_index_nexts(r_mesh);
	find_links(r_mesh);
	find_walls(r_mesh);
	find_bottlenecks(r_mesh);

	log("NavPhysics loaded mesh:");
	log(String("\tpolys: ") + r_mesh.get_num_polys());
	log(String("\tverts: ") + r_mesh.get_num_verts());
	log(String("\tinds: ") + r_mesh.get_num_inds());

	log(String("\tlinks: ") + r_mesh.get_num_links());
	log(String("\twalls: ") + r_mesh.get_num_walls());
}

} // namespace NavPhysics
