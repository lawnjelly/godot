#include "mesh_simplify.h"
#include "mesh_deduplicator.h"

#define MESH_SIMPLIFY_DEBUG_LOGGING

#ifdef MESH_SIMPLIFY_DEBUG_LOGGING
#define MS_LOG(a)      \
	do {               \
		print_line(a); \
	} while (0)
#else
#define MS_LOG(a) \
	do {          \
	} while (0)
#endif

#define MESH_SIMPLIFY_TRIS_TO_REMOVE 0

String MeshSimplify::Edge::info() const {
	return itos(get_collapse_from()) + " to " + itos(vertex_to_collapse_to);
}

Vector3i MeshSimplify::Data::find_grid_pos(const Vector3 &p_pos) const {
	Vector3i res;

	for (uint32_t n = 0; n < 3; n++) {
		double d = (p_pos[n] - bound.position[n]) / bound_extent;
		double scaled = d * grid_size;

		// Rounding is a minor tweak to help given symmetric error
		// for the buckets.
		res[n] = Math::round(scaled);
		res[n] = CLAMP(res[n], 0, grid_size - 1);
	}

	return res;
}

void MeshSimplify::declare_indices(const Span<int> &p_indices) {
	input_data.indices.resize(p_indices.size());
	if (p_indices.size()) {
		memcpy(input_data.indices.ptr(), p_indices.ptr(), p_indices.size() * sizeof(uint32_t));
		static_assert(sizeof(uint32_t) == sizeof(int), "Copying assumes int is 32 bit.");
	}
}

void MeshSimplify::declare_uvs(const Span<Vector2> &p_uvs) {
	input_data.uvs = LocalVector<Vector2>(p_uvs);
}

void MeshSimplify::declare_uv2s(const Span<Vector2> &p_uvs) {
	input_data.uv2s = LocalVector<Vector2>(p_uvs);
}

void MeshSimplify::declare_positions(const Span<Vector3> &p_positions) {
	input_data.positions = LocalVector<Vector3>(p_positions);
}

bool MeshSimplify::_is_triangle_degenerate_from_positions(const Vector3i p[3]) const {
	int64_t abx = (int64_t)p[1].x - p[0].x;
	int64_t aby = (int64_t)p[1].y - p[0].y;
	int64_t abz = (int64_t)p[1].z - p[0].z;
	int64_t acx = (int64_t)p[2].x - p[0].x;
	int64_t acy = (int64_t)p[2].y - p[0].y;
	int64_t acz = (int64_t)p[2].z - p[0].z;

	int64_t crossX = aby * acz - abz * acy;
	int64_t crossY = abz * acx - abx * acz;
	int64_t crossZ = abx * acy - aby * acx;

	return (crossX == 0 && crossY == 0 && crossZ == 0);
}

