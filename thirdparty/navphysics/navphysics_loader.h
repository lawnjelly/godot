// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#pragma once

#include "navphysics_mesh.h"
#include "navphysics_pointf.h"

namespace NavPhysics {

class RawLoader {
public:
	static bool read_u32(const u8 **pp_data, u32 &r_bytes_left, u32 &r_value);
	static bool read_i32(const u8 **pp_data, u32 &r_bytes_left, i32 &r_value);
	static bool read_f32(const u8 **pp_data, u32 &r_bytes_left, f32 &r_value);

	static bool read_ipoint2(const u8 **pp_data, u32 &r_bytes_left, IPoint2 &r_point);
	static bool read_fpoint3(const u8 **pp_data, u32 &r_bytes_left, FPoint3 &r_point);
	static bool read_fpoint2(const u8 **pp_data, u32 &r_bytes_left, FPoint2 &r_point);

	static void write_u32(TVector<uint8_t> &r_data, u32 p_val);
	static void write_i32(TVector<uint8_t> &r_data, i32 p_val);
	static void write_f32(TVector<uint8_t> &r_data, f32 p_val);

	static void write_ipoint2(TVector<uint8_t> &r_data, const IPoint2 &p_point);
	static void write_fpoint3(TVector<uint8_t> &r_data, const FPoint3 &p_point);
	static void write_fpoint2(TVector<uint8_t> &r_data, const FPoint2 &p_point);
};

class Loader {
	TVector<uint8_t> _save_data;

	struct Data {
		TVector<u32> external_connecting_wall_indices;
		TVector<u32> external_connecting_wall_ids;

		TVector<u32> internal_connecting_wall_indices;
		TVector<u32> internal_connecting_wall_ids;

		TVector<u32> poly_num_indices;
		TVector<u32> ceil_poly_num_indices;
		Vector<u8> poly_types;

	} data;

	static bool _editor_only;

public:
	struct SourceMeshData {
		const FPoint3 *verts = nullptr;
		const u32 *indices = nullptr;
		const u32 *poly_num_indices = nullptr;

		u32 num_verts = 0;
		u32 num_indices = 0;
		u32 num_polys = 0;

		Mesh::MeshParams params;
	};

	struct WorkingMeshData {
		struct SubMesh {
			const FPoint3 *verts = nullptr;
			const IPoint2 *iverts = nullptr;
			const u32 *indices = nullptr;
			const u32 *poly_num_indices = nullptr;
			const u8 *poly_type = nullptr;

			u32 num_verts = 0;
			u32 num_indices = 0;
			u32 num_polys = 0;
		};

		SubMesh floor;
		SubMesh ceiling;

		// Num indices will be twice the number of connecting walls.
		const u32 *external_connecting_wall_ids = nullptr;
		const u32 *external_connecting_wall_indices = nullptr;
		const u32 *internal_connecting_wall_ids = nullptr;
		const u32 *internal_connecting_wall_indices = nullptr;

		u32 num_internal_connecting_walls = 0;
		u32 num_external_connecting_walls = 0;

		freal float_to_fixed_point_scale;
		FPoint2 float_to_fixed_point_offset;

		freal fixed_point_to_float_scale;
		FPoint2 fixed_point_to_float_offset;

		AABB aabb;

		u32 agent_radius = 0;
	};

	bool bake_mesh(const SourceMeshData &p_source_mesh, const SourceMeshData &p_source_ceiling_mesh, Mesh &r_mesh);

	bool extract_working_data(WorkingMeshData &r_data, const Mesh &p_mesh);

	bool load_raw_data(const uint8_t *p_data, uint32_t p_num_bytes, Mesh &r_mesh, const Mesh::MeshParams &p_params);
	uint32_t prepare_raw_data(Mesh &r_mesh);
	void prepare_raw_data_submesh(const WorkingMeshData::SubMesh &p_submesh);
	bool save_raw_data(uint8_t *r_data, uint32_t p_num_bytes);

