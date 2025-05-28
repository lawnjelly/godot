#include "np_mesh.h"
#include "../source/navphysics_loader.h"
#include "../source/navphysics_log.h"
#include "../source/navphysics_map.h"
#include "../source/navphysics_pointf.h"
#include "../source/navphysics_pointi.h"
#include "../source/navphysics_vector.h"

#include "core/engine.h"
#include "modules/navigation/navigation_mesh_generator.h"
#include "scene/3d/navigation_mesh_instance.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_node.h"
#include "editor/editor_settings.h"
#endif

#ifdef GODOT_NP_MESH_GODOT_DEBUG_BAKE
#define NP_MESH_LOG(A) log(A)
#else
#define NP_MESH_LOG(A)
#endif

void NPPolyFinder::build(const Vector2 *p_verts, uint32_t p_num_verts, uint32_t *p_indices, uint32_t p_num_tris) {
	clear();
	ERR_FAIL_NULL(p_verts);
	ERR_FAIL_NULL(p_indices);
	ERR_FAIL_COND(!p_num_verts);
	ERR_FAIL_COND(!p_num_tris);

	// Seed bounds
	LocalVector<Rect2> bounds;
	bounds.resize(p_num_tris);

	_boundary.position = p_verts[0];
	_boundary.size = Vector2();

	for (u32 n = 0; n < p_num_tris; n++) {
		Rect2 &bound = bounds[n];
		bound.position = p_verts[p_indices[n * 3]];
		bound.expand_to(p_verts[p_indices[(n * 3) + 1]]);
		bound.expand_to(p_verts[p_indices[(n * 3) + 2]]);

		_boundary = _boundary.merge(bound);
	}

	double total_area = _boundary.size.x * _boundary.size.y;

	// The separation should depend on the poly count, for efficiency.
	uint32_t tiles = MAX(1, p_num_tris / 8);
	double area_per_tile = MAX(1, total_area / tiles);

	_cell_size = MAX(1, Math::sqrt(area_per_tile));
	_width_cells = (_boundary.size.x / _cell_size) + 1;
	_height_cells = (_boundary.size.y / _cell_size) + 1;

	_cells.resize(_width_cells * _height_cells);
	_build_cells.resize(_width_cells * _height_cells);

	// Place each poly into appropriate cells.
	// (This should be faster than going by cell.)
	for (u32 p = 0; p < p_num_tris; p++) {
		Rect2 poly_bound = bounds[p];

		DEV_ASSERT(poly_bound.size.x >= 0);
		DEV_ASSERT(poly_bound.size.y >= 0);

		// Polys with zero size should not be found anyway..
		if (poly_bound.get_area() == 0) {
			print_line("NavPoly with zero bound detected.");
			continue;
		}

		// convert to cell coords.
		Vector2 pbegin = poly_bound.position;

		// These may fail due to float error.
		// But the actual extents are capped later.
		// DEV_ASSERT(pbegin.x >= _boundary.position.x);
		// DEV_ASSERT(pbegin.y >= _boundary.position.y);

		pbegin -= _boundary.position;

		Vector2 pend = pbegin + poly_bound.size;

		// These may fail due to float error.
		// But the actual extents are capped later.
		// DEV_ASSERT(pend.x <= _boundary.size.x);
		// DEV_ASSERT(pend.y <= _boundary.size.y);

		pbegin.x /= _cell_size;
		pbegin.y /= _cell_size;

		//i32 mod_x = pend.x % _cell_size;
		//i32 mod_y = pend.y % _cell_size;
		pend.x /= _cell_size;
		pend.y /= _cell_size;

		// DEV_ASSERT(pbegin.x >= 0);
		// DEV_ASSERT(pbegin.y >= 0);
		// DEV_ASSERT(pend.x < _width_cells);
		// DEV_ASSERT(pend.y < _height_cells);

		//if (mod_x) {
		pend.x += 1;
		//}
		//if (mod_y) {
		pend.y += 1;
		//}

		Vector2 psize = pend - pbegin;
		DEV_ASSERT(psize.x >= 0);
		DEV_ASSERT(psize.y >= 0);

		Vector2i ipbegin = pbegin;
		Vector2i ipend = pend;

		// Enforce bounds
		ipbegin.x = MAX(ipbegin.x, 0);
		ipbegin.y = MAX(ipbegin.y, 0);

		ipend.x = MIN(ipend.x, _width_cells);
		ipend.y = MIN(ipend.y, _height_cells);

		for (i32 ty = ipbegin.y; ty < ipend.y; ty++) {
			for (i32 tx = ipbegin.x; tx < ipend.x; tx++) {
				u32 which = (ty * _width_cells) + tx;
				BuildCell &cell = _build_cells[which];
				cell.poly_ids.push_back(p);
			}
		}
	}

	// Convert to final cells.
	for (u32 y = 0; y < _height_cells; y++) {
		for (u32 x = 0; x < _width_cells; x++) {
			Cell &cell = get_cell(x, y);

			u32 which = (y * _width_cells) + x;
			const BuildCell &build_cell = _build_cells[which];

			cell.first_id = _poly_ids.size();
			cell.num_ids = build_cell.poly_ids.size();

			_poly_ids.resize(cell.first_id + cell.num_ids);

			// Fill
			for (u32 n = 0; n < cell.num_ids; n++) {
				_poly_ids[cell.first_id + n] = build_cell.poly_ids[n];
			}
		}
	}

	// Memory no longer required.
	_build_cells.clear();
}

void NPPolyFinder::find_unique_polys(const Rect2 &p_rect, LocalVector<uint32_t> &r_unique_poly_ids) const {
	LocalVector<CellResult> cells;
	find_cells(p_rect, cells);

	r_unique_poly_ids.clear();
	for (uint32_t n = 0; n < cells.size(); n++) {
		const CellResult &cr = cells[n];
		for (uint32_t i = 0; i < cr.num_polys; i++) {
			uint32_t poly_id = cr.poly_ids[i];
			if (r_unique_poly_ids.find(poly_id) == -1) {
				r_unique_poly_ids.push_back(poly_id);
			}
		}
	}
}

void NPPolyFinder::find_cells(const Rect2 &p_rect, LocalVector<CellResult> &r_cells) const {
	r_cells.clear();

	// If the polyfinder hasn't been built yet.
	if (!_cell_size) {
		WARN_PRINT_ONCE("PolyFinder contains no cells.");
		return;
	}

	Rect2 rect = p_rect;
	rect.position -= _boundary.position;

	Point2 rect_end = rect.position + rect.size;
	// Off map.
	if ((rect_end.x < 0) || (rect_end.y < 0)) {
		return;
	}

	Point2 begin = rect.position;
	begin /= _cell_size;

	if ((begin.x >= _width_cells) || (begin.y >= _height_cells)) {
		return;
	}

	Vector2i end = rect_end / _cell_size;
	end.x += 1;
	end.y += 1;
	end.x = MIN(end.x, _width_cells);
	end.y = MIN(end.y, _height_cells);

	begin.x = MAX(begin.x, 0);
	begin.y = MAX(begin.y, 0);

	for (uint32_t y = begin.y; y < end.y; y++) {
		for (uint32_t x = begin.x; x < end.x; x++) {
			const Cell &cell = get_cell(x, y);

			if (cell.num_ids) {
				CellResult cr;
				cr.poly_ids = &_poly_ids[cell.first_id];
				cr.num_polys = cell.num_ids;
				r_cells.push_back(cr);
			}
		}
	}
}

