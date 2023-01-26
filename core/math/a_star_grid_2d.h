/**************************************************************************/
/*  a_star_grid_2d.h                                                      */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef A_STAR_GRID_2D_H
#define A_STAR_GRID_2D_H

#include "core/object/gdvirtual.gen.inc"
#include "core/object/ref_counted.h"
#include "core/object/script_language.h"
#include "core/templates/bitfield_dynamic.h"
#include "core/templates/list.h"
#include "core/templates/local_vector.h"

class AStarGrid2D : public RefCounted {
	GDCLASS(AStarGrid2D, RefCounted);

public:
	enum DiagonalMode {
		DIAGONAL_MODE_ALWAYS,
		DIAGONAL_MODE_NEVER,
		DIAGONAL_MODE_AT_LEAST_ONE_WALKABLE,
		DIAGONAL_MODE_ONLY_IF_NO_OBSTACLES,
		DIAGONAL_MODE_MAX,
	};

	enum Heuristic {
		HEURISTIC_EUCLIDEAN,
		HEURISTIC_MANHATTAN,
		HEURISTIC_OCTILE,
		HEURISTIC_CHEBYSHEV,
		HEURISTIC_MAX,
	};

private:
	Size2i size;
	Vector2 offset;
	Size2 cell_size = Size2(1, 1);
	bool dirty = false;

	bool jumping_enabled = false;
	DiagonalMode diagonal_mode = DIAGONAL_MODE_ALWAYS;
	Heuristic default_compute_heuristic = HEURISTIC_EUCLIDEAN;
	Heuristic default_estimate_heuristic = HEURISTIC_EUCLIDEAN;

	// Temporary data used for pathfinding.
	struct Pass {
		Vector2i point;
		uint32_t prev_pass_id = UINT32_MAX;

		real_t g_score = 0;
		real_t f_score = 0;

		Pass() {}

		Pass(const Vector2i &p_point, real_t p_g_score = 0, real_t p_f_score = 0) :
				point(p_point), g_score(p_g_score), f_score(p_f_score) {
		}
	};

	// Compact storage for data required for the whole map.
	struct MapPoint {
		float weight_scale = 1.0;
	};

	class Map {
		BitFieldDynamic2D _bf_solid;
		LocalVector<MapPoint> _points;

	public:
		void create(uint32_t p_width, uint32_t p_height) {
			_bf_solid.create(p_width, p_height);
			_points.resize(get_map_size());
		}

		void clear() {
			_bf_solid.destroy();
			_points.clear();
		}
		void destroy() {
			clear();
		}
		uint32_t get_index(uint32_t p_x, uint32_t p_y) const { return _bf_solid.get_index(p_x, p_y); }
		uint32_t get_map_width() const { return _bf_solid.get_width(); }
		uint32_t get_map_height() const { return _bf_solid.get_height(); }
		uint32_t get_map_size() const { return _bf_solid.get_num_bits(); }
		bool is_on_map(uint32_t p_x, uint32_t p_y) const { return _bf_solid.is_within(p_x, p_y); }
		bool is_solid(uint32_t p_x, uint32_t p_y) const { return _bf_solid.get_bit(p_x, p_y); }
		void set_solid(uint32_t p_x, uint32_t p_y, uint32_t p_solid) { _bf_solid.set_bit(p_x, p_y, p_solid); }
		MapPoint &get_point(uint32_t p_x, uint32_t p_y) {
			uint32_t id = get_index(p_x, p_y);
			return _points[id];
		}
		const MapPoint &get_point(uint32_t p_x, uint32_t p_y) const {
			uint32_t id = get_index(p_x, p_y);
			return _points[id];
		}
	} map;

public:
	class OpenList {
		LocalVector<Pass> passes;
		LocalVector<uint32_t> pass_map;
		uint32_t _width = 0;
		uint32_t _height = 0;

	public:
		LocalVector<uint32_t> sorted_pass_ids;
		BitFieldDynamic2D bf_closed;
		BitFieldDynamic2D bf_visited;

		void create(uint32_t p_width, uint32_t p_height) {
			_width = p_width;
			_height = p_height;
			pass_map.resize(p_width * p_height);
			// no need to blank, as we use a bitfield to record whether visited already
			bf_closed.create(_width, _height);
			bf_visited.create(_width, _height);
		}
		void debug();

