// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#pragma once

#include "navphysics_defines.h"
#include "navphysics_plane.h"
#include "navphysics_pointf.h"
#include "navphysics_pointi.h"
#include "navphysics_rect.h"
#include "navphysics_span.h"
#include "navphysics_vector.h"

namespace NavPhysics {

// Navmesh generation is inspired by Mikko Mononen's Recast Navigation (zlib license).
// Especially voxellizing to 2D heightfield spans, partitioning into monotone regions,
// tracing and simplifying contours, and triangulating via ear-clipping.

class Baker {
	static constexpr u32 WIDTH_VOXELS = 192;
	static constexpr u32 NUM_VOXELS = WIDTH_VOXELS * WIDTH_VOXELS;

public:
	// FINAL OUTPUT
	struct NavPoly {
		u32 first_index = 0;
		u32 num_indices = 0;
	};

	struct NavMeshData {
		Vector<FPoint3> vertices;
		Vector<u32> indices;
		Vector<NavPoly> polys;
		void clear() {
			vertices.clear();
			indices.clear();
			polys.clear();
		}
	};

	struct Parameters {
		freal voxel_cell_size = 5.0 / 128;
		freal agent_radius = 0.25;
		freal agent_height = 1.0;
		freal max_climb = 0.25;
		freal walkable_slope = 45;
		u32 max_verts_per_poly = 6;
		freal max_y_deviation = 0.08; // 0.02
		freal contour_max_error_voxels = 1.3;
		freal contour_max_edge_length = 0; // 0 is disabled.
		bool input_is_counter_clockwise = false;
		bool erode_ledges = false;
	};

private:
	struct Tri {
		u32 v[3];
	};

	struct WeldKey {
		i32 x, y, z;
		bool operator==(const WeldKey &o) const { return x == o.x && y == o.y && z == o.z; }
	};

	struct WeldKeyHash {
		size_t operator()(const WeldKey &k) const {
			return (static_cast<size_t>(k.x) * 73856093u) ^
					(static_cast<size_t>(k.y) * 19349663u) ^
					(static_cast<size_t>(k.z) * 83492791u);
		}
	};

	/////////////

	struct ContourVert {
		freal x = 0; // Core tile X in [0 .. 128]
		freal y = 0; // Exact float height
		freal z = 0; // Core tile Z in [0 .. 128]
		u16 neighbor_reg = 0; // Neighbor region_id across this edge (0 = solid wall / outer tile border)
	};

	struct RegionContour {
		u16 region_id = 0;
		Vector<ContourVert> verts;
	};

	struct ErodeNode {
		u32 span_idx = 0;
		freal dist = 0; // Accumulated distance in voxel units from nearest wall/ledge
	};

	struct OpenSpan {
		freal y = 0; // Floor height (top of the solid span beneath this open space)
		freal height = 0; // Vertical clearance up to the ceiling (FLT_MAX if open sky)
		u32 con[4] = {}; // Neighbor OpenSpan index in directions: 0=(-X), 1=(+X), 2=(-Z), 3=(+Z)
						 // Use UINT32_MAX (or 0xFFFFFFFF) to mean "no connection / wall / drop"
		u16 region_id = 0;
		bool eroded = false;
		OpenSpan() {
			con[0] = UINT32_MAX;
			con[1] = UINT32_MAX;
			con[2] = UINT32_MAX;
			con[3] = UINT32_MAX;
		}
	};

	struct OpenCell {
		u32 index = 0; // Starting index in the flat OpenSpan array
		u32 count = 0; // Number of stacked walkable floors in this column
	};

	Vector<OpenSpan> open_spans;
	Vector<OpenCell> open_cells;
	Vector<RegionContour> contours; // Reused per tile

	struct LSpan {
		freal y_min = 0;
		freal y_max = 0;
		bool walkable = false;
		bool operator<(const LSpan &p_o) const {
			if (y_min != p_o.y_min) {
				return y_min < p_o.y_min;
			}
			return y_max < p_o.y_max;
		}
	};

	struct Line {
		Vector<LSpan> spans;
	};

	// Precomputed once per triangle before looping over its 2D bounding box
	struct TriVoxelCache {
		FPoint3 p[3];
		freal nx[3];
		freal nz[3]; // 2D edge normals
		freal box_r[3]; // Projected half-radius of a 1x1 voxel cell for each normal
		freal tri_min[3]; // Triangle's min projection on each normal (at origin)
		freal tri_max[3]; // Triangle's max projection on each normal (at origin)
		bool is_walkable = false;
	};