const u32 *NPPolyFinder::find_leaf(const Vector2 &p_pt, u32 &r_num_polys) const {
	// Find the cell corresponding to this point (if any).
	Vector2 pt = p_pt;
	pt -= _boundary.position;
	if ((pt.x < 0) || (pt.y < 0)) {
		// Off map.
		return nullptr;
	}

	pt.x /= _cell_size;
	pt.y /= _cell_size;

	if ((pt.x >= _width_cells) || (pt.y >= _height_cells)) {
		return nullptr;
	}

#if 0
#ifdef NP_DEV_ENABLED
	IPoint2 pt_debug = (pt * (i32) _cell_size) + _boundary_offset;
	log(String("cell covers ") + pt_debug + " to " + (pt_debug + IPoint2::make(_cell_size, _cell_size) ));
#endif
#endif

	const Cell &cell = get_cell(pt.x, pt.y);
	r_num_polys = cell.num_ids;

	return &_poly_ids[cell.first_id];
}

static bool g_np_ray_caster_log = true;

void NPRayCaster::log(String p_string) const {
	if (g_np_ray_caster_log) {
		print_line(p_string);
	}
}

void NPRayCaster::create(const Vector<float> &p_vertices, const Vector<int> &p_indices) {
	if (CHECK_PATTERN_SAMPLES > 1) {
		_samples[0] = Vector2(0, 0);
		_samples[1] = Vector2(-1, 0);
		_samples[2] = Vector2(1, 0);
		_samples[3] = Vector2(0, -1);
		_samples[4] = Vector2(0, 1);
	}

	g_np_ray_caster_log = !g_np_ray_caster_log;

	_verts.resize(p_vertices.size() / 3);
	uint32_t count = 0;

	for (uint32_t n = 0; n < _verts.size(); n++) {
		Vector3 &pt = _verts[n];

		pt.x = p_vertices[count++];
		pt.y = p_vertices[count++];
		pt.z = p_vertices[count++];
	}

	_tris.resize(p_indices.size() / 3);
	count = 0;
	uint32_t total_tris = 0;

	Vector3 v[3];

	for (uint32_t n = 0; n < _tris.size(); n++) {
		Tri &tri = _tris[total_tris];

		tri.inds[0] = p_indices[count++];
		tri.inds[2] = p_indices[count++];
		tri.inds[1] = p_indices[count++];

		get_tri_verts(total_tris, v);
		tri.plane = Plane(v[0], v[1], v[2]);

		if (tri.plane.normal.y <= 0) {
			continue;
		}

#ifdef GODOT_NP_MESH_GODOT_DEBUG_BAKE
		tri.rect.position = Vector2(v[0].x, v[0].z);
		tri.rect.expand_to(Vector2(v[1].x, v[1].z));
		tri.rect.expand_to(Vector2(v[2].x, v[2].z));
#endif

		total_tris++;
	}

	NP_MESH_LOG("NPRayCaster::create " + itos(total_tris) + " out of " + itos(_tris.size()));
	_tris.resize(total_tris);

	// Create the poly finder.
	// This is a simple 2D spatial partitioning to find quickly
	// some candidate polys for a rect2 under test.
	LocalVector<Vector2> verts2;
	verts2.resize(_verts.size());
	for (uint32_t n = 0; n < _verts.size(); n++) {
		verts2[n] = Vector2(_verts[n].x, _verts[n].z);
	}
	LocalVector<uint32_t> inds;
	inds.resize(_tris.size() * 3);
	count = 0;
	for (uint32_t n = 0; n < _tris.size(); n++) {
		inds[count++] = _tris[n].inds[0];
		inds[count++] = _tris[n].inds[1];
		inds[count++] = _tris[n].inds[2];
	}
	_poly_finder.build(verts2.ptr(), verts2.size(), inds.ptr(), _tris.size());
}

bool NPRayCaster::tri_contains_point_with_radius(uint32_t p_tri_id, const Vector3 &p_pt, float p_radius) const {
	Vector3 v[3];
	get_tri_verts(p_tri_id, v);

	NP_MESH_LOG("NPRayCaster::tri_contains_point " + itos(p_tri_id) + " ( " + String(Variant(v[0])) + " .. " + String(Variant(v[1])) + " .. " + String(Variant(v[2])) + " ) ");

	bool res = true;

	for (uint32_t n = 0; n < 3; n++) {
		uint32_t m = (n + 1) % 3;

		Vector3 o(v[m] - v[n]);
		Vector2 wall_vec(o.x, o.z);

		// Push the line by the radius along the normal direction
		Vector2 normal(wall_vec.y, -wall_vec.x);
		float l = normal.length();
		if (l == 0) {
			continue;
		}

		normal *= p_radius / l;

		Vector2 a(v[n].x, v[n].z);
		a += normal;

		Vector2 pt_vec(p_pt.x - a.x, p_pt.z - a.y);
		real_t cross = wall_vec.cross(pt_vec);
		NP_MESH_LOG("\tcross " + rtos(cross));

		if (cross < 0) {
			return false;
		}
	}

	return res;
}

bool NPRayCaster::tri_contains_point(uint32_t p_tri_id, const Vector3 &p_pt) const {
	Vector3 v[3];
	get_tri_verts(p_tri_id, v);

	NP_MESH_LOG("NPRayCaster::tri_contains_point " + itos(p_tri_id) + " ( " + String(Variant(v[0])) + " .. " + String(Variant(v[1])) + " .. " + String(Variant(v[2])) + " ) ");

	bool res = true;

	for (uint32_t n = 0; n < 3; n++) {
		uint32_t m = (n + 1) % 3;

		Vector3 o(v[m] - v[n]);
		Vector2 wall_vec(o.x, o.z);

		Vector2 pt_vec(p_pt.x - v[n].x, p_pt.z - v[n].z);
		real_t cross = wall_vec.cross(pt_vec);
		NP_MESH_LOG("\tcross " + rtos(cross));

		// NOTE: There are great perils here.
		// (1) We are using floating point, so an epsilon would be needed
		// at agent radius 0 (if in theory the navmesh exactly matched the polys).
		// Luckily for floors, the agent size should prevent the need for this.
		// (2) For ceilings, agent size 0 means verts WILL over-extend the edges.
		// For now we are not grounding ceiling verts at all because this is
		// filled with error potential.
		if (cross < 0) {
#ifdef GODOT_NP_MESH_GODOT_DEBUG_BAKE
			res = false;
#else
			return false;
#endif
		}
	}

	return res;
}

float NPRayCaster::raycast(const Vector3 &p_pt, float p_agent_radius) const {
	NP_MESH_LOG("NPRayCaster::raycast from " + String(Variant(p_pt)));
	float closest = -FLT_MAX;

	Vector3 pt_hit;

	p_agent_radius *= 0.5f;

	// Find subset of tris close to this point.
	Rect2 rect;
	rect.position = Vector2(p_pt.x, p_pt.z);
	rect.grow_by(p_agent_radius);

	LocalVector<uint32_t> test_poly_ids;
	_poly_finder.find_unique_polys(rect, test_poly_ids);

	for (uint32_t n = 0; n < test_poly_ids.size(); n++) {
		uint32_t test_poly_id = test_poly_ids[n];

#ifdef GODOT_NP_MESH_GODOT_DEBUG_BAKE
		if (!_tris[test_poly_id].rect.grow(0.1).has_point(Vector2(p_pt.x, p_pt.z))) {
			continue;
		}
#endif

		if (!tri_contains_point_with_radius(test_poly_id, p_pt, p_agent_radius)) {
			//if (!tri_contains_point(n, p_pt)) {
			continue;
		}
		NP_MESH_LOG("\ttri " + itos(test_poly_id) + " contains point.");

		const Tri &tri = _tris[test_poly_id];

		// Do several checks around the radius, not just at the center.
		for (uint32_t i = 0; i < CHECK_PATTERN_SAMPLES; i++) {
			Vector3 pos = p_pt + (Vector3(_samples[i].x, 0, _samples[i].y) * p_agent_radius);

			if (tri_contains_point(test_poly_id, pos)) {
				if (tri.plane.intersects_ray(pos, Vector3(0, -1, 0), &pt_hit)) {
					NP_MESH_LOG("\ttri " + itos(test_poly_id) + " intersect plane at " + rtos(pt_hit.y));

					if (pt_hit.y > closest) {
						closest = pt_hit.y;
					}
				} else {
					NP_MESH_LOG("\ttri " + itos(test_poly_id) + " pt " + String(Variant(pos)) + " no intersect plane " + String(Variant(tri.plane)));
				}
			}
		}
	}

	// Really should be warn print once
	if (closest == -FLT_MAX) {
		WARN_PRINT_ONCE("NPRayCaster::raycast : no hits found");
	}

	return closest;
}

