/*************************************************************************/
/*  occluder_shape_mesh.h                                                */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef OCCLUDER_SHAPE_MESH_H
#define OCCLUDER_SHAPE_MESH_H

#include "core/hash_map.h"
#include "core/local_vector.h"
#include "core/math/geometry.h"
#include "core/math/vec2i.h"
#include "occluder_shape.h"

class Spatial;
class Material;

class OccluderShapeMesh : public OccluderShape {
	GDCLASS(OccluderShapeMesh, OccluderShape);
	OBJ_SAVE_TYPE(OccluderShapeMesh);

	const int QUANTIZE_RES = 256;
	const int QUANTIZE_RES_2D = 256;

	struct Vec3i {
		bool operator==(const Vec3i &p_o) const { return x == p_o.x && y == p_o.y && z == p_o.z; }
		bool operator!=(const Vec3i &p_o) const { return (*this == p_o) == false; }
		int32_t x, y, z;
	};

	Geometry::MeshData _mesh_data;

	template <uint32_t NUM_BINS, class ELEMENT>
	struct HashTable {
		void add(const ELEMENT &p_ele, uint32_t p_hash) {
			_bins[p_hash % NUM_BINS].push_back(p_ele);
		}

		const LocalVectori<ELEMENT> &get_bin(uint32_t p_hash) const {
			return _bins[p_hash % NUM_BINS];
		}

		void clear() {
			for (uint32_t n = 0; n < NUM_BINS; n++) {
				_bins[n].clear();
			}
		}

	private:
		LocalVectori<ELEMENT> _bins[NUM_BINS];
	};

	struct HashTable_Tri {
		struct Element {
			uint32_t inds[3];
			uint32_t id;
		};
		HashTable<1024, Element> _table;

		uint32_t hash(uint32_t p_inds[3]) const {
			return p_inds[0] + p_inds[1] + p_inds[2];
		}

		void add(uint32_t p_inds[3], uint32_t p_id) {
			Element e;
			e.inds[0] = p_inds[0];
			e.inds[1] = p_inds[1];
			e.inds[2] = p_inds[2];
			e.id = p_id;

			uint32_t h = hash(p_inds);
			_table.add(e, h);
		}

		uint32_t find(uint32_t p_inds[3]) {
			uint32_t h = hash(p_inds);
			const LocalVectori<Element> &bin = _table.get_bin(h);
			uint32_t bsize = bin.size();
			for (uint32_t n = 0; n < bsize; n++) {
				const Element &e = bin[n];

				// match?

				for (int i = 0; i < 3; i++) {
					if (p_inds[i] == e.inds[0]) {
						bool matched = true;
						for (int test = 1; test < 3; test++) {
							if (p_inds[(i + test) % 3] != e.inds[test]) {
								matched = false;
								break;
							}
						}
						if (matched) {
							return e.id;
						}
					} // if found first match
				} // for i
			}
			// not found
			return UINT32_MAX;
		}
	};

	struct HashTable_Pos {
		struct Element {
			Vec3i pos;
			uint32_t id;
		};
		HashTable<4096, Element> _table;
		real_t _tolerance = 0.0001;

		uint32_t hash_pos(const Vec3i &p_pos) const {
			return p_pos.x + p_pos.y + p_pos.z;
		}

		void add(const Vec3i &p_pos, uint32_t p_id) {
			uint32_t hash = hash_pos(p_pos);
			Element e;
			e.pos = p_pos;
			e.id = p_id;
			_table.add(e, hash);
		}

		uint32_t find_single_bin(uint32_t p_hash, const Vec3i &p_pos) const {
			const LocalVectori<Element> &bin = _table.get_bin(p_hash);
			uint32_t bsize = bin.size();

			for (uint32_t n = 0; n < bsize; n++) {
				const Element &e = bin[n];
				if (e.pos == p_pos) {
					return e.id;
				}
			}
			// not found
			return UINT32_MAX;
		}

		uint32_t find(const Vec3i &p_pos) const {
			uint32_t h = hash_pos(p_pos);
			return find_single_bin(h, p_pos);
		}

		void clear() {
			_table.clear();
		}
	};