bool MeshSimplify::_can_collapse(uint32_t kept, uint32_t deleted) const {
	// Disallow seams for now.
#if 0
	if (data.verts[deleted].is_seam_or_boundary) {
		return false;
	}
#endif

	for (uint32_t n = 0; n < data.tris.size(); n++) {
		const Tri &t = data.tris[n];
		if (!t.active)
			continue;

		bool touches_deleted = false;
		Vector3i old_c[3];

		for (int i = 0; i < 3; ++i) {
			old_c[i] = data.verts[t.corn[i]].position;
			if (t.corn[i] == deleted)
				touches_deleted = true;
		}

		if (!touches_deleted)
			continue;

		if (t.corn[0] == kept || t.corn[1] == kept || t.corn[2] == kept)
			continue;

		Vector3i new_c[3];
		for (int i = 0; i < 3; ++i) {
			new_c[i] = (t.corn[i] == deleted) ? data.verts[kept].position : old_c[i];
		}

		if (_is_triangle_degenerate_from_positions(new_c)) {
			MS_LOG("_can_collapse rejecting edge from " + itos(deleted) + " to " + itos(kept) + " - _is_triangle_degenerate_from_positions");
			return false;
		}

		// Strong normal protection
		Vector3 v1b = Vector3(old_c[1] - old_c[0]);
		Vector3 v2b = Vector3(old_c[2] - old_c[0]);
		Vector3 before = v1b.cross(v2b);

		Vector3 v1a = Vector3(new_c[1] - new_c[0]);
		Vector3 v2a = Vector3(new_c[2] - new_c[0]);
		Vector3 after = v1a.cross(v2a);

		double len_b2 = before.length_squared();
		double len_a2 = after.length_squared();

		if (len_a2 < 1.0) {
			MS_LOG("_can_collapse rejecting edge from " + itos(deleted) + " to " + itos(kept) + " - len_a2");
			return false;
		}

		if (len_b2 > 1.0 && len_a2 > 1.0) {
			double cos_angle = before.dot(after) / (Math::sqrt(len_b2) * Math::sqrt(len_a2));
			if (cos_angle < 0.1) { // Very strict - almost no inversion
				MS_LOG("_can_collapse rejecting edge from " + itos(deleted) + " to " + itos(kept) + " - cos_angle");
				return false;
			}
		}

		// Strong perimeter protection for cylinders
		double old_peri = (old_c[1] - old_c[0]).length() + (old_c[2] - old_c[1]).length() + (old_c[0] - old_c[2]).length();
		double new_peri = (new_c[1] - new_c[0]).length() + (new_c[2] - new_c[1]).length() + (new_c[0] - new_c[2]).length();

		if (new_peri < old_peri * 0.85) {
			MS_LOG("_can_collapse rejecting edge from " + itos(deleted) + " to " + itos(kept) + " - peri");
			return false;
		}

		// Aspect ratio
		double sides[3] = {
			(new_c[1] - new_c[0]).length(),
			(new_c[2] - new_c[1]).length(),
			(new_c[0] - new_c[2]).length()
		};
		double max_s = MAX(sides[0], MAX(sides[1], sides[2]));
		double min_s = MIN(sides[0], MIN(sides[1], sides[2]));

		if (min_s > 0.0 && max_s / min_s > 12.0) {
			MS_LOG("_can_collapse rejecting edge from " + itos(deleted) + " to " + itos(kept) + " - min_s");
			return false;
		}

#if 0
		// STRONG CORNER / FEATURE EDGE PROTECTION
		// Reject collapses that significantly change the local shape at corners
		Vector3 old_dir_ab = Vector3(old_c[1] - old_c[0]).normalized();
		Vector3 old_dir_ac = Vector3(old_c[2] - old_c[0]).normalized();
		Vector3 new_dir_ab = Vector3(new_c[1] - new_c[0]).normalized();
		Vector3 new_dir_ac = Vector3(new_c[2] - new_c[0]).normalized();
		
		double dot_ab = old_dir_ab.dot(new_dir_ab);
		double dot_ac = old_dir_ac.dot(new_dir_ac);
		
		if (dot_ab < 0.85 || dot_ac < 0.85) { // significant direction change
			MS_LOG("_can_collapse rejecting edge from " + itos (deleted) + " to " + itos(kept)  + " - dot_ab");
			return false;
		}
#endif

#if 0
			   // Also reject large movement of corner vertices
		double move_dist = (new_c[0] - old_c[0]).length();
		if (move_dist > 300.0) { // tune to your model scale
			MS_LOG("_can_collapse rejecting edge from " + itos (deleted) + " to " + itos(kept)  + " - move_dist");
			return false;
		}
#endif
	}
	return true;
}

void MeshSimplify::_validate_and_rebuild() {
	uint32_t active_count = 0;
	for (uint32_t n = 0; n < data.tris.size(); n++) {
		Tri &t = data.tris[n];
		if (!t.active)
			continue;

		if (_is_triangle_degenerate(t.corn)) {
			t.active = false;
			continue;
		}
		active_count++;
	}
	//	print_line("Validation: active tris = " + itos(active_count));
}

void MeshSimplify::_debug_log_input_data() {
	Span<Vector3> in_verts = Span<Vector3>(input_data.positions);
	Span<Vector2> in_uvs = Span<Vector2>(input_data.uvs);

#ifdef MESH_SIMPLIFY_DEBUG_LOGGING
	MS_LOG("IN_VERTS\n");
	for (uint32_t n = 0; n < in_verts.size(); n++) {
		String sz = itos(n) + " : " + String(Variant(in_verts[n]));
		if (in_uvs.size())
			sz += ",\tuv " + String(Variant(in_uvs[n]));
		MS_LOG(sz);
	}
#endif
}