		Pass *request_pass(uint32_t &r_pass_id) {
			r_pass_id = passes.size();
			passes.resize(passes.size() + 1);
			return &passes[r_pass_id];
		}
		void sorted_add_pass(uint32_t p_pass_id) {
			// record in the pass map
			const Pass &pass = passes[p_pass_id];
			pass_map[(pass.point.y * _width) + pass.point.x] = p_pass_id;

			// add to the sorted list
			sorted_pass_ids.push_back(p_pass_id);
		}
		Pass &get_pass_at(uint32_t p_x, uint32_t p_y, uint32_t &r_id) {
			r_id = pass_map[(p_y * _width) + p_x];
			return passes[r_id];
		}
		Pass &get_pass(uint32_t p_id) {
			return passes[p_id];
		}

		Pass *pop_pass(uint32_t &r_pass_id) {
			DEV_ASSERT(sorted_pass_ids.size());
			uint32_t id = sorted_pass_ids[sorted_pass_ids.size() - 1];
			sorted_pass_ids.resize(sorted_pass_ids.size() - 1);
			DEV_ASSERT(id < passes.size());
			r_pass_id = id;
			return &passes[id];
		}
		bool is_empty() const {
			return sorted_pass_ids.is_empty();
		}
	};

private: // Internal routines.
	_FORCE_INLINE_ bool _is_walkable(int64_t p_x, int64_t p_y) const {
		if (map.is_on_map(p_x, p_y)) {
			return !map.is_solid(p_x, p_y);
		}

		return false;
	}
	Vector2 _map_position(const Vector2i &p_pos) const {
		return Vector2(offset + (Vector2(p_pos.x, p_pos.y) * cell_size));
	}

	void _get_nbors(const Vector2i &p_center, LocalVector<Vector2i> &r_nbors) const;
	bool _append_if_non_solid(int p_x, int p_y, LocalVector<Vector2i> &r_nbors) const;
	bool _solve(const Vector2i &p_begin, const Vector2i &p_end, List<Vector2i> &r_path) const;
	void _debug_open_list(const OpenList &p_open_list);

protected:
	static void _bind_methods();

	virtual real_t _estimate_cost(const Vector2i &p_from_id, const Vector2i &p_to_id) const;
	virtual real_t _compute_cost(const Vector2i &p_from_id, const Vector2i &p_to_id) const;

	GDVIRTUAL2RC(real_t, _estimate_cost, Vector2i, Vector2i)
	GDVIRTUAL2RC(real_t, _compute_cost, Vector2i, Vector2i)

public:
	void set_size(const Size2i &p_size);
	Size2i get_size() const;

	void set_offset(const Vector2 &p_offset);
	Vector2 get_offset() const;

	void set_cell_size(const Size2 &p_cell_size);
	Size2 get_cell_size() const;

	void update();

	int get_width() const;
	int get_height() const;

	_FORCE_INLINE_ bool is_in_bounds(int p_x, int p_y) const {
		return p_x >= 0 && p_x < size.width && p_y >= 0 && p_y < size.height;
	}
	_FORCE_INLINE_ bool is_in_boundsv(const Vector2i &p_id) const {
		return p_id.x >= 0 && p_id.x < size.width && p_id.y >= 0 && p_id.y < size.height;
	}

	bool is_dirty() const;

	void set_jumping_enabled(bool p_enabled);
	bool is_jumping_enabled() const;

	void set_diagonal_mode(DiagonalMode p_diagonal_mode);
	DiagonalMode get_diagonal_mode() const;

	void set_default_compute_heuristic(Heuristic p_heuristic);
	Heuristic get_default_compute_heuristic() const;

	void set_default_estimate_heuristic(Heuristic p_heuristic);
	Heuristic get_default_estimate_heuristic() const;

	void set_point_solid(const Vector2i &p_id, bool p_solid = true);
	bool is_point_solid(const Vector2i &p_id) const;

	void set_point_weight_scale(const Vector2i &p_id, real_t p_weight_scale);
	real_t get_point_weight_scale(const Vector2i &p_id) const;

	void clear();

	Vector2 get_point_position(const Vector2i &p_id) const;
	Vector<Vector2> get_point_path(const Vector2i &p_from, const Vector2i &p_to) const;
	TypedArray<Vector2i> get_id_path(const Vector2i &p_from, const Vector2i &p_to) const;
};

VARIANT_ENUM_CAST(AStarGrid2D::DiagonalMode);
VARIANT_ENUM_CAST(AStarGrid2D::Heuristic);

#endif // A_STAR_GRID_2D_H
