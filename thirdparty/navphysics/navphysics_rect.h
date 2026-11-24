// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#pragma once

#include "navphysics_pointf.h"
#include "navphysics_pointi.h"

namespace NavPhysics {

// Exclusive definition.
struct [[nodiscard]] IRect2 {
	IPoint2 position;
	IPoint2 size;

	i64 get_area() const { return (i64)size.x * (i64)size.y; }
	IPoint2 end() const { return position + size; }

	void zero() {
		position.zero();
		size.zero();
	}

	// Exclusive.
	bool clamp_from_zero(const IPoint2 &p_max_size) {
		if (position.x < 0) {
			size.x += position.x;
			position.x = 0;
		}
		if (position.y < 0) {
			size.y += position.y;
			position.y = 0;
		}

		IPoint2 e = end();
		e -= p_max_size;
		if (e.x >= 0) {
			size.x -= e.x;
		}
		if (e.y >= 0) {
			size.y -= e.y;
		}

		return (size.x > 0) && (size.y > 0);
	}

	bool intersects(const IRect2 &p_o) const {
		IPoint2 e = end();
		IPoint2 o_e = p_o.end();

		for (u32 n = 0; n < 2; n++) {
			if (p_o.position.coord[n] >= e.coord[n]) {
				return false;
			}
			if (position.coord[n] >= o_e.coord[n]) {
				return false;
			}
		}
		return true;
	}

	void expand_to(const IRect2 &o) {
		IPoint2 begin = position;
		IPoint2 end = position + size;
		IPoint2 end_o = o.end();

		for (u32 n = 0; n < 2; n++) {
			begin.coord[n] = MIN(begin.coord[n], o.position.coord[n]);
			end.coord[n] = MAX(end.coord[n], end_o.coord[n]);
		}

		position = begin;
		size = end - begin;
	}

	void expand_to_fast(const IPoint2 &p_vector) {
		IPoint2 begin = position;
		IPoint2 end = position + size;

		for (u32 n = 0; n < 2; n++) {
			begin.coord[n] = MIN(begin.coord[n], p_vector.coord[n]);
			end.coord[n] = MAX(end.coord[n], p_vector.coord[n]);
		}

		position = begin;
		size = end - begin;
	}
	void increment_size() {
		size += IPoint2::make(1, 1);
	}
	void grow(i32 p_v) {
		IPoint2 boost(p_v, p_v);
		position -= boost;
		size += (boost * 2);
	}
	bool contains_point(const IPoint2 &p_pt) const {
		IPoint2 pt = p_pt - position;
		for (u32 n = 0; n < 2; n++) {
			if (pt.coord[n] < 0) {
				return false;
			}
		}
		for (u32 n = 0; n < 2; n++) {
			if (pt.coord[n] >= size.coord[n]) {
				return false;
			}
		}

		return true;
	}

	// Distance to a point.
	i64 distance_squared(const IPoint2 &p_pt) const {
		// Empty rects contain no points.
		if (size.x <= 0 || size.y <= 0) {
			return INT64_MAX;
		}

		i64 dist_sq = 0;
		for (u32 n = 0; n < 2; n++) {
			i64 min_c = (i64)position.coord[n];
			i64 max_c = min_c + (i64)size.coord[n] - 1;
			i64 c = (i64)p_pt.coord[n];

			i64 d = 0;
			if (c < min_c) {
				d = min_c - c;
			} else if (c > max_c) {
				d = c - max_c;
			}
			dist_sq += d * d;
		}
		return dist_sq;
	}

	IRect2() = default;

	IRect2(const IPoint2 &p_position, const IPoint2 &p_size) {
		position = p_position;
		size = p_size;
	}
};

// Exclusive definition.
struct [[nodiscard]] IRect3 {
	IPoint3 position;
	IPoint3 size;

	// Getting 2D versions.
	IPoint2 get_position2() const { return position.xz(); }
	IPoint2 get_size2() const { return size.xz(); }
	IRect2 get_rect2() const { return IRect2(get_position2(), get_size2()); }

	i64 get_volume() const { return (i64)size.x * (i64)size.y * (i64)size.z; }
	IPoint3 end() const { return position + size; }

	void zero() {
		position.zero();
		size.zero();
	}

	bool intersects(const IRect3 &p_o) const {
		IPoint3 e = end();
		IPoint3 o_e = p_o.end();

		for (u32 n = 0; n < 3; n++) {
			if (p_o.position.coord[n] >= e.coord[n]) {
				return false;
			}
			if (position.coord[n] >= o_e.coord[n]) {
				return false;
			}
		}
		return true;
	}

	void expand_to(const IRect3 &o) {
		IPoint3 begin = position;
		IPoint3 end = position + size;
		IPoint3 end_o = o.end();

		for (u32 n = 0; n < 3; n++) {
			begin.coord[n] = MIN(begin.coord[n], o.position.coord[n]);
			end.coord[n] = MAX(end.coord[n], end_o.coord[n]);
		}

		position = begin;
		size = end - begin;
	}

	void expand_to_fast(const IPoint3 &p_vector) {
		IPoint3 begin = position;
		IPoint3 end = position + size;

		for (u32 n = 0; n < 3; n++) {
			begin.coord[n] = MIN(begin.coord[n], p_vector.coord[n]);
			end.coord[n] = MAX(end.coord[n], p_vector.coord[n]);
		}

		position = begin;
		size = end - begin;
	}
	void increment_size() {
		size += IPoint3(1, 1, 1);
	}
	void grow(i32 p_v) {
		IPoint3 boost(p_v, p_v, p_v);
		position -= boost;
		size += (boost * 2);
	}
	bool contains_point(const IPoint3 &p_pt) const {
		IPoint3 pt = p_pt - position;
		for (u32 n = 0; n < 3; n++) {
			if (pt.coord[n] < 0) {
				return false;
			}
		}
		for (u32 n = 0; n < 3; n++) {
			if (pt.coord[n] >= size.coord[n]) {
				return false;
			}
		}

		return true;
	}

