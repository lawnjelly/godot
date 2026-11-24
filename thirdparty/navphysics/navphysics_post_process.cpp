// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#include "navphysics_post_process.h"

namespace NavPhysics {

void PostProcess::enforce_strict_convexity(Baker::NavMeshData &r_mesh, freal p_min_sin_angle) const {
	if (r_mesh.polys.is_empty() || r_mesh.vertices.is_empty()) {
		return;
	}

	Vector<u32> new_indices;
	Vector<Baker::NavPoly> new_polys;
	Vector<Vector<u32>> pending;
	Vector<bool> is_bad;

	for (u32 p = 0; p < r_mesh.polys.size(); ++p) {
		const Baker::NavPoly &poly = r_mesh.polys[p];
		if (poly.num_indices < 3) {
			continue;
		}

		pending.clear();
		Vector<u32> root;
		root.resize(poly.num_indices);
		for (u32 k = 0; k < poly.num_indices; ++k) {
			root[k] = r_mesh.indices[poly.first_index + k];
		}
		pending.push_back(root);

		while (!pending.is_empty()) {
			const u32 last_idx = pending.size() - 1;
			Vector<u32> curr = pending[last_idx];
			pending.remove(last_idx);

			const u32 m = curr.size();
			if (m < 3) {
				continue;
			}

			freal area2 = 0;
			for (u32 k = 0; k < m; ++k) {
				const FPoint3 &a = r_mesh.vertices[curr[k]];
				const FPoint3 &b = r_mesh.vertices[curr[(k + 1) % m]];
				area2 += (a.x * b.z - b.x * a.z);
			}
			if (std::abs(area2) <= static_cast<freal>(1e-8)) {
				continue;
			}

			if (m == 3) {
				Baker::NavPoly out_poly;
				out_poly.first_index = new_indices.size();
				out_poly.num_indices = 3;
				for (u32 k = 0; k < 3; ++k) {
					new_indices.push_back(curr[k]);
				}
				new_polys.push_back(out_poly);
				continue;
			}

			const freal sign = (area2 > 0) ? static_cast<freal>(1.0) : static_cast<freal>(-1.0);

			// Flag vertices that are collinear or reflex in XZ
			is_bad.resize(m);
			bool any_bad = false;
			for (u32 i = 0; i < m; ++i) {
				const FPoint3 &a = r_mesh.vertices[curr[(i + m - 1) % m]];
				const FPoint3 &b = r_mesh.vertices[curr[i]];
				const FPoint3 &c = r_mesh.vertices[curr[(i + 1) % m]];

				const freal abx = b.x - a.x;
				const freal abz = b.z - a.z;
				const freal bcx = c.x - b.x;
				const freal bcz = c.z - b.z;
				const freal len_ab = std::sqrt(abx * abx + abz * abz);
				const freal len_bc = std::sqrt(bcx * bcx + bcz * bcz);

				bool bad = false;
				if (len_ab <= static_cast<freal>(1e-6) || len_bc <= static_cast<freal>(1e-6)) {
					bad = true;
				} else {
					const freal sin_turn = ((abx * bcz - abz * bcx) * sign) / (len_ab * len_bc);
					if (sin_turn <= p_min_sin_angle) {
						bad = true;
					}
				}

				is_bad[i] = bad;
				if (bad) {
					any_bad = true;
				}
			}

			if (!any_bad) {
				Baker::NavPoly out_poly;
				out_poly.first_index = new_indices.size();
				out_poly.num_indices = m;
				for (u32 k = 0; k < m; ++k) {
					new_indices.push_back(curr[k]);
				}
				new_polys.push_back(out_poly);
				continue;
			}

			// Find best valid split diagonal starting at a bad vertex.
			// Prefer diagonals connecting two bad vertices (fixes two at once), then shortest diagonal.
			u32 best_i = UINT32_MAX;
			u32 best_j = UINT32_MAX;
			bool best_both_bad = false;
			freal best_len_sq = FLT_MAX;

			for (u32 i = 0; i < m; ++i) {
				if (!is_bad[i]) {
					continue;
				}
				const u32 prev_i = (i + m - 1) % m;
				const u32 next_i = (i + 1) % m;

				for (u32 j = 0; j < m; ++j) {
					if (j == i || j == prev_i || j == next_i) {
						continue;
					}

					const bool both_bad = is_bad[j];
					if (best_both_bad && !both_bad) {
						continue;
					}

					const FPoint3 &a = r_mesh.vertices[curr[i]];
					const FPoint3 &b = r_mesh.vertices[curr[j]];
					const freal dx = b.x - a.x;
					const freal dz = b.z - a.z;
					const freal len_sq = dx * dx + dz * dz;

					if (both_bad == best_both_bad && len_sq >= best_len_sq) {
						continue;
					}

					if (_is_diagonal_valid_xz(curr, r_mesh.vertices, sign, i, j)) {
						best_i = i;
						best_j = j;
						best_both_bad = both_bad;
						best_len_sq = len_sq;
					}
				}
			}

			if (best_i == UINT32_MAX) {
				// No valid internal diagonal (only possible on near-degenerate slivers); emit as-is
				Baker::NavPoly out_poly;
				out_poly.first_index = new_indices.size();
				out_poly.num_indices = m;
				for (u32 k = 0; k < m; ++k) {
					new_indices.push_back(curr[k]);
				}
				new_polys.push_back(out_poly);
				continue;
			}

			u32 lo = best_i;
			u32 hi = best_j;
			if (lo > hi) {
				const u32 tmp = lo;
				lo = hi;
				hi = tmp;
			}

			Vector<u32> sub1;
			for (u32 k = lo; k <= hi; ++k) {
				sub1.push_back(curr[k]);
			}

			Vector<u32> sub2;
			for (u32 k = hi; k < m; ++k) {
				sub2.push_back(curr[k]);
			}
			for (u32 k = 0; k <= lo; ++k) {
				sub2.push_back(curr[k]);
			}

			pending.push_back(sub2);
			pending.push_back(sub1);
		}
	}

	r_mesh.indices = new_indices;
	r_mesh.polys = new_polys;
}

u64 PostProcess::_make_edge_key(u32 p_a, u32 p_b) const {
	return (static_cast<u64>(p_a) << 32) | static_cast<u64>(p_b);
}

freal PostProcess::_cross_xz(const FPoint3 &p_a, const FPoint3 &p_b, const FPoint3 &p_c) const {
	return (p_b.x - p_a.x) * (p_c.z - p_a.z) - (p_b.z - p_a.z) * (p_c.x - p_a.x);
}

bool PostProcess::_segments_intersect_xz(const FPoint3 &p_a, const FPoint3 &p_b, const FPoint3 &p_c, const FPoint3 &p_d) const {
	const freal eps = static_cast<freal>(1e-6);
	if (MAX(p_a.x, p_b.x) < MIN(p_c.x, p_d.x) - eps || MIN(p_a.x, p_b.x) > MAX(p_c.x, p_d.x) + eps ||
			MAX(p_a.z, p_b.z) < MIN(p_c.z, p_d.z) - eps || MIN(p_a.z, p_b.z) > MAX(p_c.z, p_d.z) + eps) {
		return false;
	}

	const freal d1 = _cross_xz(p_a, p_b, p_c);
	const freal d2 = _cross_xz(p_a, p_b, p_d);
	const freal d3 = _cross_xz(p_c, p_d, p_a);
	const freal d4 = _cross_xz(p_c, p_d, p_b);

	const bool straddle_ab = (d1 >= 0 && d2 <= 0) || (d1 <= 0 && d2 >= 0);
	const bool straddle_cd = (d3 >= 0 && d4 <= 0) || (d3 <= 0 && d4 >= 0);
	return straddle_ab && straddle_cd;
}

bool PostProcess::_in_cone_xz(const Vector<u32> &p_poly, const Vector<FPoint3> &p_verts, freal p_sign, u32 p_i, u32 p_j) const {
	const u32 m = p_poly.size();
	const FPoint3 &prev = p_verts[p_poly[(p_i + m - 1) % m]];
	const FPoint3 &curr = p_verts[p_poly[p_i]];
	const FPoint3 &next = p_verts[p_poly[(p_i + 1) % m]];
	const FPoint3 &target = p_verts[p_poly[p_j]];

	const freal eps = static_cast<freal>(1e-8);

	// Convex or 180-deg collinear vertex
	if (_cross_xz(prev, curr, next) * p_sign >= -eps) {
		return (_cross_xz(curr, next, target) * p_sign > eps) &&
				(_cross_xz(prev, curr, target) * p_sign > eps);
	}

	// Reflex (concave) vertex
	return !((_cross_xz(curr, prev, target) * p_sign >= -eps) &&
			(_cross_xz(next, curr, target) * p_sign >= -eps));
}

freal PostProcess::_subpoly_area2_xz(const Vector<u32> &p_poly, const Vector<FPoint3> &p_verts, u32 p_start_idx, u32 p_end_idx) const {
	const u32 m = p_poly.size();
	freal area2 = 0;
	u32 curr = p_start_idx;
	while (curr != p_end_idx) {
		const u32 next = (curr + 1) % m;
		const FPoint3 &a = p_verts[p_poly[curr]];
		const FPoint3 &b = p_verts[p_poly[next]];
		area2 += (a.x * b.z - b.x * a.z);
		curr = next;
	}
	const FPoint3 &a = p_verts[p_poly[p_end_idx]];
	const FPoint3 &b = p_verts[p_poly[p_start_idx]];
	area2 += (a.x * b.z - b.x * a.z);
	return area2;
}

bool PostProcess::_is_diagonal_valid_xz(const Vector<u32> &p_poly, const Vector<FPoint3> &p_verts, freal p_sign, u32 p_i, u32 p_j) const {
	const u32 m = p_poly.size();

	// 1. Diagonal must enter the interior at both endpoints
	if (!_in_cone_xz(p_poly, p_verts, p_sign, p_i, p_j) || !_in_cone_xz(p_poly, p_verts, p_sign, p_j, p_i)) {
		return false;
	}

	// 2. Both resulting sub-polygons must have non-degenerate area (prevents collinear slivers)
	const freal min_area2 = static_cast<freal>(1e-7);
	if (_subpoly_area2_xz(p_poly, p_verts, p_i, p_j) * p_sign <= min_area2 ||
			_subpoly_area2_xz(p_poly, p_verts, p_j, p_i) * p_sign <= min_area2) {
		return false;
	}

	// 3. Diagonal must not cross or touch any non-adjacent polygon edge
	const FPoint3 &a = p_verts[p_poly[p_i]];
	const FPoint3 &b = p_verts[p_poly[p_j]];
	for (u32 k = 0; k < m; ++k) {
		const u32 k_next = (k + 1) % m;
		if (k == p_i || k_next == p_i || k == p_j || k_next == p_j) {
			continue;
		}
		if (_segments_intersect_xz(a, b, p_verts[p_poly[k]], p_verts[p_poly[k_next]])) {
			return false;
		}
	}

	return true;
}

void PostProcess::remove_t_junctions(Baker::NavMeshData &r_mesh, freal p_xz_eps, freal p_y_eps) const {
	if (r_mesh.polys.is_empty() || r_mesh.vertices.is_empty()) {
		return;
	}

	// 1. Collect all directed edges across the entire mesh
	std::unordered_map<u64, u8> directed_edges;
	directed_edges.reserve(r_mesh.indices.size());

	for (u32 p = 0; p < r_mesh.polys.size(); ++p) {
		const Baker::NavPoly &poly = r_mesh.polys[p];
		for (u32 k = 0; k < poly.num_indices; ++k) {
			const u32 u = r_mesh.indices[poly.first_index + k];
			const u32 v = r_mesh.indices[poly.first_index + ((k + 1) % poly.num_indices)];
			directed_edges[_make_edge_key(u, v)] = 1;
		}
	}

	// 2. Collect unique vertices belonging to unmatched edges (outer borders & T-junction seams)
	Vector<bool> is_open_vert;
	is_open_vert.resize(r_mesh.vertices.size());
	for (u32 i = 0; i < is_open_vert.size(); ++i) {
		is_open_vert[i] = false;
	}

	Vector<OpenVert> open_verts;
	for (u32 p = 0; p < r_mesh.polys.size(); ++p) {
		const Baker::NavPoly &poly = r_mesh.polys[p];
		for (u32 k = 0; k < poly.num_indices; ++k) {
			const u32 u = r_mesh.indices[poly.first_index + k];
			const u32 v = r_mesh.indices[poly.first_index + ((k + 1) % poly.num_indices)];
			if (directed_edges.count(_make_edge_key(v, u)) == 0) {
				if (!is_open_vert[u]) {
					is_open_vert[u] = true;
					open_verts.push_back({ u, r_mesh.vertices[u].x });
				}
				if (!is_open_vert[v]) {
					is_open_vert[v] = true;
					open_verts.push_back({ v, r_mesh.vertices[v].x });
				}
			}
		}
	}

	if (open_verts.is_empty()) {
		return;
	}

	open_verts.sort();

	const freal xz_eps_sq = p_xz_eps * p_xz_eps;
	Vector<SplitVert> splits;
	Vector<u32> new_indices;

	// 3. Rebuild flat indices buffer while inserting collinear T-junction vertices
	for (u32 p = 0; p < r_mesh.polys.size(); ++p) {
		Baker::NavPoly &poly = r_mesh.polys[p];
		const u32 old_first = poly.first_index;
		const u32 old_count = poly.num_indices;
		const u32 new_first = new_indices.size();

		for (u32 k = 0; k < old_count; ++k) {
			const u32 u = r_mesh.indices[old_first + k];
			const u32 v = r_mesh.indices[old_first + ((k + 1) % old_count)];

			new_indices.push_back(u);

			// Skip edges that already have an exact twin (v -> u)
			if (directed_edges.count(_make_edge_key(v, u)) != 0) {
				continue;
			}

			const FPoint3 &A = r_mesh.vertices[u];
			const FPoint3 &B = r_mesh.vertices[v];
			const freal dx = B.x - A.x;
			const freal dz = B.z - A.z;
			const freal len_sq = dx * dx + dz * dz;
			if (len_sq <= static_cast<freal>(1e-10)) {
				continue;
			}

			const freal min_x = MIN(A.x, B.x) - p_xz_eps;
			const freal max_x = MAX(A.x, B.x) + p_xz_eps;
			const freal min_z = MIN(A.z, B.z) - p_xz_eps;
			const freal max_z = MAX(A.z, B.z) + p_xz_eps;

			u32 lo = 0;
			u32 hi = open_verts.size();
			while (lo < hi) {
				const u32 mid = lo + (hi - lo) / 2;
				if (open_verts[mid].x < min_x) {
					lo = mid + 1;
				} else {
					hi = mid;
				}
			}

			splits.clear();
			for (u32 ov_i = lo; ov_i < open_verts.size(); ++ov_i) {
				const OpenVert &ov = open_verts[ov_i];
				if (ov.x > max_x) {
					break;
				}
				const u32 c_idx = ov.v_idx;
				if (c_idx == u || c_idx == v) {
					continue;
				}

				const FPoint3 &P = r_mesh.vertices[c_idx];
				if (P.z < min_z || P.z > max_z) {
					continue;
				}

				const freal da_sq = (P.x - A.x) * (P.x - A.x) + (P.z - A.z) * (P.z - A.z);
				const freal db_sq = (P.x - B.x) * (P.x - B.x) + (P.z - B.z) * (P.z - B.z);
				if (da_sq <= xz_eps_sq || db_sq <= xz_eps_sq) {
					continue;
				}

				const freal t = ((P.x - A.x) * dx + (P.z - A.z) * dz) / len_sq;
				if (t <= 0 || t >= 1) {
					continue;
				}

				const freal proj_x = A.x + t * dx;
				const freal proj_z = A.z + t * dz;
				const freal perp_sq = (P.x - proj_x) * (P.x - proj_x) + (P.z - proj_z) * (P.z - proj_z);
				if (perp_sq > xz_eps_sq) {
					continue;
				}

				const freal proj_y = A.y + t * (B.y - A.y);
				if (std::abs(P.y - proj_y) > p_y_eps) {
					continue;
				}

				splits.push_back({ c_idx, t });
			}

			if (!splits.is_empty()) {
				splits.sort();
				for (u32 s = 0; s < splits.size(); ++s) {
					new_indices.push_back(splits[s].v_idx);
				}
			}
		}

		poly.first_index = new_first;
		poly.num_indices = new_indices.size() - new_first;
	}

	r_mesh.indices = new_indices;
}

void PostProcess::cap_poly_vertices(Baker::NavMeshData &r_mesh, u32 p_max_verts_per_poly) const {
	if (r_mesh.polys.is_empty() || r_mesh.vertices.is_empty()) {
		return;
	}

	const u32 max_verts = MAX(static_cast<u32>(3), p_max_verts_per_poly);

	bool any_oversized = false;
	for (u32 p = 0; p < r_mesh.polys.size(); ++p) {
		if (r_mesh.polys[p].num_indices > max_verts) {
			any_oversized = true;
			break;
		}
	}
	if (!any_oversized) {
		return;
	}

	Vector<u32> new_indices;
	Vector<Baker::NavPoly> new_polys;
	Vector<Vector<u32>> pending;

	for (u32 p = 0; p < r_mesh.polys.size(); ++p) {
		const Baker::NavPoly &poly = r_mesh.polys[p];

		if (poly.num_indices <= max_verts) {
			Baker::NavPoly out_poly;
			out_poly.first_index = new_indices.size();
			out_poly.num_indices = poly.num_indices;
			for (u32 k = 0; k < poly.num_indices; ++k) {
				new_indices.push_back(r_mesh.indices[poly.first_index + k]);
			}
			new_polys.push_back(out_poly);
			continue;
		}

		pending.clear();
		Vector<u32> root;
		root.resize(poly.num_indices);
		for (u32 k = 0; k < poly.num_indices; ++k) {
			root[k] = r_mesh.indices[poly.first_index + k];
		}
		pending.push_back(root);

		while (!pending.is_empty()) {
			const u32 last_idx = pending.size() - 1;
			Vector<u32> curr = pending[last_idx];
			pending.remove(last_idx);

			const u32 m = curr.size();
			if (m < 3) {
				continue;
			}
			if (m <= max_verts) {
				Baker::NavPoly out_poly;
				out_poly.first_index = new_indices.size();
				out_poly.num_indices = m;
				for (u32 k = 0; k < m; ++k) {
					new_indices.push_back(curr[k]);
				}
				new_polys.push_back(out_poly);
				continue;
			}

			freal area2 = 0;
			for (u32 k = 0; k < m; ++k) {
				const FPoint3 &a = r_mesh.vertices[curr[k]];
				const FPoint3 &b = r_mesh.vertices[curr[(k + 1) % m]];
				area2 += (a.x * b.z - b.x * a.z);
			}
			if (std::abs(area2) <= static_cast<freal>(1e-8)) {
				continue;
			}
			const freal sign = (area2 > 0) ? static_cast<freal>(1.0) : static_cast<freal>(-1.0);

			u32 best_i = UINT32_MAX;
			u32 best_j = UINT32_MAX;
			u32 best_imbalance = UINT32_MAX;
			freal best_len_sq = FLT_MAX;

			for (u32 i = 0; i < m; ++i) {
				for (u32 j = i + 2; j < m; ++j) {
					if (i == 0 && j == m - 1) {
						continue;
					}

					const u32 sub1_count = (j - i) + 1;
					const u32 sub2_count = (m - (j - i)) + 1;
					const u32 imbalance = (sub1_count > sub2_count) ? (sub1_count - sub2_count) : (sub2_count - sub1_count);

					if (imbalance > best_imbalance) {
						continue;
					}

					const FPoint3 &a = r_mesh.vertices[curr[i]];
					const FPoint3 &b = r_mesh.vertices[curr[j]];
					const freal dx = b.x - a.x;
					const freal dz = b.z - a.z;
					const freal len_sq = dx * dx + dz * dz;

					if (imbalance == best_imbalance && len_sq >= best_len_sq) {
						continue;
					}

					if (_is_diagonal_valid_xz(curr, r_mesh.vertices, sign, i, j)) {
						best_i = i;
						best_j = j;
						best_imbalance = imbalance;
						best_len_sq = len_sq;
					}
				}
			}

			if (best_i == UINT32_MAX) {
				Baker::NavPoly out_poly;
				out_poly.first_index = new_indices.size();
				out_poly.num_indices = m;
				for (u32 k = 0; k < m; ++k) {
					new_indices.push_back(curr[k]);
				}
				new_polys.push_back(out_poly);
				continue;
			}

			Vector<u32> sub1;
			for (u32 k = best_i; k <= best_j; ++k) {
				sub1.push_back(curr[k]);
			}

			Vector<u32> sub2;
			for (u32 k = best_j; k < m; ++k) {
				sub2.push_back(curr[k]);
			}
			for (u32 k = 0; k <= best_i; ++k) {
				sub2.push_back(curr[k]);
			}

			pending.push_back(sub2);
			pending.push_back(sub1);
		}
	}

	r_mesh.indices = new_indices;
	r_mesh.polys = new_polys;
}

} // namespace NavPhysics
