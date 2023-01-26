/**************************************************************************/
/*  a_star_grid_2d.cpp                                                    */
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

#include "a_star_grid_2d.h"

#include "core/variant/typed_array.h"

//#define GODOT_ASTARGRID2D_VERBOSE

static real_t heuristic_euclidian(const Vector2i &p_from, const Vector2i &p_to) {
	Vector2i diff = p_to - p_from;
	int64_t l2 = diff.length_squared();
	return Math::sqrt((real_t)l2);
}

static real_t heuristic_manhattan(const Vector2i &p_from, const Vector2i &p_to) {
	real_t dx = (real_t)ABS(p_to.x - p_from.x);
	real_t dy = (real_t)ABS(p_to.y - p_from.y);
	return dx + dy;
}

static real_t heuristic_octile(const Vector2i &p_from, const Vector2i &p_to) {
	real_t dx = (real_t)ABS(p_to.x - p_from.x);
	real_t dy = (real_t)ABS(p_to.y - p_from.y);
	real_t F = Math_SQRT2 - 1;
	return (dx < dy) ? F * dx + dy : F * dy + dx;
}

static real_t heuristic_chebyshev(const Vector2i &p_from, const Vector2i &p_to) {
	real_t dx = (real_t)ABS(p_to.x - p_from.x);
	real_t dy = (real_t)ABS(p_to.y - p_from.y);
	return MAX(dx, dy);
}

static real_t (*heuristics[AStarGrid2D::HEURISTIC_MAX])(const Vector2i &, const Vector2i &) = { heuristic_euclidian, heuristic_manhattan, heuristic_octile, heuristic_chebyshev };

void AStarGrid2D::set_size(const Size2i &p_size) {
	ERR_FAIL_COND(p_size.x < 0 || p_size.y < 0);

	if (p_size != size) {
		size = p_size;
		dirty = true;
		map.destroy();
	}
}

Size2i AStarGrid2D::get_size() const {
	return size;
}

void AStarGrid2D::set_offset(const Vector2 &p_offset) {
	if (!offset.is_equal_approx(p_offset)) {
		offset = p_offset;
		dirty = true;
	}
}

Vector2 AStarGrid2D::get_offset() const {
	return offset;
}

void AStarGrid2D::set_cell_size(const Size2 &p_cell_size) {
	cell_size = p_cell_size;
}

Size2 AStarGrid2D::get_cell_size() const {
	return cell_size;
}

void AStarGrid2D::update() {
	map.create(size.x, size.y);
	dirty = false;
}

bool AStarGrid2D::is_dirty() const {
	return dirty;
}

void AStarGrid2D::set_jumping_enabled(bool p_enabled) {
	jumping_enabled = p_enabled;
}

bool AStarGrid2D::is_jumping_enabled() const {
	return jumping_enabled;
}

void AStarGrid2D::set_diagonal_mode(DiagonalMode p_diagonal_mode) {
	ERR_FAIL_INDEX(p_diagonal_mode, DIAGONAL_MODE_MAX);
	diagonal_mode = p_diagonal_mode;
}

AStarGrid2D::DiagonalMode AStarGrid2D::get_diagonal_mode() const {
	return diagonal_mode;
}

void AStarGrid2D::set_default_compute_heuristic(Heuristic p_heuristic) {
	ERR_FAIL_INDEX(p_heuristic, HEURISTIC_MAX);
	default_compute_heuristic = p_heuristic;
}

AStarGrid2D::Heuristic AStarGrid2D::get_default_compute_heuristic() const {
	return default_compute_heuristic;
}

void AStarGrid2D::set_default_estimate_heuristic(Heuristic p_heuristic) {
	ERR_FAIL_INDEX(p_heuristic, HEURISTIC_MAX);
	default_estimate_heuristic = p_heuristic;
}

AStarGrid2D::Heuristic AStarGrid2D::get_default_estimate_heuristic() const {
	return default_estimate_heuristic;
}

void AStarGrid2D::set_point_solid(const Vector2i &p_id, bool p_solid) {
	ERR_FAIL_COND_MSG(dirty, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_MSG(!is_in_boundsv(p_id), vformat("Can't set if point is disabled. Point out of bounds (%s/%s, %s/%s).", p_id.x, size.width, p_id.y, size.height));

	map.set_solid(p_id.x, p_id.y, p_solid);
}

bool AStarGrid2D::is_point_solid(const Vector2i &p_id) const {
	ERR_FAIL_COND_V_MSG(dirty, false, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_id), false, vformat("Can't get if point is disabled. Point out of bounds (%s/%s, %s/%s).", p_id.x, size.width, p_id.y, size.height));
	return map.is_solid(p_id.x, p_id.y);
}

