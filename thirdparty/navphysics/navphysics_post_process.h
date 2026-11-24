// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#pragma once

#include "navphysics_baker.h"

namespace NavPhysics {

class PostProcess {
	struct OpenVert {
		u32 v_idx = 0;
		freal x = 0;
		bool operator<(const OpenVert &p_o) const {
			return x < p_o.x;
		}
	};
	
	struct SplitVert {
		u32 v_idx = 0;
		freal t = 0;
		bool operator<(const SplitVert &p_o) const {
			return t < p_o.t;
		}
	};
	
	u64 _make_edge_key(u32 p_a, u32 p_b) const;
	freal _cross_xz(const FPoint3 &p_a, const FPoint3 &p_b, const FPoint3 &p_c) const;
	bool _segments_intersect_xz(const FPoint3 &p_a, const FPoint3 &p_b, const FPoint3 &p_c, const FPoint3 &p_d) const;
	bool _in_cone_xz(const Vector<u32> &p_poly, const Vector<FPoint3> &p_verts, freal p_sign, u32 p_i, u32 p_j) const;
	freal _subpoly_area2_xz(const Vector<u32> &p_poly, const Vector<FPoint3> &p_verts, u32 p_start_idx, u32 p_end_idx) const;
	bool _is_diagonal_valid_xz(const Vector<u32> &p_poly, const Vector<FPoint3> &p_verts, freal p_sign, u32 p_i, u32 p_j) const;
	
public:
	void remove_t_junctions(Baker::NavMeshData &r_mesh, freal p_xz_eps = 1e-3, freal p_y_eps = 0.05) const;
	void enforce_strict_convexity(Baker::NavMeshData &r_mesh, freal p_min_sin_angle = 1e-3) const;
	void cap_poly_vertices(Baker::NavMeshData &r_mesh, u32 p_max_verts_per_poly = 6) const;
	
	void process(Baker::NavMeshData &r_mesh, u32 p_max_verts_per_poly = 6, freal p_xz_eps = 1e-3, freal p_y_eps = 0.05, freal p_min_sin_angle = 1e-3) const {
		remove_t_junctions(r_mesh, p_xz_eps, p_y_eps);
		enforce_strict_convexity(r_mesh, p_min_sin_angle);
		cap_poly_vertices(r_mesh, p_max_verts_per_poly);
	}
};

} // namespace NavPhysics