bool MeshSimplify::simplify_mesh() {
	// Can't simplify when no indices.
	if (!input_data.indices.size()) {
		return false;
	}

	// Duduplicate.
	MeshDeduplicator dd;

	LocalVector<Vector3> verts;
	LocalVector<uint32_t> inds;

	uint32_t num_streams = 1;
	if (input_data.uvs.size()) {
		num_streams++;
	}
	if (input_data.uv2s.size()) {
		num_streams++;
	}

	dd.set_num_attribute_streams(num_streams);

	uint32_t fill_stream = 0;

	MeshAttributeStream &as_pos = dd.get_input_attribute_stream(fill_stream++);
	as_pos.set_type(MeshAttributeStream::ATTR_POSITION);
	as_pos.vec3 = input_data.positions;

	if (input_data.uvs.size()) {
		MeshAttributeStream &as = dd.get_input_attribute_stream(fill_stream++);
		as.set_type(MeshAttributeStream::ATTR_UV);
		as.vec2 = input_data.uvs;
	}

	if (input_data.uv2s.size()) {
		MeshAttributeStream &as = dd.get_input_attribute_stream(fill_stream++);
		as.set_type(MeshAttributeStream::ATTR_UV);
		as.vec2 = input_data.uv2s;
	}

	if (!dd.process(input_data.indices, inds)) {
		return false;
	}
	verts = dd.get_output_attribute_stream(0).vec3;

	// Save the deduplicated data.
	input_data.indices = inds;
	input_data.positions = verts;

	uint32_t uv_stream = 1;
	if (input_data.uvs.size()) {
		input_data.uvs = dd.get_output_attribute_stream(uv_stream++).vec2;
	}
	if (input_data.uv2s.size()) {
		input_data.uv2s = dd.get_output_attribute_stream(uv_stream).vec2;
	}

	ERR_FAIL_COND_V(!input_data.indices.size(), false);
	ERR_FAIL_COND_V(!input_data.positions.size(), false);

	_debug_log_input_data();

	// Find world bound.
	Span<Vector3> in_verts = Span<Vector3>(input_data.positions);
	Span<Vector2> in_uvs = Span<Vector2>(input_data.uvs);
	Span<Vector2> in_uv2s = Span<Vector2>(input_data.uv2s);

	// Find bounds.
	data.bound.position = in_verts[0];
	data.bound.size = Vector3();

	for (uint32_t n = 1; n < in_verts.size(); n++) {
		data.bound.expand_to(in_verts[n]);
	}

	// Use some minimum bound, to prevent float error.
	const real_t min_bound = 1e-4f;

	if (data.bound.size.length() < min_bound) {
		data.bound.size = Vector3(min_bound, min_bound, min_bound);
	}

	// Use a single bound extent for all axes, so the scaling on each axis is the same.
	// This is wasteful of precision for the smaller axes, but makes the math *work*
	// correctly in all dimensions.
	data.bound_extent = data.bound.size.coord[data.bound.size.max_axis()];

	// Create in verts, and find their grid pos.
	data.verts.resize(in_verts.size());

	for (uint32_t n = 0; n < in_verts.size(); n++) {
		data.verts[n].position = data.find_grid_pos(in_verts[n]);

		if (in_uvs.size()) {
			data.verts[n].uv = in_uvs[n];
		}
		if (in_uv2s.size()) {
			data.verts[n].uv2 = in_uv2s[n];
		}
	}

	_create_tris();

	// No valid tris to simplify.
	if (!data.tris.size()) {
		return false;
	}

	// 2. Populate priority queue with all unique edges
	std::priority_queue<SortedEdge> queue;
	for (uint32_t n = 0; n < data.edges.size(); n++) {
		_evaluate_edge_collapse(n);
		SortedEdge e(n, data.edges[n].cost, 0);
		queue.push(e);
	}

	uint32_t current_triangle_count = data.tris.size();
	uint32_t before_triangle_count = current_triangle_count;

	uint32_t target = before_triangle_count / 8; // adjust as needed (e.g. / 2, / 8, etc.)

	if (MESH_SIMPLIFY_TRIS_TO_REMOVE != 0) {
		target = before_triangle_count - MESH_SIMPLIFY_TRIS_TO_REMOVE;
	}

	while (current_triangle_count > target && !queue.empty()) {
		SortedEdge se = queue.top();
		queue.pop();

		if (se.edge_id >= data.edges.size())
			continue;

		Edge &edge = data.edges[se.edge_id];

		if (!edge.active || se.version != edge.version || se.cost != edge.cost)
			continue;
		if (!data.verts[edge.a].active || !data.verts[edge.b].active)
			continue;

			// Disallow collapsing edges for now.
#if 0
		if (edge.is_seam_or_boundary) {
			edge.active = false;
			continue;
		}
#endif

		uint32_t kept = edge.vertex_to_collapse_to;
		uint32_t deleted = (kept == edge.a ? edge.b : edge.a);

		// Strong safety check - this is the main fix for bad visuals on the shark
		if (!_can_collapse(kept, deleted)) {
			edge.active = false;
			continue;
		}

		//			if (edge.is_seam_or_boundary) {
		//				print_line("WARNING: Collapsing seam/boundary edge " + itos(e_idx) + "!");
		//			}

#ifdef MESH_SIMPLIFY_DEBUG_LOGGING
		String sz = "collapsing edge " + itos(se.edge_id) + " : " + itos(edge.get_collapse_from()) + " to " + itos(edge.vertex_to_collapse_to);
		if (edge.is_seam_or_boundary) {
			sz += "\tSEAM";
		}
		sz += " cost : " + itos(edge.get_readable_cost());
		MS_LOG(sz);
#endif

		// Collapse to one of the original endpoints only
		Vert &kept_vert = data.verts[kept];
		Vert &deleted_vert = data.verts[deleted];

		kept_vert.Q = kept_vert.Q + deleted_vert.Q;
		deleted_vert.active = false;
		edge.active = false;

		// IMPORTANT: Deactivate ALL edges connected to the deleted vertex
		for (uint32_t n = 0; n < deleted_vert.edges.size(); n++) {
			uint32_t e_idx = deleted_vert.edges[n];
			data.edges[e_idx].active = false;
		}

		// Update triangles
		// BULLETPROOF TRIANGLE RESTITCHING
		for (uint32_t n = 0; n < data.tris.size(); n++) {
			Tri &t = data.tris[n];
			if (!t.active)
				continue;

			bool modified = false;
			int kept_count = 0;

			for (int i = 0; i < 3; i++) {
				if (t.corn[i] == deleted) {
					t.corn[i] = kept;
					modified = true;
				}
				if (t.corn[i] == kept)
					kept_count++;
			}

			if (modified) {
				// Remove if degenerate or invalid
				if (kept_count > 1 || _is_triangle_degenerate(t.corn)) {
					t.active = false;
					current_triangle_count--;
				}
			}
		}

		// Rebuild adjacency for kept vertex safely
		data.verts[kept].edges.clear();
		for (uint32_t e = 0; e < data.edges.size(); ++e) {
			Edge &edge = data.edges[e];
			if (edge.active && (edge.a == kept || edge.b == kept)) {
				data.verts[kept].edges.push_back(e);
			}
		}

		// ... after triangle update and adjacency rebuild ...
		_validate_and_rebuild();

		// Re-evaluate edges connected to the kept vertex
		const LocalVector<uint32_t> &connected = data.verts[kept].edges;
		for (uint32_t n = 0; n < connected.size(); n++) {
			uint32_t e_idx = connected[n];

			Edge &e = data.edges[e_idx];
			if (!e.active)
				continue;
			if (e.a != kept && e.b != kept)
				continue; // safety

			_evaluate_edge_collapse(e_idx);
			e.version++;
			queue.push(SortedEdge(e_idx, e.cost, e.version));
		}
#if 0	
		// Re-evaluate edges connected to the kept vertex
		for (uint32_t n = 0; n < data.edges.size(); n++) {
			Edge &e = data.edges[n];
			if (!e.active)
				continue;
			if (e.a == kept || e.b == kept) {
				_evaluate_edge_collapse(n);
				e.version++;
				queue.push(SortedEdge(n, e.cost, e.version));
			}
		}
#endif
	}

	// Final cleanup of any new degenerates
	for (uint32_t n = 0; n < data.tris.size(); n++) {
		Tri &t = data.tris[n];
		if (t.active && _is_triangle_degenerate(t.corn)) {
			t.active = false;
			current_triangle_count--;
		}
	}
	// END

	// Create final output indices.
	data.output_remapped_indices.resize(current_triangle_count * 3);
	uint32_t out_ind_count = 0;

	for (uint32_t n = 0; n < data.tris.size(); n++) {
		const Tri &t = data.tris[n];
		if (!t.active)
			continue;

#ifdef MESH_SIMPLIFY_DEBUG_LOGGING
		String sz = "final tri " + itos(n) + " : " + itos(t.corn[0]) + ", " + itos(t.corn[1]) + ", " + itos(t.corn[2]) + " : ";

		for (uint32_t i = 0; i < 3; i++) {
			sz += String(Variant(input_data.positions[t.corn[i]])) + ", ";
		}

		MS_LOG(sz);
#endif

		for (uint32_t c = 0; c < 3; c++) {
			uint32_t orig_index = dd.get_output_vertex_mapping_to_input_vertex(t.corn[c]);
			// MS_LOG("\t" + itos(orig_index));
			data.output_remapped_indices[out_ind_count++] = orig_index;
		}
	}

	// TEST
#if 0
	LocalVector<uint32_t> temp = data.output_remapped_indices;
	data.output_remapped_indices.resize(3);
	const uint32_t test_tri = 0;
	for (uint32_t n = 0; n < 3; n++) {
		data.output_remapped_indices[n] = temp[(test_tri * 3) + n];
	}
#endif

	// Debug
	print_line("simplify before_triangle_count: " + itos(before_triangle_count) + ", after_triangle_count: " + itos(current_triangle_count));

	return true;
}

