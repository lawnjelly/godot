#include "mesh_simplify.h"
#include "core/math/geometry.h"
#include "core/math/vertex_cache_optimizer.h"
#include "core/print_string.h"
#include "scene/resources/surface_tool.h"

//#define GODOT_MESH_SIMPLIFY_VERBOSE
#define GODOT_MESH_SIMPLIFY_CACHE_COLLAPSES
//#define GODOT_MESH_SIMPLIFY_USE_ZEUS

#define GSM_LOG(a)
//#define GSM_LOG(a) print_line(a)

uint32_t MeshSimplify::simplify_map(const uint32_t *p_in_inds, uint32_t p_num_in_inds, const Vector3 *p_in_verts, uint32_t p_num_in_verts, uint32_t *r_out_inds, LocalVectori<uint32_t> &r_vert_map, uint32_t &r_num_out_verts, real_t p_threshold) {
	//_threshold = p_threshold * p_threshold;

	uint32_t target_num_tris = (p_num_in_inds / 3) * p_threshold;
	target_num_tris = CLAMP(target_num_tris, 1, p_num_in_inds / 3);

	uint32_t orig_num_verts = p_num_in_verts;

	// DEDUPLICATE
	//////////////////////////////////////////////////////////////////////////////////
	LocalVectori<Vector3> deduped_verts;
	LocalVectori<uint32_t> deduped_inds;

	print_line("dedupe start");
	_deduplicator.deduplicate_map(p_in_inds, p_num_in_inds, p_in_verts, p_num_in_verts, r_vert_map, r_num_out_verts, deduped_inds);
	print_line("dedupe end");

	// construct deduped verts
	deduped_verts.resize(r_num_out_verts);

	// create a map from deduped verts to original
	// for the final shrinking stage
	LocalVectori<uint32_t> deduped_verts_source;
	deduped_verts_source.resize(deduped_verts.size());

	for (int n = 0; n < p_num_in_verts; n++) {
		uint32_t new_vert_id = r_vert_map[n];
		DEV_ASSERT(new_vert_id < r_num_out_verts);

		deduped_verts[new_vert_id] = p_in_verts[n];
		deduped_verts_source[new_vert_id] = n;
	}

	DEV_ASSERT(deduped_verts.size() <= p_num_in_verts);

	// change the indices to the deduped
	p_in_inds = &deduped_inds[0];
	p_num_in_inds = deduped_inds.size();

	p_in_verts = &deduped_verts[0];
	p_num_in_verts = deduped_verts.size();

	print_line("orig num verts " + itos(orig_num_verts) + ", after dedup : " + itos(r_num_out_verts));

#ifdef GODOT_MESH_SIMPLIFY_USE_ZEUS
	// by a percentage of the indices
	size_t target_indices = p_num_in_inds / 2;
	target_indices = CLAMP(target_indices, 3, p_num_in_inds);

	target_indices = 3000;

	LocalVectori<unsigned int> inds_out;
	inds_out.resize(p_num_in_inds);

	//size_t result = SurfaceTool::simplify_func(&inds_out[0], p_in_inds, p_num_in_inds, (const float *)p_in_verts, p_num_in_verts, sizeof(Vector3), target_indices, 0.9, nullptr);
	size_t result = SurfaceTool::simplify_func(&inds_out[0], p_in_inds, p_num_in_inds, (const float *)p_in_verts, p_num_in_verts, sizeof(Vector3), 1, p_threshold / 20.0, nullptr);

	print_line("zeus before " + itos(p_num_in_inds / 3) + " after " + itos(result / 3));

	for (int n = 0; n < result; n++) {
		r_out_inds[n] = inds_out[n];
	}

	//	r_vert_map.resize(p_num_in_verts);
	//	for (int n=0; n<p_num_in_verts; n++)
	//	{
	//		r_vert_map[n] = n;
	//	}
	//	r_num_out_verts = p_num_in_verts;
	return result;
#endif

	//////////////////////////////////////////////////////////////////////////////////

	_verts.clear();
	_tris.clear();

	// setup the verts
	_verts.resize(p_num_in_verts);
	for (int n = 0; n < p_num_in_verts; n++) {
		_verts[n].pos = p_in_verts[n];
	}

	////////////////////////////////////////////////////
	// set up linked verts
	LocalVectori<SpatialDeduplicator::LinkedVerts> linked_verts_list;
	_deduplicator.find_duplicate_positions(p_in_verts, p_num_in_verts, linked_verts_list);

	for (int n = 0; n < linked_verts_list.size(); n++) {
		const SpatialDeduplicator::LinkedVerts &lv = linked_verts_list[n];

		if (lv.vert_ids.size() > 1) {
			for (int i = 0; i < lv.vert_ids.size(); i++) {
				Vert &vert = _verts[lv.vert_ids[i]];
				vert.linked_verts = lv.vert_ids;

				// erase the id of the vert itself
				vert.linked_verts.erase(lv.vert_ids[i]);
			}
		}
	}

	////////////////////////////////////////////////////

	_create_tris(p_in_inds, p_num_in_inds);
	//_detect_mirror_verts();

#ifdef GODOT_MESH_SIMPLIFY_VERBOSE
	print_line("in verts:");
	for (int n = 0; n < _verts.size(); n++) {
		const Vert &vert = _verts[n];
		String sz = "\t" + itos(n) + " : " + String(Variant(vert.pos));
		if (vert.edge_vert)
			sz += " E";
		if (vert.edge_colinear)
			sz += "C";

		print_line(sz);
	}

	print_line("simplify start");
	int debug_count = 0;
#endif

	_create_heap();

	while (true) {
		//	for (int n = 0; n < 2500; n++) {
		if (_active_tri_count <= target_num_tris)
			break;

		GSM_LOG(itos(n));
		if (_new_simplify()) {
			//_apply_collapses();
		} else {
			break;
		}
	}

	/*
		for (int n = 0; n < 2; n++) {
			if (n == 0) {
				_mirror_verts_only = true;
			} else {
				_mirror_verts_only = false;
				// make the non-mirror verts dirty
				for (int i = 0; i < _verts.size(); i++) {
					_verts[i].dirty = true;
				}
			}

			while (_simplify()) {
				_apply_collapses();
	#ifdef GODOT_MESH_SIMPLIFY_VERBOSE
				_debug_verify_verts();
				debug_count++;
				if ((debug_count % 256) == 0)
					print_line("\tsimplify " + itos(debug_count));
	#endif
			}
	#ifdef GODOT_MESH_SIMPLIFY_VERBOSE
			print_line("simplify end");
	#endif
		}
	*/
	// export final list
	uint32_t count = 0;

	//LocalVectori<uint32_t> final_vertmap;
	LocalVectori<uint32_t> final_sourceverts;

	for (int n = 0; n < _tris.size(); n++) {
		const Tri &tri = _tris[n];
		if (tri.active) {
			for (int c = 0; c < 3; c++) {
				uint32_t deduped_vert_id = tri.corn[c];
				DEV_ASSERT(deduped_vert_id < deduped_verts.size());
				// r_out_inds[count++] = deduped_vert_id;

				uint32_t orig_source_vert_id = deduped_verts_source[deduped_vert_id];
				uint32_t final_ind = final_sourceverts.find_or_push_back(orig_source_vert_id);
				r_out_inds[count++] = final_ind;
			}
		}
	}

	// adjust the final number of verts that are actually used
	r_num_out_verts = final_sourceverts.size();
	r_vert_map.fill(UINT32_MAX);
	for (int n = 0; n < final_sourceverts.size(); n++) {
		uint32_t source_id = final_sourceverts[n];
		r_vert_map[source_id] = n;
	}

	_optimize_vertex_cache(r_out_inds, count, r_num_out_verts);

	// double check inds
#ifdef DEV_ENABLED
//	for (int n=0; n<count; n++)
//	{
//		uint32_t ind = r_out_inds[n];
//		DEV_ASSERT(ind < r_num_out_verts);
//	}
#endif

	print_line("simplify tris before : " + itos(p_num_in_inds / 3) + ", after : " + itos(count / 3) + ", orig num verts " + itos(orig_num_verts) + ", final verts : " + itos(r_num_out_verts));

	return count;
}