float NPRayCaster::raycast_old(const Vector3 &p_pt, float p_agent_radius) const {
	NP_MESH_LOG("NPRayCaster::raycast from " + String(Variant(p_pt)));
	float closest = -FLT_MAX;

	Vector3 pt_hit;

	p_agent_radius *= 0.5f;

	for (uint32_t n = 0; n < _tris.size(); n++) {
#ifdef GODOT_NP_MESH_GODOT_DEBUG_BAKE
		if (!_tris[n].rect.grow(0.1).has_point(Vector2(p_pt.x, p_pt.z))) {
			continue;
		}
#endif

		if (!tri_contains_point_with_radius(n, p_pt, p_agent_radius)) {
			//if (!tri_contains_point(n, p_pt)) {
			continue;
		}
		NP_MESH_LOG("\ttri " + itos(n) + " contains point.");

		const Tri &tri = _tris[n];

		// Do several checks around the radius, not just at the center.
		for (uint32_t i = 0; i < CHECK_PATTERN_SAMPLES; i++) {
			Vector3 pos = p_pt + (Vector3(_samples[i].x, 0, _samples[i].y) * p_agent_radius);

			if (tri_contains_point(n, pos)) {
				if (tri.plane.intersects_ray(pos, Vector3(0, -1, 0), &pt_hit)) {
					NP_MESH_LOG("\ttri " + itos(n) + " intersect plane at " + rtos(pt_hit.y));

					if (pt_hit.y > closest) {
						closest = pt_hit.y;
					}
				} else {
					NP_MESH_LOG("\ttri " + itos(n) + " pt " + String(Variant(pos)) + " no intersect plane " + String(Variant(tri.plane)));
				}
			}
		}
	}

	// Really should be warn print once
	if (closest == -FLT_MAX) {
		WARN_PRINT_ONCE("NPRayCaster::raycast : no hits found");
	}

	return closest;
}

void NPMesh::log(String p_string) const {
	print_line(p_string);
}

NPMesh::NPMesh() {
	NavPhysics::set_log_callback(_nav_physics_log_callback);
	NavPhysics::Loader::set_editor_only(Engine::get_singleton()->is_editor_hint());
	data.h_mesh = NavPhysics::g_world.safe_mesh_create();
}

NPMesh::~NPMesh() {
	_refresh_debug_geometry(false);

	if (data.h_mesh) {
		NavPhysics::g_world.safe_mesh_free(data.h_mesh);
		data.h_mesh = 0;
	}
}

void NPMesh::_nav_physics_log_callback(const char *p_string) {
	print_line(p_string);
}

void NPMesh::set_param_enabled(NPBakeParams::ParamEnabled p_param, bool p_enabled) {
	data.bake_params.set_param_enabled(p_param, p_enabled);
}

bool NPMesh::get_param_enabled(NPBakeParams::ParamEnabled p_param) {
	return data.bake_params.get_param_enabled(p_param);
}

void NPMesh::set_param(NPBakeParams::Param p_param, float p_value) {
	data.bake_params.set_param(p_param, p_value);
}

float NPMesh::get_param(NPBakeParams::Param p_param) {
	return data.bake_params.get_param(p_param);
}