void AStarGrid2D::set_point_weight_scale(const Vector2i &p_id, real_t p_weight_scale) {
	ERR_FAIL_COND_MSG(dirty, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_MSG(!is_in_boundsv(p_id), vformat("Can't set point's weight scale. Point out of bounds (%s/%s, %s/%s).", p_id.x, size.width, p_id.y, size.height));
	ERR_FAIL_COND_MSG(p_weight_scale < 0.0, vformat("Can't set point's weight scale less than 0.0: %f.", p_weight_scale));
	map.get_point(p_id.x, p_id.y).weight_scale = p_weight_scale;
}

real_t AStarGrid2D::get_point_weight_scale(const Vector2i &p_id) const {
	ERR_FAIL_COND_V_MSG(dirty, 0, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_id), 0, vformat("Can't get point's weight scale. Point out of bounds (%s/%s, %s/%s).", p_id.x, size.width, p_id.y, size.height));
	return map.get_point(p_id.x, p_id.y).weight_scale;
}

bool AStarGrid2D::_append_if_non_solid(int p_x, int p_y, LocalVector<Vector2i> &r_nbors) const {
	if (map.is_solid(p_x, p_y)) {
		return false;
	}

	r_nbors.push_back(Vector2i(p_x, p_y));
	return true;
}

void AStarGrid2D::_get_nbors(const Vector2i &p_center, LocalVector<Vector2i> &r_nbors) const {
	bool ts0 = false;
	bool td0 = false;
	bool ts1 = false;
	bool td1 = false;
	bool ts2 = false;
	bool td2 = false;
	bool ts3 = false;
	bool td3 = false;

	int cx = p_center.x;
	int cy = p_center.y;

	bool left = (cx > 0);
	bool right = (cx + 1 < size.width);
	bool top = (cy > 0);
	bool bottom = (cy + 1 < size.height);

	if (top && _append_if_non_solid(cx, cy - 1, r_nbors)) {
		ts0 = true;
	}
	if (right && _append_if_non_solid(cx + 1, cy, r_nbors)) {
		ts1 = true;
	}
	if (bottom && _append_if_non_solid(cx, cy + 1, r_nbors)) {
		ts2 = true;
	}
	if (left && _append_if_non_solid(cx - 1, cy, r_nbors)) {
		ts3 = true;
	}

	switch (diagonal_mode) {
		case DIAGONAL_MODE_ALWAYS: {
			td0 = true;
			td1 = true;
			td2 = true;
			td3 = true;
		} break;
		case DIAGONAL_MODE_NEVER: {
		} break;
		case DIAGONAL_MODE_AT_LEAST_ONE_WALKABLE: {
			td0 = ts3 || ts0;
			td1 = ts0 || ts1;
			td2 = ts1 || ts2;
			td3 = ts2 || ts3;
		} break;
		case DIAGONAL_MODE_ONLY_IF_NO_OBSTACLES: {
			td0 = ts3 && ts0;
			td1 = ts0 && ts1;
			td2 = ts1 && ts2;
			td3 = ts2 && ts3;
		} break;
		default:
			break;
	}

	if (td0 && top && left) {
		_append_if_non_solid(cx - 1, cy - 1, r_nbors);
	}
	if (td1 && top && right) {
		_append_if_non_solid(cx + 1, cy - 1, r_nbors);
	}
	if (td2 && bottom && right) {
		_append_if_non_solid(cx + 1, cy + 1, r_nbors);
	}
	if (td3 && bottom && left) {
		_append_if_non_solid(cx - 1, cy + 1, r_nbors);
	}
}

void AStarGrid2D::OpenList::debug() {
#ifdef GODOT_ASTARGRID2D_VERBOSE
	print_line("\t\topenlist " + itos(sorted_pass_ids.size()) + " items.");
	for (uint32_t n = 0; n < sorted_pass_ids.size(); n++) {
		const Pass &pass = passes[sorted_pass_ids[n]];
		print_line("\t\t\t" + String(Variant(pass.point)) + " g " + String(Variant(pass.g_score)) + ", f " + String(Variant(pass.f_score)));
	}
#endif
}

// This whole trick with passing a global to SortPoints is pretty gross, I'm not quite
// sure offhand an easy way to pass a lookup list to SortPoints (and ran out of patience), I'm pretty sure it's possible though
// (may have done it before), and it will be required for multithreading, because using a global only allows
// a single openlist at a time.
AStarGrid2D::OpenList *g_p_astar_openlist = nullptr;

