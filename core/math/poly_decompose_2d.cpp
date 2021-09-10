#include "poly_decompose_2d.h"

#include "core/debug_image.h"

#ifdef GODOT_POLY_DECOMPOSE_DEBUG_DRAW
#define GODOT_POLY_DECOMPOSE_DEBUG_VERBOSE
#endif

const real_t PolyDecompose2D::_cross_epsilon = 0.001;

String PolyDecompose2D::_debug_vector_to_string(const LocalVectori<Point> &p_list) {
	String sz;
	for (int n = 0; n < p_list.size(); n++) {
		sz += itos(p_list[n].pos_idx) + ", ";
	}
	return sz;
}

void PolyDecompose2D::_debug_draw(const LocalVectori<Point> &p_edges, String p_filename) {
#ifdef GODOT_POLY_DECOMPOSE_DEBUG_DRAW
	if (!p_edges.size()) {
		return;
	}

	DebugImage &im = _debug_image;

	im.fill();
	im.l_move(get_edge_pos(p_edges, 0));
	im.l_draw_num(p_edges[0].pos_idx);
	for (int n = 0; n < p_edges.size(); n++) {
		im.l_line_to(get_edge_pos(p_edges, n));
		im.l_draw_num(p_edges[n].pos_idx);
	}
	im.l_line_to(get_edge_pos(p_edges, 0));
	im.l_flush(false);
	im.save_png(p_filename);
#endif
}

void PolyDecompose2D::calculate_crosses(LocalVectori<Point> &r_edges) {
	// calculate lengths and crosses
	for (int n = 0; n < r_edges.size(); n++) {
		Point &pt = r_edges[n];
		pt.cross = get_cross(r_edges, n);
		pt.length = get_length(r_edges, n);
		pt.reflex = pt.cross < -_cross_epsilon;
	}
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
	LocalVectori<Point> edges;
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

		Point pt;
		pt.pos_idx = idx;
		edges.push_back(pt);
	}

	// calculate lengths and crosses
	calculate_crosses(edges);

	// as a preprocess, remove superfluous verts
	// this makes things simpler and faster
	remove_colinear(edges);

	if (edges.size() < 3) {
		return false;
	}

	split_recursive(edges, r_result);

	return true;
}

void PolyDecompose2D::remove_colinear(LocalVectori<Point> &r_edges) {
	for (int n = 0; n < r_edges.size(); n++) {
		const Point &pt = r_edges[n];
		if (Math::abs(pt.cross) < _cross_epsilon) {
			// we must add the removed length to the next
			r_edges.get_wrapped(n - 1).length += r_edges[n].length;
			r_edges.remove(n);
			n--;
		}
	}
}

void PolyDecompose2D::sort_edgelist(LocalVectori<Point> &r_edges) {
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
		LocalVectori<Point> temp;
		temp.resize(r_edges.size());
		for (int n = 0; n < r_edges.size(); n++) {
			temp[n] = r_edges[(leftmost + n) % r_edges.size()];
		}

		r_edges = temp;
	}
}

real_t PolyDecompose2D::try_split_reflex_generic(const LocalVectori<Point> &p_edges, int p_reflex_id, int p_change, const Vector2 &p_reflex_edge_a, const Vector2 &p_reflex_edge_b, int &r_seg_start, int &r_seg_end) {
	// can't use if next to another reflex, cannot form a triangle
	if (p_edges.get_wrapped(p_reflex_id + p_change).is_reflex()) {
		return FLT_MAX;
	}

	int num_edges = p_edges.size();
	const Vector2 &reflex_pos = get_edge_pos(p_edges, p_reflex_id);

	// find the furthest vert in counter clockwise direction from the reflex that is on the left of the reflex edge, or is reflex itself
	int start_vert = -1;
	int offset = 0;

	for (int n = 1; n < num_edges; n++) {
		offset += p_change;
		int t = p_edges.wrap_index(p_reflex_id + offset);

		const Vector2 &pos = get_edge_pos(p_edges, t);
		real_t cross = Geometry::vec2_cross(p_reflex_edge_a, p_reflex_edge_b, pos);

		if (cross < -_cross_epsilon) {
			// we are done, the previous start_vert must be used
			// because we are becoming concave
			break;
		}

		// don't allow if the t vert edge has the reflex vert on the wrong side
		const Vector2 &pos_before = get_edge_pos(p_edges, t - p_change);

		if (p_change > 0) {
			cross = Geometry::vec2_cross(pos_before, pos, reflex_pos);
		} else {
			cross = Geometry::vec2_cross(pos, pos_before, reflex_pos);
		}

		if (cross < -_cross_epsilon) {
			// we are done, the previous start_vert must be used
			// because we are becoming concave
			break;
		}

		start_vert = t;

		// is it reflex? if so terminate
		if (is_reflex(p_edges, t)) {
			break;
		}
	}

	// check for -1 .. should not happen...
	CRASH_COND(start_vert == -1);

	real_t fit = 0.0;
	if (p_change > 0) {
		r_seg_start = p_reflex_id;
		r_seg_end = start_vert;
	} else {
		r_seg_start = start_vert;
		r_seg_end = p_reflex_id;
	}

	//fit =  get_edge_length(p_edges, r_seg_start, r_seg_end);
	fit = (get_edge_pos(p_edges, r_seg_start) - get_edge_pos(p_edges, r_seg_end)).length();
	return fit;
}