void NPMesh::_bind_methods() {
	//	ClassDB::bind_method(D_METHOD("set_vertices", "vertices"), &NPMesh::set_vertices);
	//	ClassDB::bind_method(D_METHOD("get_vertices"), &NPMesh::get_vertices);

	//	ADD_PROPERTY(PropertyInfo(Variant::POOL_VECTOR3_ARRAY, "vertices", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_vertices", "get_vertices");

	//	ClassDB::bind_method(D_METHOD("set_indices", "indices"), &NPMesh::set_indices);
	//	ClassDB::bind_method(D_METHOD("get_indices"), &NPMesh::get_indices);

	//	ADD_PROPERTY(PropertyInfo(Variant::POOL_INT_ARRAY, "indices", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_indices", "get_indices");

	ClassDB::bind_method(D_METHOD("set_sample_partition_type", "sample_partition_type"), &NPMesh::set_sample_partition_type);
	ClassDB::bind_method(D_METHOD("get_sample_partition_type"), &NPMesh::get_sample_partition_type);

	ClassDB::bind_method(D_METHOD("set_parsed_geometry_type", "geometry_type"), &NPMesh::set_parsed_geometry_type);
	ClassDB::bind_method(D_METHOD("get_parsed_geometry_type"), &NPMesh::get_parsed_geometry_type);

	ClassDB::bind_method(D_METHOD("set_collision_mask", "mask"), &NPMesh::set_collision_mask);
	ClassDB::bind_method(D_METHOD("get_collision_mask"), &NPMesh::get_collision_mask);

	ClassDB::bind_method(D_METHOD("set_collision_mask_bit", "bit", "value"), &NPMesh::set_collision_mask_bit);
	ClassDB::bind_method(D_METHOD("get_collision_mask_bit", "bit"), &NPMesh::get_collision_mask_bit);

	ClassDB::bind_method(D_METHOD("set_param", "param", "value"), &NPMesh::set_param);
	ClassDB::bind_method(D_METHOD("get_param", "param"), &NPMesh::get_param);

	ClassDB::bind_method(D_METHOD("set_param_enabled", "param", "value"), &NPMesh::set_param_enabled);
	ClassDB::bind_method(D_METHOD("get_param_enabled", "param"), &NPMesh::get_param_enabled);

	ADD_GROUP("Exit", "exit_");
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "exit_lip", PROPERTY_HINT_RANGE, "0.01,100.0,0.01,or_greater"), "set_param", "get_param", NPBakeParams::PARAM_EXIT_LIP);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "exit_max_step_up", PROPERTY_HINT_RANGE, "0.01,100.0,0.01,or_greater"), "set_param", "get_param", NPBakeParams::PARAM_EXIT_MAX_STEP_UP);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "exit_max_drop", PROPERTY_HINT_RANGE, "0.01,100.0,0.01,or_greater"), "set_param", "get_param", NPBakeParams::PARAM_EXIT_MAX_DROP);

	ADD_GROUP("Sampling", "sample_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "sample_partition_type", PROPERTY_HINT_ENUM, "Watershed,Monotone,Layers"), "set_sample_partition_type", "get_sample_partition_type");
	ADD_GROUP("Geometry", "geometry_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "geometry_parsed_geometry_type", PROPERTY_HINT_ENUM, "Mesh Instances,Static Colliders,Both"), "set_parsed_geometry_type", "get_parsed_geometry_type");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "geometry_collision_mask", PROPERTY_HINT_LAYERS_3D_PHYSICS), "set_collision_mask", "get_collision_mask");

	ADD_GROUP("Cells", "cell_");
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "cell_size", PROPERTY_HINT_RANGE, "0.01,500.0,0.01,or_greater"), "set_param", "get_param", NPBakeParams::PARAM_CELL_SIZE);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "cell_height", PROPERTY_HINT_RANGE, "0.01,500.0,0.01,or_greater"), "set_param", "get_param", NPBakeParams::PARAM_CELL_HEIGHT);

	ADD_GROUP("Agents", "agent_");
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "agent_height", PROPERTY_HINT_RANGE, "0.01,500.0,0.01,or_greater"), "set_param", "get_param", NPBakeParams::PARAM_AGENT_HEIGHT);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "agent_radius", PROPERTY_HINT_RANGE, "0.01,500.0,0.01,or_greater"), "set_param", "get_param", NPBakeParams::PARAM_AGENT_RADIUS);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "agent_max_climb", PROPERTY_HINT_RANGE, "0.01,500.0,0.01,or_greater"), "set_param", "get_param", NPBakeParams::PARAM_AGENT_MAX_CLIMB);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "agent_max_slope", PROPERTY_HINT_RANGE, "0.02,90.0,0.01"), "set_param", "get_param", NPBakeParams::PARAM_AGENT_MAX_SLOPE);

	ADD_GROUP("Regions", "region_");
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "region_min_size", PROPERTY_HINT_RANGE, "0.0,150.0,0.01,or_greater"), "set_param", "get_param", NPBakeParams::PARAM_REGION_MIN_SIZE);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "region_merge_size", PROPERTY_HINT_RANGE, "0.0,150.0,0.01,or_greater"), "set_param", "get_param", NPBakeParams::PARAM_REGION_MERGE_SIZE);

	ADD_GROUP("Edges", "edge_");
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "edge_max_length", PROPERTY_HINT_RANGE, "0.0,50.0,0.01,or_greater"), "set_param", "get_param", NPBakeParams::PARAM_EDGE_MAX_LENGTH);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "edge_max_error", PROPERTY_HINT_RANGE, "0.1,3.0,0.01,or_greater"), "set_param", "get_param", NPBakeParams::PARAM_EDGE_MAX_ERROR);

	ADD_GROUP("Polygons", "polygon_");
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "polygon_verts_per_poly", PROPERTY_HINT_RANGE, "3.0,12.0,1.0,or_greater"), "set_param", "get_param", NPBakeParams::PARAM_VERTS_PER_POLY);

	ADD_GROUP("Details", "detail_");
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "detail_sample_distance", PROPERTY_HINT_RANGE, "0.1,16.0,0.01,or_greater"), "set_param", "get_param", NPBakeParams::PARAM_DETAIL_SAMPLE_DISTANCE);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "detail_sample_max_error", PROPERTY_HINT_RANGE, "0.1,16.0,0.01,or_greater"), "set_param", "get_param", NPBakeParams::PARAM_DETAIL_SAMPLE_MAX_ERROR);

	ClassDB::bind_method(D_METHOD("set_data", "data"), &NPMesh::set_data);
	ClassDB::bind_method(D_METHOD("get_data"), &NPMesh::get_data);

	ADD_PROPERTY(PropertyInfo(Variant::POOL_BYTE_ARRAY, "data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_data", "get_data");
}

void NPMesh::set_sample_partition_type(NPBakeParams::SamplePartitionType p_value) {
	ERR_FAIL_INDEX(p_value, NPBakeParams::SAMPLE_PARTITION_MAX);
	data.bake_params.partition_type = p_value;
}

NPBakeParams::SamplePartitionType NPMesh::get_sample_partition_type() const {
	return data.bake_params.partition_type;
}

void NPMesh::set_parsed_geometry_type(NPBakeParams::ParsedGeometryType p_value) {
	ERR_FAIL_INDEX(p_value, NPBakeParams::PARSED_GEOMETRY_MAX);
	data.bake_params.parsed_geometry_type = p_value;
	_change_notify();
}

NPBakeParams::ParsedGeometryType NPMesh::get_parsed_geometry_type() const {
	return data.bake_params.parsed_geometry_type;
}

void NPMesh::set_collision_mask(uint32_t p_mask) {
	data.bake_params.collision_mask = p_mask;
}

uint32_t NPMesh::get_collision_mask() const {
	return data.bake_params.collision_mask;
}

void NPMesh::set_collision_mask_bit(int p_bit, bool p_value) {
	ERR_FAIL_INDEX_MSG(p_bit, 32, "Collision mask bit must be between 0 and 31 inclusive.");
	uint32_t mask = data.bake_params.get_collision_mask();
	if (p_value) {
		mask |= 1 << p_bit;
	} else {
		mask &= ~(1 << p_bit);
	}
	data.bake_params.set_collision_mask(mask);
}

bool NPMesh::get_collision_mask_bit(int p_bit) const {
	ERR_FAIL_INDEX_V_MSG(p_bit, 32, false, "Collision mask bit must be between 0 and 31 inclusive.");
	return data.bake_params.get_collision_mask() & (1 << p_bit);
}

void NPMesh::set_data(const Vector<uint8_t> &p_data) {
	NavPhysics::Loader loader;
	NavPhysics::Mesh *mesh = NavPhysics::g_world.safe_get_mesh(data.h_mesh);
	NP_DEV_ASSERT(mesh);
	mesh->clear();

	debug_data.changed = true;

	if (p_data.size()) {
		NavPhysics::Mesh::MeshParams params;

		params.agent_radius = data.bake_params.get_param(NPBakeParams::PARAM_AGENT_RADIUS);
		params.agent_height = data.bake_params.get_param(NPBakeParams::PARAM_AGENT_HEIGHT);
		params.exit_lip = data.bake_params.get_param(NPBakeParams::PARAM_EXIT_LIP);
		params.exit_max_step_up = data.bake_params.get_param(NPBakeParams::PARAM_EXIT_MAX_STEP_UP);
		params.exit_max_drop = data.bake_params.get_param(NPBakeParams::PARAM_EXIT_MAX_DROP);

		loader.load_raw_data(p_data.ptr(), p_data.size(), *mesh, params);
	}
}

Vector<uint8_t> NPMesh::get_data() const {
	NavPhysics::Loader loader;
	NavPhysics::Mesh *mesh = NavPhysics::g_world.safe_get_mesh(data.h_mesh);
	NP_DEV_ASSERT(mesh);

	Vector<uint8_t> data;
	data.resize(loader.prepare_raw_data(*mesh));
	loader.save_raw_data(data.ptrw(), data.size());

	return data;
}

