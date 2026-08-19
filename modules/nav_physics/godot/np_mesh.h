#pragma once

#include "core/resource.h"
#include "np_bake_params.h"
#include "thirdparty/navphysics/navphysics_defines.h"
#include <Recast.h>

class NavigationMeshInstance;

#ifdef TOOLS_ENABLED
struct EditorProgress;
#endif

//#define GODOT_NP_MESH_GODOT_DEBUG_BAKE

class NPPolyFinder {
	LocalVector<uint32_t> _poly_ids;
	Rect2 _boundary;
	real_t _cell_size = 0;
	//Vector2 _boundary_offset;
	uint32_t _width_cells = 0;
	uint32_t _height_cells = 0;

	struct Cell {
		uint32_t first_id = 0;
		uint32_t num_ids = 0;
	};

	struct BuildCell {
		LocalVector<uint32_t> poly_ids;
	};

	LocalVector<Cell> _cells;
	LocalVector<BuildCell> _build_cells;

	void clear() {
		_poly_ids.clear();
		_boundary = Rect2();
		_cell_size = 0;
		_width_cells = 0;
		_height_cells = 0;
		_cells.clear();
		_build_cells.clear();
	}

	Cell &get_cell(u32 p_x, u32 p_y) {
		u32 which = (p_y * _width_cells) + p_x;
		return _cells[which];
	}
	const Cell &get_cell(u32 p_x, u32 p_y) const {
		u32 which = (p_y * _width_cells) + p_x;
		return _cells[which];
	}

public:
	void build(const Vector2 *p_verts, uint32_t p_num_verts, uint32_t *p_indices, uint32_t p_num_tris);

	// Find the leaf for a single point.
	const uint32_t *find_leaf(const Vector2 &p_pt, uint32_t &r_num_polys) const;

	struct CellResult {
		const uint32_t *poly_ids = nullptr;
		uint32_t num_polys = 0;
	};

	// Find a list of cells that cover a rect, used for finding jump polys.
	void find_cells(const Rect2 &p_rect, LocalVector<CellResult> &r_cells) const;
	void find_unique_polys(const Rect2 &p_rect, LocalVector<uint32_t> &r_unique_poly_ids) const;
};

class NPRayCaster {
	struct Tri {
		uint32_t inds[3];
		Plane plane;
#ifdef GODOT_NP_MESH_GODOT_DEBUG_BAKE
		Rect2 rect;
#endif
	};

	static const uint32_t CHECK_PATTERN_SAMPLES = 5;
	Vector2 _samples[CHECK_PATTERN_SAMPLES];

	void log(String p_string) const;

	LocalVector<Vector3> _verts;
	LocalVector<Tri> _tris;
	NPPolyFinder _poly_finder;

	Vector3 tri_normal(uint32_t p_tri_id) const;
	const Vector3 &tri_vert(uint32_t p_tri_id, uint32_t p_corn) const {
		return _verts[_tris[p_tri_id].inds[p_corn]];
	}
	void get_tri_verts(uint32_t p_tri_id, Vector3 r_verts[3]) const {
		r_verts[0] = tri_vert(p_tri_id, 0);
		r_verts[1] = tri_vert(p_tri_id, 1);
		r_verts[2] = tri_vert(p_tri_id, 2);
	}
	bool tri_contains_point(uint32_t p_tri_id, const Vector3 &p_pt) const;
	bool tri_contains_point_with_radius(uint32_t p_tri_id, const Vector3 &p_pt, float p_radius) const;

public:
	void create(const Vector<float> &p_vertices, const Vector<int> &p_indices);
	float raycast(const Vector3 &p_pt, float p_agent_radius) const;
	float raycast_old(const Vector3 &p_pt, float p_agent_radius) const;
};

class NPMesh : public Resource {
	//class NPMesh : public Reference {
	//	GDCLASS(NPMesh, Reference);
	GDCLASS(NPMesh, Resource);
	OBJ_SAVE_TYPE(NPMesh);
	RES_BASE_EXTENSION("npmesh");

	void log(String p_string) const;

public:
	struct Poly {
		uint32_t first_index = 0;
		uint32_t num_indices = 0;
		uint8_t type = 0;
	};

private:
	struct Data {
		np_handle h_mesh = 0;
		NPBakeParams bake_params;
	} data;

	struct DebugData {
		RID mesh;
		bool changed = true;
	} debug_data;

	static void _nav_physics_log_callback(const char *p_string);

	struct rcBakeData {
		rcContext ctx;

		rcHeightfield *hf = nullptr;
		rcCompactHeightfield *chf = nullptr;
		rcContourSet *cset = nullptr;
		rcPolyMesh *poly_mesh = nullptr;
		rcPolyMeshDetail *detail_mesh = nullptr;
	};

	struct BakedMeshData {
		LocalVector<Vector3> vertices;
		LocalVector<i32> indices;
		void clear() {
			vertices.clear();
			indices.clear();
		}
	};

	void _prepare_recast_for_baking(const NPBakeParams &p_params,
			const Vector<float> &p_vertices,
			const Vector<int> &p_indices,
			rcBakeData &r_bd);

	void _build_recast_navigation_mesh(
			const NPBakeParams &p_params,
#ifdef TOOLS_ENABLED
			EditorProgress *ep,
#endif
			const Vector<float> &p_vertices,
			const Vector<int> &p_indices,
			BakedMeshData &r_res);

	void _build_recast_ceiling_mesh(
			const NPBakeParams &p_params,
			const Vector<float> &p_vertices,
			const Vector<int> &p_indices,
			BakedMeshData &r_res);

	void _duplicate_bake_params(const NPBakeParams &p_params, NPBakeParams &r_dest) const;

	void _ground_detail_mesh(LocalVector<Vector3> &r_detail_verts, const Vector<float> &p_geom_vertices, const Vector<int> &p_geom_indices);

	void _convert_detail_mesh_to_baked_mesh_data(const rcPolyMeshDetail *p_detail_mesh, BakedMeshData &r_res);

	bool bake_load(const BakedMeshData &p_navmesh, const BakedMeshData &p_ceiling);

	void set_param_enabled(NPBakeParams::ParamEnabled p_param, bool p_enabled);
	bool get_param_enabled(NPBakeParams::ParamEnabled p_param);

	void set_param(NPBakeParams::Param p_param, float p_value);
	float get_param(NPBakeParams::Param p_param);

protected:
	static void _bind_methods();

public:
	NPMesh();
	~NPMesh();

	RID _refresh_debug_geometry(bool p_show);

	void set_sample_partition_type(NPBakeParams::SamplePartitionType p_value);
	NPBakeParams::SamplePartitionType get_sample_partition_type() const;

	void set_parsed_geometry_type(NPBakeParams::ParsedGeometryType p_value);
	NPBakeParams::ParsedGeometryType get_parsed_geometry_type() const;

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	void set_collision_mask_bit(int p_bit, bool p_value);
	bool get_collision_mask_bit(int p_bit) const;

	Vector<Vector3> get_vertices(bool p_ceiling = false) const;
	Vector<int> get_indices(bool p_ceiling = false) const;
	Vector<Poly> get_polys(bool p_ceiling = false) const;
	Vector<int> get_external_wall_connection_indices() const;
	Vector<int> get_internal_wall_connection_indices() const;

	void set_data(const Vector<uint8_t> &p_data);
	Vector<uint8_t> get_data() const;

	bool bake(Node *p_node);
	bool clear();
	bool toggle_wall_connection(const Vector3 &p_start, const Vector3 &p_end, bool p_external_or_internal);

	np_handle get_mesh_handle() { return data.h_mesh; }
};
