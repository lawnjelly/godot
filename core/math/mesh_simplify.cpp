#include "mesh_simplify.h"
#include "core/math/vertex_cache_optimizer.h"
#include "core/print_string.h"

//void MeshSimplify::_deduplicate_verts(const uint32_t *p_in_inds, uint32_t p_num_in_inds, const Vector3 *p_in_verts, uint32_t p_num_in_verts, LocalVectori<uint32_t> &r_vert_map, uint32_t &r_num_out_verts, LocalVectori<Vector3> &r_deduped_verts, LocalVectori<uint32_t> &r_deduped_verts_source, LocalVectori<uint32_t> &r_deduped_inds)
//{

//}

uint32_t MeshSimplify::simplify_map(const uint32_t *p_in_inds, uint32_t p_num_in_inds, const Vector3 *p_in_verts, uint32_t p_num_in_verts, uint32_t *r_out_inds, LocalVectori<uint32_t> &r_vert_map, uint32_t &r_num_out_verts, real_t p_threshold, MeshSimplifyCallback p_callback, void *p_userdata) {
	_callback = p_callback;
	_callback_userdata = p_userdata;

	_threshold_dist = p_threshold;

	uint32_t orig_num_verts = p_num_in_verts;

	// DEDUPLICATE
	//////////////////////////////////////////////////////////////////////////////////
	LocalVectori<Vector3> deduped_verts;
	LocalVectori<uint32_t> deduped_inds;

	_deduplicator.deduplicate_map(p_in_inds, p_num_in_inds, p_in_verts, p_num_in_verts, r_vert_map, r_num_out_verts, deduped_inds);

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
	//////////////////////////////////////////////////////////////////////////////////

	_verts.clear();
	_tris.clear();

	// setup the verts
	_verts.resize(p_num_in_verts);
	for (int n = 0; n < p_num_in_verts; n++) {
		_verts[n].pos = p_in_verts[n];
	}

	_create_tris(p_in_inds, p_num_in_inds);

	while (_simplify()) {
		;
	}

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
				uint32_t final_ind = _find_or_add(orig_source_vert_id, final_sourceverts);
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

void MeshSimplify::_optimize_vertex_cache(uint32_t *r_inds, uint32_t p_num_inds, uint32_t p_num_verts) const {
	LocalVectori<uint32_t> inds_copy;
	inds_copy.resize(p_num_inds);
	memcpy(&inds_copy[0], r_inds, p_num_inds * sizeof(uint32_t));

	VertexCacheOptimizer<uint32_t> opt;
	opt.reorder_indices(r_inds, &inds_copy[0], p_num_inds / 3, p_num_verts);
}

uint32_t MeshSimplify::_find_or_add(uint32_t p_val, LocalVectori<uint32_t> &r_list) {
	int64_t found = r_list.find(p_val);
	if (found != -1) {
		return found;
	}
	uint32_t id = r_list.size();
	r_list.push_back(p_val);
	return id;
}

// returns number of indices
uint32_t MeshSimplify::simplify(const uint32_t *p_inds, uint32_t p_num_inds, const Vector3 *p_verts, uint32_t p_num_verts, uint32_t *r_inds, Vector3 *r_deduped_verts, uint32_t &r_num_deduped_verts, real_t p_threshold, MeshSimplifyCallback p_callback, void *p_userdata) {
	//bool MeshSimplify::simplify(const LocalVectori<uint32_t> &p_inds, const LocalVectori<Vector3> &p_verts, LocalVectori<uint32_t> &r_inds) {

	_callback = p_callback;
	_callback_userdata = p_userdata;

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

	// find neighbours
	for (int n = 0; n < _tris.size(); n++) {
		for (int i = n + 1; i < _tris.size(); i++) {
			_establish_neighbours(n, i);
		}
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
				_verts[edge.a].edge_vert = true;
				_verts[edge.b].edge_vert = true;
			}
		}
	}
}