RID NPMesh::_refresh_debug_geometry(bool p_show) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return RID();
	}

	VisualServer *vs = VisualServer::get_singleton();
	RID &rid_mesh = debug_data.mesh;

	if (!p_show) {
		if (rid_mesh.is_valid()) {
			vs->free(rid_mesh);
			rid_mesh = RID();
		}
		return RID();
	}

	if (!rid_mesh.is_valid()) {
		rid_mesh = RID_PRIME(vs->mesh_create());
		debug_data.changed = true;
	}

	if (debug_data.changed) {
		debug_data.changed = false;
		NavPhysics::Loader loader;
		NavPhysics::Mesh *mesh = NavPhysics::g_world.safe_get_mesh(data.h_mesh);
		NP_DEV_ASSERT(mesh);
		NavPhysics::Loader::WorkingMeshData md;
		loader.extract_working_data(md, *mesh);

		NavPhysics::Loader::WorkingMeshData::SubMesh sm = md.floor;

		const Vector3 *source_verts = (const Vector3 *)sm.verts;
		const uint32_t *inds = sm.indices;

		Vector<Vector3> tris_area;
		Vector<Vector3> tris_narrowing;
		Vector<NPMesh::Poly> polys = get_polys();

		// Display offset.
		Vector3 off(0, 0.25, 0);

		u32 num_inds = sm.num_indices;
		u32 num_verts = sm.num_verts;

		for (uint32_t n = 0; n < sm.num_polys; n++) {
			const NPMesh::Poly &p = polys[n];

			int i0 = p.first_index;
			if (i0 >= num_inds)
				continue;
			int ind0 = inds[i0];
			if (ind0 >= num_verts) {
				continue;
			}

			for (int e = 2; e < p.num_indices; e++) {
				int i1 = e - 1 + p.first_index;
				int i2 = e + p.first_index;

				if (i1 >= num_inds)
					continue;
				if (i2 >= num_inds)
					continue;

				int ind1 = inds[i1];
				int ind2 = inds[i2];

				if ((ind1 >= num_verts) || (ind2 >= num_verts)) {
					continue;
				}

				Vector<Vector3> &tris = !p.type ? tris_area : tris_narrowing;
				tris.push_back(source_verts[ind0] + off);
				tris.push_back(source_verts[ind1] + off);
				tris.push_back(source_verts[ind2] + off);
			}
		}

		vs->mesh_clear(rid_mesh);
		Array d;
		d.resize(VS::ARRAY_MAX);
		d[VS::ARRAY_VERTEX] = tris_area;
		vs->mesh_add_surface_from_arrays(rid_mesh, VS::PRIMITIVE_TRIANGLES, d);
		d[VS::ARRAY_VERTEX] = tris_narrowing;
		vs->mesh_add_surface_from_arrays(rid_mesh, VS::PRIMITIVE_TRIANGLES, d);

		SceneTree *st = SceneTree::get_singleton();
		if (st) {
			vs->mesh_surface_set_material(rid_mesh, 0, st->get_debug_navigation_disabled_material()->get_rid());
			vs->mesh_surface_set_material(rid_mesh, 1, st->get_debug_navigation_material()->get_rid());
		}

	} // if changed

	return rid_mesh;
}

Vector<Vector3> NPMesh::get_vertices(bool p_ceiling) const {
	NavPhysics::Loader loader;
	NavPhysics::Mesh *mesh = NavPhysics::g_world.safe_get_mesh(data.h_mesh);
	NP_DEV_ASSERT(mesh);
	NavPhysics::Loader::WorkingMeshData md;
	loader.extract_working_data(md, *mesh);

	Vector<Vector3> ret;

	NavPhysics::Loader::WorkingMeshData::SubMesh *sm = p_ceiling ? &md.ceiling : &md.floor;

	if (sm->num_verts) {
		ret.resize(sm->num_verts);
		memcpy(ret.ptrw(), sm->verts, sm->num_verts * sizeof(Vector3));
	}

	return ret;
}

Vector<int> NPMesh::get_external_wall_connection_indices() const {
	NavPhysics::Loader loader;
	NavPhysics::Mesh *mesh = NavPhysics::g_world.safe_get_mesh(data.h_mesh);
	NP_DEV_ASSERT(mesh);
	NavPhysics::Loader::WorkingMeshData md;
	loader.extract_working_data(md, *mesh);

	Vector<int> ret;
	if (md.floor.num_indices) {
		ret.resize(md.num_external_connecting_walls * 2);
		static_assert(sizeof(int) == 4, "Expects 32 bit int.");
		memcpy(ret.ptrw(), md.external_connecting_wall_indices, md.num_external_connecting_walls * 2 * sizeof(int32_t));
	}
	return ret;
}

Vector<int> NPMesh::get_internal_wall_connection_indices() const {
	NavPhysics::Loader loader;
	NavPhysics::Mesh *mesh = NavPhysics::g_world.safe_get_mesh(data.h_mesh);
	NP_DEV_ASSERT(mesh);
	NavPhysics::Loader::WorkingMeshData md;
	loader.extract_working_data(md, *mesh);

	Vector<int> ret;
	if (md.floor.num_indices) {
		ret.resize(md.num_internal_connecting_walls * 2);
		static_assert(sizeof(int) == 4, "Expects 32 bit int.");
		memcpy(ret.ptrw(), md.internal_connecting_wall_indices, md.num_internal_connecting_walls * 2 * sizeof(int32_t));
	}
	return ret;
}

Vector<int> NPMesh::get_indices(bool p_ceiling) const {
	NavPhysics::Loader loader;
	NavPhysics::Mesh *mesh = NavPhysics::g_world.safe_get_mesh(data.h_mesh);
	NP_DEV_ASSERT(mesh);
	NavPhysics::Loader::WorkingMeshData md;
	loader.extract_working_data(md, *mesh);

	Vector<int> ret;

	NavPhysics::Loader::WorkingMeshData::SubMesh *sm = p_ceiling ? &md.ceiling : &md.floor;

	if (sm->num_indices) {
		ret.resize(sm->num_indices);
		static_assert(sizeof(int) == 4, "Expects 32 bit int.");
		memcpy(ret.ptrw(), sm->indices, sm->num_indices * sizeof(int32_t));
	}

	return ret;
}

Vector<NPMesh::Poly> NPMesh::get_polys(bool p_ceiling) const {
	NavPhysics::Loader loader;
	NavPhysics::Mesh *mesh = NavPhysics::g_world.safe_get_mesh(data.h_mesh);
	NP_DEV_ASSERT(mesh);
	NavPhysics::Loader::WorkingMeshData md;
	loader.extract_working_data(md, *mesh);

	Vector<Poly> ret;

	NavPhysics::Loader::WorkingMeshData::SubMesh *sm = p_ceiling ? &md.ceiling : &md.floor;

	if (sm->num_polys) {
		ret.resize(sm->num_polys);

		uint32_t index_count = 0;

		for (uint32_t n = 0; n < sm->num_polys; n++) {
			Poly p;
			p.first_index = index_count;
			p.num_indices = sm->poly_num_indices[n];
			if (!p_ceiling) {
				p.type = sm->poly_type[n];
			}
			index_count += p.num_indices;
			ret.set(n, p);
		}
	}

	return ret;
}

bool NPMesh::toggle_wall_connection(const Vector3 &p_start, const Vector3 &p_end, bool p_external_or_internal) {
#ifdef TOOLS_ENABLED
	return NavPhysics::g_world.safe_toggle_mesh_wall_connection(get_mesh_handle(), *(const NavPhysics::FPoint3 *)&p_start, *(const NavPhysics::FPoint3 *)&p_end, p_external_or_internal);
#endif
	return false;
}