// Returns:
//   > 0  -> front side (positive side, according to right-hand rule)
//   < 0  -> back side
//   == 0 -> exactly on the plane
int32_t MeshSimplify::_triangle_which_side(const Vector3i &p_a, const Vector3i &p_b, const Vector3i &p_c, const Vector3i &p_test) const {
	// Vectors from A
	int64_t ux = (int64_t)p_b.x - p_a.x;
	int64_t uy = (int64_t)p_b.y - p_a.y;
	int64_t uz = (int64_t)p_b.z - p_a.z;

	int64_t vx = (int64_t)p_c.x - p_a.x;
	int64_t vy = (int64_t)p_c.y - p_a.y;
	int64_t vz = (int64_t)p_c.z - p_a.z;

	int64_t wx = (int64_t)p_test.x - p_a.x;
	int64_t wy = (int64_t)p_test.y - p_a.y;
	int64_t wz = (int64_t)p_test.z - p_a.z;

	// Normal N = U × V  (cross product)
	int64_t nx = uy * vz - uz * vy;
	int64_t ny = uz * vx - ux * vz;
	int64_t nz = ux * vy - uy * vx;

	// Scalar = N · W
	int64_t scalar = nx * wx + ny * wy + nz * wz;

	if (scalar > 0)
		return 1; // front
	if (scalar < 0)
		return -1; // back
	return 0; // on plane
}