void MeshSimplify::_detect_mirror_verts() {
	// first create tri AABBs
	for (int n = 0; n < _tris.size(); n++) {
		Tri &tri = _tris[n];

		const Vector3 &a = _verts[tri.corn[0]].pos;
		const Vector3 &b = _verts[tri.corn[1]].pos;
		const Vector3 &c = _verts[tri.corn[2]].pos;

		tri.aabb.position = a;
		tri.aabb.size = Vector3();
		tri.aabb.expand_to(b);
		tri.aabb.expand_to(c);
		tri.aabb_volume = tri.aabb.get_area();
	}

	// first we need to find a mirror axis, and the central plane
	//	for (int axis=0; axis<3; axis++)
	//	{
	int axis_hits[3];
	axis_hits[0] = 0;
	axis_hits[1] = 0;
	axis_hits[2] = 0;

	for (int n = 0; n < _tris.size(); n++) {
		Tri &ta = _tris[n];

		for (int i = n + 1; i < _tris.size(); i++) {
			Tri &tb = _tris[i];

			int axis = -1;
			real_t midpoint = 0.0;
			if (_detect_mirror_tris(ta, tb, axis, midpoint)) {
				if (Math::is_equal_approx(midpoint, (real_t)0.0, (real_t)0.01)) {
					axis_hits[axis] += 1;
					ta.has_mirror = true;
					tb.has_mirror = true;
				}
			}
		}
	}

	// find the most common axis and mirror on this
	int best_axis = -1;
	int best_hits = 0;
	for (int n = 0; n < 3; n++) {
		if (axis_hits[n] > best_hits) {
			best_axis = n;
			best_hits = axis_hits[n];
		}
	}

	if (best_axis == -1)
		return;

	// now find mirrors
	for (int n = 0; n < _tris.size(); n++) {
		const Tri &ta = _tris[n];
		if (!ta.has_mirror)
			continue;

		for (int i = n + 1; i < _tris.size(); i++) {
			const Tri &tb = _tris[i];
			if (!tb.has_mirror)
				continue;

			_find_mirror_verts(ta, tb, best_axis, 0.0);
		}
	}

	// count for debugging
#ifdef DEV_ENABLED
	int count = 0;
	for (int n = 0; n < _verts.size(); n++) {
		if (_verts[n].mirror_vert != UINT32_MAX)
			count++;
	}

	print_line(itos(count) + " mirror verts detected.");
#endif
}

void MeshSimplify::_find_mirror_verts(const Tri &p_a, const Tri &p_b, int p_axis, real_t p_midpoint) {
	// detect mirror verts
	int matches = 0;

	uint32_t match_to[3];

	for (int n = 0; n < 3; n++) {
		const Vector3 &a = _verts[p_a.corn[n]].pos;

		// reflect
		Vector3 ra = a;
		real_t dist = p_midpoint - ra.get_axis(p_axis);
		ra.set_axis(p_axis, p_midpoint + dist);

		for (int i = 0; i < 3; i++) {
			const Vector3 &b = _verts[p_b.corn[i]].pos;

			if (b.is_equal_approx(ra, 0.01)) {
				matches++;
				match_to[n] = i;
			}
		} // for i
		if (matches != (n + 1))
			return;
	} // for n

	if (matches != 3) {
		return;
	}

	// now link
	for (int n = 0; n < 3; n++) {
		uint32_t va = p_a.corn[n];
		uint32_t vb = p_b.corn[match_to[n]];

		_verts[va].mirror_vert = vb;
		_verts[vb].mirror_vert = va;
	}
}