	struct BakeVertex {
		bool is_active() const { return linked_faces.size(); }
		bool remove_linked_face(uint32_t p_id) {
			int64_t found = linked_faces.find(p_id);
			if (found != -1) {
				linked_faces.remove_unordered(found);
				dirty = true;
				return true;
			}
			return false;
		}
		bool add_linked_face(uint32_t p_id) {
			if (linked_faces.find(p_id) == -1) {
				linked_faces.push_back(p_id);
				dirty = true;
				return true;
			}
			return false;
		}
		BakeVertex() {
			last_processed_tick = 0;
			dirty = true;
		};
		Vec3i posi;
		Vector3 posf;

		// try and merge faces from a vertex once with each run
		uint32_t last_processed_tick;
		bool dirty;
		LocalVectori<uint32_t> linked_faces;
	};

	struct BakeFace {
		BakeFace() {
			area = 0.0;
			done = false;
			last_processed_tick = 0;
			island = 0;
		}
		int num_sides() const { return indices.size(); }
		Plane plane;
		real_t area;
		bool done;
		uint32_t island;
		uint32_t last_processed_tick;
		LocalVectori<uint32_t> indices;
		LocalVectori<uint32_t> neighbour_face_ids;
		LocalVectori<uint32_t> adjacent_face_ids; // may not be neighbours if not within angle
	};

	struct BakeIsland {
		BakeIsland() {
			adjacent_null = false;
		}
		LocalVectori<uint32_t> neighbour_island_ids;

		// a face adjacent to each hole
		//LocalVectori<uint32_t> hole_face_ids;
		List<LocalVectori<uint32_t>> hole_edges;

		LocalVectori<uint32_t> outer_edge;

		// list of faces in this island
		LocalVectori<uint32_t> face_ids;

		// if any triangle in the island has an edge with no neighbour.
		// this means it cannot be an internal island.
		bool adjacent_null;
	};

	struct SortFace {
		// used for sort .. we want the largest faces first in the list
		bool operator<(const SortFace &p_o) const { return area > p_o.area; }
		uint32_t id;
		real_t area;
	};

	struct IndexedPoint {
		// used for sort
		bool operator<(const IndexedPoint &p_ip2) const { return pos < p_ip2.pos; }
		Vec2i pos;
		uint32_t idx;
	};

	// used in baking
	struct BakeData {
		void clear() {
			//face_areas.clear();
			float_input_faces.clear();
			verts.clear();
			faces.clear();
			out_faces.clear();
			//sort_faces.clear();
			islands.clear();
			hash_verts.clear();
			hash_triangles._table.clear();
			_face_process_tick = 1;
		}
		//		uint32_t find_or_create_vert(const Vector3 &p_pos, uint32_t p_face_id) {
		uint32_t find_or_create_vert(const Vector3 &p_pos) {
			Vec3i posi;
			vec3_to_vec3i(p_pos, posi);
			Vector3 posf;
			vec3i_to_vec3(posi, posf);

			uint32_t id = hash_verts.find(posi);
			if (id != UINT32_MAX) {
				//verts[id].linked_faces.push_back(p_face_id);
				return id;
			}

#if 0
			// use this to test the hash table is finding all cases
			for (uint32_t n = 0; n < verts.size(); n++) {
				if (verts[n].pos.is_equal_approx(p_pos)) {
					verts[n].linked_faces.push_back(p_face_id);
					return n;
				}
			}
#endif

			id = verts.size();
			verts.resize(id + 1);
			verts[id].posi = posi;
			verts[id].posf = posf;
			//verts[id].linked_faces.push_back(p_face_id);

			hash_verts.add(posi, id);

			return id;
		}

		void vec3_to_vec3i(Vector3 p_pt, Vec3i &r_pt) {
			p_pt -= input_aabb.position;
			p_pt *= world_to_int_multiplier;
			r_pt.x = p_pt.x;
			r_pt.y = p_pt.y;
			r_pt.z = p_pt.z;
		}

		void vec3i_to_vec3(const Vec3i &p_pt, Vector3 &r_pt) {
			r_pt.x = p_pt.x;
			r_pt.y = p_pt.y;
			r_pt.z = p_pt.z;

			r_pt *= int_to_world_multiplier;
			r_pt += input_aabb.position;
		}