bool MeshSimplify::_is_triangle_degenerate(const uint32_t p_inds[3]) const {
	if ((p_inds[0] == p_inds[1]) || (p_inds[1] == p_inds[2]) || (p_inds[0] == p_inds[2])) {
		return true;
	}

	const Vector3i &a = data.verts[p_inds[0]].position;
	const Vector3i &b = data.verts[p_inds[1]].position;
	const Vector3i &c = data.verts[p_inds[2]].position;

	// We can check for colinear points and duplicate points at the same time.
	int64_t abx = (int64_t)b.x - a.x;
	int64_t aby = (int64_t)b.y - a.y;
	int64_t abz = (int64_t)b.z - a.z;

	int64_t acx = (int64_t)c.x - a.x;
	int64_t acy = (int64_t)c.y - a.y;
	int64_t acz = (int64_t)c.z - a.z;

	int64_t crossX = aby * acz - abz * acy;
	int64_t crossY = abz * acx - abx * acz;
	int64_t crossZ = abx * acy - aby * acx;

	if (crossX == 0 && crossY == 0 && crossZ == 0) {
		// Degenerate.
		return true;
	}

	// We already have the data to compute squared magnitude.
	// We can compare this against a small threshold if required.
#if 0
	int64_t mag_sq = crossX*crossX + crossY*crossY + crossZ*crossZ;
#endif

	return false;
}

uint32_t MeshSimplify::_create_edge(uint32_t p_corn_a, uint32_t p_corn_b, uint32_t p_triangle_id) {
	Edge e;
	e.a = p_corn_a;
	e.b = p_corn_b;
	e.sort();

	for (uint32_t n = 0; n < data.edges.size(); n++) {
		if (data.edges[n] == e) {
			return n;
		}
	}

	data.edges.push_back(e);
	return data.edges.size() - 1;
}

void MeshSimplify::_triangle_calculate_plane(uint32_t p_tri_id) {
	Tri &tri = data.tris[p_tri_id];
	Vert &p0 = data.verts[tri.corn[0]];
	Vert &p1 = data.verts[tri.corn[1]];
	Vert &p2 = data.verts[tri.corn[2]];

	tri.plane = Plane_64(p0.pos(), p1.pos(), p2.pos());
}