bool MeshSimplify::_detect_mirror_tris(const Tri &p_a, const Tri &p_b, int &r_axis, real_t &r_midpoint) const {
	real_t vol_diff = Math::abs(p_b.aabb_volume - p_a.aabb_volume);
	if (vol_diff > 0.1)
		return false;

	bool axis_same[3];
	r_axis = -1;
	int count = 0;
	for (int n = 0; n < 3; n++) {
		real_t pa = p_a.aabb.position.get_axis(n);
		real_t pb = p_b.aabb.position.get_axis(n);
		axis_same[n] = Math::abs(pb - pa) < 0.01;
		if (axis_same[n]) {
			count++;
		} else {
			r_axis = n;
		}
	}

	if (count < 2)
		return false;

	if (r_axis == -1)
		return false;

	if (Math::abs(p_b.aabb.size.x - p_a.aabb.size.x) > 0.01)
		return false;
	if (Math::abs(p_b.aabb.size.y - p_a.aabb.size.y) > 0.01)
		return false;
	if (Math::abs(p_b.aabb.size.z - p_a.aabb.size.z) > 0.01)
		return false;

	// find mid point
	real_t mid_a = p_a.aabb.position.get_axis(r_axis) + (p_a.aabb.size.get_axis(r_axis) * 0.5);
	real_t mid_b = p_b.aabb.position.get_axis(r_axis) + (p_b.aabb.size.get_axis(r_axis) * 0.5);

	real_t midpoint = mid_a + ((mid_b - mid_a) * 0.5);

	// detect mirror verts
	int matches = 0;

	for (int n = 0; n < 3; n++) {
		const Vector3 &a = _verts[p_a.corn[n]].pos;

		// reflect
		Vector3 ra = a;
		real_t dist = midpoint - ra.get_axis(r_axis);
		ra.set_axis(r_axis, midpoint + dist);

		for (int i = 0; i < 3; i++) {
			const Vector3 &b = _verts[p_b.corn[i]].pos;

			if (b.is_equal_approx(ra, 0.01)) {
				matches++;
			}
		} // for i
	} // for n

	if (matches != 3) {
		return false;
	}

	r_midpoint = midpoint;
	return true;
}

void MeshSimplify::_optimize_vertex_cache(uint32_t *r_inds, uint32_t p_num_inds, uint32_t p_num_verts) const {
	LocalVectori<uint32_t> inds_copy;
	inds_copy.resize(p_num_inds);
	if (p_num_inds) {
		memcpy(&inds_copy[0], r_inds, p_num_inds * sizeof(uint32_t));

		VertexCacheOptimizer<uint32_t> opt;
		opt.reorder_indices(r_inds, &inds_copy[0], p_num_inds / 3, p_num_verts);
	}
}

// returns number of indices
uint32_t MeshSimplify::simplify(const uint32_t *p_inds, uint32_t p_num_inds, const Vector3 *p_verts, uint32_t p_num_verts, uint32_t *r_inds, Vector3 *r_deduped_verts, uint32_t &r_num_deduped_verts, real_t p_threshold) {
	/*
	_threshold_dist = p_threshold;

	LocalVectori<Vector3> deduped_verts;
	LocalVectori<uint32_t> deduped_inds;

	_deduplicator.deduplicate_verts_only(p_inds, p_num_inds, p_verts, p_num_verts, deduped_verts, deduped_inds);

	DEV_ASSERT(deduped_verts.size() <= p_num_verts);

	for (int n = 0; n < deduped_verts.size(); n++) {
		r_deduped_verts[n] = deduped_verts[n];
	}
	r_num_deduped_verts = deduped_verts.size();

	// change the indices to the deduped
	p_inds = &deduped_inds[0];
	p_num_inds = deduped_inds.size();

	p_verts = &deduped_verts[0];
	p_num_verts = deduped_verts.size();

	_verts.clear();
	_tris.clear();

	// setup the verts
	_verts.resize(p_num_verts);
	for (int n = 0; n < p_num_verts; n++) {
		_verts[n].pos = p_verts[n];
	}

	_create_tris(p_inds, p_num_inds);

	while (_simplify()) {
		;
	}

	// export final list
	uint32_t count = 0;

	for (int n = 0; n < _tris.size(); n++) {
		const Tri &tri = _tris[n];
		if (tri.active) {
			for (int c = 0; c < 3; c++) {
				DEV_ASSERT(tri.corn[c] < r_num_deduped_verts);
				r_inds[count++] = tri.corn[c];
			}

			//r_inds.push_back(tri.corn[c]);
		}
	}

	print_line("simplify tris before : " + itos(p_num_inds / 3) + ", after : " + itos(count / 3));
	return count;
*/
	return 0;
}

bool MeshSimplify::_calculate_plane(uint32_t p_corns[3], Plane &r_plane) const {
	Vector3 pts[3];
	for (int c = 0; c < 3; c++) {
		pts[c] = _verts[p_corns[c]].pos;
	}

	r_plane = Plane(pts[0], pts[1], pts[2]);
	if (r_plane.normal.length_squared() < 0.5) {
		return false;
	}

	return true;
}

void MeshSimplify::_create_tris(const uint32_t *p_inds, uint32_t p_num_inds) {
	int num_tris = p_num_inds / 3;

	uint32_t count = 0;

	for (int t = 0; t < num_tris; t++) {
		Tri tri;
		tri.corn[0] = p_inds[count++];
		tri.corn[1] = p_inds[count++];
		tri.corn[2] = p_inds[count++];

		// ignore null tris
		if ((tri.corn[0] == tri.corn[1]) || (tri.corn[1] == tri.corn[2]) || (tri.corn[0] == tri.corn[2])) {
			continue;
		}

		// calculate plane
		if (!_calculate_plane(tri.corn, tri.plane))
			continue;

		// edges
		for (int c = 0; c < 3; c++) {
			Edge &edge = tri.edge[c];
			edge.a = tri.corn[c];
			edge.b = tri.corn[(c + 1) % 3];
			edge.sort();
		}

		// add the tris to the verts
		for (int c = 0; c < 3; c++) {
			_verts[tri.corn[c]].link_tri(_tris.size());

			// mark this vertex as active (used by triangles)
			_verts[tri.corn[c]].active = true;
		}

		_tris.push_back(tri);
	}

	_active_tri_count = _tris.size();

	// find neighbours
	for (int n = 0; n < _tris.size(); n++) {
#ifdef GODOT_MESH_SIMPLIFY_OPTIMIZED_NEIGHS
		_establish_neighbours_for_tri(n);
#else
		for (int i = n + 1; i < _tris.size(); i++) {
			_establish_neighbours(n, i);
		}
#endif
	}

	// mark edge tris
	for (int n = 0; n < _tris.size(); n++) {
		Tri &t = _tris[n];
		t.num_neighs = 0;
		for (int c = 0; c < 3; c++) {
			if (t.neigh[c] != UINT32_MAX) {
				t.num_neighs += 1;
			} else {
				// mark the verts as an edge vert
				const Edge &edge = t.edge[c];
				_verts[edge.a].add_edge_vert_neigh(edge.b);
				_verts[edge.b].add_edge_vert_neigh(edge.a);
			}
		}
	}

	// for edge verts, also add a special check for colinearity
	for (int n = 0; n < _verts.size(); n++) {
		Vert &v1 = _verts[n];
		if (!v1.edge_vert)
			continue;

		const Vert &v0 = _verts[v1.edge_vert_neighs[0]];
		const Vert &v2 = _verts[v1.edge_vert_neighs[1]];

		// colinear?
		Vector3 d0 = v1.pos - v0.pos;
		Vector3 d1 = v2.pos - v1.pos;

		d0.normalize();
		d1.normalize();

		real_t dot = d0.dot(d1);
		real_t epsilon_colinear = 0.2;
		if (dot >= epsilon_colinear) {
			v1.edge_colinear = true;
		}
	}

	// add ancestral tris to each vertex
	for (int n = 0; n < _verts.size(); n++) {
		Vert &v = _verts[n];
		v.ancestral_tris = v.tris;
	}
}

