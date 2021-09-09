#include "poly_decompose_2d.h"

const real_t PolyDecompose2D::_cross_epsilon = 0.001;

String PolyDecompose2D::_debug_vector_to_string(const LocalVectori<uint32_t> &p_list) {
	String sz;
	for (int n = 0; n < p_list.size(); n++) {
		sz += itos(p_list[n]) + ", ";
	}
	return sz;
}

void PolyDecompose2D::_debug_draw(const LocalVectori<uint32_t> &p_edges, String p_filename) {
#ifdef GODOT_POLY_DECOMPOSE_DEBUG_DRAW
	if (!p_edges.size()) {
		return;
	}

	DebugImage &im = _debug_image;

	im.fill();
	im.l_move(get_edge_pos(p_edges, 0));
	im.l_draw_num(p_edges[0]);
	for (int n = 0; n < p_edges.size(); n++) {
		im.l_line_to(get_edge_pos(p_edges, n));
		im.l_draw_num(p_edges[n]);
	}
	im.l_line_to(get_edge_pos(p_edges, 0));
	im.l_flush(false);
	im.save_png(p_filename);
#endif
}

bool PolyDecompose2D::decompose(const LocalVectori<Vector2> &p_positions, List<LocalVectori<uint32_t>> &r_result) {
	if (p_positions.size() < 3) {
		return false;
	}

#ifdef GODOT_POLY_DECOMPOSE_DEBUG_DRAW
	_debug_image.create(240, 240);
#endif

	_positions = &p_positions[0];
	_num_positions = p_positions.size();

	// create a list of edges
	LocalVectori<uint32_t> edges;
	for (int n = 0; n < p_positions.size(); n++) {
		int idx = n;

		// deduplicate
#define GODOT_POLY_DECOMPOSE_DEDUPLICATE
#ifdef GODOT_POLY_DECOMPOSE_DEDUPLICATE
		for (int d = 0; d < n; d++) {
			if (p_positions[d] == p_positions[n]) {
				idx = d;
				break;
			}
		}
#endif

		edges.push_back(idx);
	}

	// as a preprocess, remove superfluous verts
	// this makes things simpler and faster
	remove_colinear(edges);

	if (edges.size() < 3) {
		return false;
	}

	split_recursive(edges, r_result);

	return true;
}

void PolyDecompose2D::remove_zero_area_segments(LocalVectori<uint32_t> &r_edges) {
	remove_colinear(r_edges);
}

void PolyDecompose2D::remove_colinear(LocalVectori<uint32_t> &r_edges) {
	for (int n = 0; n < r_edges.size(); n++) {
		real_t cross = get_cross(r_edges, n + 1);
		if (Math::abs(cross) < _cross_epsilon) {
			r_edges.remove(n);
			n--;
		}
	}
}

void PolyDecompose2D::sort_edgelist(LocalVectori<uint32_t> &r_edges) {
	// always start from the left most point, this is to ensure that
	// the first edge is part of the convex hull
	Vector2 leftmost_pt = Vector2(FLT_MAX, FLT_MAX);
	int leftmost = -1;

	for (int n = 0; n < r_edges.size(); n++) {
		const Vector2 pos = get_edge_pos(r_edges, n);

		if (pos < leftmost_pt) {
			leftmost_pt = pos;
			leftmost = n;
		}
	}

	// rejig the list P so that the leftmost is first
	if (leftmost != 0) {
		LocalVectori<uint32_t> temp;
		temp.resize(r_edges.size());
		for (int n = 0; n < r_edges.size(); n++) {
			temp[n] = r_edges[(leftmost + n) % r_edges.size()];
		}

		r_edges = temp;
	}
}

int PolyDecompose2D::split_opposite(LocalVectori<uint32_t> p_edges, int p_reflex_id) {
	// find the first edge of the reflex vert
	const Vector2 &reflex_pos = get_edge_pos(p_edges, p_reflex_id);
	const Vector2 &reflex_pos_next = get_edge_pos(p_edges, p_reflex_id + 1);

	int num_edges = p_edges.size();

	// find the furthest vert in clockwise direction from the reflex that is on the left of the reflex edge, or is reflex itself
	int start_vert = -1;
	for (int n = 1; n < num_edges; n++) {
		int t = wrap_index(num_edges, p_reflex_id + n);

		const Vector2 &pos = get_edge_pos(p_edges, t);
		real_t cross = Geometry::vec2_cross(reflex_pos, reflex_pos_next, pos);

		if (cross < -_cross_epsilon) {
			// we are done, the previous start_vert must be used
			// because we are becoming concave
			break;
		}

		start_vert = t;

		// is it reflex? if so terminate
		if (is_reflex(p_edges, t)) {
			// this should not happen
			//CRASH_COND(true);
			break;
		}
	}

	CRASH_COND(start_vert == -1);

	return start_vert;
}