void MeshSimplify::_initialize_vertex_quadrics() {
	// Step A: Calculate the quadric matrix for every triangle plane
	for (uint32_t n = 0; n < data.tris.size(); n++) {
		Tri &t = data.tris[n];

		// Make sure plane is up to date... (should be?)

		// Fetch the raw vertices using your corner indices
		const Vert &v0 = data.verts[t.corn[0]];
		const Vert &v1 = data.verts[t.corn[1]];
		const Vert &v2 = data.verts[t.corn[2]];

		Vector3_64 p0 = v0.pos();
		Vector3_64 p1 = v1.pos();
		Vector3_64 p2 = v2.pos();

		// Calculate face area and normal vector
		Vector3_64 edge1 = p1 - p0;
		Vector3_64 edge2 = p2 - p0;
		Vector3_64 cross = edge1.cross(edge2);
		double normal_length = cross.length();

		// Avoid processing degenerate, flat triangles
		if (normal_length < 1e-7) {
			continue;
		}
		double area = 0.5 * normal_length;
		Vector3_64 normal = cross / normal_length;

#if 0
		// In _triangle_calculate_plane or when building Kp:
		double area = 0.5 * (p1.pos() - p0.pos()).cross(p2.pos() - p0.pos()).length(); // or use unnormalized for speed
		// Then scale Kp by area before adding.
#endif

		// 1. STANDARD POSITION GEOMETRY QUADRIC
		// Create the fundamental error matrix Kp = p * p^T
		Quadric Kp;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				Kp.m[i][j] = t.plane.coord[i] * t.plane.coord[j];
			}
		}

		// 2. TEXTURE ATTRIBUTE QUADRICS (Hoppe's Framework)
		double base_matrix[4][4] = {
			{ p0.x, p0.y, p0.z, 1.0 },
			{ p1.x, p1.y, p1.z, 1.0 },
			{ p2.x, p2.y, p2.z, 1.0 },
			{ normal.x, normal.y, normal.z, 0.0 }
		};

		// 4x4 determinant solver
		auto det_4x4 = [](double m[4][4]) -> double {
			double sub0 = m[1][1] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) - m[1][2] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) + m[1][3] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]);
			double sub1 = m[1][0] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) - m[1][2] * (m[2][0] * m[3][3] - m[2][3] * m[3][0]) + m[1][3] * (m[2][0] * m[3][2] - m[2][2] * m[3][0]);
			double sub2 = m[1][0] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) - m[1][1] * (m[2][0] * m[3][3] - m[2][3] * m[3][0]) + m[1][3] * (m[2][0] * m[3][1] - m[2][1] * m[3][0]);
			double sub3 = m[1][0] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]) - m[1][1] * (m[2][0] * m[3][2] - m[2][2] * m[3][0]) + m[1][2] * (m[2][0] * m[3][1] - m[2][1] * m[3][0]);
			return m[0][0] * sub0 - m[0][1] * sub1 + m[0][2] * sub2 - m[0][3] * sub3;
		};

		double main_det = det_4x4(base_matrix);
		Quadric Qu, Qv;

		// Only compute UV attributes if the triangle is invertible and has valid mapping
		if (Math::abs(main_det) > 1e-7 && normal_length > 1e-7) {
			double targets_u[4] = { v0.uv.x, v1.uv.x, v2.uv.x, 0 }; // Godot Vector2 uses x,y for u,v
			double targets_v[4] = { v0.uv.y, v1.uv.y, v2.uv.y, 0 };
			double coeff_u[4] = { 0 };
			double coeff_v[4] = { 0 };

			for (int col = 0; col < 4; ++col) {
				double temp_matrix[4][4];

				// Solve U coefficients
				for (int r = 0; r < 4; ++r) {
					for (int c = 0; c < 4; ++c)
						temp_matrix[r][c] = base_matrix[r][c];
				}
				for (int row = 0; row < 4; ++row)
					temp_matrix[row][col] = targets_u[row];
				coeff_u[col] = det_4x4(temp_matrix) / main_det;

				// Solve V coefficients
				for (int r = 0; r < 4; ++r) {
					for (int c = 0; c < 4; ++c)
						temp_matrix[r][c] = base_matrix[r][c];
				}
				for (int row = 0; row < 4; ++row)
					temp_matrix[row][col] = targets_v[row];
				coeff_v[col] = det_4x4(temp_matrix) / main_det;
			}

			// Generate outer product scaled by the triangle face surface area
			for (int i = 0; i < 4; ++i) {
				for (int j = 0; j < 4; ++j) {
					Qu.m[i][j] = area * (coeff_u[i] * coeff_u[j]);
					Qv.m[i][j] = area * (coeff_v[i] * coeff_v[j]);
				}
			}
		}

		// Step B: Accumulate this plane's matrix into its three corner vertices
		for (uint32_t i = 0; i < 3; i++) {
			Vert &v = data.verts[t.corn[i]];
			v.Q = v.Q + Kp;
			v.Qu = v.Qu + Qu;
			v.Qv = v.Qv + Qv;
		}
	}
}

// Evaluates the error equation: v^T * Q * v
double MeshSimplify::_compute_quadric_error(const Vector3i &p_pos, const Quadric &Q) {
	// 1. Expand the 3D position into a 4D homogeneous vector [x, y, z, 1]
	double x = p_pos.x;
	double y = p_pos.y;
	double z = p_pos.z;
	double w = 1;

	// 2. Perform the first step: Multiply the matrix Q by the column vector v.
	// This yields an intermediate 4D vector (let's call it R).
	double rx = Q.m[0][0] * x + Q.m[0][1] * y + Q.m[0][2] * z + Q.m[0][3] * w;
	double ry = Q.m[1][0] * x + Q.m[1][1] * y + Q.m[1][2] * z + Q.m[1][3] * w;
	double rz = Q.m[2][0] * x + Q.m[2][1] * y + Q.m[2][2] * z + Q.m[2][3] * w;
	double rw = Q.m[3][0] * x + Q.m[3][1] * y + Q.m[3][2] * z + Q.m[3][3] * w;

	// 3. Perform the final step: Compute the dot product of the row vector v^T and R.
	// This yields the single scalar error value.
	double error = (x * rx) + (y * ry) + (z * rz) + (w * rw);

	return error;
}

double MeshSimplify::_compute_attribute_error(const Vector3i &p_pos, double p_attr, const Quadric &Qa) {
	double x = p_pos.x;
	double y = p_pos.y;
	double z = p_pos.z;
	double w = 1.0;

	// Term A: v^T * Qa * v
	double rx = Qa.m[0][0] * x + Qa.m[0][1] * y + Qa.m[0][2] * z + Qa.m[0][3] * w;
	double ry = Qa.m[1][0] * x + Qa.m[1][1] * y + Qa.m[1][2] * z + Qa.m[1][3] * w;
	double rz = Qa.m[2][0] * x + Qa.m[2][1] * y + Qa.m[2][2] * z + Qa.m[2][3] * w;
	double rw = Qa.m[3][0] * x + Qa.m[3][1] * y + Qa.m[3][2] * z + Qa.m[3][3] * w;
	double vt_Q_v = (x * rx) + (y * ry) + (z * rz) + (w * rw);

	// Term B: r^T * v (Dot product of the last column vector 'r' with the position vector)
	double r_t_v = Qa.m[0][3] * x + Qa.m[1][3] * y + Qa.m[2][3] * z + Qa.m[3][3] * w;

	// Term C: Bottom right element
	double s = Qa.m[3][3];

	// Full expansion evaluation
	return vt_Q_v - (2.0 * p_attr * r_t_v) + (p_attr * p_attr * s);
}