template <class PASS>
struct SortPoints {
	_FORCE_INLINE_ bool operator()(uint32_t idA, uint32_t idB) const { // Returns true when the Point A is worse than Point B.

		// See comment above, really this should either be a template param or member variable or something.
		const PASS &A = g_p_astar_openlist->get_pass(idA);
		const PASS &B = g_p_astar_openlist->get_pass(idB);

		if (A.f_score > B.f_score) {
			return true;
		} else if (A.f_score < B.f_score) {
			return false;
		} else {
			return A.g_score < B.g_score; // If the f_costs are the same then prioritize the points that are further away from the start.
		}
	}
};

bool AStarGrid2D::_solve(const Vector2i &p_begin, const Vector2i &p_end, List<Vector2i> &r_path) const {
	if (map.is_solid(p_end.x, p_end.y)) {
		return false;
	}

	uint32_t width = get_size().x;
	uint32_t height = get_size().y;

	OpenList open_list;
	open_list.create(width, height);

	g_p_astar_openlist = &open_list;
	SortArray<uint32_t, SortPoints<Pass>> sorter;

	// seed open list
	uint32_t pass_id = UINT32_MAX;
	Pass *first_pass = open_list.request_pass(pass_id);
	first_pass->point = p_begin;
	first_pass->f_score = _estimate_cost(p_begin, p_end);
	open_list.sorted_add_pass(pass_id);

	open_list.bf_visited.set_bit(p_begin);

	LocalVector<Vector2i> nbors;

	while (!open_list.is_empty()) {
		open_list.debug();

		// Make sure the best pass is at the end of the open list.
		sorter.pop_heap(0, open_list.sorted_pass_ids.size(), open_list.sorted_pass_ids.ptr());

		uint32_t curr_pass_id = UINT32_MAX;
		Pass pass = *open_list.pop_pass(curr_pass_id);
		const Vector2i &pt = pass.point;

#ifdef GODOT_ASTARGRID2D_VERBOSE
		print_line("processing point from open list " + String(Variant(pt)));
#endif

		if (pt == p_end) {
			// Create a path and return.
			Pass *passp = &open_list.get_pass(curr_pass_id);

			while (passp->point != p_begin) {
				r_path.push_front(passp->point);
				passp = &open_list.get_pass(passp->prev_pass_id);
				DEV_ASSERT(passp);
			}
			r_path.push_front(p_begin);
			return true;
		}

		// Mark the point as closed.
		open_list.bf_closed.set_bit(pt);

		nbors.clear();
		_get_nbors(pt, nbors);

		for (uint32_t n = 0; n < nbors.size(); n++) {
			const Vector2i &n_pt = nbors[n];

			if (open_list.bf_closed.get_bit(n_pt.x, n_pt.y)) {
				continue;
			}

			Pass *n_pass = nullptr;
			uint32_t new_pass_id = UINT32_MAX;
			bool new_point = false;

			const MapPoint &mp = map.get_point(n_pt.x, n_pt.y);

			// The test for solid should have already been done when finding neighbours.
			real_t weight_scale = mp.weight_scale;

			// If this is the first time visiting this point, add it to the bitfield and create a new pass.
			if (open_list.bf_visited.check_and_set(n_pt)) {
				n_pass = open_list.request_pass(new_pass_id);
				n_pass->point = n_pt;
				n_pass->prev_pass_id = curr_pass_id;

				new_point = true;
			} else {
				// There is an existing pass, let's look it up.
				n_pass = &open_list.get_pass_at(n_pt.x, n_pt.y, new_pass_id);
			}

			real_t tentative_g_score = pass.g_score + _compute_cost(pt, n_pt) * weight_scale;

			if (!new_point && (tentative_g_score >= n_pass->g_score)) { // The new path is worse than the previous.
#ifdef GODOT_ASTARGRID2D_VERBOSE
				print_line("\t\tpoint at " + String(Variant(n_pt)) + " cost is worse, rejecting.");
#endif
				continue;
			}

			n_pass->prev_pass_id = curr_pass_id;
			n_pass->g_score = tentative_g_score;
			n_pass->f_score = n_pass->g_score + _estimate_cost(n_pt, p_end);

			if (!new_point) {
#ifdef GODOT_ASTARGRID2D_VERBOSE
				print_line("\tpoint at " + String(Variant(n_pt)) + " cost is better " + String(Variant(n_pass->g_score)));
#endif
				sorter.push_heap(0, open_list.sorted_pass_ids.find(new_pass_id), 0, new_pass_id, open_list.sorted_pass_ids.ptr());
			} else {
				open_list.sorted_add_pass(new_pass_id);
#ifdef GODOT_ASTARGRID2D_VERBOSE
				print_line("\tnew point at " + String(Variant(n_pt)) + " cost " + String(Variant(n_pass->g_score)));
#endif

				sorter.push_heap(0, open_list.sorted_pass_ids.size() - 1, 0, new_pass_id, open_list.sorted_pass_ids.ptr());
			}
		}
	}

	return false;
}