		// floating point coords to integer
		LocalVectori<Face3> float_input_faces;
		AABB input_aabb;
		real_t world_to_int_multiplier;
		real_t int_to_world_multiplier;

		LocalVectori<BakeVertex> verts;
		LocalVectori<BakeFace> faces;
		LocalVectori<BakeFace> out_faces;
		LocalVectori<BakeIsland> islands;

		HashTable_Pos hash_verts;
		HashTable_Tri hash_triangles;

		uint32_t _face_process_tick = 1;

	} _bd;

	NodePath _settings_bake_path;
	real_t _settings_threshold_input_size = 1.0;
	real_t _settings_threshold_input_size_squared = 1.0;
	real_t _settings_threshold_output_size = 1.0;
	real_t _settings_simplify = 0.0;
	real_t _settings_plane_simplify_degrees = 11.0;
	real_t _settings_plane_simplify_dot = 0.98;
	real_t _settings_remove_floor_dot = 0.0;
	real_t _settings_vertex_tolerance = 0.001;
	int _settings_remove_floor_angle = 20;
	uint32_t _settings_bake_mask = 0xFFFFFFFF;

	int _settings_debug_face_id = 0;

	bool _bake_material_check(Ref<Material> p_material);
	void _bake_quantize_float_faces();
	void _bake_input_face(const Face3 &p_face);
	void _bake_recursive(Spatial *p_node);
	bool _try_bake_face(const Face3 &p_face);
	void _simplify_triangles();

	// new method
	bool _make_faces_new(uint32_t p_process_tick);
	bool _flood_fill_from_face(int p_face_id, uint32_t p_process_tick, LocalVectori<uint32_t> &r_face_ids);
	bool _merge_face_zone(const LocalVectori<uint32_t> &p_face_ids);
	void _find_face_zone_edges(const LocalVectori<uint32_t> &p_face_ids, LocalVectori<uint32_t> &r_edges);
	uint32_t _trace_zone_edge(uint32_t p_face_id, uint32_t &r_join_vert_id, LocalVectori<uint32_t> &r_edges);
	Vector3 _normal_from_edge_verts_newell(const LocalVectori<uint32_t> &p_edge_verts) const;
	bool _make_convex_chunk_external(const LocalVectori<uint32_t> &p_edge_verts, const Plane &p_poly_plane, LocalVectori<uint32_t> &r_convex_inds);
	void _process_out_faces();
	bool _any_further_points_within(const Vector<IndexedPoint> &p_pts, int p_test_pt) const;
	void _finalize_out_face(BakeFace &r_face);
	bool _can_see(const Vector<IndexedPoint> &p_points, int p_test_point) const;
	bool face_has_worked(const LocalVectori<uint32_t> &p_face, const Vector3 &p_face_normal) const;

	void _debug_print_face(uint32_t p_face_id, String p_before_string = "");
	String _debug_vector_to_string(const LocalVectori<uint32_t> &p_list);
	void _print_line(String p_sz);

	void _finalize_faces();
	//real_t _find_face_area(const Geometry::MeshData::Face &p_face) const;
	bool _are_faces_neighbours(const BakeFace &p_a, const BakeFace &p_b, int &r_edge_a, int &r_edge_b) const;
	real_t _find_matching_faces_total_area(const LocalVectori<uint32_t> &p_faces) const;

	void _verify_verts();
	void _find_neighbour_face_ids();
	void _process_islands();
	bool _process_islands_trace_hole(uint32_t p_face_id, uint32_t p_process_tick);
	void _edgelist_add_holes(uint32_t p_island_id, LocalVectori<uint32_t> &r_edges, const Vector3 &p_face_normal);

	real_t _angle_between_vectors(Vector3 p_a, Vector3 p_b, const Vector3 &p_normal) const;