void MeshSimplify::_evaluate_edge_collapse(uint32_t p_edge_id) {
	Edge &edge = data.edges[p_edge_id];
	const Vert &a = data.verts[edge.a];
	const Vert &b = data.verts[edge.b];

	Quadric Q_new = a.Q + b.Q;
	Quadric Qu_new = a.Qu + b.Qu;
	Quadric Qv_new = a.Qv + b.Qv;

	// User defined weighting balancing factor. Tune this to your preference.
	// 1.0 to 10.0 handles texture preservation nicely without stalling geometry changes.
	const double beta = 150;

	//double distance_cost = (a.pos() - b.pos()).length();
	double distance_cost = 0;

	double geom_cost_a = _compute_quadric_error(a.position, Q_new);
	double geom_cost_b = _compute_quadric_error(b.position, Q_new);

	double attr_a = 0, attr_b = 0;

	// Only compute if UVs exist
	if (input_data.uvs.size()) { // were these declared?
		attr_a += _compute_attribute_error(a.position, a.uv.x, Qu_new);
		attr_a += _compute_attribute_error(a.position, a.uv.y, Qv_new);

		attr_b += _compute_attribute_error(b.position, b.uv.x, Qu_new);
		attr_b += _compute_attribute_error(b.position, b.uv.y, Qv_new);
	}

	double total_a = distance_cost + geom_cost_a + beta * attr_a;
	double total_b = distance_cost + geom_cost_b + beta * attr_b;

	// === SEAM LINE DISRUPTION PENALTY ===

	// Add penalty for collapsing from a seam.
	if (a.is_seam_or_boundary) {
		bool breaks_line = true;

		if (a.seam_neighbour_verts.size() == 2) {
			for (uint32_t n = 0; n < a.seam_neighbour_verts.size(); n++) {
				if (a.seam_neighbour_verts[n] == edge.b) {
					breaks_line = false; // direct neighbour on scene line
					break;
				}
			}
		} else {
			breaks_line = false;
		}

		if (breaks_line) {
			total_b *= 60;
			total_b = MAX(total_b, 60.0);
		} else {
			// Calculate angle penalty.
			const Vector3i &v0 = data.verts[a.seam_neighbour_verts[0]].position;
			const Vector3i &v1 = data.verts[a.seam_neighbour_verts[1]].position;

			double area_before = a.position.calculate_triangle_area(v0, v1);
			// double area_after = b.position.calculate_triangle_area(v0, v1);

			//double change = Math::absd(area_after - area_before);
			double change = Math::absd(area_before);
			total_b += change;
		}
	}
	if (b.is_seam_or_boundary) {
		bool breaks_line = true;
		for (uint32_t n = 0; n < b.seam_neighbour_verts.size(); n++) {
			if (b.seam_neighbour_verts[n] == edge.a) {
				breaks_line = false; // direct neighbour on scene line
				break;
			}
		}

		if (breaks_line) {
			total_a *= 60;
			total_a = MAX(total_a, 60.0);
		} else {
			// Calculate angle penalty.
			const Vector3i &v0 = data.verts[b.seam_neighbour_verts[0]].position;
			const Vector3i &v1 = data.verts[b.seam_neighbour_verts[1]].position;

			double area_before = b.position.calculate_triangle_area(v0, v1);
			double area_after = a.position.calculate_triangle_area(v0, v1);

			double change = Math::absd(area_after - area_before);
			total_a += change;
		}
	}

	if (total_a < total_b) {
		edge.vertex_to_collapse_to = edge.a;
		edge.cost = total_a;
	} else {
		edge.vertex_to_collapse_to = edge.b;
		edge.cost = total_b;
	}

	// ==========================================
	// UV SEAM AND VISUAL SILHOUETTE PROTECTOR
	// ==========================================
#if 0
	if (edge.is_seam_or_boundary) {
		// Apply a massive penalty multiplier to the final cost.
		// This forces the queue to decimate flat, internal areas completely
		// before it even considers altering a UV boundary.
		edge.cost *= 1000;
		edge.cost = MAX(edge.cost, 1000.0);

		// OPTIONAL STRICT PROTECTION EXTRA:
		// If you want to absolutely guarantee that UV borders never distort their shape,
		// you can flag an edge as completely non-collapsable if it crosses distinct islands,
		// or freeze its cost to a fixed value.
	}
#endif

#ifdef MESH_SIMPLIFY_DEBUG_LOGGING
	print_line("\tevaluating edge " + itos(edge.get_collapse_from()) + " to " + itos(edge.vertex_to_collapse_to) + " ... cost " + itos(edge.get_readable_cost()));
#endif
}