real_t AStarGrid2D::_estimate_cost(const Vector2i &p_from_id, const Vector2i &p_to_id) const {
	real_t scost;
	if (GDVIRTUAL_CALL(_estimate_cost, p_from_id, p_to_id, scost)) {
		return scost;
	}
	return heuristics[default_estimate_heuristic](p_from_id, p_to_id);
}

real_t AStarGrid2D::_compute_cost(const Vector2i &p_from_id, const Vector2i &p_to_id) const {
	real_t scost;
	if (GDVIRTUAL_CALL(_compute_cost, p_from_id, p_to_id, scost)) {
		return scost;
	}
	return heuristics[default_compute_heuristic](p_from_id, p_to_id);
}

void AStarGrid2D::clear() {
	map.clear();
	size = Vector2i();
}

Vector2 AStarGrid2D::get_point_position(const Vector2i &p_id) const {
	ERR_FAIL_COND_V_MSG(dirty, Vector2(), "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_id), Vector2(), vformat("Can't get point's position. Point out of bounds (%s/%s, %s/%s).", p_id.x, size.width, p_id.y, size.height));

	return _map_position(p_id);
}

Vector<Vector2> AStarGrid2D::get_point_path(const Vector2i &p_from_id, const Vector2i &p_to_id) const {
	ERR_FAIL_COND_V_MSG(dirty, Vector<Vector2>(), "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_from_id), Vector<Vector2>(), vformat("Can't get id path. Point out of bounds (%s/%s, %s/%s)", p_from_id.x, size.width, p_from_id.y, size.height));
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_to_id), Vector<Vector2>(), vformat("Can't get id path. Point out of bounds (%s/%s, %s/%s)", p_to_id.x, size.width, p_to_id.y, size.height));

	if (p_from_id == p_to_id) {
		Vector<Vector2> ret;
		ret.push_back(_map_position(p_from_id));
		return ret;
	}

	List<Vector2i> point_path;
	if (!_solve(p_from_id, p_to_id, point_path)) {
		return Vector<Vector2>();
	}

	Vector<Vector2> path;
	path.resize(point_path.size());
	Vector2 *w = path.ptrw();

	int64_t idx = 0;
	for (const Vector2i &p : point_path) {
		w[idx++] = _map_position(p);
	}

	return path;
}

TypedArray<Vector2i> AStarGrid2D::get_id_path(const Vector2i &p_from_id, const Vector2i &p_to_id) const {
	ERR_FAIL_COND_V_MSG(dirty, TypedArray<Vector2i>(), "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_from_id), TypedArray<Vector2i>(), vformat("Can't get id path. Point out of bounds (%s/%s, %s/%s)", p_from_id.x, size.width, p_from_id.y, size.height));
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_to_id), TypedArray<Vector2i>(), vformat("Can't get id path. Point out of bounds (%s/%s, %s/%s)", p_to_id.x, size.width, p_to_id.y, size.height));

	if (p_from_id == p_to_id) {
		TypedArray<Vector2i> ret;
		ret.push_back(p_from_id);
		return ret;
	}

	List<Vector2i> point_path;
	if (!_solve(p_from_id, p_to_id, point_path)) {
		return TypedArray<Vector2i>();
	}

	TypedArray<Vector2i> path;
	path.resize(point_path.size());

	int64_t idx = 0;
	for (const Vector2i &p : point_path) {
		path[idx++] = p;
	}

	return path;
}