void MeshSimplify::_establish_neighbours_for_tri(int p_tri_id) {
	// can only possibly be neighbours with tris that share our vertices.
	Tri &tri = _tris[p_tri_id];
	if (!tri.active) {
		return;
	}

	// First reset all neighbours to NULL
	// to clear any previous neighbour triangles that might have been
	// removed.
	for (int n = 0; n < 3; n++) {
		tri.neigh[n] = UINT32_MAX;
	}

	// now create a list of neighbours
	_possible_tris.clear();
	for (int c = 0; c < 3; c++) {
		const Vert &vert = _verts[tri.corn[c]];
		for (int n = 0; n < vert.tris.size(); n++) {
			uint32_t ntri_id = vert.tris[n];
			_possible_tris.find_or_push_back(ntri_id);
		}
	}

	for (int n = 0; n < _possible_tris.size(); n++) {
		uint32_t ntri_id = _possible_tris[n];

		if (_tris[ntri_id].active) {
			_establish_neighbours(p_tri_id, _possible_tris[n]);
		}
	}
}

void MeshSimplify::_establish_neighbours(int p_tri0, int p_tri1) {
	if (p_tri0 == p_tri1) {
		// Finding neighbouring edges with ourself
		// will always find 3 matches, and pollute the outgoing neighbours,
		// so we need to avoid testing against ourself.
		return;
	}

	Tri &t0 = _tris[p_tri0];
	Tri &t1 = _tris[p_tri1];

	if (!t0.active || !t1.active) {
		return;
	}

	for (int c = 0; c < 3; c++) {
		for (int i = 0; i < 3; i++) {
			if (t0.edge[c] == t1.edge[i]) {
				// link!
				t0.neigh[c] = p_tri1;
				t1.neigh[i] = p_tri0;
			}
		}
	}
}

uint32_t MeshSimplify::_choose_vert_to_merge(uint32_t p_start_from) const {
	// find a vertex that is not an edge vert
	for (int n = p_start_from; n < _verts.size(); n++) {
		const Vert &vert = _verts[n];
#ifdef GODOT_MESH_SIMPLIFY_USE_DIRTY_VERTS
		if (!vert.dirty)
			continue;
#endif

		if (!vert.active)
			continue;

		if (vert.locked)
			continue;

		if (_mirror_verts_only && (vert.mirror_vert == UINT32_MAX))
			continue;

		if (vert.edge_vert) {
			if (vert.edge_colinear) {
				// only allow if there is 1 or less linked tris
				if (vert.linked_verts.size() > 1)
					continue;

				// if at the side of an edge, don't allow collapse
				if (vert.has_edge_vert_neigh(UINT32_MAX))
					continue;

				//			print_line("choose_vert_to_merge from " + String(Variant(vert.pos)));
			} else {
				continue;
			}
		}

		return n;
	}

	return UINT32_MAX;
}

//bool MeshSimplify::_check_merge_allowed(uint32_t p_vert_from, uint32_t p_vert_to) const {
//	const Vert &a = _verts[p_vert_from];
//	// const Vert &b = _verts[p_vert_to];

//	if (a.edge_vert) {
//		DEV_ASSERT(_is_reciprocal_edge(p_vert_from, p_vert_to));
//	}
//	return true;
//}

bool MeshSimplify::_is_reciprocal_edge(uint32_t p_vert_a, uint32_t p_vert_b) const {
	const Vert &a = _verts[p_vert_a];
	const Vert &b = _verts[p_vert_b];

	bool res = a.has_edge_vert_neigh(p_vert_b) && b.has_edge_vert_neigh(p_vert_a);
	return res;
}

bool MeshSimplify::_find_vert_to_merge_to(uint32_t p_vert_from) {
	_merge_vert_ids.clear();

	// can join to any using this edge
	const Vert &vert = _verts[p_vert_from];

	if (!vert.edge_vert) {
		for (int t = 0; t < vert.tris.size(); t++) {
			uint32_t tri_id = vert.tris[t];
			DEV_ASSERT(tri_id < _tris.size());

			const Tri &tri = _tris[tri_id];
			if (!tri.active)
				continue;
			for (int c = 0; c < 3; c++) {
				uint32_t vid = tri.corn[c];
				if (vid != p_vert_from) {
					_merge_vert_ids.find_or_push_back(vid);
				}
			}
		} // for t through tris
	} else {
		// edge vert
		for (int t = 0; t < vert.tris.size(); t++) {
			uint32_t tri_id = vert.tris[t];
			DEV_ASSERT(tri_id < _tris.size());

			const Tri &tri = _tris[tri_id];
			if (!tri.active)
				continue;
			for (int c = 0; c < 3; c++) {
				uint32_t vid = tri.corn[c];
				if (vid != p_vert_from) {
					if (_verts[vid].edge_vert)

						// there must be a reciprocal edge for this collapse to be possible
						if (_is_reciprocal_edge(p_vert_from, vid))
							_merge_vert_ids.find_or_push_back(vid);
				}
			}
		} // for t through tris
	} // from edge vert
	return _merge_vert_ids.size() != 0;
}