bool NPMesh::bake(Node *p_node) {
#ifdef TOOLS_ENABLED
	EditorProgress *ep(nullptr);
	// FIXME
#endif
#if 0
	// After discussion on devchat disabled EditorProgress for now as it is not thread-safe and uses hacks and Main::iteration() for steps.
	// EditorProgress randomly crashes the Engine when the bake function is used with a thread e.g. inside Editor with a tool script and procedural navigation
	// This was not a problem in older versions as previously Godot was unable to (re)bake NavigationMesh at runtime.
	// If EditorProgress is fixed and made thread-safe this should be enabled again.
	if (Engine::get_singleton()->is_editor_hint()) {
		ep = memnew(EditorProgress("bake", TTR("Navigation Mesh Generator Setup:"), 11));
	}

	if (ep)
		ep->step(TTR("Parsing Geometry..."), 0);
#endif

	const NPBakeParams &params = data.bake_params;
	Vector<float> vertices;
	Vector<int> indices;

	Transform navmesh_xform = Object::cast_to<Spatial>(p_node)->get_global_transform().affine_inverse();

	List<Node *> parse_nodes;

	if (params.get_source_geometry_mode() == NPBakeParams::SOURCE_GEOMETRY_NAVMESH_CHILDREN) {
		parse_nodes.push_back(p_node);
	} else {
		p_node->get_tree()->get_nodes_in_group(params.get_source_group_name(), &parse_nodes);
	}

	for (const List<Node *>::Element *E = parse_nodes.front(); E; E = E->next()) {
		NPBakeParams::ParsedGeometryType geometry_type = params.get_parsed_geometry_type();
		uint32_t collision_mask = params.get_collision_mask();
		bool recurse_children = params.get_source_geometry_mode() != NPBakeParams::SOURCE_GEOMETRY_GROUPS_EXPLICIT;

		Node *node = E->get();
#ifdef DEV_ENABLED
		print_verbose("Baking NPMesh for node " + node->get_name());
#endif
		NavigationMeshGenerator::_parse_geometry(navmesh_xform, node, vertices, indices, geometry_type, collision_mask, recurse_children);
	}

	print_line("Baked " + itos(vertices.size()) + " vertices, " + itos(indices.size()) + " indices.");

	if (vertices.size() > 0 && indices.size() > 0) {
		BakedMeshData nav_mesh;
		BakedMeshData ceiling_mesh;

		_build_recast_navigation_mesh(
				params,
#ifdef TOOLS_ENABLED
				ep,
#endif
				vertices,
				indices,
				nav_mesh);

		_build_recast_ceiling_mesh(params, vertices, indices, ceiling_mesh);

		//bake_load(nav_mesh.vertices.ptr(), nav_mesh.vertices.size(), (const u32 *)nav_mesh.indices.ptr(), nav_mesh.indices.size());
		bake_load(nav_mesh, ceiling_mesh);
	}

#ifdef TOOLS_ENABLED
	if (ep)
		ep->step(TTR("Done!"), 11);

	if (ep)
		memdelete(ep);
#endif

	return true;
}

void NPMesh::_prepare_recast_for_baking(const NPBakeParams &p_params,
		const Vector<float> &p_vertices,
		const Vector<int> &p_indices,
		rcBakeData &r_bd) {
	const float *verts = p_vertices.ptr();
	const int nverts = p_vertices.size() / 3;
	const int *tris = p_indices.ptr();
	const int ntris = p_indices.size() / 3;

	float bmin[3], bmax[3];
	rcCalcBounds(verts, nverts, bmin, bmax);

	rcConfig cfg;
	memset(&cfg, 0, sizeof(cfg));

	cfg.cs = p_params.get_param(NPBakeParams::PARAM_CELL_SIZE);
	cfg.ch = p_params.get_param(NPBakeParams::PARAM_CELL_HEIGHT);
	cfg.walkableSlopeAngle = p_params.get_param(NPBakeParams::PARAM_AGENT_MAX_SLOPE);
	cfg.walkableHeight = (int)Math::ceil(p_params.get_param(NPBakeParams::PARAM_AGENT_HEIGHT) / cfg.ch);
	cfg.walkableClimb = (int)Math::floor(p_params.get_param(NPBakeParams::PARAM_AGENT_MAX_CLIMB) / cfg.ch);
	cfg.walkableRadius = (int)Math::ceil(p_params.get_param(NPBakeParams::PARAM_AGENT_RADIUS) / cfg.cs);
	cfg.maxEdgeLen = (int)(p_params.get_param(NPBakeParams::PARAM_EDGE_MAX_LENGTH) / cfg.cs);
	cfg.maxSimplificationError = p_params.get_param(NPBakeParams::PARAM_EDGE_MAX_ERROR);
	cfg.minRegionArea = (int)(p_params.get_param(NPBakeParams::PARAM_REGION_MIN_SIZE) * p_params.get_param(NPBakeParams::PARAM_REGION_MIN_SIZE));
	cfg.mergeRegionArea = (int)(p_params.get_param(NPBakeParams::PARAM_REGION_MERGE_SIZE) * p_params.get_param(NPBakeParams::PARAM_REGION_MERGE_SIZE));
	cfg.maxVertsPerPoly = (int)p_params.get_param(NPBakeParams::PARAM_VERTS_PER_POLY);
	cfg.detailSampleDist = MAX(cfg.cs * p_params.get_param(NPBakeParams::PARAM_DETAIL_SAMPLE_DISTANCE), 0.1f);
	cfg.detailSampleMaxError = cfg.ch * p_params.get_param(NPBakeParams::PARAM_DETAIL_SAMPLE_MAX_ERROR);

	cfg.bmin[0] = bmin[0];
	cfg.bmin[1] = bmin[1];
	cfg.bmin[2] = bmin[2];
	cfg.bmax[0] = bmax[0];
	cfg.bmax[1] = bmax[1];
	cfg.bmax[2] = bmax[2];

	/*
	AABB baking_aabb = p_nav_mesh->get_filter_baking_aabb();

	 bool aabb_has_no_volume = baking_aabb.has_no_area();

	  if (!aabb_has_no_volume) {
		  Vector3 baking_aabb_offset = p_nav_mesh->get_filter_baking_aabb_offset();

		 cfg.bmin[0] = baking_aabb.position[0] + baking_aabb_offset.x;
		 cfg.bmin[1] = baking_aabb.position[1] + baking_aabb_offset.y;
		 cfg.bmin[2] = baking_aabb.position[2] + baking_aabb_offset.z;
		 cfg.bmax[0] = cfg.bmin[0] + baking_aabb.size[0];
		 cfg.bmax[1] = cfg.bmin[1] + baking_aabb.size[1];
		 cfg.bmax[2] = cfg.bmin[2] + baking_aabb.size[2];
	 }
 */

#if 0
#ifdef TOOLS_ENABLED
	if (ep)
		ep->step(TTR("Calculating grid size..."), 2);
#endif
#endif

	rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

	// ~30000000 seems to be around sweetspot where Editor baking breaks
	if ((cfg.width * cfg.height) > 30000000) {
		WARN_PRINT("NavigationMesh baking process will likely fail."
				   "\nSource geometry is suspiciously big for the current Cell Size and Cell Height in the NavMesh Resource bake settings."
				   "\nIf baking does not fail, the resulting NavigationMesh will create serious pathfinding performance issues."
				   "\nIt is advised to increase Cell Size and/or Cell Height in the NavMesh Resource bake settings or reduce the size / scale of the source geometry.");
	}

	//#ifdef TOOLS_ENABLED
	//	if (ep)
	//		ep->step(TTR("Creating heightfield..."), 3);
	//#endif
	r_bd.hf = rcAllocHeightfield();

	ERR_FAIL_COND(!r_bd.hf);
	ERR_FAIL_COND(!rcCreateHeightfield(&r_bd.ctx, *r_bd.hf, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch));

	//#ifdef TOOLS_ENABLED
	//	if (ep)
	//		ep->step(TTR("Marking walkable triangles..."), 4);
	//#endif
	{
		Vector<unsigned char> tri_areas;
		tri_areas.resize(ntris);

		ERR_FAIL_COND(tri_areas.size() == 0);

		memset(tri_areas.ptrw(), 0, ntris * sizeof(unsigned char));
		rcMarkWalkableTriangles(&r_bd.ctx, cfg.walkableSlopeAngle, verts, nverts, tris, ntris, tri_areas.ptrw());

		ERR_FAIL_COND(!rcRasterizeTriangles(&r_bd.ctx, verts, nverts, tris, tri_areas.ptr(), ntris, *r_bd.hf, cfg.walkableClimb));
	}

	if (p_params.get_param_enabled(NPBakeParams::PARAM_ENABLED_FILTER_LOW_HANGING_OBSTACLES)) {
		rcFilterLowHangingWalkableObstacles(&r_bd.ctx, cfg.walkableClimb, *r_bd.hf);
	}
	if (p_params.get_param_enabled(NPBakeParams::PARAM_ENABLED_FILTER_LEDGE_SPANS)) {
		rcFilterLedgeSpans(&r_bd.ctx, cfg.walkableHeight, cfg.walkableClimb, *r_bd.hf);
	}
	if (p_params.get_param_enabled(NPBakeParams::PARAM_ENABLED_FILTER_WALKABLE_LOW_HEIGHT_SPANS)) {
		rcFilterWalkableLowHeightSpans(&r_bd.ctx, cfg.walkableHeight, *r_bd.hf);
	}

	//#ifdef TOOLS_ENABLED
	//	if (ep)
	//		ep->step(TTR("Constructing compact heightfield..."), 5);
	//#endif

	r_bd.chf = rcAllocCompactHeightfield();

	ERR_FAIL_COND(!r_bd.chf);
	ERR_FAIL_COND(!rcBuildCompactHeightfield(&r_bd.ctx, cfg.walkableHeight, cfg.walkableClimb, *r_bd.hf, *r_bd.chf));

	rcFreeHeightField(r_bd.hf);
	r_bd.hf = nullptr;

	//#ifdef TOOLS_ENABLED
	//	if (ep)
	//		ep->step(TTR("Eroding walkable area..."), 6);
	//#endif

	ERR_FAIL_COND(!rcErodeWalkableArea(&r_bd.ctx, cfg.walkableRadius, *r_bd.chf));

	//#ifdef TOOLS_ENABLED
	//	if (ep)
	//		ep->step(TTR("Partitioning..."), 7);
	//#endif

	if (p_params.get_sample_partition_type() == NPBakeParams::SAMPLE_PARTITION_WATERSHED) {
		ERR_FAIL_COND(!rcBuildDistanceField(&r_bd.ctx, *r_bd.chf));
		ERR_FAIL_COND(!rcBuildRegions(&r_bd.ctx, *r_bd.chf, 0, cfg.minRegionArea, cfg.mergeRegionArea));
	} else if (p_params.get_sample_partition_type() == NPBakeParams::SAMPLE_PARTITION_MONOTONE) {
		ERR_FAIL_COND(!rcBuildRegionsMonotone(&r_bd.ctx, *r_bd.chf, 0, cfg.minRegionArea, cfg.mergeRegionArea));
	} else {
		ERR_FAIL_COND(!rcBuildLayerRegions(&r_bd.ctx, *r_bd.chf, 0, cfg.minRegionArea));
	}

	//#ifdef TOOLS_ENABLED
	//	if (ep)
	//		ep->step(TTR("Creating contours..."), 8);
	//#endif

	r_bd.cset = rcAllocContourSet();

	ERR_FAIL_COND(!r_bd.cset);
	ERR_FAIL_COND(!rcBuildContours(&r_bd.ctx, *r_bd.chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *r_bd.cset));

	//#ifdef TOOLS_ENABLED
	//	if (ep)
	//		ep->step(TTR("Creating polymesh..."), 9);
	//#endif

	r_bd.poly_mesh = rcAllocPolyMesh();
	ERR_FAIL_COND(!r_bd.poly_mesh);
	ERR_FAIL_COND(!rcBuildPolyMesh(&r_bd.ctx, *r_bd.cset, cfg.maxVertsPerPoly, *r_bd.poly_mesh));

	r_bd.detail_mesh = rcAllocPolyMeshDetail();
	ERR_FAIL_COND(!r_bd.detail_mesh);
	ERR_FAIL_COND(!rcBuildPolyMeshDetail(&r_bd.ctx, *r_bd.poly_mesh, *r_bd.chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *r_bd.detail_mesh));

	rcFreeCompactHeightfield(r_bd.chf);
	r_bd.chf = nullptr;
	rcFreeContourSet(r_bd.cset);
	r_bd.cset = nullptr;
}