void MeshSimplify::_detect_seam_edges() {
	// Reset
	for (uint32_t i = 0; i < data.edges.size(); i++) {
		data.edges[i].is_seam_or_boundary = false;
		data.edges[i].triangle_count = 0;
	}

	// Count how many triangles use each edge
	for (uint32_t n = 0; n < data.tris.size(); n++) {
		const Tri &t = data.tris[n];
		if (!t.active)
			continue;

		for (int i = 0; i < 3; ++i) {
			uint32_t eid = t.edge_ids[i];
			if (eid < data.edges.size()) {
				data.edges[eid].triangle_count++;
			}
		}
	}

	// Flag true boundaries (edges with only 1 triangle)
	int seam_count = 0;
	for (uint32_t i = 0; i < data.edges.size(); i++) {
		Edge &e = data.edges[i];

		if (e.triangle_count <= 1) {
			e.is_seam_or_boundary = true;
			seam_count++;
		} else if (e.triangle_count > 2) {
			// Non-manifold edge - also protect
			e.is_seam_or_boundary = true;
			seam_count++;
		}

		// Make the verts on this edge as seams.
		if (e.is_seam_or_boundary) {
			data.verts[e.a].is_seam_or_boundary = true;
			data.verts[e.b].is_seam_or_boundary = true;

			if (e.triangle_count == 1) {
				data.verts[e.a].seam_neighbour_verts.push_back(e.b);
				data.verts[e.b].seam_neighbour_verts.push_back(e.a);
			}
		}

#ifdef MESH_SIMPLIFY_DEBUG_LOGGING
		String sz = "edge " + itos(i) + " : " + itos(e.a) + " to " + itos(e.b);
		if (e.is_seam_or_boundary) {
			sz += "\tSEAM";
		}
		MS_LOG(sz);
#endif
	}

#ifdef MESH_SIMPLIFY_DEBUG_LOGGING
	for (uint32_t n = 0; n < data.verts.size(); n++) {
		const Vert &vert = data.verts[n];
		String sz = "vert " + itos(n) + " : ";
		if (vert.is_seam_or_boundary) {
			sz += "seam ";
		}
		sz += String(vert.position);
		MS_LOG(sz);
	}
#endif

	print_line("Detected " + itos(seam_count) + " seam/boundary edges out of " + itos(data.edges.size()));
}

void MeshSimplify::_create_tris() {
	uint32_t num_orig_tris = input_data.indices.size() / 3;

	// Better to overestimate at first.
	data.tris.resize(num_orig_tris);

	uint32_t index_count = 0;
	uint32_t valid_tri_count = 0;

	for (uint32_t t = 0; t < num_orig_tris; t++) {
		Tri &tri = data.tris[valid_tri_count];
		tri.corn[0] = input_data.indices[index_count++];
		tri.corn[1] = input_data.indices[index_count++];
		tri.corn[2] = input_data.indices[index_count++];

		// Ignore null tris.
		if (_is_triangle_degenerate(tri.corn)) {
			continue;
		}

		tri.edge_ids[0] = _create_edge(tri.corn[0], tri.corn[1], valid_tri_count);
		tri.edge_ids[1] = _create_edge(tri.corn[1], tri.corn[2], valid_tri_count);
		tri.edge_ids[2] = _create_edge(tri.corn[2], tri.corn[0], valid_tri_count);

		// Add the tri to the verts.
		for (uint32_t c = 0; c < 3; c++) {
			Vert &v = data.verts[tri.corn[c]];
			v.active = true;
		}

		// Tri was valid.
		valid_tri_count++;
	}

	// Build vertex / edge adjacency.
	for (uint32_t e = 0; e < data.edges.size(); ++e) {
		const Edge &edge = data.edges[e];
		data.verts[edge.a].edges.push_back(e);
		data.verts[edge.b].edges.push_back(e);
	}

	// Reserve capacity to prevent reallocations that could invalidate references later
	for (uint32_t i = 0; i < data.verts.size(); ++i) {
		data.verts[i].edges.reserve(16); // typical valence for manifold meshes
	}

	if (valid_tri_count) {
		print_line("Simplify valid tris " + itos(valid_tri_count) + ", degenerate " + itos(num_orig_tris - valid_tri_count));

#if 0
		for (uint32_t n=0; n<data.edges.size(); n++)
		{
			const Edge &e = data.edges[n];
			print_line("\tedge " + itos(n) + " : from " + itos(e.a) + " to " + itos(e.b) + " with " + itos(e.tris.size()) + " tris.");
		}
#endif
	}

	// Resize to exact size the tri array.
	// (could possibly be omitted if we store count externally to the LocalVector)
	data.tris.resize(valid_tri_count);

	for (uint32_t n = 0; n < valid_tri_count; n++) {
		_triangle_calculate_plane(n);
	}

	_detect_seam_edges();
	_initialize_vertex_quadrics();
}