bool MeshSimplify::_evaluate_collapse(uint32_t p_vert_from_id, uint32_t p_vert_to_id, CollapseGroup &r_cg, bool p_second_edge) {
	Vert &vert_from = _verts[p_vert_from_id];
	Vert &vert_to = _verts[p_vert_to_id];

	// does the collapse already exist in the heap?
	// (neighbouring edges will try and add similar collapses)
	if (vert_from.heap_collapse_to.find(p_vert_to_id) != -1)
		return false;

	real_t error = 0.0;

	// disallow if edge verts and not linked
	//	if (vert_from.edge_vert || vert_to.edge_vert) {
	if (vert_from.edge_vert) {
		if (!_is_reciprocal_edge(p_vert_from_id, p_vert_to_id))
			return false;

		// if at the side of an edge, don't allow collapse
		//		if (vert_from.has_edge_vert_neigh(UINT32_MAX))
		//			return false;

		// don't allow collapsing t junction edges
		if (vert_from.linked_verts.size() > 1)
			return false;

		// the change to the edge angle must form
		// part of the error metric, so we don't e.g. collapse
		// right angle edges
		uint32_t prev_vert_id = vert_from.get_other_edge_vert_neigh(p_vert_to_id);
		if (prev_vert_id == UINT32_MAX)
			return false;

		const Vert &vert_prev = _verts[prev_vert_id];

		real_t dist;
		if (dist_point_from_line(vert_from.pos, vert_prev.pos, vert_to.pos, dist)) {
			error += dist * dist;
		} else {
			// should hopefully not happen, as similar
			// points should already be merged by this point.
			return false;
		}
	}

	// is this suitable for collapse?
	for (int n = 0; n < vert_from.tris.size(); n++) {
		uint32_t tri_id = vert_from.tris[n];
		const Tri &tri = _tris[tri_id];

		// various checks for validity of the collapse
		if (!_allow_collapse(tri_id, p_vert_from_id, p_vert_to_id))
			return false;

		// distance to new point
		real_t dist = tri.plane.distance_to(vert_to.pos);

		// used distance squared metric, and make sure positive
		dist *= dist;

		// keep track of total error
		error += dist;
	}

	r_cg.add_collapse(p_vert_from_id, p_vert_to_id, error);

	// attempt to add second edge .. if this fails, abort
	if (!p_second_edge && vert_from.edge_vert && vert_from.linked_verts.size()) {
		// is this a joined edge?
		//		if (vert_from.linked_verts.size() != 1)
		//			return false;

		if (vert_to.linked_verts.size() == 0)
			return false;

		uint32_t from_id2, to_id2;
		if (_find_second_edge_from_to(p_vert_from_id, p_vert_to_id, from_id2, to_id2)) {
			// if the two edges continue to vertex that are not linked, prevent collapsing from
			// (this prevents unzipping edge joins)
			const Vert &from2 = _verts[from_id2];
			uint32_t next2 = from2.get_other_edge_vert_neigh(to_id2);
			uint32_t next1 = vert_from.get_other_edge_vert_neigh(p_vert_to_id);

			// are these linked?
			if (_are_verts_linked(next1, next2) && _evaluate_collapse(from_id2, to_id2, r_cg, true)) {
				return true;
			}
		}

		// abort, can't add second edge
		return false;
	}

	// As well as storing on the heap, store a quick
	// to id on the vertex itself so we can quick reject adding
	// collapses more than once.
	//_heap.push_back(c);
	//vert_from.heap_collapse_to.push_back(p_vert_to_id);
	//vert_to.heap_collapse_from.push_back(p_vert_from_id);
	return true;
}

bool MeshSimplify::_are_verts_linked(uint32_t p_id_a, uint32_t p_id_b) const {
	if (p_id_a == UINT32_MAX)
		return false;

	const Vert &vert = _verts[p_id_a];
	return vert.is_linked_to(p_id_b);
}

bool MeshSimplify::_find_second_edge_from_to(uint32_t p_vert_from_id, uint32_t p_vert_to_id, uint32_t &r_vert_from_id, uint32_t &r_vert_to_id) const {
	const Vert &vert_from = _verts[p_vert_from_id];
	const Vert &vert_to = _verts[p_vert_to_id];

	r_vert_from_id = vert_from.linked_verts[0];
	const Vert &from = _verts[r_vert_from_id];
	for (int n = 0; n < vert_to.linked_verts.size(); n++) {
		uint32_t id = vert_to.linked_verts[n];
		if ((from.edge_vert_neighs[0] == id) || (from.edge_vert_neighs[1] == id)) {
			r_vert_to_id = id;
			return true;
		}
	}
	return false;
}

//void MeshSimplify::_add_collapse(uint32_t p_vert_from_id, uint32_t p_vert_to_id, real_t p_max_displacement) {
//#ifdef DEV_ENABLED
//	const Vert &vert_from = _verts[p_vert_from_id];
//	const Vert &vert_to = _verts[p_vert_to_id];

//	DEV_ASSERT(vert_from.active);
//	DEV_ASSERT(vert_to.active);
//#endif

//#ifdef GODOT_MESH_SIMPLIFY_CACHE_COLLAPSES
//	Collapse c;
//	c.from = p_vert_from_id;
//	c.to = p_vert_to_id;
//	c.error = p_max_displacement;
//	_collapses.push_back(c);
//#else
//	_finalize_merge(p_vert_from_id, p_vert_to_id, p_max_displacement);
//#endif
//}

void MeshSimplify::_finalize_merge(uint32_t p_vert_from_id, uint32_t p_vert_to_id) {
	Vert &vert_from = _verts[p_vert_from_id];
	vert_from.active = false;

	Vert &vert_to = _verts[p_vert_to_id];
#ifdef GODOT_MESH_SIMPLIFY_VERBOSE
	print_line("finalize merge from " + itos(p_vert_from_id) + " (" + String(Variant(vert_from.pos)) + ") to " + itos(p_vert_to_id) + " (" + String(Variant(vert_to.pos)) + ")");
#endif

	// add the vert as an ancestor to the to vert, and also the original tris
	vert_to.ancestral_verts.push_back(p_vert_from_id);
	for (int n = 0; n < vert_from.tris.size(); n++) {
		vert_to.ancestral_tris.find_or_push_back(vert_from.tris[n]);
	}

	//real_t &new_max_displacement = vert_to.displacement;
	//new_max_displacement = MAX(new_max_displacement, p_max_displacement);

	// adjust all attached tris
	for (int n = 0; n < vert_from.tris.size(); n++) {
		_adjust_tri(vert_from.tris[n], p_vert_from_id, p_vert_to_id);
	}

	for (int n = 0; n < vert_from.tris.size(); n++) {
		_resync_tri(vert_from.tris[n]);
	}

	// special case, if we are merging between 2 edge verts, we need to rejig the neighbours to be correct after the merge
	if (vert_from.edge_vert) {
		//		if (p_vert_from_id == 1350) {
		//			;
		//		}

		uint32_t other_from = vert_from.get_other_edge_vert_neigh(p_vert_to_id);

		if (!vert_to.locked) {
			uint32_t other_to = vert_to.get_other_edge_vert_neigh(p_vert_from_id);

			// only the merged vertex left needs to be altered ... the next neigh is the same,
			// but the previous neighbour should be the previous neighbour of the deleted vertex
			vert_to.set_other_edge_vert_neigh(other_to, other_from);
		}

		if (other_from != UINT32_MAX) {
			Vert &vert_prev = _verts[other_from];
			if (!vert_prev.locked)
				vert_prev.exchange_edge_vert_neigh(p_vert_from_id, p_vert_to_id);
		}

		//DEV_ASSERT(!((vert_to.edge_vert_neighs[0] == UINT32_MAX) && (vert_to.edge_vert_neighs[1] == UINT32_MAX)));
	}

	// new heap based system
	_reevaluate_from_changed_vertex(p_vert_from_id, p_vert_to_id);
}