// return goodness of fit
void PolyDecompose2D::try_split_reflex(const LocalVectori<Point> &p_edges, int p_reflex_id, int &r_seg_start, int &r_seg_end, real_t &r_best_fit) {
	//	real_t best_fit = 0.0;

	//print_line("try_split_reflex " + itos(p_edges.get_wrapped(p_reflex_id).pos_idx));

	// try counterclockwise

	int seg_start = -1;
	int seg_end = -1;

	{
		// find the first edge of the reflex vert
		const Vector2 &reflex_pos = get_edge_pos(p_edges, p_reflex_id);
		const Vector2 &reflex_pos_next = get_edge_pos(p_edges, p_reflex_id + 1);

		real_t fit = try_split_reflex_generic(p_edges, p_reflex_id, 1, reflex_pos, reflex_pos_next, seg_start, seg_end);
		if (fit < FLT_MAX) {
			//print_line("\t\tccw " + rtos(fit));
		} else {
			//print_line("\t\tccw -");
		}
		if (fit < r_best_fit) {
			r_best_fit = fit;
			r_seg_start = seg_start;
			r_seg_end = seg_end;
		}
	}

	// try clockwise
	{
		// find the first edge of the reflex vert
		const Vector2 &reflex_pos_prev = get_edge_pos(p_edges, p_reflex_id - 1);
		const Vector2 &reflex_pos = get_edge_pos(p_edges, p_reflex_id);

		real_t fit = try_split_reflex_generic(p_edges, p_reflex_id, -1, reflex_pos_prev, reflex_pos, seg_start, seg_end);
		if (fit < FLT_MAX) {
			//print_line("\t\tcw " + rtos(fit));
		} else {
			//print_line("\t\tcw -");
		}
		if (fit < r_best_fit) {
			r_best_fit = fit;
			r_seg_start = seg_start;
			r_seg_end = seg_end;
		}
	}
}

void PolyDecompose2D::split_recursive(LocalVectori<Point> p_edges, List<LocalVectori<uint32_t>> &r_result, int p_count) {
	int num_edges = p_edges.size();
	sort_edgelist(p_edges);

#ifdef GODOT_POLY_DECOMPOSE_DEBUG_VERBOSE
	_debug_draw(p_edges, "input/facein" + itos(p_count) + ".png");
	print_line("split " + itos(p_count) + " : " + _debug_vector_to_string(p_edges));
#endif

	// find all the reflex verts, and consider each in turn
	//LocalVectori<uint32_t> reflexes;
	real_t best_fit = FLT_MAX;
	//	int best_reflex = -1;
	int seg_start = -1;
	int seg_end = -1;

	for (int n = 0; n < p_edges.size(); n++) {
		if (p_edges[n].is_reflex()) {
			try_split_reflex(p_edges, n, seg_start, seg_end, best_fit);
			//			if (fit > best_fit)
			//			{
			//				best_fit = fit;
			//				best_reflex = n;
			//			}
			//reflexes.push_back(n);
		}
	}

	// no more reflex, just copy the remaining edges to the result
	if (seg_start == seg_end) {
		write_result(p_edges, r_result);
		return;
	}

	// create a new segment for this edgelist
	LocalVectori<Point> segment;

	// make sure we loop upwards
	if (seg_start > seg_end) {
		seg_start -= num_edges;
	}

#ifdef GODOT_POLY_DECOMPOSE_DEBUG_VERBOSE
	print_line("\tseg_start: " + itos(seg_start) + " (" + itos(p_edges.get_wrapped(seg_start).pos_idx) + "), seg_end: " + itos(seg_end) + " (" + itos(p_edges.get_wrapped(seg_end).pos_idx) + ")");
#endif

	// add the clockwise segment (not including zero)
	for (int n = seg_start; n <= seg_end; n++) {
		segment.push_back(p_edges.get_wrapped(n));
	}

	write_result(segment, r_result);

#ifdef GODOT_POLY_DECOMPOSE_DEBUG_VERBOSE
	_debug_draw(segment, "output/faceout" + itos(p_count) + ".png");
	print_line("\tcut : " + _debug_vector_to_string(segment));
#endif

	// remove the segment from the original edgelist, and call recursively

	// reuse same vector .. add the remnants to it
	segment.clear();

	seg_start += num_edges;

	for (int n = seg_end; n <= seg_start; n++) {
		segment.push_back(p_edges.get_wrapped(n));
	}

	// make sure crosses are up to date
	calculate_crosses(segment);

	// as a result of a removal we can sometimes end up with a zero area
	// segment .. this can muck up and cause folding in on itself so we need to remove these
	remove_colinear(segment);

	split_recursive(segment, r_result, p_count + 1);
}

void PolyDecompose2D::write_result(const LocalVectori<Point> &p_edges, List<LocalVectori<uint32_t>> &r_result) {
	LocalVectori<uint32_t> res;
	res.resize(p_edges.size());

	for (int n = 0; n < p_edges.size(); n++) {
		res[n] = p_edges[n].pos_idx;
	}
	r_result.push_back(res);
}
