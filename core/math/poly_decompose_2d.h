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
	struct Point {
		Point() {
			pos_idx = -1;
			cross = 0.0;
			length = 0.0;
			reflex = false;
		}
		uint32_t pos_idx; // position index
		real_t cross;
		real_t length; // edge length formed from this to next point
		bool reflex;
		bool is_reflex() const { return reflex; }
	};

public:
	// ordered list of positions, counterclockwise
	bool decompose(const LocalVectori<Vector2> &p_positions, List<LocalVectori<uint32_t>> &r_result);

private:
	// helper funcs
	void remove_colinear(LocalVectori<Point> &r_edges);
	void sort_edgelist(LocalVectori<Point> &r_edges);
	void split_recursive(LocalVectori<Point> p_edges, List<LocalVectori<uint32_t>> &r_result, int p_count = 0);
	bool can_see(const LocalVectori<Point> &p_edges, int p_from, int p_to, bool p_change_is_positive) const;

	void try_split_reflex(const LocalVectori<Point> &p_edges, int p_reflex_id, int &r_seg_start, int &r_seg_end, real_t &r_best_fit);
	real_t try_split_reflex_generic(const LocalVectori<Point> &p_edges, int p_reflex_id, int p_change, const Vector2 &p_reflex_edge_a, const Vector2 &p_reflex_edge_b, int &r_seg_start, int &r_seg_end);

	void calculate_crosses(LocalVectori<Point> &r_edges);
	bool line_intersect_test(const Vector2 &p_0, const Vector2 &p_1, const Vector2 &p_2, const Vector2 &p_3) const;

	void write_result(const LocalVectori<Point> &p_edges, List<LocalVectori<uint32_t>> &r_result);

	// debugging
	void _debug_draw(const LocalVectori<Point> &p_edges, String p_filename);
	String _debug_vector_to_string(const LocalVectori<Point> &p_list);

	// inlines
	bool is_reflex(const LocalVectori<Point> &p_edges, int p_test_id) {
		return p_edges.get_wrapped(p_test_id).is_reflex();
	}

	real_t get_cross(const LocalVectori<Point> &p_edges, int p_test_id) const {
		const Vector2 &prev = get_edge_pos(p_edges, p_test_id - 1);
		const Vector2 &curr = get_edge_pos(p_edges, p_test_id);
		const Vector2 &next = get_edge_pos(p_edges, p_test_id + 1);
		return Geometry::vec2_cross(prev, curr, next);
	}

	real_t get_length(const LocalVectori<Point> &p_edges, int p_test_id) const {
		const Vector2 &next = get_edge_pos(p_edges, p_test_id + 1);
		const Vector2 &curr = get_edge_pos(p_edges, p_test_id);
		return (next - curr).length();
	}

	real_t get_edge_length(const LocalVectori<Point> &p_edges, int p_from, int p_to) const {
		if (p_from > p_to) {
			p_from -= p_edges.size();
		}
		real_t l = 0.0;
		for (int n = p_from; n < p_to; n++) {
			l += p_edges.get_wrapped(n).length;
		}
		return l;
	}

	const Vector2 &get_edge_pos(const LocalVectori<Point> &p_edges, int p_id) const {
		return _positions[p_edges.get_wrapped(p_id).pos_idx];
	}

	// mem vars
	const Vector2 *_positions = nullptr;
	int _num_positions = 0;
	const static real_t _cross_epsilon;

#ifdef GODOT_POLY_DECOMPOSE_DEBUG_DRAW
	DebugImage _debug_image;
#endif
};