//uint32_t MeshSimplify::_heap_find_collapse(uint32_t p_vert_from_id, uint32_t p_vert_to_id) {
//	for (int n = 0; n < _heap.size(); n++) {
//		const Collapse &c = _heap[n];
//		if ((c.from == p_vert_from_id) || (c.to == p_vert_to_id)) {
//			return n;
//		}
//	}

//	return UINT32_MAX;
//}

void MeshSimplify::_destroy_collapse_group(const CollapseGroup &p_cg) {
	// remove from vertex
	for (uint32_t n = 0; n < p_cg.size; n++) {
		uint32_t from_id = p_cg.c[n].from;
		uint32_t to_id = p_cg.c[n].to;

		Vert &vert = _verts[from_id];
		int64_t found = vert.heap_collapse_to.find(to_id);
		if (found != -1)
			vert.heap_collapse_to.remove_unordered(found);
	}
}

void MeshSimplify::_heap_remove(uint32_t p_vert_id) {
	for (int n = 0; n < _heap.size(); n++) {
		const CollapseGroup &cg = _heap[n];

		if (cg.contains(p_vert_id)) {
			_debug_print_collapse_group(cg, 1, "heap_remove cg:");

			_destroy_collapse_group(cg);

			// could do remove_unordered if sorting later?
			_heap.remove(n);
			n--;
		}
	}
}

void MeshSimplify::_reevaluate_from_changed_vertex(uint32_t p_deleted_vert, uint32_t p_central_vert) {
	// first remove any collapses in the heap referencing the deleted vert
	_heap_remove(p_deleted_vert);
	//_heap_remove(p_central_vert);

	return;

	//////////////////////////
	_evaluate_collapses_from_vertex(p_central_vert);

	const Vert &vert = _verts[p_central_vert];

	LocalVectori<uint32_t> possibles;

	for (int t = 0; t < vert.tris.size(); t++) {
		uint32_t tri_id = vert.tris[t];
		DEV_ASSERT(tri_id < _tris.size());

		const Tri &tri = _tris[tri_id];
		if (!tri.active)
			continue;
		for (int c = 0; c < 3; c++) {
			uint32_t vid = tri.corn[c];
			if (vid != p_central_vert) {
				possibles.find_or_push_back(vid);
			}
		}
	} // for t through tris

	for (int n = 0; n < possibles.size(); n++) {
		_evaluate_collapses_from_vertex(possibles[n]);
	}

	// expensive - can this be avoided?
	_heap.sort();
}

bool MeshSimplify::_evaluate_collapses_from_vertex(uint32_t p_vert_from_id) {
	Vert &vert = _verts[p_vert_from_id];
	if (!vert.active)
		return false;

		// mark this vertex as no longer dirty
#ifdef GODOT_MESH_SIMPLIFY_USE_DIRTY_VERTS
	_verts[p_vert_from_id].dirty = false;
#endif

	// get a list of possible verts to merge to
	if (!_find_vert_to_merge_to(p_vert_from_id))
		return false;

	for (int n = 0; n < _merge_vert_ids.size(); n++) {
		CollapseGroup cg;

		if (!_evaluate_collapse(p_vert_from_id, _merge_vert_ids[n], cg)) {
			cg.size = 0;
		}

		// if the collapse was any good, add to the heap
		if (cg.size > 0) {
			_heap.push_back(cg);

			// As well as storing on the heap, store a quick
			// to id on the vertex itself so we can quick reject adding
			// collapses more than once.
			for (int i = 0; i < cg.size; i++) {
				const Collapse &c = cg.c[i];
				_verts[c.from].heap_collapse_to.push_back(c.to);
			}
		}
	}

	return true;
}

void MeshSimplify::_create_heap() {
	//uint32_t start_from = 0;

	for (int n = 0; n < _verts.size(); n++) {
		Vert &vert = _verts[n];

		// already done (edge or mirror)
		if (!vert.dirty)
			continue;

		_evaluate_collapses_from_vertex(n);
	}

	//	while (true) {
	//		uint32_t vert_from_id = _choose_vert_to_merge(start_from);
	//		if (vert_from_id == UINT32_MAX)
	//			return;

	//		_evaluate_collapses_from_vertex(vert_from_id);

	//		// for next loop
	//		start_from = vert_from_id + 1;
	//	}

	_heap.sort();

	// print initial heap
#ifdef DEV_ENABLED
#ifdef GODOT_MESH_SIMPLIFY_VERBOSE
	GSM_LOG("\nInitialHeap\n***********");
	for (int n = 0; n < _heap.size(); n++) {
		_debug_print_collapse_group(_heap[n]);
	}
	GSM_LOG("");
#endif
#endif

	print_line("_create_heap heap size " + itos(_heap.size()));
}

void MeshSimplify::_debug_print_collapse_group(const CollapseGroup &p_cg, int p_tabs, String p_title) const {
#ifdef DEV_ENABLED
	if (p_title != String()) {
		GSM_LOG(p_title);
	}

	String sz;
	for (int n = 0; n < p_tabs; n++)
		sz += "\t";

	GSM_LOG(sz + "collapsegroup size " + itos(p_cg.size) + " error " + String(Variant(p_cg.error)));

	for (uint32_t n = 0; n < p_cg.size; n++) {
		const Collapse &c = p_cg.c[n];
		GSM_LOG(sz + "\tcollapse " + itos(c.from) + " to " + itos(c.to));
	}
#endif
}

