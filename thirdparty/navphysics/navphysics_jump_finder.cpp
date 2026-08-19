// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#include "navphysics_jump_finder.h"
#include "navphysics_mesh_funcs.h"

namespace NavPhysics {

void JumpFinder::find_jumps(Mesh &r_mesh) {
	r_mesh.jump_data.clear();
	data.clear();

	for (u32 w = 0; w < r_mesh.get_num_walls(); w++) {
		r_mesh._walls[w].jump_info_id = UINT32_MAX;
	}

	data.wall_jump_info.resize(r_mesh.get_num_walls());
	data.wall_jump_info.fill(UINT32_MAX);

	//	for (u32 n = 0; n < r_mesh.get_num_polys(); n++) {
	//		r_mesh.get_poly_extra(n).jump_poly = UINT32_MAX;
	//	}

	// Find all walls that are connecting walls.
	for (u32 n = 0; n < r_mesh.get_num_walls(); n++) {
		//if (r_mesh.is_external_connecting_wall(n)) {
		if (r_mesh.is_link_internal(n) || r_mesh.is_link_external(n)) {
			find_wall_jumps(r_mesh, n);
		}
	}

	// Finalize the jumps.
	//r_mesh.jump_data.wall_jump_info.resize(r_mesh.get_num_walls());
	//r_mesh.jump_data.wall_jump_info.fill(UINT32_MAX);

	for (u32 w = 0; w < data.wall_jump_info.size(); w++) {
		u32 jump_info_id = data.wall_jump_info[w];
		if (jump_info_id != UINT32_MAX) {
			const JumpInfo &source_jump_info = data.jump_infos[jump_info_id];

			r_mesh._walls[w].jump_info_id = r_mesh.jump_data.jump_info.size();
			Mesh::JumpInfo &dest_jump_info = r_mesh.jump_data.jump_info.request();

			dest_jump_info.first_wall_jump = r_mesh.jump_data.jump_wall_ids.size();
			dest_jump_info.num_wall_jumps = source_jump_info.to_wall_id.size();

			dest_jump_info.first_poly_jump = r_mesh.jump_data.jump_poly_ids.size();
			dest_jump_info.num_poly_jumps = source_jump_info.to_poly_id.size();

			for (u32 n = 0; n < source_jump_info.to_wall_id.size(); n++) {
				u32 jump_wall_id = source_jump_info.to_wall_id[n];
				r_mesh.jump_data.jump_wall_ids.push_back(jump_wall_id);
			}

			for (u32 n = 0; n < source_jump_info.to_poly_id.size(); n++) {
				u32 jump_poly_id = source_jump_info.to_poly_id[n];
				r_mesh.jump_data.jump_poly_ids.push_back(jump_poly_id);
			}
		}
	}
}

bool JumpFinder::jump_wall_within_range(const IPoint2 &p_a, const IPoint2 &p_b, const IPoint2 &p_c, const IPoint2 &p_d, freal p_range) const {
	// Bandit the float routine, this doesn't need to be super accurate anyway.
	MeshFuncs funcs;
	freal dist = funcs.get_closest_distance_between_segments(p_a, p_b, p_c, p_d);

	return dist <= p_range;
}

void JumpFinder::add_poly_jump(u32 p_wall_id_from, u32 p_poly_id_to) {
	if (data.wall_jump_info[p_wall_id_from] == UINT32_MAX) {
		data.wall_jump_info[p_wall_id_from] = data.jump_infos.size();
		data.jump_infos.resize(data.jump_infos.size() + 1);
	}

	u32 jump_info_id = data.wall_jump_info[p_wall_id_from];
	JumpInfo &jump_info = data.jump_infos[jump_info_id];

	if (jump_info.to_poly_id.find(p_poly_id_to) == -1) {
		jump_info.to_poly_id.push_back(p_poly_id_to);
		log(String("Adding jump poly from wall ") + p_wall_id_from + " to poly " + p_poly_id_to);
	}
}

void JumpFinder::add_wall_jump(Mesh &r_mesh, u32 p_wall_id_from, u32 p_wall_id_to) {
	// First ensure that both polys are marked for internal connection.
	if (r_mesh.is_link_hard(p_wall_id_from)) {
		r_mesh._links[p_wall_id_from] |= Mesh::LINK_FLAG_INTERNAL;
	}

	if (r_mesh.is_link_hard(p_wall_id_to)) {
		r_mesh._links[p_wall_id_to] |= Mesh::LINK_FLAG_INTERNAL;
	}

	//	const Wall &wall_from = r_mesh.get_wall(p_wall_from);
	//	const Wall &wall_to = r_mesh.get_wall(p_wall_to);

	//	NP_DEV_ASSERT (wall_from.poly_id != UINT32_MAX);
	//	NP_DEV_ASSERT (wall_to.poly_id != UINT32_MAX);

	if (data.wall_jump_info[p_wall_id_from] == UINT32_MAX) {
		data.wall_jump_info[p_wall_id_from] = data.jump_infos.size();
		data.jump_infos.resize(data.jump_infos.size() + 1);
	}

	u32 jump_info_id = data.wall_jump_info[p_wall_id_from];
	JumpInfo &jump_info = data.jump_infos[jump_info_id];

	if (jump_info.to_wall_id.find(p_wall_id_to) == -1) {
		jump_info.to_wall_id.push_back(p_wall_id_to);
	}
}

void JumpFinder::find_wall_jumps(Mesh &r_mesh, u32 p_wall_id) {
	const Wall &source_wall = r_mesh.get_wall(p_wall_id);

	// Scan out in the direction of the normal for any other suitable polys within range.

	const freal range = r_mesh.extension_data.agent_radius * 4;

	IPoint2 a, b;
	a = r_mesh.get_vert(source_wall.get_swapped_vert_a());
	b = r_mesh.get_vert(source_wall.get_swapped_vert_b());

	IPoint2 wall_vec = b - a;

	IPoint2 c = a;
	IPoint2 d = b;

	IPoint2 vec = -source_wall.normal;
	vec.normalize_to_scale(range);

	c += vec;
	d += vec;

	IRect2 rect;
	rect.position = a;
	rect.expand_to_fast(b);
	rect.expand_to_fast(c);
	rect.expand_to_fast(d);
	rect.increment_size();

	Vector<PolyFinder::CellResult> results;
	r_mesh.floor.poly_finder.find_cells(rect, results);

//#define NAVPHYSICS_JUMPFINDER_EXPORT_SVG
#ifdef NAVPHYSICS_JUMPFINDER_EXPORT_SVG
	Vector<u32> export_walls;
	export_walls.push_back(p_wall_id);
	Vector<u32> export_polys;
#endif

	// Find all polys within range of the wall,
	// and all walls within range facing the source wall.
	for (u32 r = 0; r < results.size(); r++) {
		const PolyFinder::CellResult &cr = results[r];

		for (u32 p = 0; p < cr.num_polys; p++) {
			u32 poly_id = cr.poly_ids[p];

			find_poly_jumps(r_mesh, p_wall_id, poly_id);

			const Poly &poly = r_mesh.get_poly(poly_id);

			for (u32 w = 0; w < poly.num_inds; w++) {
				u32 wid = poly.first_ind + w;

				// Is the wall marked as internal?
				if (!r_mesh.is_link_internal(wid)) {
					continue;
				}

				const Wall &wall = r_mesh.get_wall(wid);

				// Normals?
				float dot = source_wall.normal.dot_normalized(wall.normal);
				if (dot < 0) {
					IPoint2 j, k;
					r_mesh.get_wall_verts(wid, j, k);

					// Both points must be ahead of the source wall.
					i64 cross = wall_vec.cross(j - a);

					if (cross > 0) {
						continue;
					}

					cross = wall_vec.cross(k - a);
					if (cross > 0) {
						continue;
					}

					if (jump_wall_within_range(a, b, j, k, range)) {
						// Add the jump wall (to both).
						// log(String("\t") + p_wall_id + " adding jump wall " + j + " -> " + k);
						add_wall_jump(r_mesh, p_wall_id, wid);
						add_wall_jump(r_mesh, wid, p_wall_id);
					}
				}
			}
		} // for p
	} // for r

#ifdef NAVPHYSICS_JUMPFINDER_EXPORT_SVG
	u32 jump_info_id = data.wall_jump_info[p_wall_id];
	if (jump_info_id != UINT32_MAX) {
		const JumpInfo &ji = data.jump_infos[jump_info_id];

#if 0
		if (ji.to_poly_id.size())
		{
			for (u32 n=0; n<ji.to_poly_id.size(); n++)
			{
				export_polys.push_back(ji.to_poly_id[n]);
			}
			
			r_mesh.svg_export_custom(String("../jump_wall_") + p_wall_id + ".svg", export_walls, export_polys);
		}
#else
		if (ji.to_wall_id.size()) {
			for (u32 n = 0; n < ji.to_wall_id.size(); n++) {
				u32 wall_id = ji.to_wall_id[n];
				const Wall &wall = r_mesh.get_wall(wall_id);
				if (wall.poly_id != UINT32_MAX) {
					if (export_polys.find(wall.poly_id) == -1) {
						export_polys.push_back(wall.poly_id);
					}
				}
			}
			if (export_polys.size() > 1) {
				log(String("lots of export polys") + export_polys.size());
				r_mesh.svg_export_custom(String("../jump_wall_wall_") + p_wall_id + ".svg", export_walls, export_polys);
			}
		}
#endif
	}
#endif
}

void JumpFinder::find_poly_jumps(Mesh &r_mesh, u32 p_wall_id, u32 p_poly_id) {
	const Poly &poly = r_mesh.get_poly(p_poly_id);

	// Basically we need a line segment poly test.
	// We can test whether either of the wall points are inside the poly.
	// If not we should still intersect the wall with each poly wall, if there is a hit,
	// then we add the poly to the jump list.
	IPoint2 a, b;
	r_mesh.get_wall_verts(p_wall_id, a, b);

	const Wall &wall = r_mesh.get_wall(p_wall_id);

	if (r_mesh.poly_contains_vert(p_poly_id, wall.vert_a)) {
		return;
	}
	if (r_mesh.poly_contains_vert(p_poly_id, wall.vert_b)) {
		return;
	}

	bool within = false;

	within = r_mesh.poly_contains_point(p_poly_id, a) || r_mesh.poly_contains_point(p_poly_id, b);

	if (!within) {
		for (u32 w = 0; w < poly.num_inds; w++) {
			u32 poly_wall_id = poly.first_ind + w;
			IPoint2 pt_hit;
			if (r_mesh.wall_segments_find_intersect(poly_wall_id, a, b, pt_hit)) {
				within = true;
				break;
			}
		}
	}

	if (within) {
		log(String("Wall ") + a + " to " + b + " is within");
		r_mesh.debug_poly(p_poly_id);

		if (r_mesh.poly_contains_vert(p_poly_id, wall.vert_a)) {
			return;
		}
		if (r_mesh.poly_contains_vert(p_poly_id, wall.vert_b)) {
			return;
		}

		add_poly_jump(p_wall_id, p_poly_id);
	}
}

} //namespace NavPhysics
