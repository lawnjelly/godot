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

	static void write_u32(Vector<uint8_t> &r_data, u32 p_val);
	static void write_i32(Vector<uint8_t> &r_data, i32 p_val);
	static void write_f32(Vector<uint8_t> &r_data, f32 p_val);

	static void write_ipoint2(Vector<uint8_t> &r_data, const IPoint2 &p_point);
	static void write_fpoint3(Vector<uint8_t> &r_data, const FPoint3 &p_point);
	static void write_fpoint2(Vector<uint8_t> &r_data, const FPoint2 &p_point);
};

class Loader {
	Vector<uint8_t> _save_data;

public:
	struct SourceMeshData {
		const FPoint3 *verts = nullptr;
		const u32 *indices = nullptr;

		const u32 *poly_num_indices = nullptr;

		u32 num_verts = 0;
		u32 num_indices = 0;
		u32 num_polys = 0;
	};

	struct WorkingMeshData {
		const FPoint3 *verts = nullptr;
		const IPoint2 *iverts = nullptr;
		const u32 *indices = nullptr;
		const u32 *poly_num_indices = nullptr;

		u32 num_verts = 0;
		u32 num_indices = 0;
		u32 num_polys = 0;

		FPoint2 float_to_fixed_point_scale;
		FPoint2 float_to_fixed_point_offset;

		FPoint2 fixed_point_to_float_scale;
		FPoint2 fixed_point_to_float_offset;
	};

	bool load_mesh(const SourceMeshData &p_source_mesh, Mesh &r_mesh);
	//	SourceMeshData save_mesh(const Mesh &p_mesh);
	void llog(String p_sz);

	bool load_working_data(const WorkingMeshData &p_data, Mesh &r_mesh);
	bool extract_working_data(WorkingMeshData &r_data, const Mesh &p_mesh);

	bool load_raw_data(const uint8_t *p_data, uint32_t p_num_bytes, Mesh &r_mesh);
	uint32_t prepare_raw_data(const Mesh &p_mesh);
	bool save_raw_data(uint8_t *r_data, uint32_t p_num_bytes);

private:
	void _load(Mesh &r_mesh);
	bool load_polys(const SourceMeshData &p_mesh, Mesh &r_dest);
	bool _load_polys(u32 p_num_polys, const u32 *p_num_poly_inds, Mesh &r_mesh);
	bool _load_polys_old(const SourceMeshData &p_mesh, Mesh &r_dest);

	u32 find_or_create_vert(Mesh &r_dest, const FPoint3 &p_pt);
	void plane_from_poly_newell(Mesh &r_dest, Poly &r_poly) const;

	void load_fixed_point_verts(Mesh &r_dest);
	void find_links(Mesh &r_dest);
	void find_walls(Mesh &r_dest);
	void find_bottlenecks(Mesh &r_dest);

	void find_index_nexts(Mesh &r_dest);
	u32 find_linked_poly(Mesh &r_dest, u32 p_poly_from, u32 p_ind_a, u32 p_ind_b, u32 &r_linked_poly) const;
	void wall_add_neighbour_wall(Mesh &r_dest, u32 p_a, u32 p_b);
};

} // namespace NavPhysics
