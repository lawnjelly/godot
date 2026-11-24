// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#include "navphysics_baker.h"
#include "navphysics_post_process.h"

namespace NavPhysics {

void Baker::_merge_small_regions(u32 min_region_area) {
	if (min_region_area <= 1 || open_spans.is_empty()) {
		return;
	}

	const u32 grid_dim = 192;
	constexpr u32 CORE_MIN = 32;
	constexpr u32 CORE_MAX = 160;

	bool merged_any = true;
	while (merged_any) {
		merged_any = false;

		// Count span area per region
		u16 max_reg = 0;
		for (u32 z = CORE_MIN; z < CORE_MAX; ++z) {
			for (u32 x = CORE_MIN; x < CORE_MAX; ++x) {
				const OpenCell &cell = open_cells[z * grid_dim + x];
				for (u32 i = 0; i < cell.count; ++i) {
					const u16 r = open_spans[cell.index + i].region_id;
					if (r > max_reg) {
						max_reg = r;
					}
				}
			}
		}
		if (max_reg == 0) {
			return;
		}

		Vector<u32> reg_area;
		reg_area.resize(max_reg + 1);
		for (u32 i = 0; i < reg_area.size(); ++i) {
			reg_area[i] = 0;
		}
		for (u32 z = CORE_MIN; z < CORE_MAX; ++z) {
			for (u32 x = CORE_MIN; x < CORE_MAX; ++x) {
				const OpenCell &cell = open_cells[z * grid_dim + x];
				for (u32 i = 0; i < cell.count; ++i) {
					const u16 r = open_spans[cell.index + i].region_id;
					if (r != 0) {
						reg_area[r]++;
					}
				}
			}
		}

		_trace_region_contours(contours);

		Vector<u16> remap;
		remap.resize(max_reg + 1);
		for (u32 i = 0; i <= max_reg; ++i) {
			remap[i] = static_cast<u16>(i);
		}

		for (u32 c = 0; c < contours.size(); ++c) {
			const RegionContour &rc = contours[c];
			const u16 r = rc.region_id;
			if (r == 0 || reg_area[r] >= min_region_area || rc.verts.size() < 3) {
				continue;
			}

			// Find neighbor regions that touch 'r' along exactly 1 contiguous boundary arc
			// (guarantees merging won't create holes around pillars/obstacles).
			struct NeighborInfo {
				u16 reg;
				u32 edges;
				u32 arcs;
			};
			Vector<NeighborInfo> neighbors;
			const u32 vn = rc.verts.size();

			for (u32 k = 0; k < vn; ++k) {
				const u16 nreg = rc.verts[k].neighbor_reg;
				const u16 prev_nreg = rc.verts[(k + vn - 1) % vn].neighbor_reg;
				if (nreg == 0) {
					continue;
				}
				u32 idx = UINT32_MAX;
				for (u32 ni = 0; ni < neighbors.size(); ++ni) {
					if (neighbors[ni].reg == nreg) {
						idx = ni;
						break;
					}
				}
				if (idx == UINT32_MAX) {
					idx = neighbors.size();
					neighbors.push_back({ nreg, 0, 0 });
				}
				neighbors[idx].edges++;
				if (prev_nreg != nreg) {
					neighbors[idx].arcs++;
				}
			}

			u16 best_target = 0;
			u32 best_edges = 0;
			for (u32 ni = 0; ni < neighbors.size(); ++ni) {
				const u16 cand = neighbors[ni].reg;
				// Only merge into a strictly larger (or same-size higher-id) region that hasn't itself been remapped
				if (neighbors[ni].arcs == 1 && remap[cand] == cand &&
						(reg_area[cand] > reg_area[r] || (reg_area[cand] == reg_area[r] && cand > r))) {
					if (neighbors[ni].edges > best_edges) {
						best_edges = neighbors[ni].edges;
						best_target = cand;
					}
				}
			}

			if (best_target != 0) {
				remap[r] = best_target;
				reg_area[best_target] += reg_area[r];
				merged_any = true;
				break; // Re-trace before merging another so multi-step merges stay hole-free
			}
		}

		if (merged_any) {
			for (u32 z = CORE_MIN; z < CORE_MAX; ++z) {
				for (u32 x = CORE_MIN; x < CORE_MAX; ++x) {
					const OpenCell &cell = open_cells[z * grid_dim + x];
					for (u32 i = 0; i < cell.count; ++i) {
						OpenSpan &s = open_spans[cell.index + i];
						if (s.region_id != 0) {
							s.region_id = remap[s.region_id];
						}
					}
				}
			}
		}
	}
}

void Baker::_point_segment_error_xz_y(const ContourVert &p, const ContourVert &a, const ContourVert &b,
		freal &r_xz_dist_sq, freal &r_y_err) const {
	const freal dx = b.x - a.x;
	const freal dz = b.z - a.z;
	const freal len_sq = dx * dx + dz * dz;
	if (len_sq <= static_cast<freal>(1e-8)) {
		const freal px = p.x - a.x;
		const freal pz = p.z - a.z;
		r_xz_dist_sq = px * px + pz * pz;
		r_y_err = std::abs(p.y - a.y);
		return;
	}
	freal t = ((p.x - a.x) * dx + (p.z - a.z) * dz) / len_sq;
	t = (t < 0) ? 0 : ((t > 1) ? 1 : t);
	const freal proj_x = a.x + t * dx;
	const freal proj_y = a.y + t * (b.y - a.y);
	const freal proj_z = a.z + t * dz;
	const freal ex = p.x - proj_x;
	const freal ez = p.z - proj_z;
	r_xz_dist_sq = ex * ex + ez * ez;
	r_y_err = std::abs(p.y - proj_y);
}

freal Baker::_cross_2d_xz(const ContourVert &a, const ContourVert &b, const ContourVert &c) const {
	return (b.x - a.x) * (c.z - a.z) - (b.z - a.z) * (c.x - a.x);
}

bool Baker::_point_in_tri_xz(const ContourVert &p, const ContourVert &a, const ContourVert &b, const ContourVert &c, freal sign) const {
	const freal c1 = _cross_2d_xz(a, b, p) * sign;
	const freal c2 = _cross_2d_xz(b, c, p) * sign;
	const freal c3 = _cross_2d_xz(c, a, p) * sign;
	return (c1 >= -1e-6) && (c2 >= -1e-6) && (c3 >= -1e-6);
}

void Baker::_triangulate_contour(const Vector<ContourVert> &verts, Vector<Tri> &out_tris) const {
	out_tris.clear();
	const u32 n = verts.size();
	if (n < 3)
		return;

	freal area2 = 0;
	for (u32 i = 0; i < n; ++i) {
		const ContourVert &a = verts[i];
		const ContourVert &b = verts[(i + 1) % n];
		area2 += (a.x * b.z - b.x * a.z);
	}
	if (std::abs(area2) < 1e-6)
		return;
	const freal sign = (area2 > 0) ? 1.0 : -1.0;

	Vector<u32> rem;
	rem.resize(n);
	for (u32 i = 0; i < n; ++i)
		rem[i] = i;

	u32 guard = 0;
	const u32 max_iters = n * n + 4;

	while (rem.size() > 3 && guard++ < max_iters) {
		const u32 m = rem.size();
		freal best_diag_len_sq = FLT_MAX;
		u32 best_ear_idx = UINT32_MAX;

		for (u32 i = 0; i < m; ++i) {
			const u32 i_prev = rem[(i + m - 1) % m];
			const u32 i_curr = rem[i];
			const u32 i_next = rem[(i + 1) % m];

			const ContourVert &a = verts[i_prev];
			const ContourVert &b = verts[i_curr];
			const ContourVert &c = verts[i_next];

			if (_cross_2d_xz(a, b, c) * sign <= 1e-6) {
				continue;
			}

			const freal dx = c.x - a.x;
			const freal dz = c.z - a.z;
			const freal diag_len_sq = dx * dx + dz * dz;
			if (diag_len_sq >= best_diag_len_sq) {
				continue;
			}

			bool contains_point = false;
			for (u32 j = 0; j < m; ++j) {
				const u32 idx = rem[j];
				if (idx == i_prev || idx == i_curr || idx == i_next)
					continue;
				if (_point_in_tri_xz(verts[idx], a, b, c, sign)) {
					contains_point = true;
					break;
				}
			}

			if (!contains_point) {
				best_diag_len_sq = diag_len_sq;
				best_ear_idx = i;
			}
		}

		if (best_ear_idx != UINT32_MAX) {
			const u32 i_prev = rem[(best_ear_idx + m - 1) % m];
			const u32 i_curr = rem[best_ear_idx];
			const u32 i_next = rem[(best_ear_idx + 1) % m];
			out_tris.push_back({ { i_prev, i_curr, i_next } });
			rem.remove(best_ear_idx);
		} else {
			// Fallback: pick any remaining vertex with positive signed area (ignoring point-in-tri)
			// so we never emit an inverted/downward-facing triangle.
			u32 fallback_idx = UINT32_MAX;
			freal fallback_len_sq = FLT_MAX;
			for (u32 i = 0; i < m; ++i) {
				const ContourVert &a = verts[rem[(i + m - 1) % m]];
				const ContourVert &b = verts[rem[i]];
				const ContourVert &c = verts[rem[(i + 1) % m]];
				if (_cross_2d_xz(a, b, c) * sign > static_cast<freal>(1e-6)) {
					const freal dx = c.x - a.x;
					const freal dz = c.z - a.z;
					const freal d_sq = dx * dx + dz * dz;
					if (d_sq < fallback_len_sq) {
						fallback_len_sq = d_sq;
						fallback_idx = i;
					}
				}
			}
			if (fallback_idx != UINT32_MAX) {
				out_tris.push_back({ { rem[(fallback_idx + m - 1) % m], rem[fallback_idx], rem[(fallback_idx + 1) % m] } });
				rem.remove(fallback_idx);
			} else {
				break;
			}
		}
#if 0
		const u32 m = rem.size();
		bool ear_found = false;

		for (u32 i = 0; i < m; ++i) {
			const u32 i_prev = rem[(i + m - 1) % m];
			const u32 i_curr = rem[i];
			const u32 i_next = rem[(i + 1) % m];

			const ContourVert &a = verts[i_prev];
			const ContourVert &b = verts[i_curr];
			const ContourVert &c = verts[i_next];

			if (_cross_2d_xz(a, b, c) * sign <= 1e-6) {
				continue;
			}

			bool contains_point = false;
			for (u32 j = 0; j < m; ++j) {
				const u32 idx = rem[j];
				if (idx == i_prev || idx == i_curr || idx == i_next)
					continue;
				if (_point_in_tri_xz(verts[idx], a, b, c, sign)) {
					contains_point = true;
					break;
				}
			}

			if (!contains_point) {
				out_tris.push_back({ { i_prev, i_curr, i_next } });
				rem.remove(i);
				ear_found = true;
				break;
			}
		}

		if (!ear_found) {
			out_tris.push_back({ { rem[0], rem[1], rem[2] } });
			rem.remove(1);
		}
#endif
	}

	if (rem.size() == 3) {
		out_tris.push_back({ { rem[0], rem[1], rem[2] } });
	}
}

bool Baker::_is_poly_convex_xz(const Vector<u32> &poly, const Vector<ContourVert> &verts, freal sign) const {
	const u32 n = poly.size();
	for (u32 i = 0; i < n; ++i) {
		const ContourVert &a = verts[poly[(i + n - 1) % n]];
		const ContourVert &b = verts[poly[i]];
		const ContourVert &c = verts[poly[(i + 1) % n]];
		if (_cross_2d_xz(a, b, c) * sign <= 1e-5) {
			return false;
		}
	}
	return true;
}

bool Baker::_is_poly_coplanar(const Vector<u32> &poly, const Vector<ContourVert> &verts, freal max_y_deviation) const {
	if (poly.size() <= 3)
		return true;

	const ContourVert &p0 = verts[poly[0]];
	const ContourVert &p1 = verts[poly[1]];
	const ContourVert &p2 = verts[poly[2]];

	const freal ux = p1.x - p0.x, uy = p1.y - p0.y, uz = p1.z - p0.z;
	const freal vx = p2.x - p0.x, vy = p2.y - p0.y, vz = p2.z - p0.z;

	freal nx = uy * vz - uz * vy;
	freal ny = uz * vx - ux * vz;
	freal nz = ux * vy - uy * vx;
	const freal len = std::sqrt(nx * nx + ny * ny + nz * nz);
	if (len <= 1e-6 || std::abs(ny) <= 1e-5)
		return false;
	nx /= len;
	ny /= len;
	nz /= len;

	for (u32 i = 3; i < poly.size(); ++i) {
		const ContourVert &p = verts[poly[i]];
		const freal dist = std::abs(nx * (p.x - p0.x) + ny * (p.y - p0.y) + nz * (p.z - p0.z));
		if (dist > max_y_deviation) {
			return false;
		}
	}
	return true;
}

void Baker::_merge_tris_to_convex_polys(const Vector<ContourVert> &verts, const Vector<Tri> &tris,
		u32 max_verts_per_poly, freal max_y_deviation,
		Vector<Vector<u32>> &out_polys) const {
	out_polys.clear();
	if (tris.is_empty())
		return;

	freal area2 = 0;
	for (u32 i = 0; i < verts.size(); ++i) {
		const ContourVert &a = verts[i];
		const ContourVert &b = verts[(i + 1) % verts.size()];
		area2 += (a.x * b.z - b.x * a.z);
	}
	const freal sign = (area2 > 0) ? 1.0 : -1.0;

	for (u32 i = 0; i < tris.size(); ++i) {
		Vector<u32> p;
		p.push_back(tris[i].v[0]);
		p.push_back(tris[i].v[1]);
		p.push_back(tris[i].v[2]);
		out_polys.push_back(p);
	}

	if (max_verts_per_poly <= 3)
		return;

	while (true) {
		freal best_edge_len_sq = -1.0;
		u32 best_pa = UINT32_MAX, best_pb = UINT32_MAX;
		u32 best_ea = 0, best_eb = 0;

		for (u32 i = 0; i < out_polys.size(); ++i) {
			const Vector<u32> &pa = out_polys[i];
			for (u32 j = i + 1; j < out_polys.size(); ++j) {
				const Vector<u32> &pb = out_polys[j];
				if (pa.size() + pb.size() - 2 > max_verts_per_poly)
					continue;

				for (u32 ea = 0; ea < pa.size(); ++ea) {
					const u32 u = pa[ea];
					const u32 v = pa[(ea + 1) % pa.size()];

					for (u32 eb = 0; eb < pb.size(); ++eb) {
						if (pb[eb] == v && pb[(eb + 1) % pb.size()] == u) {
							const freal dx = verts[v].x - verts[u].x;
							const freal dz = verts[v].z - verts[u].z;
							const freal len_sq = dx * dx + dz * dz;

							if (len_sq > best_edge_len_sq) {
								Vector<u32> merged;
								for (u32 k = 1; k < pa.size(); ++k) {
									merged.push_back(pa[(ea + k) % pa.size()]);
								}
								for (u32 k = 1; k < pb.size(); ++k) {
									merged.push_back(pb[(eb + k) % pb.size()]);
								}

								if (_is_poly_convex_xz(merged, verts, sign) &&
										_is_poly_coplanar(merged, verts, max_y_deviation)) {
									best_edge_len_sq = len_sq;
									best_pa = i;
									best_pb = j;
									best_ea = ea;
									best_eb = eb;
								}
							}
						}
					}
				}
			}
		}

		if (best_pa == UINT32_MAX)
			break;

		const Vector<u32> &pa = out_polys[best_pa];
		const Vector<u32> &pb = out_polys[best_pb];
		Vector<u32> merged;
		for (u32 k = 1; k < pa.size(); ++k) {
			merged.push_back(pa[(best_ea + k) % pa.size()]);
		}
		for (u32 k = 1; k < pb.size(); ++k) {
			merged.push_back(pb[(best_eb + k) % pb.size()]);
		}

		out_polys[best_pa] = merged;
		out_polys.remove(best_pb);
	}
}

u32 Baker::_get_or_add_welded_vertex(NavMeshData &mesh, std::unordered_map<WeldKey, u32, WeldKeyHash> &weld_map,
		const FPoint3 &world_pt, freal weld_eps) const {
	const freal inv = static_cast<freal>(1.0) / weld_eps;
	const freal eps_sq = weld_eps * weld_eps;
	const i32 kx = static_cast<i32>(std::floor(world_pt.x * inv));
	const i32 ky = static_cast<i32>(std::floor(world_pt.y * inv));
	const i32 kz = static_cast<i32>(std::floor(world_pt.z * inv));

	for (i32 dx = -1; dx <= 1; ++dx) {
		for (i32 dy = -1; dy <= 1; ++dy) {
			for (i32 dz = -1; dz <= 1; ++dz) {
				auto it = weld_map.find(WeldKey{ kx + dx, ky + dy, kz + dz });
				if (it != weld_map.end()) {
					const FPoint3 &v = mesh.vertices[it->second];
					const freal ex = v.x - world_pt.x;
					const freal ey = v.y - world_pt.y;
					const freal ez = v.z - world_pt.z;
					if (ex * ex + ey * ey + ez * ez <= eps_sq) {
						return it->second;
					}
				}
			}
		}
	}

	const u32 new_idx = mesh.vertices.size();
	mesh.vertices.push_back(world_pt);
	weld_map.insert({ WeldKey{ kx, ky, kz }, new_idx });
	return new_idx;
}

void Baker::_build_tile_polygons(NavMeshData &out_mesh,
		std::unordered_map<WeldKey, u32, WeldKeyHash> &weld_map,
		freal tile_core_origin_x, freal tile_core_origin_z,
		freal cell_size, u32 max_verts_per_poly, freal max_y_deviation) const {
	Vector<Tri> tris;
	Vector<Vector<u32>> local_polys;

	for (u32 c = 0; c < contours.size(); ++c) {
		const Vector<ContourVert> &cverts = contours[c].verts;
		if (cverts.size() < 3)
			continue;

		_triangulate_contour(cverts, tris);
		_merge_tris_to_convex_polys(cverts, tris, max_verts_per_poly, max_y_deviation, local_polys);

		for (u32 p = 0; p < local_polys.size(); ++p) {
			const Vector<u32> &poly = local_polys[p];
			if (poly.size() < 3)
				continue;

			// In navphysics_baker.cpp -> Baker::_build_tile_polygons
			const u32 first_index = out_mesh.indices.size();

			// Reverse order (k from poly.size()-1 down to 0) so output is CW for Godot 3.x.
			for (u32 k = poly.size(); k > 0; --k) {
				const ContourVert &cv = cverts[poly[k - 1]];
				FPoint3 world_pt{
					tile_core_origin_x + cv.x * cell_size,
					cv.y,
					tile_core_origin_z + cv.z * cell_size
				};
				const u32 v_idx = _get_or_add_welded_vertex(out_mesh, weld_map, world_pt);

				// Skip consecutive duplicate vertex indices
				if (out_mesh.indices.size() == first_index || out_mesh.indices[out_mesh.indices.size() - 1] != v_idx) {
					out_mesh.indices.push_back(v_idx);
				}
			}

			// Collapse wrap-around duplicate if last vertex welded to first
			u32 num_indices = out_mesh.indices.size() - first_index;
			if (num_indices > 1 && out_mesh.indices[out_mesh.indices.size() - 1] == out_mesh.indices[first_index]) {
				out_mesh.indices.remove(out_mesh.indices.size() - 1);
				num_indices--;
			}

			// Reject degenerate zero-area polygons (e.g. collinear 3-vertex slivers)
			freal area2 = 0;
			if (num_indices >= 3) {
				for (u32 k = 0; k < num_indices; ++k) {
					const FPoint3 &a = out_mesh.vertices[out_mesh.indices[first_index + k]];
					const FPoint3 &b = out_mesh.vertices[out_mesh.indices[first_index + ((k + 1) % num_indices)]];
					area2 += (a.x * b.z - b.x * a.z);
				}
			}

			if (num_indices >= 3 && area2 > static_cast<freal>(1e-7)) {
				NavPoly nav_poly;
				nav_poly.first_index = first_index;
				nav_poly.num_indices = num_indices;
				out_mesh.polys.push_back(nav_poly);
			} else {
				out_mesh.indices.resize(first_index);
			}
		}
	}
}

void Baker::_simplify_contour(RegionContour &contour, freal max_error_voxels, freal max_y_deviation) {
	const u32 n = contour.verts.size();
	if (n <= 3)
		return;

	Vector<bool> keep;
	keep.resize(n);
	for (u32 i = 0; i < n; ++i) {
		keep[i] = false;
	}

	// 1. Always keep mandatory anchor vertices:
	//    - Where neighbor_reg changes (portal start/end between regions)
	//    - Vertices sitting on the 128x128 tile boundary (ensures seamless tile stitching)
	u32 anchor_count = 0;
	for (u32 i = 0; i < n; ++i) {
		const u32 prev = (i + n - 1) % n;
		const ContourVert &v = contour.verts[i];

		const bool portal_change = (v.neighbor_reg != contour.verts[(i + 1) % n].neighbor_reg);

		const ContourVert &v_prev = contour.verts[prev];
		const ContourVert &v_next = contour.verts[(i + 1) % n];

		const bool on_x0 = (v.x <= 0) && (v_prev.x <= 0) && (v_next.x <= 0);
		const bool on_x1 = (v.x >= 128) && (v_prev.x >= 128) && (v_next.x >= 128);
		const bool on_z0 = (v.z <= 0) && (v_prev.z <= 0) && (v_next.z <= 0);
		const bool on_z1 = (v.z >= 128) && (v_prev.z >= 128) && (v_next.z >= 128);
		const bool collinear_tile_edge = on_x0 || on_x1 || on_z0 || on_z1;
		const bool on_tile_edge = (v.x <= 0 || v.x >= 128 || v.z <= 0 || v.z >= 128) && !collinear_tile_edge;

		if (portal_change || on_tile_edge) {
			keep[i] = true;
			anchor_count++;
		}
	}

	// If a region is an isolated island with no portals or tile edges, seed 2 opposite extreme anchors
	if (anchor_count < 2) {
		u32 min_i = 0, max_i = 0;
		for (u32 i = 1; i < n; ++i) {
			if (contour.verts[i].x < contour.verts[min_i].x ||
					(contour.verts[i].x == contour.verts[min_i].x && contour.verts[i].z < contour.verts[min_i].z)) {
				min_i = i;
			}
			if (contour.verts[i].x > contour.verts[max_i].x ||
					(contour.verts[i].x == contour.verts[max_i].x && contour.verts[i].z > contour.verts[max_i].z)) {
				max_i = i;
			}
		}
		keep[min_i] = true;
		keep[max_i] = true;
	}

	// 2. Iterative RDP subdivision between consecutive kept anchors
	const freal inv_max_err_sq = static_cast<freal>(1.0) / (max_error_voxels * max_error_voxels);
	const freal inv_max_y_sq = static_cast<freal>(1.0) / (max_y_deviation * max_y_deviation);
	bool added = true;
	while (added) {
		added = false;
		u32 i = 0;
		while (i < n && !keep[i])
			++i;
		const u32 first_anchor = i;

		u32 curr = first_anchor;
		do {
			u32 next = (curr + 1) % n;
			while (!keep[next]) {
				next = (next + 1) % n;
			}

			// Find farthest intermediate vertex between curr and next (in normalized XZ or Y error)
			freal worst_score = 0;
			u32 worst_idx = UINT32_MAX;
			for (u32 k = (curr + 1) % n; k != next; k = (k + 1) % n) {
				freal d_sq = 0;
				freal y_err = 0;
				_point_segment_error_xz_y(contour.verts[k], contour.verts[curr], contour.verts[next], d_sq, y_err);
				const freal score = MAX(d_sq * inv_max_err_sq, (y_err * y_err) * inv_max_y_sq);
				if (score > worst_score) {
					worst_score = score;
					worst_idx = k;
				}
			}

			if (worst_idx != UINT32_MAX && worst_score > static_cast<freal>(1.0)) {
				keep[worst_idx] = true;
				added = true;
			}

			curr = next;
		} while (curr != first_anchor);
	}

	// 2b. Subdivide overly long outer wall edges to prevent long triangle fans to curves
	// 2b. Optional: subdivide overly long outer wall edges
	if (parameters.contour_max_edge_length > static_cast<freal>(0)) {
		const freal max_edge_len_voxels = parameters.contour_max_edge_length / parameters.voxel_cell_size;
		const freal max_edge_len_sq = max_edge_len_voxels * max_edge_len_voxels;

		added = true;
		while (added) {
			added = false;
			u32 i = 0;
			while (i < n && !keep[i])
				++i;
			const u32 first_anchor = i;

			u32 curr = first_anchor;
			do {
				u32 next = (curr + 1) % n;
				while (!keep[next]) {
					next = (next + 1) % n;
				}

				// Remember: contour.verts[next].neighbor_reg is the neighbor across segment (curr -> next)
				if (contour.verts[next].neighbor_reg == 0) {
					const ContourVert &a = contour.verts[curr];
					const ContourVert &b = contour.verts[next];
					const bool on_tile_border =
							(a.x <= 0 && b.x <= 0) || (a.x >= 128 && b.x >= 128) ||
							(a.z <= 0 && b.z <= 0) || (a.z >= 128 && b.z >= 128);

					if (!on_tile_border) {
						const freal dx = b.x - a.x;
						const freal dz = b.z - a.z;
						const freal len_sq = dx * dx + dz * dz;

						if (len_sq > max_edge_len_sq) {
							// Pick intermediate vertex closest to the midpoint of (curr -> next)
							const freal mid_x = static_cast<freal>(0.5) * (a.x + b.x);
							const freal mid_z = static_cast<freal>(0.5) * (a.z + b.z);
							freal best_d_sq = FLT_MAX;
							u32 best_idx = UINT32_MAX;

							for (u32 k = (curr + 1) % n; k != next; k = (k + 1) % n) {
								const freal kx = contour.verts[k].x - mid_x;
								const freal kz = contour.verts[k].z - mid_z;
								const freal d_sq = kx * kx + kz * kz;
								if (d_sq < best_d_sq) {
									best_d_sq = d_sq;
									best_idx = k;
								}
							}

							if (best_idx != UINT32_MAX) {
								keep[best_idx] = true;
								added = true;
							}
						}
					}
				}

				curr = next;
			} while (curr != first_anchor);
		}
	} // if max edge length not 0 (disabled)

	// 3. Compact kept vertices in-place
	Vector<ContourVert> simplified;
	for (u32 i = 0; i < n; ++i) {
		if (keep[i]) {
			simplified.push_back(contour.verts[i]);
		}
	}

	if (simplified.size() < 3) {
		// Thin (<= max_error_voxels) strip: RDP kept <= 2 anchors on one edge.
		// Strip collinear intermediate vertices so it simplifies to a single quad/poly
		// instead of falling back to a row of 1x1 voxel quads.
		simplified.clear();
		for (u32 i = 0; i < n; ++i) {
			if (keep[i]) {
				simplified.push_back(contour.verts[i]);
				continue;
			}
			const ContourVert &v_prev = contour.verts[(i + n - 1) % n];
			const ContourVert &v_curr = contour.verts[i];
			const ContourVert &v_next = contour.verts[(i + 1) % n];
			freal d_sq = 0;
			freal y_err = 0;
			_point_segment_error_xz_y(v_curr, v_prev, v_next, d_sq, y_err);
			if (d_sq > static_cast<freal>(1e-6) || y_err > static_cast<freal>(1e-4)) {
				simplified.push_back(v_curr);
			}
		}
	}

	if (simplified.size() >= 3) {
		contour.verts = simplified;
	}
}

void Baker::_trace_region_contours(Vector<RegionContour> &out_contours) {
	const u32 grid_dim = 192;
	constexpr u32 CORE_MIN = 32;
	constexpr u32 CORE_MAX = 160;

	// Clockwise mapping into OpenSpan::con indices: 0=(-X), 3=(+Z), 1=(+X), 2=(-Z)
	constexpr int CW_TO_CON[4] = { 0, 3, 1, 2 };
	constexpr int CW_DX[4] = { -1, 0, 1, 0 };
	constexpr int CW_DZ[4] = { 0, 1, 0, -1 };
	// Corner offsets (dx, dz) at the front-right of each cw_dir
	constexpr int CORNER_DX[4] = { 0, 1, 1, 0 };
	constexpr int CORNER_DZ[4] = { 1, 1, 0, 0 };

	Vector<bool> span_visited;
	span_visited.resize(open_spans.size());
	for (u32 i = 0; i < span_visited.size(); ++i) {
		span_visited[i] = false;
	}

	// Track which region_ids already have a traced contour
	// (Max region_id is bounded by span count; use a simple bool array)
	Vector<bool> region_done;
	region_done.resize(open_spans.size() + 1);
	for (u32 i = 0; i < region_done.size(); ++i) {
		region_done[i] = false;
	}

	out_contours.clear();

	for (u32 z = CORE_MIN; z < CORE_MAX; ++z) {
		for (u32 x = CORE_MIN; x < CORE_MAX; ++x) {
			const OpenCell &cell = open_cells[z * grid_dim + x];

			for (u32 i = 0; i < cell.count; ++i) {
				const u32 start_idx = cell.index + i;
				const OpenSpan &start_span = open_spans[start_idx];
				const u16 reg = start_span.region_id;

				if (reg == 0 || region_done[reg]) {
					continue;
				}

				// Find an exterior boundary direction to start on (prefer West: cw_dir = 0)
				int start_cw = -1;
				for (int d = 0; d < 4; ++d) {
					const u32 n_idx = start_span.con[CW_TO_CON[d]];
					const u16 n_reg = (n_idx != UINT32_MAX) ? open_spans[n_idx].region_id : 0;
					if (n_reg != reg) {
						start_cw = d;
						break;
					}
				}
				if (start_cw == -1) {
					continue; // Interior span, keep scanning
				}

				region_done[reg] = true;

				RegionContour contour;
				contour.region_id = reg;

				u32 curr_idx = start_idx;
				u32 curr_x = x;
				u32 curr_z = z;
				int cw_dir = start_cw;

				// Walk clockwise around the perimeter of 'reg'
				do {
					const OpenSpan &curr_span = open_spans[curr_idx];
					const u32 n_idx = curr_span.con[CW_TO_CON[cw_dir]];
					const u16 n_reg = (n_idx != UINT32_MAX) ? open_spans[n_idx].region_id : 0;

					if (n_reg != reg) {
						// Neighbor is outside our region: emit boundary vertex & turn right (+90 deg)
						ContourVert v;
						v.x = static_cast<freal>(static_cast<int>(curr_x) - static_cast<int>(CORE_MIN) + CORNER_DX[cw_dir]);
						v.z = static_cast<freal>(static_cast<int>(curr_z) - static_cast<int>(CORE_MIN) + CORNER_DZ[cw_dir]);

						const int dir_a = CW_TO_CON[cw_dir];
						const int dir_b = CW_TO_CON[(cw_dir + 1) & 3];

						freal corner_y = curr_span.y;
						const u32 n_a = curr_span.con[dir_a];
						if (n_a != UINT32_MAX) {
							corner_y = std::max(corner_y, open_spans[n_a].y);
							const u32 n_ab = open_spans[n_a].con[dir_b];
							if (n_ab != UINT32_MAX) {
								corner_y = std::max(corner_y, open_spans[n_ab].y);
							}
						}
						const u32 n_b = curr_span.con[dir_b];
						if (n_b != UINT32_MAX) {
							corner_y = std::max(corner_y, open_spans[n_b].y);
							const u32 n_ba = open_spans[n_b].con[dir_a];
							if (n_ba != UINT32_MAX) {
								corner_y = std::max(corner_y, open_spans[n_ba].y);
							}
						}
						v.y = corner_y;

						v.neighbor_reg = n_reg;

						contour.verts.push_back(v);
						cw_dir = (cw_dir + 1) & 3; // Turn clockwise (right)
					} else {
						// Neighbor is inside our region: step forward & turn left (-90 deg)
						curr_idx = n_idx;
						curr_x += CW_DX[cw_dir];
						curr_z += CW_DZ[cw_dir];
						cw_dir = (cw_dir + 3) & 3; // Turn counter-clockwise (left)
					}
				} while (curr_idx != start_idx || cw_dir != start_cw);

				if (contour.verts.size() >= 3) {
					out_contours.push_back(contour);
				}
			}
		}
	}
}

void Baker::_erode_walkable_area(freal erode_radius_voxels) {
	const u32 count = open_spans.size();

	for (u32 i = 0; i < count; ++i) {
		open_spans[i].eroded = false;
	}
	if (erode_radius_voxels <= static_cast<freal>(0)) {
		return;
	}

	Vector<freal> dist_grid;
	dist_grid.resize(count);
	for (u32 i = 0; i < count; ++i) {
		dist_grid[i] = FLT_MAX;
		open_spans[i].eroded = false;
	}

	Vector<ErodeNode> queue;
	u32 head = 0;

	const u32 grid_dim = 192;
	constexpr int dx[4] = { -1, 1, 0, 0 };
	constexpr int dz[4] = { 0, 0, -1, 1 };
	constexpr freal WALL_EPS = static_cast<freal>(0.02);
	const freal agent_height = parameters.agent_height;
	const bool erode_ledges = parameters.erode_ledges;

	// 1. Seed queue with boundary spans (walls/overhangs always; ledges only if erode_ledges is true)
	for (u32 z = 0; z < grid_dim; ++z) {
		for (u32 x = 0; x < grid_dim; ++x) {
			const OpenCell &cell = open_cells[z * grid_dim + x];
			for (u32 c = 0; c < cell.count; ++c) {
				const u32 i = cell.index + c;
				const OpenSpan &s = open_spans[i];

				bool is_boundary = false;
				for (int dir = 0; dir < 4; ++dir) {
					if (s.con[dir] != UINT32_MAX) {
						continue;
					}
					if (erode_ledges) {
						is_boundary = true;
						break;
					}

					const int nx = static_cast<int>(x) + dx[dir];
					const int nz = static_cast<int>(z) + dz[dir];
					if (nx < 0 || nx >= static_cast<int>(grid_dim) ||
							nz < 0 || nz >= static_cast<int>(grid_dim)) {
						continue;
					}

					const Line &nl = voxels.get_line(static_cast<u32>(nz) * grid_dim + static_cast<u32>(nx));
					const freal body_min = s.y + WALL_EPS;
					const freal body_max = s.y + agent_height;
					for (u32 k = 0; k < nl.spans.size(); ++k) {
						const LSpan &ns = nl.spans[k];
						if (ns.y_max > body_min && ns.y_min < body_max) {
							is_boundary = true;
							break;
						}
					}
					if (is_boundary) {
						break;
					}
				}

				if (is_boundary) {
					dist_grid[i] = 0.0f;
					open_spans[i].eroded = true;
					queue.push_back({ i, 0.0f });
				}
			}
		}
	}

	// 2. Expand inward using cardinal (1.0) and diagonal (1.414) steps
	//    Note: Diagonal reach is checked via two orthogonal cardinal steps so we don't cut through walls
	constexpr freal DIAG_COST = 1.41421356f;

	while (head < queue.size()) {
		const ErodeNode curr = queue[head++];
		if (curr.dist > dist_grid[curr.span_idx]) {
			continue; // Outdated entry
		}

		const OpenSpan &s = open_spans[curr.span_idx];

		// Cardinal neighbors (cost 1.0)
		for (int dir = 0; dir < 4; ++dir) {
			const u32 n_idx = s.con[dir];
			if (n_idx == UINT32_MAX)
				continue;

			const freal ndist = curr.dist + 1.0f;
			if (ndist < erode_radius_voxels && ndist < dist_grid[n_idx]) {
				dist_grid[n_idx] = ndist;
				open_spans[n_idx].eroded = true;
				queue.push_back({ n_idx, ndist });
			}

			// Diagonal neighbors via this cardinal neighbor (cost 1.414)
			// Directions: 0=(-X), 1=(+X), 2=(-Z), 3=(+Z)
			const int perp_a = (dir < 2) ? 2 : 0;
			const int perp_b = (dir < 2) ? 3 : 1;
			const int perps[2] = { perp_a, perp_b };

			for (int p : perps) {
				const u32 diag_idx = open_spans[n_idx].con[p];
				if (diag_idx == UINT32_MAX)
					continue;

				const freal ddist = curr.dist + DIAG_COST;
				if (ddist < erode_radius_voxels && ddist < dist_grid[diag_idx]) {
					dist_grid[diag_idx] = ddist;
					open_spans[diag_idx].eroded = true;
					queue.push_back({ diag_idx, ddist });
				}
			}
		}
	}
}

void Baker::_build_regions_monotone() {
	const u32 grid_dim = 192;
	constexpr u32 CORE_MIN = 32;
	constexpr u32 CORE_MAX = 160; // Exclusive upper bound (covers indices 32..159)

	u16 next_region_id = 1;

	// Temporary sweep buffers for a single row of spans
	// Vector<u16> prev_row_regions;
	// Vector<u16> curr_row_regions;

	for (u32 z = CORE_MIN; z < CORE_MAX; ++z) {
		// Assign region IDs row by row
		for (u32 x = CORE_MIN; x < CORE_MAX; ++x) {
			const OpenCell &cell = open_cells[z * grid_dim + x];

			for (u32 i = 0; i < cell.count; ++i) {
				const u32 span_idx = cell.index + i;
				OpenSpan &span = open_spans[span_idx];

				// Skip spans marked unwalkable during agent_radius erosion
				if (span.eroded || span.region_id != 0) {
					continue;
				}

				// 1. Walk eastward (+X) to find a contiguous horizontal run of unassigned spans
				Vector<u32> run_spans;
				u32 curr_idx = span_idx;
				u32 curr_x = x;
				while (curr_idx != UINT32_MAX && curr_x < CORE_MAX) {
					OpenSpan &s = open_spans[curr_idx];
					if (s.eroded || s.region_id != 0) {
						break;
					}
					run_spans.push_back(curr_idx);
					curr_idx = s.con[1]; // Direction 1 = (+X)
					curr_x++;
				}

				// 2. Check south (-Z, direction 2) connections of this horizontal run
				u16 candidate_region = 0;
				bool can_merge = true;

				for (u32 k = 0; k < run_spans.size(); ++k) {
					const u32 south_idx = open_spans[run_spans[k]].con[2]; // Direction 2 = (-Z)
					if (south_idx != UINT32_MAX && !open_spans[south_idx].eroded) {
						const u16 south_reg = open_spans[south_idx].region_id;
						if (south_reg != 0) {
							if (candidate_region == 0) {
								candidate_region = south_reg;
							} else if (candidate_region != south_reg) {
								// Run touches two different regions to the south -> cannot merge
								can_merge = false;
								break;
							}
						}
					}
				}

				// 3. Verify that 'candidate_region' in row (z - 1) doesn't also connect north (+Z)
				//    to any span OUTSIDE this run (prevents U-shapes / holes around pillars)
				if (can_merge && candidate_region != 0 && z > 0) {
					for (u32 px = 0; px < grid_dim; ++px) {
						const OpenCell &pcell = open_cells[(z - 1) * grid_dim + px];
						for (u32 pi = 0; pi < pcell.count; ++pi) {
							const u32 p_idx = pcell.index + pi;
							const OpenSpan &pspan = open_spans[p_idx];
							if (pspan.region_id == candidate_region) {
								const u32 north_idx = pspan.con[3]; // Direction 3 = (+Z)
								if (north_idx != UINT32_MAX && !open_spans[north_idx].eroded) {
									// Check if north_idx is part of our current run_spans
									bool in_run = false;
									for (u32 k = 0; k < run_spans.size(); ++k) {
										if (run_spans[k] == north_idx) {
											in_run = true;
											break;
										}
									}
									if (!in_run) {
										can_merge = false;
										break;
									}
								}
							}
						}
						if (!can_merge)
							break;
					}
				}

				// 4. Assign either the inherited region_id or a brand-new one
				const u16 assigned_id = (can_merge && candidate_region != 0)
						? candidate_region
						: next_region_id++;

				for (u32 k = 0; k < run_spans.size(); ++k) {
					open_spans[run_spans[k]].region_id = assigned_id;
				}
			}
		}
	}
}

void Baker::_link_open_spans(freal agent_height, freal agent_max_climb) {
	const u32 grid_dim = 192;
	constexpr int dx[4] = { -1, 1, 0, 0 };
	constexpr int dz[4] = { 0, 0, -1, 1 };

	for (u32 z = 0; z < grid_dim; ++z) {
		for (u32 x = 0; x < grid_dim; ++x) {
			const OpenCell &cell = open_cells[z * grid_dim + x];

			for (u32 i = 0; i < cell.count; ++i) {
				const u32 span_idx = cell.index + i;
				OpenSpan &span = open_spans[span_idx];
				const freal span_ceil = (span.height == FLT_MAX) ? FLT_MAX : (span.y + span.height);

				for (int dir = 0; dir < 4; ++dir) {
					const int nx = static_cast<int>(x) + dx[dir];
					const int nz = static_cast<int>(z) + dz[dir];

					// Out of tile bounds -> leave as NO_CONNECTION (acts as border)
					if (nx < 0 || nx >= static_cast<int>(grid_dim) ||
							nz < 0 || nz >= static_cast<int>(grid_dim)) {
						continue;
					}

					const OpenCell &ncell = open_cells[nz * grid_dim + nx];
					freal best_dy = FLT_MAX;
					u32 best_neighbor = UINT32_MAX;

					for (u32 j = 0; j < ncell.count; ++j) {
						const u32 n_idx = ncell.index + j;
						const OpenSpan &nspan = open_spans[n_idx];

						// 1. Check step-up / step-down height
						const freal dy = std::abs(nspan.y - span.y);
						if (dy > agent_max_climb) {
							continue;
						}

						// 2. Check doorway / lintel clearance across the transition
						const freal nspan_ceil = (nspan.height == FLT_MAX) ? FLT_MAX : (nspan.y + nspan.height);
						const freal gap_bottom = std::max(span.y, nspan.y);
						const freal gap_top = std::min(span_ceil, nspan_ceil);

						if ((gap_top - gap_bottom) >= agent_height) {
							// Pick the closest vertical floor if multiple layers are within climb range
							if (dy < best_dy) {
								best_dy = dy;
								best_neighbor = n_idx;
							}
						}
					}

					span.con[dir] = best_neighbor;
				}
			}
		}
	}
}

void Baker::_build_open_spans(freal agent_height) {
	const u32 grid_dim = 192;
	open_cells.resize(grid_dim * grid_dim);
	open_spans.clear();

	for (u32 z = 0; z < grid_dim; ++z) {
		for (u32 x = 0; x < grid_dim; ++x) {
			const u32 line_id = z * grid_dim + x;
			const Line &l = voxels.get_line(line_id);
			OpenCell &cell = open_cells[line_id];

			cell.index = open_spans.size();
			cell.count = 0;

			const u32 solid_count = l.spans.size();
			for (u32 i = 0; i < solid_count; ++i) {
				const LSpan &floor_span = l.spans[i];

				// Skip steep walls / unwalkable surfaces
				if (!floor_span.walkable) {
					continue;
				}

				const freal floor_y = floor_span.y_max;
				const freal ceiling_y = (i + 1 < solid_count) ? l.spans[i + 1].y_min : FLT_MAX;
				const freal clearance = ceiling_y - floor_y;

				// Filter out low ceilings / crawlspaces too short for the agent
				if (clearance < agent_height) {
					continue;
				}

				OpenSpan os;
				os.y = floor_y;
				os.height = clearance;
				os.con[0] = os.con[1] = os.con[2] = os.con[3] = UINT32_MAX;

				open_spans.push_back(os);
				cell.count++;
			}
		}
	}
}

void Baker::_bake_tile(Tile &r_tile, Span<u32> p_indices, Span<FPoint3> p_verts) {
	FPoint3 c[3];

	for (u32 n = 0; n < r_tile.tris.size(); n++) {
		u32 tri_id = r_tile.tris[n];

		const u32 *inds = p_indices.ptr() + (tri_id * 3);

		c[0] = p_verts[*inds++];
		c[1] = p_verts[*inds++];
		c[2] = p_verts[*inds++];

		_bake_tri(r_tile, c);
	}

	for (u32 l = 0; l < NUM_VOXELS; l++) {
		_consolidate_spans(l);
	}
}

void Baker::_bake_tri(Tile &r_tile, const FPoint3 p_c[3]) {
	Rect2 rect;
	rect.position = p_c[0].xz();
	rect.expand_to(p_c[1].xz());
	rect.expand_to(p_c[2].xz());

	// Quick reject.
	if (!rect.intersects(r_tile.expanded_rect)) {
		return;
	}

	const freal cell_size = parameters.voxel_cell_size;

	// Make the bound relative to the tile.
	rect.position -= r_tile.expanded_rect.position;
	rect.position /= cell_size;
	rect.size /= cell_size;

	IRect2 bound;
	bound.position = IPoint2::truncate(rect.position);
	bound.size = IPoint2::truncate(rect.size) + IPoint2(2, 2);

	if (!bound.clamp_from_zero(IPoint2(WIDTH_VOXELS, WIDTH_VOXELS))) {
		return;
	}

	IPoint2 end = bound.end();

	// Pre-scale the triangle vertices into voxel space as a one-off.
	FPoint3 c[3];
	for (u32 n = 0; n < 3; n++) {
		// Don't alter y, because we want the y in world space.
		c[n].x = (p_c[n].x - r_tile.expanded_rect.position.x) / cell_size;
		c[n].y = p_c[n].y;
		c[n].z = (p_c[n].z - r_tile.expanded_rect.position.y) / cell_size;
	}

	TriVoxelCache tri = prepare_tri_cache(c);

	for (u32 y = bound.position.y; y < end.y; y++) {
		for (u32 x = bound.position.x; x < end.x; x++) {
			_bake_voxel(x, y, tri);
		}
	}
}

u32 Baker::_clip_plane(const FPoint3 *in_v, u32 in_n, FPoint3 *out_v, i32 axis, freal limit, bool keep_greater) const {
	if (in_n == 0)
		return 0;
	u32 out_n = 0;

	FPoint3 prev = in_v[in_n - 1];
	freal prev_val = (axis == 0) ? prev.x : prev.z;
	bool prev_in = keep_greater ? (prev_val >= limit) : (prev_val <= limit);

	for (u32 i = 0; i < in_n; ++i) {
		FPoint3 curr = in_v[i];
		freal curr_val = (axis == 0) ? curr.x : curr.z;
		bool curr_in = keep_greater ? (curr_val >= limit) : (curr_val <= limit);

		if (curr_in != prev_in) {
			freal t = (limit - prev_val) / (curr_val - prev_val);
			out_v[out_n++] = {
				prev.x + t * (curr.x - prev.x),
				prev.y + t * (curr.y - prev.y),
				prev.z + t * (curr.z - prev.z)
			};
		}
		if (curr_in) {
			out_v[out_n++] = curr;
		}
		prev = curr;
		prev_val = curr_val;
		prev_in = curr_in;
	}
	return out_n;
}

void Baker::_bake_voxel(u32 x, u32 y, const TriVoxelCache &tri) {
	const freal cell_x = static_cast<freal>(x);
	const freal cell_z = static_cast<freal>(y);

	// 1. Cheap 3-axis SAT check (skips empty corner voxels of the triangle's AABB)
	if (!overlaps_voxel_fast(tri, cell_x, cell_z)) {
		return;
	}

	// 2. Clip triangle against the 4 vertical walls of the voxel column

	// Clipping a triangle against a voxel can end up with a polygon with at most 7 verts.
	// 3 + 4 (1 extra vert per wall).
	FPoint3 buf_a[7];
	FPoint3 buf_b[7];

	u32 count = _clip_plane(tri.p, 3, buf_a, 0, cell_x, true);
	count = _clip_plane(buf_a, count, buf_b, 0, cell_x + 1.0f, false);
	count = _clip_plane(buf_b, count, buf_a, 2, cell_z, true);
	count = _clip_plane(buf_a, count, buf_b, 2, cell_z + 1.0f, false);

	if (count == 0)
		return;

	// 3. Extract exact Y min and max inside this voxel column
	freal y_min = buf_b[0].y;
	freal y_max = buf_b[0].y;
	for (u32 i = 1; i < count; ++i) {
		y_min = MIN(y_min, buf_b[i].y);
		y_max = MAX(y_max, buf_b[i].y);
	}

	// 4. Quantize to vertical voxel spans
	//	u32 span_min = static_cast<u32>(MAX(0.0f, Math::floor_real(y_min)));
	//	u32 span_max = static_cast<u32>(MAX(0.0f, Math::ceil_real(y_max)));
	//	if (span_max == span_min) {
	//		span_max = span_min + 1;
	//	}

	// _insert_span(x, y, span_min, span_max);
	_insert_span(x, y, y_min, y_max, tri.is_walkable);
}

void Baker::_consolidate_spans(u32 p_line_id) {
	Line &l = voxels.get_line(p_line_id);

	const u32 count = l.spans.size();
	if (count <= 1) {
		return;
	}

	// 1. Sort spans from lowest y_min to highest y_min directly in-place
	l.spans.sort();

	// 2. Linearly merge overlapping or touching spans in-place
	constexpr freal MERGE_EPS = 1e-4;
	u32 write_idx = 0;

	for (u32 read_idx = 1; read_idx < count; ++read_idx) {
		LSpan &curr = l.spans[write_idx];
		const LSpan &next = l.spans[read_idx];

		if (next.y_min <= curr.y_max + MERGE_EPS) {
			// Walkability rule when merging overlapping solid spans:
			// - If 'next' extends strictly higher, the new top surface belongs to 'next'.
			// - If top heights match (e.g. a wall base touching a floor edge), unwalkable wins.
			if (Math::fabs_real(next.y_max - curr.y_max) <= MERGE_EPS) {
				curr.walkable = curr.walkable || next.walkable;
			} else if (next.y_max > curr.y_max) {
				curr.walkable = next.walkable;
			}

			if (next.y_max > curr.y_max) {
				curr.y_max = next.y_max;
			}
		} else {
			++write_idx;
			l.spans[write_idx] = next;
		}
	}

	// 3. Shrink array to final merged count (no reallocation for trivially copyable LSpan)
	l.spans.resize(write_idx + 1);
}

void Baker::_insert_span(u32 x, u32 y, freal p_min_y, freal p_max_y, bool p_walkable) {
	LSpan l;
	l.y_min = p_min_y;
	l.y_max = p_max_y;
	l.walkable = p_walkable;

	voxels.get_line_at_pos(IPoint2(x, y)).spans.push_back(l);
}

// Call once per triangle before your X/Z voxel loops
Baker::TriVoxelCache Baker::prepare_tri_cache(const FPoint3 p[3]) const {
	TriVoxelCache c;
	c.p[0] = p[0];
	c.p[1] = p[1];
	c.p[2] = p[2];

	// 1. Compute 3D triangle normal to check walkable slope
	// Note that x and y are in voxel space so have to be scaled back
	// to world space to get a valid normal.
	const freal cell_size = parameters.voxel_cell_size;
	const freal e1x = (p[1].x - p[0].x) * cell_size;
	const freal e1y = p[1].y - p[0].y;
	const freal e1z = (p[1].z - p[0].z) * cell_size;
	const freal e2x = (p[2].x - p[0].x) * cell_size;
	const freal e2y = p[2].y - p[0].y;
	const freal e2z = (p[2].z - p[0].z) * cell_size;

	const freal norm_x = e1y * e2z - e1z * e2y;
	const freal norm_y = e1z * e2x - e1x * e2z;
	const freal norm_z = e1x * e2y - e1y * e2x;

	const freal max_slope_deg = parameters.walkable_slope;

	const freal len = Math::sqrt_real(norm_x * norm_x + norm_y * norm_y + norm_z * norm_z);
	const freal cos_max_slope = Math::cos_real(max_slope_deg * static_cast<freal>(Math::NP_PI / 180.0));

	// Godot 3.x standard CW front faces have negative norm_y when facing +Y.
	// (Use +norm_y instead of -norm_y if feeding CCW triangles.)
	const freal up_y = parameters.input_is_counter_clockwise ? norm_y : -norm_y;

	// If your level geometry has clean outward-facing normals,
	// check the signed up-component so undersides of ceilings aren't marked walkable.
	// Using abs(norm_y) works if you also filter headroom in _build_open_spans:
	c.is_walkable = (len > (freal)(1e-6)) && ((Math::fabs_real(up_y) / len) >= cos_max_slope);

	for (i32 i = 0; i < 3; ++i) {
		i32 next = (i + 1) % 3;
		freal nx = -(p[next].z - p[i].z);
		freal nz = (p[next].x - p[i].x);

		c.nx[i] = nx;
		c.nz[i] = nz;
		c.box_r[i] = 0.5f * (Math::fabs_real(nx) + Math::fabs_real(nz));

		freal d0 = nx * p[0].x + nz * p[0].z;
		freal d1 = nx * p[1].x + nz * p[1].z;
		freal d2 = nx * p[2].x + nz * p[2].z;

		c.tri_min[i] = MIN(MIN(d0, d1), d2);
		c.tri_max[i] = MAX(MAX(d0, d1), d2);
	}
	return c;
}

// Ultra-cheap per-voxel overlap check (no AABB, no std::abs, just 3 dot products)
bool Baker::overlaps_voxel_fast(const TriVoxelCache &c, freal cell_x, freal cell_z) const {
	const freal cx = cell_x + 0.5f;
	const freal cz = cell_z + 0.5f;

	for (i32 i = 0; i < 3; ++i) {
		// Shift precomputed triangle interval by the cell center's projection
		freal center_proj = c.nx[i] * cx + c.nz[i] * cz;
		if ((c.tri_min[i] - center_proj) > c.box_r[i] ||
				(c.tri_max[i] - center_proj) < -c.box_r[i]) {
			return false;
		}
	}
	return true;
}

void Baker::bake(const Parameters &p_parameters, Span<u32> p_indices, Span<FPoint3> p_verts, NavMeshData &r_out_mesh) {
	if (!p_verts.size() || !p_indices.size()) {
		return;
	}

	// Store parameters locally rather than pass as an argument.
	parameters = p_parameters;

	// Prevent artefacts by scaling max y deviation according to the step size.
	parameters.max_y_deviation = parameters.max_climb * 0.6;

	NP_ERR_FAIL_COND((p_indices.size() % 3) != 0);
	u32 num_tris = p_indices.size() / 3;

	// Find find the overall AABB.
	AABB overall_aabb;
	overall_aabb.position = p_verts[0];

	for (u32 n = 0; n < p_indices.size(); n++) {
		overall_aabb.expand_to(p_verts[p_indices[n]]);
	}

	// The tile width, margin are derived from the voxel size,
	// which is the only thing the user has to set.
	data.tile_width = parameters.voxel_cell_size * 128;
	data.tile_width_expanded = data.tile_width * 1.5;
	freal tile_width = data.tile_width;
	const freal tile_margin = tile_width * 0.25;

	// Calculate how many tiles, and where they should be.
	data.tile_dims.x = (overall_aabb.size.x / tile_width) + 1;
	data.tile_dims.y = (overall_aabb.size.z / tile_width) + 1;

	log(String("Baker: tile dims ") + data.tile_dims);

	u32 total_tiles = data.tile_dims.x * data.tile_dims.y;
	data.tiles.clear();
	data.tiles.resize(total_tiles);

	// Initialize the tiles.
	for (u32 y = 0; y < data.tile_dims.y; y++) {
		for (u32 x = 0; x < data.tile_dims.x; x++) {
			Tile &tile = data.get_tile(IPoint2(x, y));
			tile.rect.position = overall_aabb.position.xz();
			tile.rect.position.x += tile_width * x;
			tile.rect.position.y += tile_width * y;
			tile.rect.size = FPoint2(tile_width, tile_width);

			tile.expanded_rect = tile.rect;
			tile.expanded_rect.position -= FPoint2(tile_margin, tile_margin);
			tile.expanded_rect.size += FPoint2(tile_margin * 2, tile_margin * 2);
		}
	}

	Vector<AABB> tri_aabbs;
	tri_aabbs.resize(num_tris);

	const u32 *inds = p_indices.ptr();

	for (u32 n = 0; n < num_tris; n++) {
		u32 i0 = *inds++;
		u32 i1 = *inds++;
		u32 i2 = *inds++;

		const FPoint3 &a = p_verts[i0];
		const FPoint3 &b = p_verts[i1];
		const FPoint3 &c = p_verts[i2];

		AABB &tri_bb = tri_aabbs[n];
		tri_bb.position = a;
		tri_bb.expand_to(b);
		tri_bb.expand_to(c);

		// Expand the triangle bb by the tile margin in worldspace.
		tri_bb.grow(tile_margin);

		// Make the tri_bb relative to the overall_bb.
		tri_bb.position.x -= overall_aabb.position.x;
		tri_bb.position.z -= overall_aabb.position.z;

		// Separate into bins.
		IPoint2 tri_start = IPoint2::truncate(tri_bb.position.xz() / FPoint2(tile_width, tile_width));
		IPoint2 tri_end = IPoint2::truncate(tri_bb.end().xz() / FPoint2(tile_width, tile_width));

		//tri_start -= IPoint2::one();
		tri_end += IPoint2::one();

		tri_start.clamp(IPoint2::constant_zero(), data.tile_dims - IPoint2::one());
		tri_end.clamp(IPoint2::constant_zero(), data.tile_dims);

		for (u32 y = tri_start.y; y < tri_end.y; y++) {
			for (u32 x = tri_start.x; x < tri_end.x; x++) {
				data.get_tile(IPoint2(x, y)).tris.push_back(n);
			}
		}

	} // for n

	// Shared weld map across ALL tiles so adjacent tiles share exact vertex indices along seams
	std::unordered_map<WeldKey, u32, WeldKeyHash> weld_map;
	r_out_mesh.clear();

	// Bake the tiles
	for (u32 y = 0; y < data.tile_dims.y; y++) {
		for (u32 x = 0; x < data.tile_dims.x; x++) {
			Tile &tile = data.get_tile(IPoint2(x, y));

			voxels.clear();

			log(String("Baker: baking tile ") + IPoint2(x, y) + ", contains " + tile.tris.size() + " tris.");
			_bake_tile(tile, p_indices, p_verts);

			_build_open_spans(parameters.agent_height);
			_link_open_spans(parameters.agent_height, parameters.max_climb);

			// Measured in voxels.
			_erode_walkable_area(parameters.agent_radius / parameters.voxel_cell_size);
			_build_regions_monotone();

			// Merge regions smaller than 16 voxels.
			_merge_small_regions(16);

			freal cell_size = parameters.voxel_cell_size;

			// Trace perimeter contours and simplify jagged voxel edges
			_trace_region_contours(contours);
			for (u32 i = 0; i < contours.size(); ++i) {
				// Default 1.3 voxels irons out diagonal stair-stepping cleanly.
				_simplify_contour(contours[i], parameters.contour_max_error_voxels, parameters.max_y_deviation);
			}

			// 4. Triangulate, merge into max-6-gon convex polys, transform to world space, and weld into r_out_mesh
			// Note: tile.rect.pos.x and tile.rect.pos.y must be the world X and Z origin of the inner 128x128 core

			_build_tile_polygons(
					r_out_mesh,
					weld_map,
					tile.rect.position.x,
					tile.rect.position.y,
					cell_size,
					parameters.max_verts_per_poly,
					parameters.max_y_deviation);
		}
	}

	const freal post_y_eps = MIN(
			parameters.agent_height * static_cast<freal>(0.5),
			MAX(parameters.max_y_deviation, parameters.max_climb) + static_cast<freal>(0.05));

	PostProcess pp;
	pp.process(r_out_mesh, parameters.max_verts_per_poly, static_cast<freal>(1e-3), post_y_eps);
}

} //namespace NavPhysics