	bool is_empty_or_negative() const {
		return size.x <= 0 || size.y <= 0 || size.z <= 0;
	}

	// Distance to a point.
	i64 distance_squared(const IPoint3 &p_pt) const {
		// Empty rects contain no points.
		if (is_empty_or_negative()) {
			return INT64_MAX;
		}

		i64 dist_sq = 0;
		for (u32 n = 0; n < 3; n++) {
			i64 min_c = (i64)position.coord[n];
			i64 max_c = min_c + (i64)size.coord[n] - 1;
			i64 c = (i64)p_pt.coord[n];

			i64 d = 0;
			if (c < min_c) {
				d = min_c - c;
			} else if (c > max_c) {
				d = c - max_c;
			}
			dist_sq += d * d;
		}
		return dist_sq;
	}
};

struct [[nodiscard]] Rect2 {
	FPoint2 position;
	FPoint2 size;

	freal get_area() const { return size.x * size.y; }
	FPoint2 end() const { return position + size; }

	void expand_to(const FPoint2 &p_vector) {
		FPoint2 begin = position;
		FPoint2 end = position + size;

		for (u32 n = 0; n < 2; n++) {
			begin.coord[n] = MIN(begin.coord[n], p_vector.coord[n]);
			end.coord[n] = MAX(end.coord[n], p_vector.coord[n]);
		}

		position = begin;
		size = end - begin;
	}
	bool intersects(const Rect2 &p_o) const {
		FPoint2 e = end();
		FPoint2 o_e = p_o.end();

		for (u32 n = 0; n < 2; n++) {
			if (p_o.position.coord[n] > e.coord[n]) {
				return false;
			}
			if (position.coord[n] > o_e.coord[n]) {
				return false;
			}
		}
		return true;
	}
};

struct [[nodiscard]] AABB {
	FPoint3 position;
	FPoint3 size;

	freal get_volume() const { return size.x * size.y * size.z; }
	void zero() {
		position.zero();
		size.zero();
	}
	freal end_coord(u32 p_coord) const { return position.coord[p_coord] + size.coord[p_coord]; }
	FPoint3 end() const { return position + size; }
	
	void grow(freal p_change)
	{
		FPoint3 change(p_change, p_change, p_change);
		position -= change;
		size += (change * 2);
	}
	
	void expand_to(const FPoint3 &p_vector) {
		FPoint3 begin = position;
		FPoint3 end = position + size;

		for (u32 n = 0; n < 3; n++) {
			begin.coord[n] = MIN(begin.coord[n], p_vector.coord[n]);
			end.coord[n] = MAX(end.coord[n], p_vector.coord[n]);
		}

		position = begin;
		size = end - begin;
	}

	// p_aabb is above, NOT this AABB.
	bool overlaps_aabb_or_above(const AABB &p_aabb) const {
		for (u32 n = 0; n < 3; n++) {
			if (p_aabb.end_coord(n) < position.coord[n]) {
				return false;
			}
		}
		if (end_coord(0) < p_aabb.position.x) {
			return false;
		}
		if (end_coord(2) < p_aabb.position.z) {
			return false;
		}

		return true;
	}

	bool contains_point_or_above(const FPoint3 &p_pt) const {
		FPoint3 pt = p_pt - position;
		if (pt.x < 0) {
			return false;
		}
		if (pt.y < 0) {
			return false;
		}
		if (pt.z < 0) {
			return false;
		}
		if (pt.x > size.x) {
			return false;
		}
		if (pt.z > size.z) {
			return false;
		}

		return true;
	}

	freal distance_squared_to(const FPoint3 &p_pt) const {
		freal sq_distance = 0;

		freal min_x = position.x;
		freal max_x = position.x + size.x;
		if (p_pt.x < min_x) {
			freal v = p_pt.x - min_x;
			sq_distance += v * v;
		} else if (p_pt.x > max_x) {
			freal v = p_pt.x - max_x;
			sq_distance += v * v;
		}

		freal min_y = position.y;
		freal max_y = position.y + size.y;
		if (p_pt.y < min_y) {
			freal v = p_pt.y - min_y;
			sq_distance += v * v;
		} else if (p_pt.y > max_y) {
			freal v = p_pt.y - max_y;
			sq_distance += v * v;
		}

		freal min_z = position.z;
		freal max_z = position.z + size.z;
		if (p_pt.z < min_z) {
			freal v = p_pt.z - min_z;
			sq_distance += v * v;
		} else if (p_pt.z > max_z) {
			freal v = p_pt.z - max_z;
			sq_distance += v * v;
		}

		return sq_distance;
	}

	freal distance_to(const FPoint3 &p_pt) const {
		return Math::sqrt_real(distance_squared_to(p_pt));
	}

	void confine_point(FPoint3 &r_pt) const {
		FPoint3 end = position + size;
		for (u32 n = 0; n < 3; n++) {
			r_pt.coord[n] = MAX(r_pt.coord[n], position.coord[n]);
			r_pt.coord[n] = MIN(r_pt.coord[n], end.coord[n]);
		}
	}

	bool contains_point(const FPoint3 &p_pt) const {
		FPoint3 pt = p_pt - position;
		for (u32 n = 0; n < 3; n++) {
			if (pt.coord[n] < 0) {
				return false;
			}
		}
		for (u32 n = 0; n < 3; n++) {
			if (pt.coord[n] > size.coord[n]) {
				return false;
			}
		}

		return true;
	}
};

} // namespace NavPhysics