void AStarGrid2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_size", "size"), &AStarGrid2D::set_size);
	ClassDB::bind_method(D_METHOD("get_size"), &AStarGrid2D::get_size);
	ClassDB::bind_method(D_METHOD("set_offset", "offset"), &AStarGrid2D::set_offset);
	ClassDB::bind_method(D_METHOD("get_offset"), &AStarGrid2D::get_offset);
	ClassDB::bind_method(D_METHOD("set_cell_size", "cell_size"), &AStarGrid2D::set_cell_size);
	ClassDB::bind_method(D_METHOD("get_cell_size"), &AStarGrid2D::get_cell_size);
	ClassDB::bind_method(D_METHOD("is_in_bounds", "x", "y"), &AStarGrid2D::is_in_bounds);
	ClassDB::bind_method(D_METHOD("is_in_boundsv", "id"), &AStarGrid2D::is_in_boundsv);
	ClassDB::bind_method(D_METHOD("is_dirty"), &AStarGrid2D::is_dirty);
	ClassDB::bind_method(D_METHOD("update"), &AStarGrid2D::update);
	ClassDB::bind_method(D_METHOD("set_jumping_enabled", "enabled"), &AStarGrid2D::set_jumping_enabled);
	ClassDB::bind_method(D_METHOD("is_jumping_enabled"), &AStarGrid2D::is_jumping_enabled);
	ClassDB::bind_method(D_METHOD("set_diagonal_mode", "mode"), &AStarGrid2D::set_diagonal_mode);
	ClassDB::bind_method(D_METHOD("get_diagonal_mode"), &AStarGrid2D::get_diagonal_mode);
	ClassDB::bind_method(D_METHOD("set_default_compute_heuristic", "heuristic"), &AStarGrid2D::set_default_compute_heuristic);
	ClassDB::bind_method(D_METHOD("get_default_compute_heuristic"), &AStarGrid2D::get_default_compute_heuristic);
	ClassDB::bind_method(D_METHOD("set_default_estimate_heuristic", "heuristic"), &AStarGrid2D::set_default_estimate_heuristic);
	ClassDB::bind_method(D_METHOD("get_default_estimate_heuristic"), &AStarGrid2D::get_default_estimate_heuristic);
	ClassDB::bind_method(D_METHOD("set_point_solid", "id", "solid"), &AStarGrid2D::set_point_solid, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("is_point_solid", "id"), &AStarGrid2D::is_point_solid);
	ClassDB::bind_method(D_METHOD("set_point_weight_scale", "id", "weight_scale"), &AStarGrid2D::set_point_weight_scale);
	ClassDB::bind_method(D_METHOD("get_point_weight_scale", "id"), &AStarGrid2D::get_point_weight_scale);
	ClassDB::bind_method(D_METHOD("clear"), &AStarGrid2D::clear);

	ClassDB::bind_method(D_METHOD("get_point_position", "id"), &AStarGrid2D::get_point_position);
	ClassDB::bind_method(D_METHOD("get_point_path", "from_id", "to_id"), &AStarGrid2D::get_point_path);
	ClassDB::bind_method(D_METHOD("get_id_path", "from_id", "to_id"), &AStarGrid2D::get_id_path);

	GDVIRTUAL_BIND(_estimate_cost, "from_id", "to_id")
	GDVIRTUAL_BIND(_compute_cost, "from_id", "to_id")

	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "size"), "set_size", "get_size");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "offset"), "set_offset", "get_offset");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "cell_size"), "set_cell_size", "get_cell_size");

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "jumping_enabled"), "set_jumping_enabled", "is_jumping_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "default_compute_heuristic", PROPERTY_HINT_ENUM, "Euclidean,Manhattan,Octile,Chebyshev"), "set_default_compute_heuristic", "get_default_compute_heuristic");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "default_estimate_heuristic", PROPERTY_HINT_ENUM, "Euclidean,Manhattan,Octile,Chebyshev"), "set_default_estimate_heuristic", "get_default_estimate_heuristic");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "diagonal_mode", PROPERTY_HINT_ENUM, "Never,Always,At Least One Walkable,Only If No Obstacles"), "set_diagonal_mode", "get_diagonal_mode");

	BIND_ENUM_CONSTANT(HEURISTIC_EUCLIDEAN);
	BIND_ENUM_CONSTANT(HEURISTIC_MANHATTAN);
	BIND_ENUM_CONSTANT(HEURISTIC_OCTILE);
	BIND_ENUM_CONSTANT(HEURISTIC_CHEBYSHEV);
	BIND_ENUM_CONSTANT(HEURISTIC_MAX);

	BIND_ENUM_CONSTANT(DIAGONAL_MODE_ALWAYS);
	BIND_ENUM_CONSTANT(DIAGONAL_MODE_NEVER);
	BIND_ENUM_CONSTANT(DIAGONAL_MODE_AT_LEAST_ONE_WALKABLE);
	BIND_ENUM_CONSTANT(DIAGONAL_MODE_ONLY_IF_NO_OBSTACLES);
	BIND_ENUM_CONSTANT(DIAGONAL_MODE_MAX);
}