bool MeshSimplify::_collapse_matching_edge(const Collapse &p_collapse) {
	Vert &orig_vert_from = _verts[p_collapse.from];
	if (!orig_vert_from.edge_vert)
		return true;

	//return false;

	Vert &orig_vert_to = _verts[p_collapse.to];

	DEV_ASSERT(orig_vert_from.linked_verts.size() == 1);
	uint32_t from_id = orig_vert_from.linked_verts[0];

	GSM_LOG("\tedge link from " + itos(from_id));

	Vert &from = _verts[from_id];
	if (!from.active) {
		GSM_LOG("\t\tfrom inactive");
		return false;
	}

	// vertex on edge that is shared by vertex to
	DEV_ASSERT(orig_vert_to.linked_verts.size() > 0);
	uint32_t to_id = UINT32_MAX;

	for (int n = 0; n < orig_vert_to.linked_verts.size(); n++) {
		uint32_t id = orig_vert_to.linked_verts[n];
		if (from.has_edge_vert_neigh(id)) {
			to_id = id;
			break;
		}
	}

	// no match, don't collapse
	if (to_id == -1) {
		GSM_LOG("\t\tto_id -1");
		return false;
	}

	// does the corresponding collapse exist on the heap?
	// quick reject
	if (from.heap_collapse_to.find(to_id) == -1) {
		GSM_LOG("\t\theap_collapse_to not found");
		return false;
	}

	// may not be required
	Vert &to = _verts[to_id];
	if (!to.active) {
		GSM_LOG("\t\tto inactive");
		return false;
	}

	// find on heap the collapse
	// is this actually required? perhaps not
	//uint32_t collapse_id = _heap_find_collapse(from_id, to_id);
	//DEV_ASSERT(collapse_id != UINT32_MAX);

	GSM_LOG("\t\tadd collapse " + itos(from_id) + " to " + itos(to_id));
	//_add_collapse(from_id, to_id, 0.0);

	return true;
}

bool MeshSimplify::_new_simplify() {
	// pop off the heap
	if (!_heap.size())
		return false;

	// pop collapse
	CollapseGroup cg = _heap[_heap.size() - 1];
	_heap.resize(_heap.size() - 1);

	//	if (cg.error > _threshold)
	//		return false;

	GSM_LOG("pop collapsegroup size " + itos(cg.size) + " error " + String(Variant(cg.error)));

	// don't blindly do the collapse.. check for flipped triangles
	if (_cg_creates_flipped_tris(cg))
		return true;

	// verify
	for (uint32_t n = 0; n < cg.size; n++) {
		const Collapse &c = cg.c[n];

		GSM_LOG("\tcollapse " + itos(c.from) + " to " + itos(c.to));

#ifdef DEV_ENABLED
		const Vert &vert_from = _verts[c.from];
		const Vert &vert_to = _verts[c.to];

		DEV_ASSERT(vert_from.active);
		DEV_ASSERT(vert_to.active);
#endif

		_finalize_merge(c.from, c.to);
	}

	// is this collapse on an edge? if so the edge matching must also be able to collapse
	//	if (_collapse_matching_edge(c)) {
	//		//_simplify_vert_primary(c.from, c.to);

	//		GSM_LOG("\tadd collapse " + itos(c.from) + " to " + itos(c.to));
	//		_add_collapse(c.from, c.to, 0.0);
	//		//_finalize_merge(c.from, c.to, 0.0);
	//	}

	return true;
}

bool MeshSimplify::_cg_creates_flipped_tris(const CollapseGroup &p_cg) {
	for (uint32_t n = 0; n < p_cg.size; n++) {
		const Collapse &c = p_cg.c[n];

		const Vert &vert_from = _verts[c.from];

		// is this suitable for collapse?
		for (int n = 0; n < vert_from.tris.size(); n++) {
			uint32_t tri_id = vert_from.tris[n];
			//const Tri &tri = _tris[tri_id];

			// various checks for validity of the collapse
			if (!_allow_collapse(tri_id, c.from, c.to)) {
				return true;
			}
		}
	}

	return false;
}

bool MeshSimplify::_allow_collapse(uint32_t p_tri_id, uint32_t p_vert_from, uint32_t p_vert_to) const {
	const Tri &t = _tris[p_tri_id];

	uint32_t new_corn[3];

	for (int c = 0; c < 3; c++) {
		new_corn[c] = t.corn[c];

		if (new_corn[c] == p_vert_from) {
			new_corn[c] = p_vert_to;
		}
	}

	// delete degenerates
	if ((new_corn[0] == new_corn[1]) || (new_corn[1] == new_corn[2]) || (new_corn[0] == new_corn[2])) {
		return true;
	}

	Plane new_plane;
	if (!_calculate_plane(new_corn, new_plane)) {
		return false;
	}

	// has the normal flipped?
	if (new_plane.normal.dot(t.plane.normal) < 0.0) {
		return false;
	}

#if 0
	// find the distance of the old vertex from this new plane ..
	// if it is more than a certain threshold, disallow the collapse
	Vector3 pt = _verts[p_vert_from].pos;

//	real_t displacement = 0.0;
	real_t dist = new_plane.distance_to(pt);
//	if (p_test_displacement) {
//		real_t displacement = _verts[p_vert_from].displacement + Math::abs(dist);
//		if (displacement > _threshold_dist)
//			return false;
//	}

	// test user attributes
	if (_deduplicator._attributes.size()) {
		// find closest point on plane of deleted point
		pt += new_plane.normal * -dist;

		// find barycentric coordinates of the deleted vertex
		Vector3 by = Geometry::barycentric_coordinates_3d(pt, _verts[new_corn[0]].pos, _verts[new_corn[1]].pos, _verts[new_corn[2]].pos);

		// is it inside the new triangle?
		// maybe increase this a little to catch colinear points? NYI
		if ((by.x >= 0.0) && (by.y >= 0.0) && (by.z >= 0.0) && (by.x <= 1.0) && (by.y <= 1.0) && (by.z <= 1.0)) {
			// if so compare the interpolated attributes with the delete vertex attributes
			for (int n = 0; n < _deduplicator._attributes.size(); n++) {
				const SpatialDeduplicator::Attribute &attr = _deduplicator._attributes[n];
				switch (attr.type) {
					case SpatialDeduplicator::Attribute::AT_UV: {
						const Vector2 &uv0 = attr.vec2s[new_corn[0]];
						const Vector2 &uv1 = attr.vec2s[new_corn[1]];
						const Vector2 &uv2 = attr.vec2s[new_corn[2]];

						// barycentric interpolation
						Vector2 inter = (uv0 * by.x) + (uv1 * by.y) + (uv2 * by.z);

						// find offset from old vertex UV
						const Vector2 &uv_old = attr.vec2s[p_vert_from];

						Vector2 offset = inter - uv_old;
						real_t sl = offset.length_squared();
						if (sl > attr.epsilon_merge) {
							return false;
						}
					} break;
					case SpatialDeduplicator::Attribute::AT_NORMAL: {
						const Vector3 &n0 = attr.vec3s[new_corn[0]];
						const Vector3 &n1 = attr.vec3s[new_corn[1]];
						const Vector3 &n2 = attr.vec3s[new_corn[2]];

						// barycentric interpolation
						Vector3 inter = (n0 * by.x) + (n1 * by.y) + (n2 * by.z);
						inter.normalize();

						// find offset from old vertex UV
						const Vector3 &n_old = attr.vec3s[p_vert_from];

						real_t dot = n_old.dot(inter);
						if (dot < attr.epsilon_merge) {
							return false;
						}
					} break;
					default: {
					} break;
				}
			}
		}
	}
#endif

	//	r_max_displacement = MAX(r_max_displacement, displacement);

	return true;
}

