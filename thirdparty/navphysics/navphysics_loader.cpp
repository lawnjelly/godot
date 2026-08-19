#include "navphysics_loader.h"
#include "navphysics_error.h"
#include "navphysics_extender.h"
#include "navphysics_jump_finder.h"
#include "navphysics_mesh_funcs.h"
#include "navphysics_rect.h"
#include "navphysics_simplifier.h"

#include <cstring>

#define NAVPHYSICS_FILE_VERSION 105

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

void RawLoader::write_u32(TVector<uint8_t> &r_data, u32 p_val) {
	uint8_t *p = (uint8_t *)&p_val;
	r_data.push_back(*p++);
	r_data.push_back(*p++);
	r_data.push_back(*p++);
	r_data.push_back(*p);
}

void RawLoader::write_i32(TVector<uint8_t> &r_data, i32 p_val) {
	uint8_t *p = (uint8_t *)&p_val;
	r_data.push_back(*p++);
	r_data.push_back(*p++);
	r_data.push_back(*p++);
	r_data.push_back(*p);
}

void RawLoader::write_f32(TVector<uint8_t> &r_data, f32 p_val) {
	uint8_t *p = (uint8_t *)&p_val;
	r_data.push_back(*p++);
	r_data.push_back(*p++);
	r_data.push_back(*p++);
	r_data.push_back(*p);
}

void RawLoader::write_ipoint2(TVector<uint8_t> &r_data, const IPoint2 &p_point) {
	write_i32(r_data, p_point.x);
	write_i32(r_data, p_point.y);
}

void RawLoader::write_fpoint3(TVector<uint8_t> &r_data, const FPoint3 &p_point) {
	write_f32(r_data, p_point.x);
	write_f32(r_data, p_point.y);
	write_f32(r_data, p_point.z);
}

void RawLoader::write_fpoint2(TVector<uint8_t> &r_data, const FPoint2 &p_point) {
	write_f32(r_data, p_point.x);
	write_f32(r_data, p_point.y);
}

////////////////////////////////////////////////////////

bool Loader::_editor_only = false;

void Loader::llog(String p_sz) {
	//log(p_sz);
}
void Loader::log_load(String p_sz) {
	log(String("log_load:\t") + p_sz);
}

void Loader::floodfill_islands(Mesh &r_mesh) {
	u32 num_polys = r_mesh.get_num_polys();

	Vector<u32> flood_list;

	u32 island_id = 0;

	for (u32 n = 0; n < num_polys; n++) {
		if (r_mesh.get_poly_extra(n).island_id != UINT16_MAX) {
			continue;
		}

		flood_list.push_back(n);

		while (!flood_list.is_empty()) {
			u32 id;
			flood_list.pop_back(id);
			r_mesh.get_poly_extra(id).island_id = island_id;

			const Poly &poly = r_mesh.get_poly(id);

			// Neighbours.
			for (u32 i = 0; i < poly.num_inds; i++) {
				if (!r_mesh.is_link_hard(poly.first_ind + i)) {
					u32 linked_poly_id = r_mesh.get_link(poly.first_ind + i);
					if (r_mesh.get_poly_extra(linked_poly_id).island_id == UINT16_MAX) {
						flood_list.push_back(linked_poly_id);
					}
				}
			}
		}

		island_id++;
	}

	log(String("Floodfill islands ") + island_id + " islands found.");
}

