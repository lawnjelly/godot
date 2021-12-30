#pragma once

#include "core/local_vector.h"
#include "core/math/rect2.h"
#include "core/math/vector2.h"
#include "core/math/vector3.h"

#define SPATIAL_DEDUPLICATOR_USE_REFERENCE_IMPL

class SpatialDeduplicator {
	real_t _epsilon = 0.01;

	class Grid {
		enum { GRID_SIZE = 64 };

		struct Vert {
			Vector3 pos;
			uint32_t id;
		};

		struct Cell {
			LocalVectori<Vert> verts;
		};

		Vector2 _bound_min;
		Vector2 _bound_mult;

		LocalVectori<Cell> _cells;

	public:
		Grid() {
			_cells.resize(GRID_SIZE * GRID_SIZE);
		}
		Vector2 vec3_xy(const Vector3 &p_pt) const { return Vector2(p_pt.x, p_pt.z); }
		Cell &get_cell(int x, int y) {
			x = CLAMP(x, 0, GRID_SIZE - 1);
			y = CLAMP(y, 0, GRID_SIZE - 1);
			return _cells[(y * GRID_SIZE) + x];
		}
		const Cell &get_cell(int x, int y) const {
			x = CLAMP(x, 0, GRID_SIZE - 1);
			y = CLAMP(y, 0, GRID_SIZE - 1);
			return _cells[(y * GRID_SIZE) + x];
		}
		bool is_on_map(int x, int y) const {
			if ((x < 0) || (y < 0) || (x >= GRID_SIZE) || (y >= GRID_SIZE)) {
				return false;
			}
			return true;
		}
		Vector2i get_cell_xy(const Vector3 &p_pos) const {
			real_t x = p_pos.x;
			real_t y = p_pos.z;
			x -= _bound_min.x;
			x *= _bound_mult.x;
			y -= _bound_min.y;
			y *= _bound_mult.y;
			return Vector2i(x, y);
		}
		const Cell &get_cell_at_pos(const Vector3 &p_pos) const {
			Vector2i loc = get_cell_xy(p_pos);
			return get_cell(loc.x, loc.y);
		}
		void calc_bound(const Vector3 *p_verts, uint32_t p_num_verts);
		void add(const Vector3 &p_pos, uint32_t p_id);
		bool find(const Vector3 &p_pos, real_t p_epsilon, LocalVectori<uint32_t> &r_ids) const;
	};

public:
	struct Attribute {
		enum Type {
			AT_NORMAL,
			AT_UV,
			AT_POSITION,
		};
		Type type = AT_NORMAL;
		real_t epsilon_dedup = 0.0;
		real_t epsilon_merge = 0.0;
		union {
			const Vector3 *vec3s = nullptr;
			const Vector2 *vec2s;
		};
	};

	struct LinkedVerts {
		Vector3 pos;
		LocalVectori<uint32_t> vert_ids;
	};

	// As well as position, vertices can prevent merging based on
	// multiple optional custom attributes.
	// These will be tested prior to merging.
	LocalVectori<Attribute> _attributes;

	bool deduplicate_verts_only(const uint32_t *p_in_inds, uint32_t p_num_in_inds, const Vector3 *p_in_verts, uint32_t p_num_in_verts, LocalVectori<Vector3> &r_out_verts, LocalVectori<uint32_t> &r_out_inds, real_t p_epsilon = 0.01);

	bool deduplicate_map(const uint32_t *p_in_inds, uint32_t p_num_in_inds, const Vector3 *p_in_verts, uint32_t p_num_in_verts, LocalVectori<uint32_t> &r_vert_map, uint32_t &r_num_out_verts, LocalVectori<uint32_t> &r_out_inds, real_t p_epsilon = 0.01);

	void find_duplicate_positions(const Vector3 *p_in_verts, uint32_t p_num_in_verts, LocalVectori<LinkedVerts> &r_linked_verts_list, real_t p_epsilon = 0.01);
};