void MeshSimplify::_delete_triangle(uint32_t p_tri_id) {
	Tri &t = _tris[p_tri_id];
	t.active = false;
	_active_tri_count--;

	// move from vertices
	for (int n = 0; n < 3; n++) {
		uint32_t vert_id = t.corn[n];
		Vert &vert = _verts[vert_id];
		vert.tris.erase(p_tri_id);
	}

	// remove from triangles that count it as a neighbour?
	// not sure this needs doing...
}

void MeshSimplify::_debug_verify_verts() {
	for (int n = 0; n < _verts.size(); n++) {
		_debug_verify_vert(n);
	}
}

void MeshSimplify::_debug_verify_vert(uint32_t p_vert_id) {
	const Vert &vert = _verts[p_vert_id];
	if (!vert.active)
		return;

	if (!vert.edge_vert)
		return;

	for (int n = 0; n < 2; n++) {
		uint32_t vn = vert.edge_vert_neighs[n];
		if (vn != UINT32_MAX) {
			const Vert &vert2 = _verts[vn];
			DEV_ASSERT(vert2.has_edge_vert_neigh(p_vert_id));
		}
	}
}

void MeshSimplify::_adjust_tri(uint32_t p_tri_id, uint32_t p_vert_from, uint32_t p_vert_to) {
	Tri &t = _tris[p_tri_id];

	for (int c = 0; c < 3; c++) {
		if (t.corn[c] == p_vert_from) {
			t.corn[c] = p_vert_to;
		}
	}

	// delete degenerates
	if ((t.corn[0] == t.corn[1]) || (t.corn[1] == t.corn[2]) || (t.corn[0] == t.corn[2])) {
#ifdef GODOT_MESH_SIMPLIFY_VERBOSE
		print_line("removed degenerate tri " + itos(p_tri_id) + " (" + itos(t.corn[0]) + ", " + itos(t.corn[1]) + ", " + itos(t.corn[2]) + ")");
#endif
		_delete_triangle(p_tri_id);
		return;
	}

	// link the tri to the vert if not linked already
	Vert &vert = _verts[p_vert_to];
	vert.link_tri(p_tri_id);

	// make all corners dirty
#ifdef GODOT_MESH_SIMPLIFY_USE_DIRTY_VERTS
	for (int c = 0; c < 3; c++) {
		Vert &vert = _verts[t.corn[c]];
		vert.dirty = true;
	}
#endif
}

void MeshSimplify::_resync_tri(uint32_t p_tri_id) {
	Tri &tri = _tris[p_tri_id];

	if (!tri.active)
		return;

	// edges
	for (int c = 0; c < 3; c++) {
		Edge &edge = tri.edge[c];
		edge.a = tri.corn[c];
		edge.b = tri.corn[(c + 1) % 3];
		edge.sort();

		// neighbours
		tri.neigh[c] = UINT32_MAX;
	}

#ifdef GODOT_MESH_SIMPLIFY_OPTIMIZED_NEIGHS
	_establish_neighbours_for_tri(p_tri_id);

#if 0
	struct Vector3i {
		Vector3i() { ; }
		Vector3i(int a, int b, int c) {
			x = a;
			y = b;
			z = c;
		}
		bool operator==(const Vector3i &o) const { return x == o.x && y == o.y && z == o.z; }
		bool operator!=(const Vector3i &o) const { return (*this == o) == false; }
		int x, y, z;
	};

	// verify
	LocalVectori<Vector3i> ver_neighs;
	ver_neighs.resize(_tris.size());
	for (int n = 0; n < _tris.size(); n++) {
		const Tri &nt = _tris[n];
		ver_neighs[n] = Vector3i(nt.neigh[0], nt.neigh[1], nt.neigh[2]);
	}

	// re-establish neighbours
	for (int t = 0; t < _tris.size(); t++) {
		if (t == p_tri_id)
			continue;

		_establish_neighbours(p_tri_id, t);
	}

	//  test
	for (int n = 0; n < _tris.size(); n++) {
		const Tri &nt = _tris[n];
		if (nt.active) {
			if (ver_neighs[n] != Vector3i(nt.neigh[0], nt.neigh[1], nt.neigh[2])) {
				_establish_neighbours_for_tri(p_tri_id);
			}
		}
		//DEV_ASSERT(ver_neighs[n] == Vector3i(nt.neigh[0], nt.neigh[1], nt.neigh[2]));
	}
#endif

#else
	// re-establish neighbours
	for (int t = 0; t < _tris.size(); t++) {
		if (t == p_tri_id)
			continue;

		_establish_neighbours(p_tri_id, t);
	}
#endif
}