void Loader::find_index_nexts(Mesh &r_dest) {
	r_dest._inds_next.clear();

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

u32 Loader::find_or_create_vert(TVector<FPoint3> &r_verts, const FPoint3 &p_pt) {
	// Slow for now...
	for (u32 n = 0; n < r_verts.size(); n++) {
		if (r_verts[n].is_equal_approx(p_pt)) {
			return n;
		}
	}

	// Not found .. add.
	r_verts.push_back(p_pt);
	return r_verts.size() - 1;
}

bool Loader::_load_ceiling_polys(u32 p_num_polys, const u32 *p_num_poly_inds, Mesh &r_mesh) {
	if (!p_num_polys)
		return false;
	if (!p_num_poly_inds)
		return false;
	if (!r_mesh.ceiling.fverts3.size())
		return false;

	r_mesh.ceiling.polys.resize(p_num_polys);

	u32 index_count = 0;

	// For some reason, not all loaded polys are
	// valid, there are some degenerate, so we want to ignore these.
	u32 poly_count = 0;

	for (u32 n = 0; n < p_num_polys; n++) {
		Poly &dpoly = r_mesh.ceiling.polys[poly_count++];
		dpoly.init();

		dpoly.first_ind = index_count;
		dpoly.center3.zero();

		u32 num_inds = p_num_poly_inds[n];

		NP_LLOG(String("Ceil Poly ") + n);

		dpoly.num_inds = num_inds;

		if (!MeshExtender::fill_poly(r_mesh, r_mesh.ceiling, dpoly)) {
			// Degenerate, ignore.
			NP_WARN_PRINT_ONCE("Degenerate poly detected, ignoring.");
			poly_count--;
		} else {
			// Check for those not facing down.
			if (dpoly.plane.normal.y <= 0.001f) {
				NP_WARN_PRINT_ONCE("Ceiling poly not facing down detected, ignoring.");
				poly_count--;
			}
		}

		index_count += num_inds;
	}

	// The TRUE count of polys not including degenerates.
	r_mesh.ceiling.polys.resize(poly_count);

	return true;
}

void Loader::_calculate_poly_areas(Mesh &r_mesh) {
	u32 num_polys = r_mesh.get_num_polys();

	Vector<IPoint2> v;
	v.resize(NAV_PHYSICS_POLY_MAX_VERTS);

	u64 largest_area = 0;

	for (u32 n = 0; n < num_polys; n++) {
		// Calculate area.
		u32 num_verts = r_mesh.get_poly_verts(n, v.ptr(), NAV_PHYSICS_POLY_MAX_VERTS);

		const IPoint2 &a = v[0];

		u64 area = 0;

		for (u32 i = 2; i < num_verts; i++) {
			const IPoint2 &b = v[i - 1];
			const IPoint2 &c = v[i];

			IPoint2 vec_a = b - a;
			IPoint2 vec_b = c - a;

			i64 tri_area = vec_a.cross(vec_b);

#ifdef NP_OVERFLOW_CHECKS
			{
				NP_CHECK_64(tri_area + area);
			}
#endif

			area += ABS(tri_area);
		}

		r_mesh.get_poly_extra(n).area_absolute = area;
		largest_area = MAX(largest_area, area);
	}

	// Relative areas (to max size poly).
	// These are more useful at runtime for some purposes.
	for (u32 n = 0; n < num_polys; n++) {
		PolyExtra &ex = r_mesh.get_poly_extra(n);
		ex.area = (f64)ex.area_absolute / (f64)largest_area;
	}
}

bool Loader::_load_polys(u32 p_num_polys, const u32 *p_num_poly_inds, u32 p_ceiling_num_polys, const u32 *p_ceiling_num_poly_inds, Mesh &r_mesh) {
	if (!p_num_polys)
		return false;
	if (!p_num_poly_inds)
		return false;
	if (!r_mesh.floor.fverts3.size())
		return false;

	r_mesh.floor.polys.resize(p_num_polys);
	r_mesh._polys_extra.resize(p_num_polys);
	r_mesh.floor.poly_bounds.resize(p_num_polys);
	r_mesh.ceiling.poly_bounds.resize(r_mesh.ceiling.polys.size());

	u32 index_count = 0;

	for (u32 n = 0; n < p_num_polys; n++) {
		Poly &dpoly = r_mesh.floor.polys[n];
		dpoly.init();

		dpoly.first_ind = index_count;
		dpoly.center3.zero();

		u32 num_inds = p_num_poly_inds[n];

		NP_LLOG(String("Poly ") + n);

		dpoly.num_inds = num_inds;

		MeshExtender::fill_poly(r_mesh, r_mesh.floor, dpoly);

		/*
				for (u32 v = 0; v < num_inds; v++) {
					u32 ind = r_mesh._inds[index_count++];

					NP_DEV_ASSERT(ind < r_mesh._fverts3.size());
					const FPoint3 &pt = r_mesh.get_fvert3(ind);

					//u32 new_ind = find_or_create_vert(r_dest, pt);
					//NP_LLOG(String("\told_ind ") + ind + ",\tnew_ind " + new_ind + ", :\t" + pt);
					//r_dest._inds.push_back(new_ind);
					dpoly.center3 += pt;
				}

				if (num_inds) {
					dpoly.center3 /= (float)num_inds;
				}

				// Clockwise flag not yet dealt with...
				plane_from_poly_newell(r_mesh, dpoly);
		*/
		index_count += num_inds;
		//MeshExtender::calculate_poly_bound(r_mesh, n);
	}

	_load_ceiling_polys(p_ceiling_num_polys, p_ceiling_num_poly_inds, r_mesh);

	return true;
}

void Loader::_calculate_poly_bounds(Mesh::SubMesh &r_submesh) {
	// Make sure the right size array.
	u32 num_polys = r_submesh.polys.size();
	r_submesh.poly_bounds.resize(num_polys);

	for (u32 n = 0; n < num_polys; n++) {
		MeshExtender::calculate_poly_bound(r_submesh, n);
	}
}

bool Loader::_is_bake_poly_valid(const SourceMeshData &p_mesh, u32 p_first_index, u32 p_num_indices, bool p_check_ceiling) const {
	if (p_num_indices < 3) {
		return false;
	}

	//	u32 index_count = p_first_index;

	FPoint3 normal;

	//	FPoint3 * corns = (FPoint3 *) alloca(sizeof (FPoint3) * p_num_indices);
	//	for (u32 v = 0; v < num_inds; v++) {
	//		u32 ind = p_mesh.indices[index_count++];

	//		NP_DEV_ASSERT(ind < p_mesh.num_verts);
	//		corns[v] = p_mesh.verts[ind];
	//	}

	for (int i = 0; i < p_num_indices; i++) {
		int j = (i + 1) % p_num_indices;

		u32 ind_j = p_mesh.indices[p_first_index + j];
		u32 ind_i = p_mesh.indices[p_first_index + i];

		const FPoint3 &pi = p_mesh.verts[ind_i];
		const FPoint3 &pj = p_mesh.verts[ind_j];

		//center += pi;

		normal.x += (((pi.z) + (pj.z)) * ((pj.y) - (pi.y)));
		normal.y += (((pi.x) + (pj.x)) * ((pj.z) - (pi.z)));
		normal.z += (((pi.y) + (pj.y)) * ((pj.x) - (pi.x)));
	}

	if (normal.length_squared() <= 0.0001f) {
		//log("Loader::_is_bake_poly_valid : poly with invalid normal detected.");
		NP_WARN_PRINT("Loader::_is_bake_poly_valid : poly with invalid normal detected.");
		for (int i = 0; i < p_num_indices; i++) {
			u32 ind_i = p_mesh.indices[p_first_index + i];
			const FPoint3 &pi = p_mesh.verts[ind_i];
			log(String("\t\t") + i + " : " + pi);
		}
		return false;
	}

	if (p_check_ceiling) {
		if (normal.y <= 0.001f) {
			//log("Loader::_is_bake_poly_valid : ceiling poly not facing downward detected.");
			NP_WARN_PRINT("Loader::_is_bake_poly_valid : ceiling poly not facing downward detected.");
			return false;
		}
	}

	return true;
}

bool Loader::bake_load_polys(const SourceMeshData &p_mesh, const SourceMeshData &p_ceil_mesh, Mesh &r_dest) {
	if (!p_mesh.num_verts)
		return false;
	if (!p_mesh.num_indices)
		return false;

	r_dest.mesh_params = p_mesh.params;

	// Note : We want to sanitize input polys here,
	// as we can't trust the client code.
	// Degenerate tris should be removed,
	// and ceiling polys not pointing downward.

	// Assuming polys are tris.
	u32 index_count = 0;
	u32 valid_floor_poly_count = 0;
	Vector<u32> valid_floor_poly_num_indices;
	valid_floor_poly_num_indices.resize(p_mesh.num_polys);

	for (u32 n = 0; n < p_mesh.num_polys; n++) {
		u32 num_inds = p_mesh.poly_num_indices[n];

		NP_LLOG(String("Poly ") + n);

		if (!_is_bake_poly_valid(p_mesh, index_count, num_inds, false)) {
			log(String("Floor poly ") + n + " is invalid, ignoring.");
			index_count += num_inds;
			continue;
		}

		for (u32 v = 0; v < num_inds; v++) {
			u32 ind = p_mesh.indices[index_count++];

			NP_DEV_ASSERT(ind < p_mesh.num_verts);
			const FPoint3 &pt = p_mesh.verts[ind];

			u32 new_ind = find_or_create_vert(r_dest.floor.fverts3, pt);
			NP_LLOG(String("\told_ind ") + ind + ",\tnew_ind " + new_ind + ", :\t" + pt);
			r_dest.floor.inds.push_back(new_ind);
		}

		valid_floor_poly_num_indices[valid_floor_poly_count] = num_inds;
		valid_floor_poly_count++;
	}

	index_count = 0;
	u32 valid_ceiling_poly_count = 0;
	Vector<u32> valid_ceiling_poly_num_indices;
	valid_ceiling_poly_num_indices.resize(p_ceil_mesh.num_polys);

	for (u32 n = 0; n < p_ceil_mesh.num_polys; n++) {
		u32 num_inds = p_ceil_mesh.poly_num_indices[n];

		NP_LLOG(String("Ceil Poly ") + n);
		if (!_is_bake_poly_valid(p_ceil_mesh, index_count, num_inds, true)) {
			log(String("Ceiling poly ") + n + " is invalid, ignoring.");
			index_count += num_inds;
			continue;
		}

		for (u32 v = 0; v < num_inds; v++) {
			u32 ind = p_ceil_mesh.indices[index_count++];

			NP_DEV_ASSERT(ind < p_ceil_mesh.num_verts);
			const FPoint3 &pt = p_ceil_mesh.verts[ind];

			u32 new_ind = find_or_create_vert(r_dest.ceiling.fverts3, pt);
			NP_LLOG(String("\told_ind ") + ind + ",\tnew_ind " + new_ind + ", :\t" + pt);
			r_dest.ceiling.inds.push_back(new_ind);
		}

		valid_ceiling_poly_num_indices[valid_ceiling_poly_count] = num_inds;
		valid_ceiling_poly_count++;
	}

	//bool success = _load_polys(p_mesh.num_polys, p_mesh.poly_num_indices, p_ceil_mesh.num_polys, p_ceil_mesh.poly_num_indices, r_dest);
	bool success = _load_polys(valid_floor_poly_count, valid_floor_poly_num_indices.ptr(), valid_ceiling_poly_count, valid_ceiling_poly_num_indices.ptr(), r_dest);

	return success;
}

void Loader::_get_poly_points(const Mesh &p_mesh, u32 p_poly_id, bool p_ceiling, Vector<IPoint2> &r_pts, Vector<FPoint3> &r_pts3) const {
	const Mesh::SubMesh *sm = &p_mesh.floor;
	if (p_ceiling) {
		sm = &p_mesh.ceiling;
	}

	const Poly &poly = sm->polys[p_poly_id];

	r_pts.resize(poly.num_inds);
	r_pts3.resize(poly.num_inds);

	for (u32 n = 0; n < poly.num_inds; n++) {
		u32 ind = sm->inds[poly.first_ind + n];
		r_pts[n] = sm->verts[ind];
		r_pts3[n] = sm->fverts3[ind];
	}
}

bool Loader::does_floor_and_ceiling_poly_collide(const Mesh &p_mesh, u32 p_floor_poly_id, u32 p_ceiling_poly_id) const {
	// Get the points of each....
	Vector<IPoint2> fpoints;
	Vector<FPoint3> fpoints3;
	Vector<IPoint2> cpoints;
	Vector<FPoint3> cpoints3;

	_get_poly_points(p_mesh, p_floor_poly_id, false, fpoints, fpoints3);
	_get_poly_points(p_mesh, p_ceiling_poly_id, true, cpoints, cpoints3);

	// Ignore if ceiling is below the floor.
	freal floor_lowest = FLT_MAX;
	for (u32 n = 0; n < fpoints3.size(); n++) {
		floor_lowest = MIN(floor_lowest, fpoints3[n].y);
	}
	freal ceil_highest = -FLT_MAX;
	for (u32 n = 0; n < cpoints3.size(); n++) {
		ceil_highest = MAX(ceil_highest, cpoints3[n].y);
	}
	if (ceil_highest <= floor_lowest) {
		return false;
	}

	// Several ways of doing this.
	// If all the points of one are outside the edge of another, they don't collide.

	// Point in poly test for any point in either means they collide.
	for (u32 loop = 0; loop < 2; loop++) {
		// Whether the TEST poly is the ceiling (points come from opposite submesh)
		bool ceiling = loop == 1;
		u32 test_poly_id = ceiling ? p_ceiling_poly_id : p_floor_poly_id;

		if (!ceiling) {
			for (u32 n = 0; n < cpoints.size(); n++) {
				if (p_mesh.poly_contains_point(test_poly_id, cpoints[n], ceiling)) {
					return true;
				}
			}
		} else {
			for (u32 n = 0; n < fpoints.size(); n++) {
				if (p_mesh.poly_contains_point(test_poly_id, fpoints[n], ceiling)) {
					return true;
				}
			}
		}

	} // for loop

	// If no points, do edge tests.
	// Collide each edge of floor against each edge of ceiling
	IPoint2 pt_hit;

	for (u32 i = 0; i < fpoints.size(); i++) {
		const IPoint2 &a = fpoints[i];
		const IPoint2 &b = fpoints[(i + 1) % fpoints.size()];

		for (u32 j = 0; j < cpoints.size(); j++) {
			const IPoint2 &c = cpoints[j];
			const IPoint2 &d = cpoints[(j + 1) % cpoints.size()];

			if (p_mesh.find_line_segments_intersect_integer(a, b, c, d, pt_hit)) {
				return true;
			}
		}
	}

	return false;
}

void Loader::sort_floor_ceiling_links(Mesh &r_mesh, u32 p_floor_poly_id) {
	PolyExtra &ex = r_mesh.get_poly_extra(p_floor_poly_id);
	if (ex.num_ceiling_links <= 1) {
		return;
	}

	struct PolyHeight {
		u32 poly_id = UINT32_MAX;
		freal height = 0;

		bool operator<(const PolyHeight &p_o) const {
			return height < p_o.height;
		}
	};

	Vector<PolyHeight> list;
	list.resize(ex.num_ceiling_links);

	// Fill the list
	Vector<IPoint2> fpoints;
	Vector<FPoint3> fpoints3;
	_get_poly_points(r_mesh, p_floor_poly_id, false, fpoints, fpoints3);

	for (u32 n = 0; n < ex.num_ceiling_links; n++) {
		PolyHeight &ph = list[n];
		ph.poly_id = r_mesh._ceiling_links[ex.first_ceiling_link + n];
		ph.height = FLT_MAX;

		// We want the lowest height of the ceiling poly for sorting.
		for (u32 i = 0; i < fpoints.size(); i++) {
			// Calculate height on vertex of floor at the ceiling poly plane
			freal height = r_mesh.find_height_on_poly_plane(ph.poly_id, fpoints[i], true);
			ph.height = MIN(ph.height, height);
		}
	}

	// Sort
	list.sort();

	// Resave.
	for (u32 n = 0; n < ex.num_ceiling_links; n++) {
		const PolyHeight &ph = list[n];
		r_mesh._ceiling_links[ex.first_ceiling_link + n] = ph.poly_id;
	}
}

void Loader::find_floor_ceiling_links(Mesh &r_mesh) {
	r_mesh._ceiling_links.clear();

	Vector<PolyFinder::CellResult> results;

	for (u32 n = 0; n < r_mesh.floor.get_num_polys(); n++) {
		PolyExtra &ex = r_mesh.get_poly_extra(n);
		ex.first_ceiling_link = r_mesh._ceiling_links.size();
		ex.num_ceiling_links = 0;

		// Find the ceiling links for this poly.
		const IRect2 &rect = r_mesh.floor.poly_bounds[n];

		r_mesh.ceiling.poly_finder.find_cells(rect, results);

		for (u32 r = 0; r < results.size(); r++) {
			const PolyFinder::CellResult &cr = results[r];

			for (u32 i = 0; i < cr.num_polys; i++) {
				u32 ceiling_poly_id = cr.poly_ids[i];

				if (!does_floor_and_ceiling_poly_collide(r_mesh, n, ceiling_poly_id)) {
					continue;
				}

				// Does this link already exist from this poly?
				// Prevent duplicates.
				bool exists = false;

				for (u32 t = 0; t < ex.num_ceiling_links; t++) {
					if (r_mesh._ceiling_links[ex.first_ceiling_link + t] == ceiling_poly_id) {
						exists = true;
						break;
					}
				}

				if (!exists) {
					r_mesh._ceiling_links.push_back(ceiling_poly_id);
					ex.num_ceiling_links += 1;

					//log(String("\tfloor poly ") + n + " links to ceiling poly " + ceiling_poly_id);
				}
			}
		} // for r

		sort_floor_ceiling_links(r_mesh, n);
	} // for floor poly.
}

void Loader::find_extended_aabb(Mesh &r_mesh) {
	AABB &aabb = r_mesh._extended_aabb;
	aabb.zero();

	u32 num_verts = r_mesh.get_num_verts();

	if (!num_verts)
		return;

	aabb.position = r_mesh.get_fvert3(0);

	for (u32 n = 1; n < num_verts; n++) {
		const FPoint3 &vert = r_mesh.get_fvert3(n);
		aabb.expand_to(vert);
	}
}

void Loader::_calculate_extension_params(Mesh &r_mesh) {
	const AABB &aabb = r_mesh._aabb;
	freal aabb_max_size = MAX(aabb.size.x, aabb.size.z);

	// Scale the agent size in fixed point, this is for use later
	// in extending planks.
	if (aabb_max_size > 0) {
		float agent_radius = (r_mesh.mesh_params.agent_radius / aabb_max_size) * FPoint2::FP_RANGE;
		r_mesh.extension_data.agent_radius = MAX(0, agent_radius);

		float agent_lip = (r_mesh.mesh_params.exit_lip / aabb_max_size) * FPoint2::FP_RANGE;
		r_mesh.extension_data.agent_lip = MAX(0, agent_lip);
	} else {
		log("WARNING: NavMesh AABB dimension is zero, agent radius cannot be calculated.");
	}
}

void Loader::bake_fixed_point_verts(Mesh &r_dest) {
	//const Vector<FPoint2> &sverts = r_dest._fverts;

	// first find the offset and scale
	u32 num_verts = r_dest.floor.fverts3.size();

	if (!num_verts) {
		return;
	}

	AABB aabb;
	aabb.position = r_dest.get_fvert3(0);

	//	rect.position = r_dest.get_fvert(0);

	for (u32 i = 1; i < num_verts; i++) {
		//rect.expand_to(r_dest.get_fvert(i));
		aabb.expand_to(r_dest.get_fvert3(i));
	}

	//	NP_LLOG(String("Internal Map Rect is ") + String(rect.position) + ", " + String(rect.size));
	NP_LLOG(String("Internal Map AABB is ") + String(aabb.position) + ", " + String(aabb.size));

	freal aabb_max_size = MAX(aabb.size.x, aabb.size.z);

	r_dest._f32_to_fp_scale = FPoint2::FP_RANGE / aabb_max_size;
	r_dest._f32_to_fp_offset = -FPoint2::make(aabb.position.x, aabb.position.z);

	r_dest._fp_to_f32_offset = -r_dest._f32_to_fp_offset;
	r_dest._fp_to_f32_scale = aabb_max_size / FPoint2::FP_RANGE;

	NP_LLOG(String("_f32_to_fp_scale ") + r_dest._f32_to_fp_scale);
	NP_LLOG(String("_f32_to_fp_offset ") + r_dest._f32_to_fp_offset);

	NP_LLOG(String("_fp_to_f32_offset ") + r_dest._fp_to_f32_offset);
	NP_LLOG(String("_fp_to_f32_scale ") + r_dest._fp_to_f32_scale);

	r_dest._aabb = aabb;

	//_calculate_extension_params(r_dest);

	r_dest.floor.verts.resize(num_verts);

	for (u32 i = 0; i < num_verts; i++) {
		FPoint2 v = r_dest.get_fvert(i);
		IPoint2 vfp = r_dest.float_to_fixed_point_2(v);
		// print("vert " + str(i) + " : " + vfp.sz())
		//r_dest._verts.push_back(vfp);
		r_dest.floor.verts[i] = vfp;

		// var verify = _dmap._fpvec2_to_float(vfp)
		// var l = (verify- v).length()
		// print("length " + str(l))
	}

	// Ceiling verts.
	r_dest.ceiling.verts.resize(r_dest.ceiling.fverts3.size());
	for (u32 i = 0; i < r_dest.ceiling.fverts3.size(); i++) {
		const FPoint3 &pt = r_dest.ceiling.fverts3[i];
		FPoint2 v(pt.x, pt.z);
		IPoint2 vfp = r_dest.float_to_fixed_point_2(v);
		r_dest.ceiling.verts[i] = vfp;
	}

	// find the poly fixed point centers
	TVector<Poly> &dpolys = r_dest.floor.polys;

	for (u32 n = 0; n < dpolys.size(); n++) {
		Poly &dpoly = dpolys[n];
		dpoly.center = r_dest.float_to_fixed_point_2(FPoint2::make(dpoly.center3.x, dpoly.center3.z));
	}

	// Print verts
	NP_LLOG(r_dest.verts_to_string());
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
	TVector<uint32_t> &links = r_dest._links;
	links.resize(r_dest.get_num_inds());
	//links.fill(UINT32_MAX);
	links.fill(Mesh::LINK_FLAG_HARD);
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
	NP_LLOG("Links:");
	for (u32 n = 0; n < links.size(); n++) {
		NP_LLOG(String("\t") + links[n]);
	}

	// Mark links that are connections
	for (u32 n = 0; n < r_dest.data.external_wall_ids.size(); n++) {
		u32 wall_id = r_dest.data.external_wall_ids[n];
		NP_DEV_ASSERT(links.size() > wall_id);

		// This should not trigger in extended meshes, only
		// in meshes before extension.
		if (links[wall_id] == Mesh::LINK_FLAG_HARD) {
			links[wall_id] = Mesh::LINK_FLAG_EXTERNAL | Mesh::LINK_FLAG_HARD;
		}
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
			wall.verts_swapped = true;
		}
	}
	// Now assign a poly to each wall
	for (u32 p = 0; p < r_dest.get_num_polys(); p++) {
		const Poly &poly = r_dest.get_poly(p);
		NP_ERR_CONTINUE(poly.num_inds < 3);
		for (u32 i = 0; i < poly.num_inds; i++) {
			u32 wall_id = poly.first_ind + i;
#if 0
			if (!r_dest.is_hard_wall(wall_id)) {
				continue;
			}
#endif
			r_dest._walls[wall_id].poly_id = p;
		}
	}
	// Now find previous and next walls
	for (u32 wa = 0; wa < num_walls; wa++) {
		if (!r_dest.is_link_hard(wa)) {
			continue;
		}
		const Wall &wall_a = r_dest.get_wall(wa);
		for (u32 wb = wa + 1; wb < num_walls; wb++) {
			if (!r_dest.is_link_hard(wb)) {
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
	NP_LLOG("Walls:");
	for (u32 n = 0; n < r_dest._walls.size(); n++) {
#ifdef NP_LLOG_ACTIVE
		const Wall &wall = r_dest._walls[n];
#endif
		NP_LLOG(String("\twall ") + n);
		NP_LLOG(String("\t\tprev_wall ") + wall.prev_wall);
		NP_LLOG(String("\t\tnext_wall ") + wall.next_wall);
		NP_LLOG(String("\t\tnormal ") + wall.normal);
		NP_LLOG(String("\t\tpoly_id ") + wall.poly_id);
		NP_LLOG(String("\t\tvert_a ") + wall.vert_a);
		NP_LLOG(String("\t\tvert_b ") + wall.vert_b);
		NP_LLOG(String("\t\twall_vec ") + wall.wall_vec);
	}

	// Find external and internal walls
	r_dest.data.external_wall_ids_final.clear();
	_finalize_wall_pairs(r_dest, r_dest.data.external_wall_pairs, Mesh::LINK_FLAG_EXTERNAL);
	_finalize_wall_pairs(r_dest, r_dest.data.internal_wall_pairs, Mesh::LINK_FLAG_INTERNAL);

	//	if (r_dest.data.external_wall_pairs.size()) {
	//		for (uint32_t w = 0; w < r_dest.get_num_links(); w++) {
	//			u32 &link = r_dest._links[w];

	//			// Only test hard walls, as these are the only ones
	//			// that can be external.
	//			if (r_dest.is_link_hard(w)) {
	//				const Wall &wall = r_dest.get_wall(w);
	//				NP_LLOG(String("\tassessing wall ") + w + " from " + wall.vert_a + ", " + wall.vert_b + " poly " + wall.poly_id);

	//				// Go through all pairs, find any that match this wall
	//				for (u32 e = 0; e < r_dest.data.external_wall_pairs.size(); e++) {
	//					const Mesh::WallPair &pair = r_dest.data.external_wall_pairs[e];
	//					if (wall.has_vert(pair.vert_ids[0]) && wall.has_vert(pair.vert_ids[1])) {
	//						// Mark
	//						//if (link == UINT32_MAX) {
	//						if (link == Mesh::LINK_FLAG_HARD) {
	//							link = Mesh::LINK_FLAG_EXTERNAL | Mesh::LINK_FLAG_HARD;
	//							NP_LLOG(String("Found external wall inds ") + pair.vert_ids[0] + ", " + pair.vert_ids[1] + " : wall ID " + w);
	//						}
	//					}
	//				}
	//			}
	//		}
	//	}
}

void Loader::_finalize_wall_pairs(Mesh &r_mesh, const TVector<Mesh::WallPair> &p_wall_pairs, u32 p_wall_flags) {
	if (p_wall_pairs.size()) {
		for (uint32_t w = 0; w < r_mesh.get_num_links(); w++) {
			u32 &link = r_mesh._links[w];

			// Only test hard walls, as these are the only ones
			// that can be external.
			if (r_mesh.is_link_hard(w)) {
				const Wall &wall = r_mesh.get_wall(w);
				NP_LLOG(String("\tassessing wall ") + w + " from " + wall.vert_a + ", " + wall.vert_b + " poly " + wall.poly_id);

				// Go through all pairs, find any that match this wall
				for (u32 e = 0; e < p_wall_pairs.size(); e++) {
					const Mesh::WallPair &pair = p_wall_pairs[e];
					if (wall.has_vert(pair.vert_ids[0]) && wall.has_vert(pair.vert_ids[1])) {
						// Mark
						link |= p_wall_flags;
						NP_LLOG(String("Found wall inds ") + pair.vert_ids[0] + ", " + pair.vert_ids[1] + " : wall ID " + w);
						// External walls are used at runtime for finding jump links between meshes.
						if (p_wall_flags == Mesh::LINK_FLAG_EXTERNAL) {
							r_mesh.data.external_wall_ids_final.push_back(w);
						}
					}
				}
			}
		}
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

bool Loader::extract_working_data(WorkingMeshData &r_data, const Mesh &p_mesh) {
	WorkingMeshData &d = r_data;

	for (uint32_t n = 0; n < 2; n++) {
		bool is_floor = n == 0;

		WorkingMeshData::SubMesh *dsm = is_floor ? &d.floor : &d.ceiling;
		const Mesh::SubMesh *ssm = is_floor ? &p_mesh.floor : &p_mesh.ceiling;
		TVector<u32> *dest_poly_num_indices = is_floor ? &data.poly_num_indices : &data.ceil_poly_num_indices;

		u32 num_polys = ssm->polys.size();

		if (is_floor) {
			//			dsm = &d.floor;
			//			ssm = &p_mesh.floor;
			//			dest_poly_num_indices = &data.poly_num_indices;

			data.poly_types.resize(num_polys);
			data.poly_types.fill(0);

			for (u32 p = 0; p < num_polys; p++) {
				data.poly_types[p] = p_mesh.get_poly_extra(p).is_area() ? 0 : 1;
			}

			dsm->poly_type = data.poly_types.size() ? data.poly_types.ptr() : nullptr;
		}

		dsm->num_verts = ssm->fverts3.size();
		dsm->verts = ssm->fverts3.ptr();
		dsm->iverts = ssm->verts.ptr();

		dsm->num_indices = ssm->inds.size();
		dsm->indices = ssm->inds.ptr();

		dsm->num_polys = num_polys;
		dest_poly_num_indices->resize(num_polys);

		for (u32 n = 0; n < dsm->num_polys; n++) {
			(*dest_poly_num_indices)[n] = ssm->polys[n].num_inds;
		}

		dsm->poly_num_indices = dest_poly_num_indices->ptr();
	}

	d.agent_radius = p_mesh.extension_data.agent_radius;

	/*
	d.num_verts = p_mesh.get_num_verts();
	d.verts = p_mesh.floor.fverts3.ptr();
	d.iverts = p_mesh.floor.verts.ptr();

	d.num_indices = p_mesh.get_num_inds();
	d.indices = p_mesh.floor.inds.ptr();

	d.num_polys = p_mesh.floor.polys.size();
	data.poly_num_indices.resize(d.num_polys);
	for (u32 n = 0; n < d.num_polys; n++) {
		u32 num_inds = p_mesh.get_poly(n).num_inds;
		data.poly_num_indices[n] = num_inds;
	}
	d.poly_num_indices = data.poly_num_indices.ptr();

	// Ceiling
	const Mesh::SubMesh &c = p_mesh.ceiling;

	d.ceil_num_verts = c.fverts3.size();
	d.ceil_verts = c.fverts3.ptr();
	d.ceil_iverts = c.verts.ptr();

	d.ceil_num_indices = c.inds.size();
	d.ceil_indices = c.inds.ptr();

	d.ceil_num_polys = c.polys.size();
	data.ceil_poly_num_indices.resize(d.ceil_num_polys);
	for (u32 n = 0; n < d.ceil_num_polys; n++) {
		data.ceil_poly_num_indices[n] = c.polys[n].num_inds;
	}
	d.ceil_poly_num_indices = data.ceil_poly_num_indices.ptr();
	*/

	// External connections...
	d.num_external_connecting_walls = p_mesh.data.external_wall_ids.size();
	data.external_connecting_wall_indices.resize(d.num_external_connecting_walls * 2);
	data.external_connecting_wall_ids.resize(d.num_external_connecting_walls);

	for (u32 n = 0; n < d.num_external_connecting_walls; n++) {
		u32 wall_id = p_mesh.data.external_wall_ids[n];
		const Wall &wall = p_mesh.get_wall(wall_id);
		data.external_connecting_wall_indices[n * 2] = wall.vert_a;
		data.external_connecting_wall_indices[(n * 2) + 1] = wall.vert_b;

		data.external_connecting_wall_ids[n] = wall_id;
	}

	// Internal connections...
	d.num_internal_connecting_walls = p_mesh.data.internal_wall_ids.size();
	data.internal_connecting_wall_indices.resize(d.num_internal_connecting_walls * 2);
	data.internal_connecting_wall_ids.resize(d.num_internal_connecting_walls);

	for (u32 n = 0; n < d.num_internal_connecting_walls; n++) {
		u32 wall_id = p_mesh.data.internal_wall_ids[n];
		const Wall &wall = p_mesh.get_wall(wall_id);
		data.internal_connecting_wall_indices[n * 2] = wall.vert_a;
		data.internal_connecting_wall_indices[(n * 2) + 1] = wall.vert_b;

		data.internal_connecting_wall_ids[n] = wall_id;
	}

	d.external_connecting_wall_indices = d.num_external_connecting_walls ? data.external_connecting_wall_indices.ptr() : nullptr;
	d.external_connecting_wall_ids = d.num_external_connecting_walls ? data.external_connecting_wall_ids.ptr() : nullptr;

	d.internal_connecting_wall_indices = d.num_internal_connecting_walls ? data.internal_connecting_wall_indices.ptr() : nullptr;
	d.internal_connecting_wall_ids = d.num_internal_connecting_walls ? data.internal_connecting_wall_ids.ptr() : nullptr;

	d.fixed_point_to_float_offset = p_mesh._fp_to_f32_offset;
	d.fixed_point_to_float_scale = p_mesh._fp_to_f32_scale;
	d.float_to_fixed_point_offset = p_mesh._f32_to_fp_offset;
	d.float_to_fixed_point_scale = p_mesh._f32_to_fp_scale;
	d.aabb = p_mesh._aabb;

	return true;
}

bool Loader::load_raw_data(const uint8_t *p_data, uint32_t p_num_bytes, Mesh &r_mesh, const Mesh::MeshParams &p_params) {
	// FOURCC?

	// Version
	u32 version;
	if (!RawLoader::read_u32(&p_data, p_num_bytes, version)) {
		return false;
	}

	if (version != NAVPHYSICS_FILE_VERSION) {
		log("NavMesh data version incorrect.");
		return false;
	}
	log_load("Version correct.");

	// We need to know the lip size in fixed point in order to do extending.
	// Extended lips are NOT saved with the mesh, so is done at runtime.
	r_mesh.mesh_params = p_params;
	//	r_mesh.extension_data.agent_radius_f32 = p_params.agent_radius;
	//	r_mesh.extension_data.agent_lip_f32 = p_params.exit_lip;

	WorkingMeshData md;
	if (!RawLoader::read_u32(&p_data, p_num_bytes, md.floor.num_verts))
		return false;

	if (!RawLoader::read_u32(&p_data, p_num_bytes, md.floor.num_indices))
		return false;

	if (!RawLoader::read_u32(&p_data, p_num_bytes, md.floor.num_polys))
		return false;

	if (!RawLoader::read_u32(&p_data, p_num_bytes, md.ceiling.num_verts))
		return false;

	if (!RawLoader::read_u32(&p_data, p_num_bytes, md.ceiling.num_indices))
		return false;

	if (!RawLoader::read_u32(&p_data, p_num_bytes, md.ceiling.num_polys))
		return false;

	if (!RawLoader::read_u32(&p_data, p_num_bytes, md.num_internal_connecting_walls))
		return false;

	if (!RawLoader::read_u32(&p_data, p_num_bytes, md.num_external_connecting_walls))
		return false;

	log_load("Numbers read OK.");

	if (!md.floor.num_verts)
		return false;
	if (!md.floor.num_indices)
		return false;
	if (!md.floor.num_polys)
		return false;

	if (!RawLoader::read_fpoint2(&p_data, p_num_bytes, md.fixed_point_to_float_offset)) {
		log("Failed to read fixed_point_to_float_offset.");
		return false;
	}
	if (!RawLoader::read_f32(&p_data, p_num_bytes, md.fixed_point_to_float_scale)) {
		log("Failed to read fixed_point_to_float_scale.");
		return false;
	}
	if (!RawLoader::read_fpoint2(&p_data, p_num_bytes, md.float_to_fixed_point_offset)) {
		log("Failed to read float_to_fixed_point_offset.");
		return false;
	}
	if (!RawLoader::read_f32(&p_data, p_num_bytes, md.float_to_fixed_point_scale)) {
		log("Failed to read float_to_fixed_point_scale.");
		return false;
	}
	log_load("Offsets and scales read OK.");

	if (!RawLoader::read_fpoint3(&p_data, p_num_bytes, md.aabb.position)) {
		log("Failed to read aabb.position.");
		return false;
	}
	if (!RawLoader::read_fpoint3(&p_data, p_num_bytes, md.aabb.size)) {
		log("Failed to read aabb.size.");
		return false;
	}
	log_load("AABB read OK.");
	if (!RawLoader::read_u32(&p_data, p_num_bytes, md.agent_radius)) {
		log("Failed to read agent_radius.");
		return false;
	}

	TVector<FPoint3> verts;
	verts.resize(md.floor.num_verts);

	TVector<IPoint2> iverts;
	iverts.resize(md.floor.num_verts);

	TVector<u32> inds;
	inds.resize(md.floor.num_indices);

	TVector<u32> poly_num_inds;
	poly_num_inds.resize(md.floor.num_polys);

	TVector<FPoint3> ceil_verts;
	ceil_verts.resize(md.ceiling.num_verts);

	TVector<IPoint2> ceil_iverts;
	ceil_iverts.resize(md.ceiling.num_verts);

	TVector<u32> ceil_inds;
	ceil_inds.resize(md.ceiling.num_indices);

	TVector<u32> ceil_poly_num_inds;
	ceil_poly_num_inds.resize(md.ceiling.num_polys);

	TVector<u32> internal_connecting_wall_ids;
	internal_connecting_wall_ids.resize(md.num_internal_connecting_walls);

	TVector<u32> external_connecting_wall_ids;
	external_connecting_wall_ids.resize(md.num_external_connecting_walls);

	for (u32 n = 0; n < md.floor.num_verts; n++) {
		if (!RawLoader::read_fpoint3(&p_data, p_num_bytes, verts[n])) {
			log("Failed to read vert.");
			return false;
		}
	}
	log_load("Verts read OK.");
	for (u32 n = 0; n < md.floor.num_verts; n++) {
		if (!RawLoader::read_ipoint2(&p_data, p_num_bytes, iverts[n])) {
			log("Failed to read ivert.");
			return false;
		}
	}
	log_load("IVerts read OK.");
	for (u32 n = 0; n < md.floor.num_indices; n++) {
		if (!RawLoader::read_u32(&p_data, p_num_bytes, inds[n])) {
			log("Failed to read index.");
			return false;
		}
	}
	log_load("Indices read OK.");
	for (u32 n = 0; n < md.floor.num_polys; n++) {
		if (!RawLoader::read_u32(&p_data, p_num_bytes, poly_num_inds[n])) {
			log("Failed to read poly.");
			return false;
		}
	}
	log_load("Polys read OK.");

	for (u32 n = 0; n < md.ceiling.num_verts; n++) {
		if (!RawLoader::read_fpoint3(&p_data, p_num_bytes, ceil_verts[n])) {
			log("Failed to read ceil vert.");
			return false;
		}
	}
	log_load("Ceil Verts read OK.");
	for (u32 n = 0; n < md.ceiling.num_verts; n++) {
		if (!RawLoader::read_ipoint2(&p_data, p_num_bytes, ceil_iverts[n])) {
			log("Failed to read ceil ivert.");
			return false;
		}
	}
	log_load("Ceil IVerts read OK.");
	for (u32 n = 0; n < md.ceiling.num_indices; n++) {
		if (!RawLoader::read_u32(&p_data, p_num_bytes, ceil_inds[n])) {
			log("Failed to read ceil index.");
			return false;
		}
	}
	log_load("Ceil Indices read OK.");
	for (u32 n = 0; n < md.ceiling.num_polys; n++) {
		if (!RawLoader::read_u32(&p_data, p_num_bytes, ceil_poly_num_inds[n])) {
			log("Failed to read ceil poly.");
			return false;
		}
	}
	log_load("Ceil Polys read OK.");

	for (u32 n = 0; n < md.num_internal_connecting_walls; n++) {
		if (!RawLoader::read_u32(&p_data, p_num_bytes, internal_connecting_wall_ids[n])) {
			log("Failed to read internal connecting wall id.");
			return false;
		}
	}
	log_load("Internal connecting walls read OK.");
	for (u32 n = 0; n < md.num_external_connecting_walls; n++) {
		if (!RawLoader::read_u32(&p_data, p_num_bytes, external_connecting_wall_ids[n])) {
			log("Failed to read external connecting wall id.");
			return false;
		}
	}
	log_load("External connecting walls read OK.");

	md.floor.verts = verts.ptr();
	md.floor.iverts = iverts.ptr();
	md.floor.indices = inds.ptr();
	md.floor.poly_num_indices = poly_num_inds.ptr();

	md.ceiling.verts = ceil_verts.ptr();
	md.ceiling.iverts = ceil_iverts.ptr();
	md.ceiling.indices = ceil_inds.ptr();
	md.ceiling.poly_num_indices = ceil_poly_num_inds.ptr();

	md.external_connecting_wall_ids = external_connecting_wall_ids.ptr();
	md.internal_connecting_wall_ids = internal_connecting_wall_ids.ptr();

	return load_working_data(md, r_mesh);
}

void Loader::prepare_raw_data_submesh(const WorkingMeshData::SubMesh &p_submesh) {
	for (u32 n = 0; n < p_submesh.num_verts; n++) {
		RawLoader::write_fpoint3(_save_data, p_submesh.verts[n]);
	}
	for (u32 n = 0; n < p_submesh.num_verts; n++) {
		RawLoader::write_ipoint2(_save_data, p_submesh.iverts[n]);
	}
	for (u32 n = 0; n < p_submesh.num_indices; n++) {
		RawLoader::write_u32(_save_data, p_submesh.indices[n]);
	}
	for (u32 n = 0; n < p_submesh.num_polys; n++) {
		RawLoader::write_u32(_save_data, p_submesh.poly_num_indices[n]);
	}
}

uint32_t Loader::prepare_raw_data(Mesh &r_mesh) {
	_save_data.clear();

	// Unextend the data
	unextend_mesh(r_mesh);

	// FOURCC?

	// Version
	RawLoader::write_u32(_save_data, NAVPHYSICS_FILE_VERSION);

	WorkingMeshData md;
	extract_working_data(md, r_mesh);

	RawLoader::write_u32(_save_data, md.floor.num_verts);
	RawLoader::write_u32(_save_data, md.floor.num_indices);
	RawLoader::write_u32(_save_data, md.floor.num_polys);

	RawLoader::write_u32(_save_data, md.ceiling.num_verts);
	RawLoader::write_u32(_save_data, md.ceiling.num_indices);
	RawLoader::write_u32(_save_data, md.ceiling.num_polys);

	RawLoader::write_u32(_save_data, md.num_internal_connecting_walls);
	RawLoader::write_u32(_save_data, md.num_external_connecting_walls);

	RawLoader::write_fpoint2(_save_data, md.fixed_point_to_float_offset);
	RawLoader::write_f32(_save_data, md.fixed_point_to_float_scale);
	RawLoader::write_fpoint2(_save_data, md.float_to_fixed_point_offset);
	RawLoader::write_f32(_save_data, md.float_to_fixed_point_scale);

	RawLoader::write_fpoint3(_save_data, md.aabb.position);
	RawLoader::write_fpoint3(_save_data, md.aabb.size);

	RawLoader::write_u32(_save_data, md.agent_radius);

	prepare_raw_data_submesh(md.floor);
	prepare_raw_data_submesh(md.ceiling);

	for (u32 n = 0; n < md.num_internal_connecting_walls; n++) {
		RawLoader::write_u32(_save_data, md.internal_connecting_wall_ids[n]);
	}
	for (u32 n = 0; n < md.num_external_connecting_walls; n++) {
		RawLoader::write_u32(_save_data, md.external_connecting_wall_ids[n]);
	}

	log(String("save_data size ") + _save_data.size() + " bytes.");

	// Re-extend after saving
	extend_mesh(r_mesh);

	return _save_data.size();
}

bool Loader::save_raw_data(uint8_t *r_data, uint32_t p_num_bytes) {
	if (p_num_bytes != _save_data.size())
		return false;

	memcpy(r_data, _save_data.ptr(), p_num_bytes);
	return true;
}

bool Loader::load_working_data_submesh(const WorkingMeshData::SubMesh &p_data, Mesh::SubMesh &r_submesh) {
	// Verts
	r_submesh.fverts3.resize(p_data.num_verts);
	r_submesh.verts.resize(p_data.num_verts);

	for (u32 n = 0; n < p_data.num_verts; n++) {
		r_submesh.fverts3[n] = p_data.verts[n];
		r_submesh.verts[n] = p_data.iverts[n];
	}

	// Inds
	r_submesh.inds.resize(p_data.num_indices);

	for (u32 n = 0; n < p_data.num_indices; n++) {
		r_submesh.inds[n] = p_data.indices[n];
	}

	// Polys
	r_submesh.polys.resize(p_data.num_polys);
	return true;
}

bool Loader::load_working_data(const WorkingMeshData &p_data, Mesh &r_mesh) {
	r_mesh.clear();

	if (!p_data.floor.verts)
		return false;
	if (!p_data.floor.indices)
		return false;
	if (!p_data.floor.poly_num_indices)
		return false;
	if (!p_data.floor.num_verts)
		return false;
	if (!p_data.floor.num_indices)
		return false;
	if (!p_data.floor.num_polys)
		return false;

	r_mesh.extension_data.agent_radius = p_data.agent_radius;

	// FLOOR
	load_working_data_submesh(p_data.floor, r_mesh.floor);
	r_mesh._polys_extra.resize(p_data.floor.num_polys);
	// CEILING
	load_working_data_submesh(p_data.ceiling, r_mesh.ceiling);

	r_mesh._f32_to_fp_offset = p_data.float_to_fixed_point_offset;
	r_mesh._f32_to_fp_scale = p_data.float_to_fixed_point_scale;
	r_mesh._fp_to_f32_offset = p_data.fixed_point_to_float_offset;
	r_mesh._fp_to_f32_scale = p_data.fixed_point_to_float_scale;

	r_mesh._aabb = p_data.aabb;

	// Connecting walls
	r_mesh.data.internal_wall_ids.resize(p_data.num_internal_connecting_walls);
	for (u32 n = 0; n < p_data.num_internal_connecting_walls; n++) {
		r_mesh.data.internal_wall_ids[n] = p_data.internal_connecting_wall_ids[n];
	}

	r_mesh.data.external_wall_ids.resize(p_data.num_external_connecting_walls);
	r_mesh.data.lipped_wall_ids.resize(p_data.num_external_connecting_walls);
	for (u32 n = 0; n < p_data.num_external_connecting_walls; n++) {
		r_mesh.data.external_wall_ids[n] = p_data.external_connecting_wall_ids[n];
		r_mesh.data.lipped_wall_ids[n] = p_data.external_connecting_wall_ids[n];
	}

	if (!_load_polys(p_data.floor.num_polys, p_data.floor.poly_num_indices, p_data.ceiling.num_polys, p_data.ceiling.poly_num_indices, r_mesh))
		return false;

	// The poly bounds and finder for the ceiling only need to be done once.
	// No need to recalculate during _load() which is called after re-extending the mesh.
	_calculate_poly_bounds(r_mesh.ceiling);
	if (!_editor_only) {
		r_mesh.ceiling.poly_finder.build(r_mesh, true);
	}

	_load(r_mesh);

	extend_mesh(r_mesh);

	if (!_editor_only) {
		_calculate_poly_areas(r_mesh);
		floodfill_islands(r_mesh);
		find_bottlenecks(r_mesh);
	}

	return true;
}

void Loader::set_editor_only(bool p_editor_only) {
	_editor_only = p_editor_only;
}

void Loader::extend_mesh(Mesh &r_mesh) {
	r_mesh._check_for_duplicate_verts();

	_calculate_extension_params(r_mesh);

	MeshExtender ex;
	ex.extend_mesh(r_mesh);
	_load(r_mesh);

	r_mesh._check_for_duplicate_verts();

	if (!_editor_only) {
		r_mesh.floor.poly_finder.build(r_mesh, false);

		JumpFinder finder;
		finder.find_jumps(r_mesh);
	}
}

void Loader::unextend_mesh(Mesh &r_mesh) {
	MeshExtender ex;
	ex.unextend_mesh(r_mesh);
	_load(r_mesh);
}

bool Loader::bake_mesh(const SourceMeshData &p_source_mesh, const SourceMeshData &p_source_ceiling_mesh, Mesh &r_mesh) {
	r_mesh.clear();

	if (!p_source_mesh.verts)
		return false;
	if (!p_source_mesh.indices)
		return false;

	Simplifier simp;
	SourceMeshData smd = simp.simplify(p_source_mesh);

	Simplifier simp2;
	SourceMeshData smd_ceiling = simp2.simplify(p_source_ceiling_mesh);

	if (!bake_load_polys(smd, smd_ceiling, r_mesh)) {
		goto failed;
	}

	bake_fixed_point_verts(r_mesh);

	// The poly bounds and finder for the ceiling only need to be done once.
	// No need to recalculate during _load() which is called after re-extending the mesh.
	_calculate_poly_bounds(r_mesh.ceiling);
	r_mesh.ceiling.poly_finder.build(r_mesh, true);

	_load(r_mesh);

	// Always extend after baking.
	// This will usually be a NOOP,
	// but it stores the unextended size of
	// the arrays, which is necessary for saving correctly.
	extend_mesh(r_mesh);

	return true;

failed:
	r_mesh.clear();
	return false;
}

void Loader::_load(Mesh &r_mesh) {
	_calculate_poly_bounds(r_mesh.floor);

	find_index_nexts(r_mesh);
	find_links(r_mesh);
	find_walls(r_mesh);
	find_extended_aabb(r_mesh);
	find_floor_ceiling_links(r_mesh);

	NP_LLOG("NavPhysics loaded mesh:");
	NP_LLOG(String("\tpolys: ") + r_mesh.get_num_polys());
	NP_LLOG(String("\tinds: ") + r_mesh.get_num_inds());

	NP_LLOG(String("\tlinks: ") + r_mesh.get_num_links());
	NP_LLOG(String("\twalls: ") + r_mesh.get_num_walls());

	NP_LLOG(String("\tverts: ") + r_mesh.get_num_verts());
	for (u32 n = 0; n < r_mesh.get_num_verts(); n++) {
		NP_LLOG(String("\t\tvert ") + n + " : " + r_mesh.get_vert(n));
	}

	// If we were loading from extended mesh, we no longer require storing the external wall pairs.
	r_mesh.data.internal_wall_pairs.clear();
	r_mesh.data.external_wall_pairs.clear();
}

void Loader::replace_poly_narrow_dist(PolyTemp &r_poly, u32 p_dist) {
	//	if (r_poly.narrowing_width && (r_poly.narrowing_width < p_dist))
	//		return;
	//	r_poly.narrowing_width = p_dist;

	if (p_dist < r_poly.narrowing_width) {
		r_poly.narrowing_width = p_dist;
	}
}

u32 Loader::flood_fill_bottleneck(Mesh &r_dest, u32 p_poly_id, u32 p_start_wall_id, u32 p_flood_fill_counter, const freal p_threshold) {
	PolyTemp &ex = _poly_temps[p_poly_id];
	if (ex.flood_fill_counter == (uint16_t)p_flood_fill_counter) {
		// already flood filled
		return ex.narrowing_width;
	}
	if (!ex.could_be_narrowing) {
		return UINT32_MAX;
	}

	// mark poly as done
	ex.flood_fill_counter = p_flood_fill_counter;

	//////

	const Poly &poly = r_dest.get_poly(p_poly_id);

	//bool was_bottleneck = false;

	// Unset
	uint32_t narrowest_dist = UINT32_MAX;

	//uint32_t furthest_wall = UINT32_MAX;
	const Wall &start_wall = r_dest.get_wall(p_start_wall_id);
	//const freal threshold = NAVPHYSICS_MESH_FP_RANGE / 4.0f;
	//const freal threshold = r_dest.extension_data.agent_radius * 2;
	const freal threshold = p_threshold;

	uint32_t ind_a = start_wall.get_swapped_vert_a();
	uint32_t ind_b = start_wall.get_swapped_vert_b();
	const IPoint2 &wa = r_dest.get_vert(ind_a);
	const IPoint2 &wb = r_dest.get_vert(ind_b);

	MeshFuncs funcs;

	// try every edge
	for (u32 i = 0; i < poly.num_inds; i++) {
		u32 wall_id = poly.first_ind + i;
		if (wall_id == p_start_wall_id) {
			continue;
		}

		const Wall &wall = r_dest.get_wall(wall_id);

		// ignore if not opposite to startwall
		freal dot = wall.normal.dot_normalized(start_wall.normal);
		if (dot > -0.5f)
			//if (dot > 0)
			continue;

		u32 ind_c = wall.get_swapped_vert_a();
		u32 ind_d = wall.get_swapped_vert_b();

		const IPoint2 &wc = r_dest.get_vert(ind_c);
		const IPoint2 &wd = r_dest.get_vert(ind_d);
		// get the wall
		uint32_t linked_poly_id = r_dest.get_link(wall_id);

		//		if (linked_poly_id != UINT32_MAX) {
		//			const Poly &next_poly = r_dest.get_poly(linked_poly_id);
		//			if (next_poly.curr_agents == p_flood_fill_counter) {
		//				// done already
		//				continue;
		//			}
		//		}

		freal dist = funcs.get_closest_distance_between_segments(wa, wb, wc, wd);
		if (dist > threshold) {
			continue;
		}

		//log(String("\tto wall ") + ind_c + " to " + ind_b + ", dist is " + dist);

		// ignore adjoining walls
		//		if (dist < 1.0f) {
		//			continue;
		//		}
		if (r_dest.is_link_hard(wall_id)) {
			//			if (ind_c == ind_b)
			//				continue;
			//			if (ind_d == ind_a)
			//				continue;
			//const Wall &wall = r_dest.get_wall(ind);

			// ignore if not opposite to startwall
			if (dot > -0.5f)
				continue;
				// special case, sharp angle within same triangle
#if 0
			if (p_poly_id == start_wall.poly_id) {
				dist = (wd - wa).lengthf();
				if (dist == 0) {
					dist = (wc - wb).lengthf();
				}
				
				NP_DEV_ASSERT(dist <= threshold);
			}
#endif
			replace_poly_narrow_dist(ex, dist);
			// print_line("poly " + itos(p_poly_id) + " wall was bottleneck.");
			return dist;
		} else {
			// recurse
			uint32_t ret = flood_fill_bottleneck(r_dest, linked_poly_id, p_start_wall_id, p_flood_fill_counter, p_threshold);
			//			if (ret) {
			//				if (!narrowest_dist || (ret < narrowest_dist)) {
			//					narrowest_dist = ret;
			//				}
			if (ret != UINT32_MAX) {
				replace_poly_narrow_dist(ex, ret);
			}
			// print_line("poly " + itos(p_poly_id) + " was bottleneck.");
			//			}
		}
	}
	return narrowest_dist;
}

bool Loader::flood_fill_narrowing(Mesh &r_dest, u32 p_poly_id, u32 p_narrowing_id, u32 p_narrowing_width) {
	//return true;
	const Poly &poly = r_dest.get_poly(p_poly_id);
	PolyTemp &ex = _poly_temps[p_poly_id];

	if (ex.narrowing_width == UINT32_MAX) {
		return false;
	}

	//	if (ex.narrowing_width != p_narrowing_width) {
	//		return false;
	//	}
	// prevent being hit again
	ex.narrowing_width = UINT32_MAX;

	r_dest.get_poly_extra(p_poly_id).set_narrowing_id(p_narrowing_id);

	// recurse
	for (u32 i = 0; i < poly.num_inds; i++) {
		u32 ind = poly.first_ind + i;

		if (!r_dest.is_link_hard(ind)) {
			flood_fill_narrowing(r_dest, r_dest.get_link(ind), p_narrowing_id, p_narrowing_width);
		}
	}
	return true;
}

// TODO
// This routine is not reliable yet as it sees around corners.
bool Loader::does_narrow_poly_have_neighbour(Mesh &r_dest, u32 p_wid, const Poly &p_poly, freal p_threshold) const {
	return false;

	for (u32 w = 0; w < p_poly.num_inds; w++) {
		u32 wid = p_poly.first_ind + w;

		if (wid == p_wid)
			continue;

		if (r_dest.is_link_hard(wid))
			continue;

		u32 neigh_poly_id = r_dest.get_link(wid);

		const Poly &npoly = r_dest.get_poly(neigh_poly_id);

		// Each edge must have at least one vertex out from it, so as not to be a narrowing.
		for (u32 v = 0; v < npoly.num_inds; v++) {
			u32 ind = r_dest.get_ind(npoly.first_ind + v);
			freal dist = r_dest.get_distance_from_wall_edge(p_wid, r_dest.get_vert(ind));
			//log(String("\t\tdist ") + dist);
			if (dist >= p_threshold) {
				return true;
			}
		}
	}

	return false;
}

void Loader::find_bottlenecks(Mesh &r_dest) {
	_poly_temps.resize(r_dest.get_num_polys());

	// If all the edges of a poly have another point more than threshold away from it,
	// it can't be a narrowing.
	//#define NP_DISABLE_NARROWINGS
	const freal threshold = r_dest.extension_data.agent_radius * 4; // 2

	for (u32 n = 0; n < r_dest.get_num_polys(); n++) {
		const Poly &poly = r_dest.get_poly(n);

		//if (poly.num_inds != 4)
		//	continue;

		//log(String("assessing poly ") + n);
		bool any_edges_smaller = false;

		for (u32 w = 0; w < poly.num_inds; w++) {
			u32 wid = poly.first_ind + w;
			bool edge_ok = false;
			//log(String("\tedge wall ") + wid);

			// Each edge must have at least one vertex out from it, so as not to be a narrowing.
			for (u32 v = 0; v < poly.num_inds; v++) {
				u32 ind = r_dest.get_ind(poly.first_ind + v);
				freal dist = r_dest.get_distance_from_wall_edge(wid, r_dest.get_vert(ind));
				//log(String("\t\tdist ") + dist);
				if (dist >= threshold) {
					edge_ok = true;
					break;
				}
			}

			// If no points of this poly were okay, check a potential neighbouring poly
			if (!edge_ok) {
				if (does_narrow_poly_have_neighbour(r_dest, wid, poly, threshold)) {
					edge_ok = true;
				}
			}

			if (!edge_ok) {
				any_edges_smaller = true;
				break;
			}
		}

		if (!any_edges_smaller) {
			// This poly CAN'T BE A NARROWING.
			//log(String("Poly ") + n + " can't be a narrowing.");
			_poly_temps[n].could_be_narrowing = false;
		} else {
			// log(String("Poly ") + n + " could be a narrowing.");
		}
#ifdef NP_DISABLE_NARROWINGS
		_poly_temps[n].could_be_narrowing = false;
#endif
	}

	// Just for debugging mark the could be narrowings.
#if 0
	for (u32 n = 0; n < r_dest.get_num_polys(); n++) {
		if (_poly_temps[n].could_be_narrowing == false)
			continue;

		r_dest.get_poly_extra(n).set_narrowing_id(n);
		//const Poly &poly = r_dest.get_poly(n);
	}
	_poly_temps.clear();
	r_dest.svg_export(String("../test_narrowings_") + r_dest.get_num_polys() + ".svg");
	return;
#endif

	u32 flood_fill_counter = 0;
	// go through and find each hard wall, and see if this wall creates a bottleneck
	for (u32 w = 0; w < r_dest.get_num_links(); w++) {
		if (r_dest.is_link_hard(w)) {
			const Wall &wall = r_dest.get_wall(w);
			if ((wall.poly_id == UINT32_MAX) || !_poly_temps[wall.poly_id].could_be_narrowing) {
				continue;
			}

			//log(String("Flood fill from wall ") + wall.get_swapped_vert_a() + " to " + wall.get_swapped_vert_b());

			u32 poly_id = wall.poly_id;
			if (poly_id != UINT32_MAX) {
				flood_fill_counter++;
				flood_fill_bottleneck(r_dest, poly_id, w, flood_fill_counter, threshold);
			}
		}
	}
	for (u32 n = 0; n < r_dest.get_num_polys(); n++) {
		PolyTemp &poly = _poly_temps[n];
		poly.flood_fill_counter = 0;
		// print_line("c++ poly " + itos(n) + " dist " + itos(poly.narrowing_width));
		// translate narrowing width to num agents
		//poly.narrowing_width = MAX(poly.narrowing_width / 2500, 1);
		//poly.narrowing_width = MAX(poly.narrowing_width / 1200, 1);
	}

	//r_dest.svg_export("../test_narrowing_width.svg");

	for (u32 n = 0; n < r_dest.get_num_polys(); n++) {
		const PolyTemp &poly = _poly_temps[n];
		if (poly.narrowing_width < UINT32_MAX) {
			// create a narrowing
			u32 narrowing_id = r_dest._narrowings.size();
			r_dest._narrowings.resize(narrowing_id + 1);
			//log(String("Creating narrowing ") + narrowing_id);

			// record the width for the narrowing

			// Abandoned storing the width on the narrowing, just know it's not enough
			// for 2 agents to pass.
			// r_dest._narrowings[narrowing_id].available = poly.narrowing_width;
			flood_fill_narrowing(r_dest, n, narrowing_id, poly.narrowing_width);
		}

		//log(String("Poly ") + n + " narrowing id " + poly.narrowing_id + ", width " + poly.narrowing_width);
	}

	// Areas
	u32 area_id = 0;
	NP_DEV_ASSERT(r_dest._areas.is_empty());

	for (u32 n = 0; n < r_dest.get_num_polys(); n++) {
		if (r_dest.get_poly_extra(n).is_area_unset()) {
			r_dest._areas.request();
			floodfill_area(r_dest, n, area_id++);
		}
	}

	/////////////////////////////////////////////////////////////////////////
	find_zone_links(r_dest);
	_poly_temps.clear();
	//r_dest.svg_export(String("../test_narrowings_") + r_dest.get_num_polys() + ".svg");

	// Find zone centres (for pathfinding zones).
	Vector<u32> zone_poly_counts;
	zone_poly_counts.resize(r_dest._zones.size());
	zone_poly_counts.fill(0);

	for (u32 n = 0; n < r_dest.get_num_polys(); n++) {
		PolyExtra &ex = r_dest.get_poly_extra(n);
		const Poly &poly = r_dest.get_poly(n);
		//ex.zone_id = !ex.is_narrowing() ? ex.get_area_id() : ex.get_narrowing_id() + start_narrowings;
		Zone &zone = r_dest._zones[ex.zone_id];
		zone.local_pos3 += poly.center3;
		zone_poly_counts[ex.zone_id] += 1;
	}
	// Average.
	for (u32 n = 0; n < r_dest._zones.size(); n++) {
		Zone &zone = r_dest._zones[n];
		u32 poly_count = zone_poly_counts[n];

		if (poly_count) {
			zone.local_pos3 /= (float)poly_count;
		}
	}

#if 1

	// Calculate max agents per area / narrowing.
	{
		Vector<u64> zone_areas;
		zone_areas.resize(r_dest.get_num_zones());
		zone_areas.fill(0);

		//		Vector<u64> area_areas;
		//		area_areas.resize(r_dest.get_num_areas());
		//		area_areas.fill(0);
		//		Vector<u64> narrowing_areas;
		//		narrowing_areas.resize(r_dest.get_num_narrowings());
		//		narrowing_areas.fill(0);

		for (u32 n = 0; n < r_dest.get_num_polys(); n++) {
			const PolyExtra &ex = r_dest.get_poly_extra(n);
			u64 &area = zone_areas[ex.zone_id];
			area += ex.area_absolute;

			//			if (ex.is_area()) {
			//				u64 &area = area_areas[ex.get_area_id()];
			//				area += ex.area_absolute;
			//			}
			//			if (ex.is_narrowing()) {
			//				u64 &area = narrowing_areas[ex.get_narrowing_id()];
			//				area += ex.area_absolute;
			//			}
		}

		// What is area needed for one agent?
		u64 agent_area = r_dest.extension_data.agent_radius;
		// Area of circle is PI r squared, so we approximate this.
		agent_area *= agent_area;
		agent_area *= 4;

		// for debugging
		//agent_area *= 5;

		// Now estimate max agents per area.
		for (u32 n = 0; n < r_dest.get_num_zones(); n++) {
			Zone &zone = r_dest._zones[n];
			zone.max_agents = MAX(1, zone_areas[n] / agent_area);

			// debug
			//zone.max_agents = 2;

			log(String("Zone ") + n + "\tmax_agents " + zone.max_agents);
		}

		//		for (u32 n = 0; n < r_dest.get_num_areas(); n++) {
		//			Area &area = r_dest._areas[n];
		//			area.max_agents = MAX(1, area_areas[n] / agent_area);
		//			log(String("Area ") + n + "\tmax_agents " + area.max_agents);
		//		}
		//		for (u32 n = 0; n < r_dest.get_num_narrowings(); n++) {
		//			Narrowing &narr = r_dest._narrowings[n];
		//			narr.max_agents = MAX(1, narrowing_areas[n] / agent_area);
		//			log(String("Narr ") + n + "\tmax_agents " + narr.max_agents);
		//		}
	}
#endif
}

IPoint2 Loader::_find_crossing_point_in_poly(Mesh &p_mesh, u32 p_poly_id, const IPoint2 p_seed_pos) const {
	// Simplest case.
	if (p_mesh.poly_contains_point(p_poly_id, p_seed_pos)) {
		return p_seed_pos;
	}

	// Progressively move out from the seed point until we find a point inside the poly.
	IPoint2 pos;

	for (int32_t y = -1; y <= 1; y++) {
		for (int32_t x = -1; x <= 1; x++) {
			if ((x * y) == 0)
				continue;

			pos.x = p_seed_pos.x + x;
			pos.y = p_seed_pos.y + y;

			if (p_mesh.poly_contains_point(p_poly_id, pos)) {
				return pos;
			}
		}
	}

	// None of them contain it!
	NP_ERR_PRINT("_find_crossing_point_in_poly failed.");
	return p_seed_pos;
}

void Loader::find_zone_links(Mesh &r_dest) {
	// Zones
	r_dest._zones.clear();
	r_dest._zone_links.clear();

	u32 num_areas = r_dest._areas.size();
	u32 num_narrowings = r_dest._narrowings.size();
	u32 num_zones = num_areas + num_narrowings;

	u32 start_narrowings = num_areas;
	r_dest._zones.resize(num_zones);

	// Add area zones
	for (u32 n = 0; n < num_areas; n++) {
		Zone &zone = r_dest._zones[n];
		zone.type = ZONE_TYPE_AREA;
		zone.area_or_narrowing_id = n;
	}

	// Add narrowing zones.
	for (u32 n = 0; n < num_narrowings; n++) {
		Zone &zone = r_dest._zones[start_narrowings + n];
		zone.type = ZONE_TYPE_NARROWING;
		zone.area_or_narrowing_id = n;
	}

	// Set the zone ids on the polys
	for (u32 n = 0; n < r_dest.get_num_polys(); n++) {
		PolyExtra &ex = r_dest.get_poly_extra(n);
		ex.zone_id = !ex.is_narrowing() ? ex.get_area_id() : ex.get_narrowing_id() + start_narrowings;
		// log(String("Poly ") + n + " is zone " + ex.zone_id);
	}

	// We want to build a list of poly edges between narrowings
	// and areas, so that we can identify the best crossing points
	// to make zone waypoints.
	struct ZEdge {
		u32 poly_from_id = UINT32_MAX;
		u32 poly_to_id = UINT32_MAX;
		u32 poly_from_wall_id = UINT32_MAX;
		u32 wall_vert_a = UINT32_MAX;
		u32 wall_vert_b = UINT32_MAX;
	};

	struct ZLink {
		u32 to_zone_id = UINT32_MAX;
		Vector<ZEdge> edges;
	};
	struct ZLinks {
		ZLink *find_link_to(u32 p_zone_id) {
			for (u32 n = 0; n < links.size(); n++) {
				if (links[n].to_zone_id == p_zone_id) {
					return &links[n];
				}
			}
			return nullptr;
		}
		Vector<ZLink> links;
	};

	Vector<ZLinks> zlinks;
	zlinks.resize(r_dest._zones.size());

	// Link zones.
	NP_DEV_ASSERT(r_dest._zone_links.is_empty());
	for (u32 n = 0; n < r_dest.get_num_polys(); n++) {
		const PolyExtra &ex = r_dest.get_poly_extra(n);
		const Poly &poly = r_dest.get_poly(n);

		for (u32 i = 0; i < poly.num_inds; i++) {
			u32 wall_id = poly.first_ind + i;

			if (!r_dest.is_link_regular(wall_id))
				continue;

			u32 npoly_id = r_dest.get_link(wall_id);
			NP_DEV_ASSERT(npoly_id != UINT32_MAX);

			const PolyExtra &nex = r_dest.get_poly_extra(npoly_id);

			u32 zone_from_id = ex.zone_id;
			u32 zone_to_id = nex.zone_id;

			// Same zone? Ignore.
			if (zone_from_id == zone_to_id)
				continue;

			// Only do one way relationship, to save calculating the edges twice.
			if (zone_from_id > zone_to_id)
				continue;

			ZLinks &zlinks_from = zlinks[zone_from_id];

			ZLink *zl = zlinks_from.find_link_to(zone_to_id);
			if (!zl) {
				ZLink zl_new;
				zl_new.to_zone_id = zone_to_id;

				zlinks_from.links.push_back(zl_new);
				zl = zlinks_from.links.get_last();

				// log(String("Linking zone ") + zone_from_id + " to zone " + zone_to_id);
			}
			NP_DEV_ASSERT(zl);
			ZEdge e;
			e.poly_from_id = n;
			e.poly_to_id = npoly_id;
			e.poly_from_wall_id = wall_id;

			const Wall &wall = r_dest.get_wall(wall_id);
			e.wall_vert_a = wall.get_swapped_vert_a();
			e.wall_vert_b = wall.get_swapped_vert_b();

			zl->edges.push_back(e);
			// log(String("\tadding edge for zone ") + zone_from_id + " to zone " + zone_to_id + ", edge " + e.wall_vert_a + " to " + e.wall_vert_b);
		} // for i
	} // for n

	// Sort the link edges to a continuous edge.
	for (u32 a = 0; a < zlinks.size(); a++) {
		ZLinks &links = zlinks[a];

		for (u32 l = 0; l < links.links.size(); l++) {
			ZLink &link = links.links[l];

			// First find edge with no neighbour.
			u32 first_id = UINT32_MAX;
			bool found_neighbour = false;

			for (u32 n = 0; n < link.edges.size(); n++) {
				const ZEdge &edge_a = link.edges[n];

				for (u32 m = 0; m < link.edges.size(); m++) {
					if (n == m)
						continue;

					const ZEdge &edge_b = link.edges[m];

					if (edge_a.wall_vert_a == edge_b.wall_vert_b) {
						found_neighbour = true;
						break;
					}
				} // for m
				if (!found_neighbour) {
					first_id = n;
					break;
				}
			} // for n

			// Ignore, nothing to sort.
			if (first_id == UINT32_MAX) {
				continue;
			}

			// Start from first edge.
			Vector<ZEdge> sorted_edges;
			sorted_edges.push_back(link.edges[first_id]);
			link.edges.remove_unordered(first_id);

			// Watch for potential infinite loop here.
			while (link.edges.size()) {
				const ZEdge &edge_prev = sorted_edges.last();

				bool match = false;

				for (u32 n = 0; n < link.edges.size(); n++) {
					const ZEdge &edge = link.edges[n];
					if (edge_prev.wall_vert_b == edge.wall_vert_a) {
						// We have a match!
						sorted_edges.push_back(edge);
						link.edges.remove_unordered(n);
						match = true;
						break;
					}
				}

				// If we got here without a match, we have a broken edge.
				if (!match) {
					log(String("Adding split link to zone ") + link.to_zone_id + " containing " + link.edges.size() + " edges.");
					ZLink &split_link = links.links.request();
					split_link.edges = link.edges;
					split_link.to_zone_id = link.to_zone_id;
					link.edges.clear();
				}
			}
			link.edges = sorted_edges;

			// Debug
#if 1
			if (sorted_edges.size() > 1) {
				String sz = String("Link ") + l + ",\t";
				for (u32 n = 0; n < sorted_edges.size(); n++) {
					sz += String(sorted_edges[n].wall_vert_a) + "-" + sorted_edges[n].wall_vert_b + ", ";
				}
				log(sz);
			}
#endif

		} // for l
	} // for a

	struct ZoneLinkList {
		Vector<ZoneLink> links;
	};
	Vector<ZoneLinkList> zonelinks_temp;
	zonelinks_temp.resize(num_zones);

	// Find crossing points.
	for (u32 a = 0; a < zlinks.size(); a++) {
		ZLinks &links = zlinks[a];

		for (u32 l = 0; l < links.links.size(); l++) {
			ZLink &link = links.links[l];

			// We want to choose a crossing point in the centre of the edge.
			// We do this by first calculating the total distance of the multi-edge,
			// then a second pass finding which edge is in the centre, and what point along to place
			// the crossing point.
			freal total_dist = 0;

			for (u32 n = 0; n < link.edges.size(); n++) {
				const ZEdge &e = link.edges[n];

				total_dist += r_dest.get_wall(e.poly_from_wall_id).wall_vec.lengthf();
			}

			// Second pass.
			freal crossing_dist = total_dist * 0.5;
			total_dist = 0;
			for (u32 n = 0; n < link.edges.size(); n++) {
				const ZEdge &e = link.edges[n];

				freal dist_before = total_dist;
				freal wall_length = r_dest.get_wall(e.poly_from_wall_id).wall_vec.lengthf();
				total_dist += wall_length;

				if (total_dist > crossing_dist) {
					// This is the crossing edge.
					freal dist_along_edge = crossing_dist - dist_before;
					freal fract_along_edge = dist_along_edge / wall_length;

					IPoint2 pt_a, pt_b;
					r_dest.get_swapped_wall_verts(e.poly_from_wall_id, pt_a, pt_b);
					IPoint2 crossing_pt = pt_a + ((pt_b - pt_a) * fract_along_edge);

					//log(String("Crossing pt found at ") + crossing_pt + " in poly id " + crossing_poly_id);

					// Fill the link info.
					// NOTE that split links may occur more than once.
					ZoneLinkList &zll_from = zonelinks_temp[a];
					ZoneLinkList &zll_to = zonelinks_temp[link.to_zone_id];

					ZoneLink zl;
#ifdef NP_DEV_ENABLED
					zl.zone_from_id = a;
#endif
					zl.zone_to_id = link.to_zone_id;
					zl.zone_from_poly_id = e.poly_from_id;
					zl.zone_to_poly_id = e.poly_to_id;
					zl.pt_crossing = _find_crossing_point_in_poly(r_dest, zl.zone_to_poly_id, crossing_pt);
					zl.pt_crossing3 = r_dest.local_point_to_point3(zl.pt_crossing, zl.zone_to_poly_id);

					zll_from.links.push_back(zl);

#ifdef NP_DEV_ENABLED
					zl.zone_from_id = link.to_zone_id;
#endif
					zl.zone_to_id = a;
					SWAP(zl.zone_from_poly_id, zl.zone_to_poly_id);
					zl.pt_crossing = _find_crossing_point_in_poly(r_dest, zl.zone_to_poly_id, crossing_pt);
					zl.pt_crossing3 = r_dest.local_point_to_point3(zl.pt_crossing, zl.zone_to_poly_id);

					zll_to.links.push_back(zl);

					break;
				}
			}
		}
	}

	// Create opposite zone links. This is for higher number zones to lower, and will to some extent duplicate
	// the existing links, but the crossing point and crossing poly will be different.

	// Create final zone links.
	for (u32 n = 0; n < num_zones; n++) {
		Zone &zone = r_dest._zones[n];
		zone.first_link = r_dest._zone_links.size();
		NP_DEV_ASSERT(zone.num_links == 0);

		const ZoneLinkList &zll = zonelinks_temp[n];

		for (u32 l = 0; l < zll.links.size(); l++) {
			const ZoneLink &zl = zll.links[l];
			NP_DEV_ASSERT(zl.zone_from_id == n);

			r_dest._zone_links.push_back(zl);
			zone.num_links += 1;
		}
	}
}

void Loader::floodfill_area(Mesh &r_mesh, u32 p_poly_id, u32 p_area_id) {
	PolyExtra &ex = r_mesh.get_poly_extra(p_poly_id);
	if (!ex.is_area_unset()) {
		return;
	}
	ex.set_area_id(p_area_id);

	const Poly &poly = r_mesh.get_poly(p_poly_id);

	for (u32 i = 0; i < poly.num_inds; i++) {
		u32 ind = poly.first_ind + i;

		if (!r_mesh.is_link_hard(ind)) {
			floodfill_area(r_mesh, r_mesh.get_link(ind), p_area_id);
		}
	}
}

} // namespace NavPhysics