	uint32_t _find_or_create_vert(const Vector3 &p_pos) {
		// quantize
		Vec3i posi;
		_bd.vec3_to_vec3i(p_pos, posi);

		uint32_t id = _bd.hash_verts.find(posi);

		if (id != UINT32_MAX) {
			return id;
		}

		// back to quantized world coords
		Vector3 pos_world;
		_bd.vec3i_to_vec3(posi, pos_world);

#if 0
		// use this to detect any that are present but not found in the hash table
		// .. this should not happen.
		for (uint32_t n = 0; n < _mesh_data.vertices.size(); n++) {
			if (_mesh_data.vertices[n].is_equal_approx(p_pos)) {
				WARN_PRINT("FOUND WITH FULL LOOKUP");
				return n;
			}
		}
#endif

		id = _mesh_data.vertices.size();
		_mesh_data.vertices.push_back(pos_world);

		_bd.hash_verts.add(posi, id);

		return id;
	}

	//bool _create_merged_convex_face(BakeFace &r_face, real_t p_old_face_area, real_t &r_new_total_area);
	String _vec3_to_string(const Vector3 &p_pt) const;
	void _tri_face_remove_central_and_duplicates(BakeFace &p_face, uint32_t p_central_idx) const;
	bool _are_faces_coplanar_for_merging(const BakeFace &p_a, const BakeFace &p_b, real_t &r_fit) const {
		r_fit = p_a.plane.normal.dot(p_b.plane.normal);
		return r_fit >= _settings_plane_simplify_dot;
	}
	int _get_wrapped_face_index(const Geometry::MeshData::Face &p_face, int p_index) const {
		return (p_index + p_face.indices.size()) % p_face.indices.size();
	}

	void set_bake_path(const NodePath &p_path) { _settings_bake_path = p_path; }
	NodePath get_bake_path() { return _settings_bake_path; }

	void set_threshold_input_size(real_t p_threshold) {
		_settings_threshold_input_size = p_threshold;
		// can't be zero, we want to prevent zero area triangles which could
		// cause divide by zero in the occlusion culler goodness of fit

		// NOTE: Investigate this. Tiny triangles that are not registered could break up larger faces.
		// Maybe the cap against zero area could be later in the pipeline.
		_settings_threshold_input_size_squared = MAX(p_threshold * p_threshold, 0.00001);
	}
	real_t get_threshold_input_size() const { return _settings_threshold_input_size; }

	void set_threshold_output_size(real_t p_threshold) {
		_settings_threshold_output_size = p_threshold;
	}
	real_t get_threshold_output_size() const { return _settings_threshold_output_size; }

	void set_simplify(real_t p_simplify) { _settings_simplify = p_simplify; }
	real_t get_simplify() const { return _settings_simplify; }

	void set_plane_simplify_angle(real_t p_angle) { _settings_plane_simplify_degrees = p_angle; }
	real_t get_plane_simplify_angle() const { return _settings_plane_simplify_degrees; }

	void set_vertex_tolerance(real_t p_tolerance) { _settings_vertex_tolerance = p_tolerance; }
	real_t get_vertex_tolerance() const { return _settings_vertex_tolerance; }

	void set_remove_floor(int p_angle);
	int get_remove_floor() const { return _settings_remove_floor_angle; }

	void set_bake_mask(uint32_t p_mask);
	uint32_t get_bake_mask() const;

	void set_bake_mask_value(int p_layer_number, bool p_value);
	bool get_bake_mask_value(int p_layer_number) const;

	void set_debug_face_id(int p_id) { _settings_debug_face_id = p_id; }
	int get_debug_face_id() const { return _settings_debug_face_id; }

	// serializing
	void _set_data(const Dictionary &p_d);
	Dictionary _get_data() const;

	void _log(String p_string);

protected:
	static void _bind_methods();

public:
	const Geometry::MeshData &get_mesh_data() const { return _mesh_data; }

	virtual void notification_enter_world(RID p_scenario);
	virtual void update_shape_to_visual_server();
	virtual void update_transform_to_visual_server(const Transform &p_global_xform);
	virtual Transform center_node(const Transform &p_global_xform, const Transform &p_parent_xform, real_t p_snap);

	void clear();
	void bake(Node *owner);

	OccluderShapeMesh();
};

#endif // OCCLUDER_SHAPE_MESH_H