	void extend_mesh(Mesh &r_mesh);
	void unextend_mesh(Mesh &r_mesh);

	static void set_editor_only(bool p_editor_only);

private:
	bool load_working_data(const WorkingMeshData &p_data, Mesh &r_mesh);
	bool load_working_data_submesh(const WorkingMeshData::SubMesh &p_data, Mesh::SubMesh &r_submesh);

	void llog(String p_sz);
	void log_load(String p_sz);

	void _load(Mesh &r_mesh);

	bool _is_bake_poly_valid(const SourceMeshData &p_mesh, u32 p_first_index, u32 p_num_indices, bool p_check_ceiling) const;

	bool bake_load_polys(const SourceMeshData &p_mesh, const SourceMeshData &p_ceil_mesh, Mesh &r_dest);
	bool _load_polys(u32 p_num_polys, const u32 *p_num_poly_inds, u32 p_ceiling_num_polys, const u32 *p_ceiling_num_poly_inds, Mesh &r_mesh);
	bool _load_ceiling_polys(u32 p_num_polys, const u32 *p_num_poly_inds, Mesh &r_mesh);
	void _calculate_poly_bounds(Mesh::SubMesh &r_submesh);
	void _calculate_poly_areas(Mesh &r_mesh);

	u32 find_or_create_vert(TVector<FPoint3> &r_verts, const FPoint3 &p_pt);

	void _calculate_extension_params(Mesh &r_mesh);
	void bake_fixed_point_verts(Mesh &r_dest);
	void find_links(Mesh &r_dest);
	void find_walls(Mesh &r_dest);
	void find_extended_aabb(Mesh &r_mesh);
	void find_floor_ceiling_links(Mesh &r_mesh);
	void sort_floor_ceiling_links(Mesh &r_mesh, u32 p_floor_poly_id);
	void _finalize_wall_pairs(Mesh &r_mesh, const TVector<Mesh::WallPair> &p_wall_pairs, u32 p_wall_flags);
	bool does_floor_and_ceiling_poly_collide(const Mesh &p_mesh, u32 p_floor_poly_id, u32 p_ceiling_poly_id) const;
	void _get_poly_points(const Mesh &p_mesh, u32 p_poly_id, bool p_ceiling, Vector<IPoint2> &r_pts, Vector<FPoint3> &r_pts3) const;

	void find_index_nexts(Mesh &r_dest);
	u32 find_linked_poly(Mesh &r_dest, u32 p_poly_from, u32 p_ind_a, u32 p_ind_b, u32 &r_linked_poly) const;
	void wall_add_neighbour_wall(Mesh &r_dest, u32 p_a, u32 p_b);

	// Temp data for polys used while finding narrowings
	// etc.
	struct PolyTemp {
		u32 narrowing_id = UINT32_MAX;
		u32 narrowing_width = UINT32_MAX;
		u32 flood_fill_counter = 0; // doubles as a flood fill counter for finding bottleneck areas
		bool could_be_narrowing = true;
	};
	Vector<PolyTemp> _poly_temps;

	void floodfill_islands(Mesh &r_mesh);
	void floodfill_area(Mesh &r_mesh, u32 p_poly_id, u32 p_area_id);

	void find_bottlenecks(Mesh &r_dest);
	void find_zone_links(Mesh &r_dest);
	u32 flood_fill_bottleneck(Mesh &r_dest, u32 p_poly_id, u32 p_start_wall_id, u32 p_flood_fill_counter, const freal p_threshold);
	bool flood_fill_narrowing(Mesh &r_dest, u32 p_poly_id, u32 p_narrowing_id, u32 p_narrowing_width);
	void replace_poly_narrow_dist(PolyTemp &r_poly, u32 p_dist);
	IPoint2 _find_crossing_point_in_poly(Mesh &p_mesh, u32 p_poly_id, const IPoint2 p_seed_pos) const;

	bool does_narrow_poly_have_neighbour(Mesh &r_dest, u32 p_wid, const Poly &p_poly, freal p_threshold) const;
};

} // namespace NavPhysics