// Mostly copied from Resource::duplicate()
void NPMesh::_duplicate_bake_params(const NPBakeParams &p_params, NPBakeParams &r_dest) const {
	List<PropertyInfo> plist;
	p_params.get_property_list(&plist);

	bool subresources = true;

	for (List<PropertyInfo>::Element *E = plist.front(); E; E = E->next()) {
		if (!(E->get().usage & PROPERTY_USAGE_STORAGE)) {
			continue;
		}
		Variant p = get(E->get().name);

		if ((p.get_type() == Variant::DICTIONARY || p.get_type() == Variant::ARRAY)) {
			r_dest.set(E->get().name, p.duplicate(subresources));
		} else if (p.get_type() == Variant::OBJECT && (subresources || (E->get().usage & PROPERTY_USAGE_DO_NOT_SHARE_ON_DUPLICATE))) {
			RES sr = p;
			if (sr.is_valid()) {
				r_dest.set(E->get().name, sr->duplicate(subresources));
			}
		} else {
			r_dest.set(E->get().name, p);
		}
	}
}

void NPMesh::_build_recast_ceiling_mesh(
		const NPBakeParams &p_params,
		const Vector<float> &p_vertices,
		const Vector<int> &p_indices,
		BakedMeshData &r_res) {
	rcBakeData bd;

	// Calculate the bounds so we can flip the y axis.
	const float *verts = p_vertices.ptr();
	const int nverts = p_vertices.size() / 3;

	float bmin[3], bmax[3];
	rcCalcBounds(verts, nverts, bmin, bmax);

	Vector<float> flipped_verts = p_vertices;
	uint32_t num_verts = flipped_verts.size() / 3;

	float min_y = bmin[1];
	float y_range = bmax[1] - bmin[1];

	for (uint32_t n = 0; n < num_verts; n++) {
		uint32_t idx = (n * 3) + 1;

		float y = flipped_verts[idx];

		// Orientate around 0
		y -= min_y;

		// Flip
		y = y_range - y;

		flipped_verts.set(idx, y);
	}

	Vector<int> flipped_indices = p_indices;
	uint32_t num_tris = flipped_indices.size() / 3;
	for (uint32_t n = 0; n < num_tris; n++) {
		int t = flipped_indices[n * 3];
		flipped_indices.set(n * 3, flipped_indices[(n * 3) + 1]);
		flipped_indices.set((n * 3) + 1, t);
	}

	// Change parameters specifically for ceiling baking.
	NPBakeParams params;
	_duplicate_bake_params(p_params, params);

	params.set_param(NPBakeParams::PARAM_AGENT_RADIUS, 0);
	params.set_param(NPBakeParams::PARAM_AGENT_MAX_SLOPE, 80);
	//params.set_param(NPBakeParams::PARAM_AGENT_MAX_CLIMB, 10);

	_prepare_recast_for_baking(params, flipped_verts, flipped_indices, bd);

	NP_MESH_LOG("CEILING");
	_convert_detail_mesh_to_baked_mesh_data(bd.detail_mesh, r_res);

	// NEW - don't ground the ceiling, because the radius is zero,
	// it has verts close / OVER the edges of ceiling polys,
	// which may extend off the side and miss the ceiling.
	// We will have to use some other method to estimate ceiling height,
	// or be conservative.
	// _ground_detail_mesh(r_res.vertices, flipped_verts, flipped_indices);

	// Flip the detail verts.
	for (uint32_t n = 0; n < r_res.vertices.size(); n++) {
		Vector3 &pt = r_res.vertices[n];

		// Flip
		pt.y = y_range - pt.y;

		// Orientate around min
		pt.y += min_y;
	}

	//_convert_detail_mesh(bd.detail_mesh, &detail_verts, r_res, flipped_verts, flipped_indices);
	//_convert_detail_mesh_to_native_ceiling_mesh(bd.detail_mesh, detail_verts);

	rcFreePolyMesh(bd.poly_mesh);
	bd.poly_mesh = nullptr;
	rcFreePolyMeshDetail(bd.detail_mesh);
	bd.detail_mesh = nullptr;
}