	class BitVoxels {
		Line lines[NUM_VOXELS];
		u32 which_line(u32 p_x, u32 p_y) const {
			NP_DEV_ASSERT(p_x < WIDTH_VOXELS);
			NP_DEV_ASSERT(p_y < WIDTH_VOXELS);
			return (p_y * WIDTH_VOXELS) + p_x;
		}

	public:
		Line &get_line(u32 p_id) {
			return lines[p_id];
		}
		const Line &get_line(u32 p_id) const {
			return lines[p_id];
		}
		Line &get_line_at_pos(const IPoint2 &p_pos) {
			u32 l = which_line(p_pos.x, p_pos.y);
			return lines[l];
		}
		const Line &get_line_at_pos(const IPoint2 &p_pos) const {
			u32 l = which_line(p_pos.x, p_pos.y);
			return lines[l];
		}
		void clear() {
			for (u32 n = 0; n < NUM_VOXELS; n++) {
				lines[n].spans.clear();
			}
		}
	} voxels;

	struct Tile {
		Rect2 expanded_rect;
		Rect2 rect;
		Vector<u32> tris;
	};

	Parameters parameters;

	struct Data {
		freal tile_width = 5.0;
		freal tile_width_expanded = 7.5;
		//freal voxel_cell_size = 0;

		Vector<Tile> tiles;
		IPoint2 tile_dims;
		Tile &get_tile(const IPoint2 &p_loc) {
			u32 id = (p_loc.y * tile_dims.x) + p_loc.x;
			return tiles[id];
		}

	} data;

	u32 _clip_plane(const FPoint3 *in_v, u32 in_n, FPoint3 *out_v, i32 axis, freal limit, bool keep_greater) const;
	TriVoxelCache prepare_tri_cache(const FPoint3 p[3]) const;
	bool overlaps_voxel_fast(const TriVoxelCache &c, freal cell_x, freal cell_z) const;

	void _bake_tile(Tile &r_tile, Span<u32> p_indices, Span<FPoint3> p_verts);
	void _bake_tri(Tile &r_tile, const FPoint3 p_c[3]);
	void _bake_voxel(u32 x, u32 y, const TriVoxelCache &tri);
	void _insert_span(u32 x, u32 y, freal p_min_y, freal p_max_y, bool p_walkable);

	void _consolidate_spans(u32 p_line_id);
	void _build_open_spans(freal agent_height);
	void _link_open_spans(freal agent_height, freal agent_max_climb);
	void _build_regions_monotone();
	void _erode_walkable_area(freal erode_radius_voxels);
	void _trace_region_contours(Vector<RegionContour> &out_contours);
	void _simplify_contour(RegionContour &contour, freal max_error_voxels, freal max_y_deviation);
	void _point_segment_error_xz_y(const ContourVert &p, const ContourVert &a, const ContourVert &b, freal &r_xz_dist_sq, freal &r_y_err) const;

	// Triangulation, polygon merging, and welding member helpers
	freal _cross_2d_xz(const ContourVert &a, const ContourVert &b, const ContourVert &c) const;
	bool _point_in_tri_xz(const ContourVert &p, const ContourVert &a, const ContourVert &b, const ContourVert &c, freal sign) const;
	void _triangulate_contour(const Vector<ContourVert> &verts, Vector<Tri> &out_tris) const;
	bool _is_poly_convex_xz(const Vector<u32> &poly, const Vector<ContourVert> &verts, freal sign) const;
	bool _is_poly_coplanar(const Vector<u32> &poly, const Vector<ContourVert> &verts, freal max_y_deviation) const;
	void _merge_tris_to_convex_polys(const Vector<ContourVert> &verts, const Vector<Tri> &tris,
			u32 max_verts_per_poly, freal max_y_deviation,
			Vector<Vector<u32>> &out_polys) const;
	u32 _get_or_add_welded_vertex(NavMeshData &mesh, std::unordered_map<WeldKey, u32, WeldKeyHash> &weld_map,
			const FPoint3 &world_pt, freal weld_eps = 1e-3) const;
	void _build_tile_polygons(NavMeshData &out_mesh,
			std::unordered_map<WeldKey, u32, WeldKeyHash> &weld_map,
			freal tile_core_origin_x, freal tile_core_origin_z,
			freal cell_size, u32 max_verts_per_poly, freal max_y_deviation) const;
	void _merge_small_regions(u32 min_region_area);	

public:
	void bake(const Parameters &p_parameters, Span<u32> p_indices, Span<FPoint3> p_verts, NavMeshData &r_out_mesh);
};

} //namespace NavPhysics