void MeshSimplify::_establish_neighbours(int p_tri0, int p_tri1) {
	Tri &t0 = _tris[p_tri0];
	Tri &t1 = _tris[p_tri1];

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
		if (!vert.active)
			continue;

		if (vert.edge_vert)
			continue;

		return n;
	}

	return UINT32_MAX;
}

bool MeshSimplify::_find_vert_to_merge_to(uint32_t p_vert_from) {
	_merge_vert_ids.clear();

	// can join to any using this edge
	const Vert &vert = _verts[p_vert_from];

	for (int t = 0; t < vert.tris.size(); t++) {
		uint32_t tri_id = vert.tris[t];
		DEV_ASSERT(tri_id < _tris.size());

		const Tri &tri = _tris[tri_id];
		if (!tri.active)
			continue;
		for (int c = 0; c < 3; c++) {
			if (tri.corn[c] != p_vert_from) {
				_merge_vert_ids.push_back(tri.corn[c]);
				//return tri.corn[c];
			}
		}
	}
	return _merge_vert_ids.size() != 0;
}

bool MeshSimplify::_simplify_vert(uint32_t p_vert_id) {
	if (!_find_vert_to_merge_to(p_vert_id))
		return false;

	Vert &vert = _verts[p_vert_id];

	uint32_t merge_vert = UINT32_MAX;
	real_t max_displacement = 0.0;

	for (int m = 0; m < _merge_vert_ids.size(); m++) {
		uint32_t test_merge_vert = _merge_vert_ids[m];
		max_displacement = 0.0;

		// is this suitable for collapse?
		bool allow = true;
		for (int n = 0; n < vert.tris.size(); n++) {
			if (!_allow_collapse(vert.tris[n], p_vert_id, test_merge_vert, max_displacement)) {
				allow = false;
				break;
			}
		}

		if (allow) {
			merge_vert = test_merge_vert;
			break;
		}
	}

	if (merge_vert == UINT32_MAX)
		return false;

	// print_line("merging vert " + itos(p_vert_id) + " to vert " + itos(merge_vert));

	vert.active = false;

	real_t &new_max_displacement = _verts[merge_vert].displacement;
	new_max_displacement = MAX(new_max_displacement, max_displacement);

	// adjust all attached tris
	for (int n = 0; n < vert.tris.size(); n++) {
		_adjust_tri(vert.tris[n], p_vert_id, merge_vert);
	}

	for (int n = 0; n < vert.tris.size(); n++) {
		_resync_tri(vert.tris[n]);
	}

	return true;
}

bool MeshSimplify::_simplify() {
	uint32_t start_from = 0;

	while (true) {
		uint32_t vert_id = _choose_vert_to_merge(start_from);
		if (vert_id == UINT32_MAX)
			return false;

		// for next loop
		start_from = vert_id + 1;

		if (_simplify_vert(vert_id)) {
			return true;
		}
	}

	return false;
}

bool MeshSimplify::_allow_collapse(uint32_t p_tri_id, uint32_t p_vert_from, uint32_t p_vert_to, real_t &r_max_displacement) const {
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

	// find the distance of the old vertex from this new plane ..
	// if it is more than a certain threshold, disallow the collapse
	Vector3 pt = _verts[p_vert_from].pos;
	real_t dist = new_plane.distance_to(pt);

	real_t displacement = _verts[p_vert_from].displacement + Math::abs(dist);
	if (displacement > _threshold_dist)
		return false;

	r_max_displacement = MAX(r_max_displacement, displacement);

	// user callback?
	if (_callback) {
		if (!_callback(_callback_userdata, t.corn, new_corn))
			return false;
	}

	return true;
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
		// print_line("removed tri " + itos(p_tri_id));
		t.active = false;
		return;
	}

	// link the tri to the vert if not linked already
	Vert &vert = _verts[p_vert_to];
	vert.link_tri(p_tri_id);
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

	// re-establish neighbours
	for (int t = 0; t < _tris.size(); t++) {
		if (t == p_tri_id)
			continue;

		_establish_neighbours(p_tri_id, t);
	}
}