void NPMesh::_build_recast_navigation_mesh(
		const NPBakeParams &p_params,
#ifdef TOOLS_ENABLED
		EditorProgress *ep,
#endif
		const Vector<float> &p_vertices,
		const Vector<int> &p_indices,
		BakedMeshData &r_res) {

	rcBakeData bd;

#ifdef TOOLS_ENABLED
	if (ep)
		ep->step(TTR("Setting up Configuration..."), 1);
#endif

	_prepare_recast_for_baking(p_params, p_vertices, p_indices, bd);

#ifdef TOOLS_ENABLED
	if (ep)
		ep->step(TTR("Converting to native navigation mesh..."), 10);
#endif

	NP_MESH_LOG("FLOOR");

	_convert_detail_mesh_to_baked_mesh_data(bd.detail_mesh, r_res);
	_ground_detail_mesh(r_res.vertices, p_vertices, p_indices);

	//_convert_detail_mesh(bd.detail_mesh, nullptr, r_res, p_vertices, p_indices);
	//_convert_detail_mesh_to_native_navigation_mesh(bd.detail_mesh);

	rcFreePolyMesh(bd.poly_mesh);
	bd.poly_mesh = nullptr;
	rcFreePolyMeshDetail(bd.detail_mesh);
	bd.detail_mesh = nullptr;
}

void NPMesh::_ground_detail_mesh(LocalVector<Vector3> &r_detail_verts, const Vector<float> &p_geom_vertices, const Vector<int> &p_geom_indices) {
	// First build a ray caster
	NPRayCaster rayc;
	rayc.create(p_geom_vertices, p_geom_indices);

	float radius = data.bake_params.get_param(NPBakeParams::PARAM_AGENT_RADIUS);

	for (uint32_t n = 0; n < r_detail_verts.size(); n++) {
		Vector3 &pt = r_detail_verts[n];
		pt.y = rayc.raycast(pt, radius);
	}
}

void NPMesh::_convert_detail_mesh_to_baked_mesh_data(const rcPolyMeshDetail *p_detail_mesh, BakedMeshData &r_res) {
	r_res.vertices.resize(p_detail_mesh->nverts);

	for (int i = 0; i < p_detail_mesh->nverts; i++) {
		const float *v = &p_detail_mesh->verts[i * 3];
		r_res.vertices[i] = Vector3(v[0], v[1], v[2]);
	}

	for (int i = 0; i < p_detail_mesh->nmeshes; i++) {
		const unsigned int *m = &p_detail_mesh->meshes[i * 4];
		const unsigned int bverts = m[0];
		const unsigned int btris = m[2];
		const unsigned int ntris = m[3];
		const unsigned char *tris = &p_detail_mesh->tris[btris * 4];
		for (unsigned int j = 0; j < ntris; j++) {
			// Polygon order in recast is opposite than godot's
			r_res.indices.push_back(bverts + tris[j * 4 + 0]);
			r_res.indices.push_back(bverts + tris[j * 4 + 2]);
			r_res.indices.push_back(bverts + tris[j * 4 + 1]);
		}
	}
}

bool NPMesh::clear() {
	return true;
}

#define GODOT_VECTOR_TO_NP(SOURCE, DEST, T, NP_T, OP)  \
	{                                                  \
		DEST.resize(SOURCE.size());                    \
		for (uint32_t n = 0; n < SOURCE.size(); n++) { \
			const T &pt = SOURCE[n];                   \
			DEST[n] = OP;                              \
		}                                              \
	}

bool NPMesh::bake_load(const BakedMeshData &p_navmesh, const BakedMeshData &p_ceiling) {
	//bool NPMesh::bake_load(const Vector3 *p_verts, uint32_t p_num_verts, const uint32_t *p_indices, uint32_t p_num_indices) {
	NavPhysics::Loader loader;
	NavPhysics::Loader::SourceMeshData source;
	NavPhysics::Loader::SourceMeshData source_ceiling;

	NavPhysics::TVector<NavPhysics::FPoint3> np_verts;
	NavPhysics::TVector<u32> np_inds;
	NavPhysics::TVector<u32> poly_num_inds;

	NavPhysics::TVector<NavPhysics::FPoint3> np_ceil_verts;
	NavPhysics::TVector<u32> np_ceil_inds;
	NavPhysics::TVector<u32> ceil_poly_num_inds;

	GODOT_VECTOR_TO_NP(p_navmesh.vertices, np_verts, Vector3, NavPhysics::FPoint3, NavPhysics::FPoint3::make(pt.x, pt.y, pt.z));

	GODOT_VECTOR_TO_NP(p_navmesh.indices, np_inds, i32, u32, pt);

	poly_num_inds.resize(p_navmesh.indices.size() / 3);
	for (u32 n = 0; n < poly_num_inds.size(); n++) {
		poly_num_inds[n] = 3;
	}

	GODOT_VECTOR_TO_NP(p_ceiling.vertices, np_ceil_verts, Vector3, NavPhysics::FPoint3, NavPhysics::FPoint3::make(pt.x, pt.y, pt.z));

	GODOT_VECTOR_TO_NP(p_ceiling.indices, np_ceil_inds, i32, u32, pt);

	ceil_poly_num_inds.resize(p_ceiling.indices.size() / 3);
	for (u32 n = 0; n < ceil_poly_num_inds.size(); n++) {
		ceil_poly_num_inds[n] = 3;
	}

	source.num_verts = np_verts.size();
	source.verts = np_verts.ptr();
	source.num_indices = np_inds.size();
	source.indices = np_inds.ptr();
	source.num_polys = poly_num_inds.size();
	source.poly_num_indices = poly_num_inds.ptr();

	source.params.agent_radius = data.bake_params.get_param(NPBakeParams::PARAM_AGENT_RADIUS);
	source.params.exit_lip = data.bake_params.get_param(NPBakeParams::PARAM_EXIT_LIP);

	source_ceiling.num_verts = np_ceil_verts.size();
	source_ceiling.verts = np_ceil_verts.ptr();
	source_ceiling.num_indices = np_ceil_inds.size();
	source_ceiling.indices = np_ceil_inds.ptr();
	source_ceiling.num_polys = ceil_poly_num_inds.size();
	source_ceiling.poly_num_indices = ceil_poly_num_inds.ptr();

	NavPhysics::Mesh *mesh = NavPhysics::g_world.safe_get_mesh(data.h_mesh);
	NP_DEV_ASSERT(mesh);

	loader.bake_mesh(source, source_ceiling, *mesh);

	return true;
}