void PolyDecompose2D::split_recursive(LocalVectori<uint32_t> p_edges, List<LocalVectori<uint32_t>> &r_result, int p_count) {
	int num_edges = p_edges.size();
	sort_edgelist(p_edges);

	_debug_draw(p_edges, "input/facein" + itos(p_count) + ".png");
	print_line("split " + itos(p_count) + " : " + _debug_vector_to_string(p_edges));

	// find first reflex
	int reflex_id = -1;
	for (int n = 0; n < num_edges; n++) {
		if (is_reflex(p_edges, n)) {
			//  special .. can't use it if the vertex prior is also reflex (can't form triangle)
			if (is_reflex(p_edges, n - 1))
				continue;

			reflex_id = n;
			break;
		}
	}

	// no more reflex, just copy the remaining edges to the result
	if (reflex_id == -1) {
		r_result.push_back(p_edges);
		return;
	}

	// find the first edge of the reflex vert
	const Vector2 &reflex_pos_prev = get_edge_pos(p_edges, reflex_id - 1);
	const Vector2 &reflex_pos = get_edge_pos(p_edges, reflex_id);

	// find the furthest vert in clockwise direction from the reflex that is on the left of the reflex edge, or is reflex itself
	int start_vert = -1;
	for (int n = 1; n < num_edges; n++) {
		int t = wrap_index(num_edges, reflex_id - n);

		const Vector2 &pos = get_edge_pos(p_edges, t);
		real_t cross = Geometry::vec2_cross(reflex_pos_prev, reflex_pos, pos);

		if (cross < -_cross_epsilon) {
			// we are done, the previous start_vert must be used
			// because we are becoming concave
			break;
		}

		start_vert = t;

		// is it reflex? if so terminate
		if (is_reflex(p_edges, t)) {
			// this reflex may be better than the first .. we need to check this
			const Vector2 &reflex_pos2 = get_edge_pos(p_edges, t);
			const Vector2 &reflex_pos_next2 = get_edge_pos(p_edges, t + 1);

			int start_vert_opp = split_opposite(p_edges, t);

			real_t cross2 = Geometry::vec2_cross(reflex_pos2, reflex_pos_next2, reflex_pos);
			if ((cross2 < -_cross_epsilon) || (start_vert_opp != reflex_id)) {
				// we need to take the opposite approach
				//				reflex_id = t;
				//				start_vert = split_opposite(p_edges, reflex_id);
				reflex_id = t;
				start_vert = start_vert_opp;

				// we need to reverse the order of the reflex and start vertex, because the remaining logic is based
				// around the cut segment from start to reflex, not reflex to start
				SWAP(start_vert, reflex_id);
			}
			break;
		}
	}

	// check for -1 .. should not happen...
	CRASH_COND(start_vert == -1);

	// create a new segment for this edgelist
	LocalVectori<uint32_t> segment;

	// make sure we loop upwards
	if (start_vert > reflex_id) {
		start_vert -= num_edges;
	}

	print_line("\tstart_vert: " + itos(start_vert) + " (" + itos(p_edges[wrap_index(num_edges, start_vert)]) + "), reflex_vert: " + itos(reflex_id) + " (" + itos(p_edges[wrap_index(num_edges, reflex_id)]) + ")");

	// add the clockwise segment (not including zero)
	for (int n = start_vert; n <= reflex_id; n++) {
		int i = wrap_index(num_edges, n);
		segment.push_back(p_edges[i]);
	}

	r_result.push_back(segment);

	_debug_draw(segment, "output/faceout" + itos(p_count) + ".png");
	print_line("\tcut : " + _debug_vector_to_string(segment));

	// remove the segment from the original edgelist, and call recursively

	// reuse same vector .. add the remnants to it
	segment.clear();

	start_vert += num_edges;

	for (int n = reflex_id; n <= start_vert; n++) {
		int i = wrap_index(num_edges, n);
		segment.push_back(p_edges[i]);
	}

	// as a result of a removal we can sometimes end up with a zero area
	// segment .. this can muck up and cause folding in on itself so we need to remove these
	remove_zero_area_segments(segment);

	split_recursive(segment, r_result, p_count + 1);
}
