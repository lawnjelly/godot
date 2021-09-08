#pragma once

#include "core/list.h"
#include "core/local_vector.h"
#include "core/math/geometry.h"
#include "core/math/vector2.h"

//#define GODOT_POLY_DECOMPOSE_DEBUG_DRAW
#ifdef GODOT_POLY_DECOMPOSE_DEBUG_DRAW
#include "core/debug_image.h"
#endif

class PolyDecompose2D {
public:
	// ordered list of positions, counterclockwise
	bool decompose(const LocalVectori<Vector2> &p_positions, List<LocalVectori<uint32_t>> &r_result);

private:
	void remove_colinear(LocalVectori<uint32_t> &r_edges);
	void sort_edgelist(LocalVectori<uint32_t> &r_edges);
	void split_recursive(LocalVectori<uint32_t> p_edges, List<LocalVectori<uint32_t>> &r_result, int p_count = 0);
	int split_opposite(LocalVectori<uint32_t> p_edges, int p_reflex_id);

	void _debug_draw(const LocalVectori<uint32_t> &p_edges, String p_filename);
	String _debug_vector_to_string(const LocalVectori<uint32_t> &p_list);

	bool is_reflex(const LocalVectori<uint32_t> &p_edges, int p_test_id) {
		return get_cross(p_edges, p_test_id + 1) < -_cross_epsilon;
	}

	// note this is from the edge BEFORE the test vert
	real_t get_cross(const LocalVectori<uint32_t> &p_edges, int p_test_id) const {
		const Vector2 &prev2 = get_edge_pos(p_edges, p_test_id - 2);
		const Vector2 &prev = get_edge_pos(p_edges, p_test_id - 1);
		const Vector2 &curr = get_edge_pos(p_edges, p_test_id);
		return Geometry::vec2_cross(prev2, prev, curr);
	}

	int wrap_index(int p_size, int p_idx) const {
		if (p_idx >= 0) {
			return p_idx % p_size;
		}
		return p_idx + p_size;
	}

	const Vector2 &get_edge_pos(const LocalVectori<uint32_t> &p_edges, int p_id) const {
		p_id = wrap_index(p_edges.size(), p_id);
		return _positions[p_edges[p_id]];
	}

	const Vector2 *_positions = nullptr;
	int _num_positions = 0;
	const static real_t _cross_epsilon;

#ifdef GODOT_POLY_DECOMPOSE_DEBUG_DRAW
	DebugImage _debug_image;
#endif
};
