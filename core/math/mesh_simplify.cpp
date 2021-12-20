#include "mesh_simplify.h"
#include "core/print_string.h"

// returns number of indices
uint32_t MeshSimplify::simplify(const uint32_t *p_inds, uint32_t p_num_inds, const Vector3 *p_verts, uint32_t p_num_verts, uint32_t *r_inds) {
	//bool MeshSimplify::simplify(const LocalVectori<uint32_t> &p_inds, const LocalVectori<Vector3> &p_verts, LocalVectori<uint32_t> &r_inds) {
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
			for (int c = 0; c < 3; c++)
				r_inds[count++] = tri.corn[c];
			//r_inds.push_back(tri.corn[c]);
		}
	}

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
			_verts[tri.corn[c]].link_tri(t);

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

uint32_t MeshSimplify::_choose_vert_to_merge() const {
	// find a vertex that is not an edge vert
	for (int n = 0; n < _verts.size(); n++) {
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
		const Tri &tri = _tris[vert.tris[t]];
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

bool MeshSimplify::_simplify() {
	uint32_t vert_id = _choose_vert_to_merge();
	if (vert_id == UINT32_MAX)
		return false;

	if (!_find_vert_to_merge_to(vert_id))
		return false;

	Vert &vert = _verts[vert_id];

	uint32_t merge_vert = UINT32_MAX;
	for (int m = 0; m < _merge_vert_ids.size(); m++) {
		uint32_t test_merge_vert = _merge_vert_ids[m];

		// is this suitable for collapse?
		bool allow = true;
		for (int n = 0; n < vert.tris.size(); n++) {
			if (!_allow_collapse(vert.tris[n], vert_id, test_merge_vert)) {
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

	print_line("merging vert " + itos(vert_id) + " to vert " + itos(merge_vert));

	vert.active = false;

	// adjust all attached tris
	for (int n = 0; n < vert.tris.size(); n++) {
		_adjust_tri(vert.tris[n], vert_id, merge_vert);
	}

	for (int n = 0; n < vert.tris.size(); n++) {
		_resync_tri(vert.tris[n]);
	}

	return true;
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
		print_line("removed tri " + itos(p_tri_id));
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
