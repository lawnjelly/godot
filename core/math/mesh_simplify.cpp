#include "mesh_simplify.h"
#include "core/os/os.h"
#include "mesh_deduplicator.h"

//#define MESH_SIMPLIFY_DISALLOW_SEAMS
//#define MESH_SIMPLIFY_ONE_AT_A_TIME

#define MESH_SIMPLIFY_FACTOR(a) ((a * 1) / 3)

//#define MESH_SIMPLIFY_DEBUG_LOGGING
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

#define MESH_NUM_EDGES_TO_COLLAPSE 0

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

void MeshSimplify::declare_normals(const Span<Vector3> &p_normals) {
	input_data.normals = LocalVector<Vector3>(p_normals);
}

void MeshSimplify::declare_colors(const Span<Color> &p_colors) {
	input_data.colors = LocalVector<Color>(p_colors);
}

void MeshSimplify::declare_floats(const Span<float> &p_floats) {
	input_data.floats = LocalVector<float>(p_floats);
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

bool MeshSimplify::_can_collapse_test_tri(uint32_t kept, uint32_t deleted, uint32_t p_tri_id) const {
	const Tri &t = data.tris[p_tri_id];
	if (!t.active)
		return true;

	bool touches_deleted = false;
	Vector3i old_c[3];

	for (int i = 0; i < 3; ++i) {
		old_c[i] = data.verts[t.corn[i]].position;
		if (t.corn[i] == deleted)
			touches_deleted = true;
	}

	if (!touches_deleted)
		return true;

	if (t.corn[0] == kept || t.corn[1] == kept || t.corn[2] == kept)
		return true;

	Vector3i new_c[3];
	for (int i = 0; i < 3; ++i) {
		new_c[i] = (t.corn[i] == deleted) ? data.verts[kept].position : old_c[i];
	}

	if (_is_triangle_degenerate_from_positions(new_c)) {
		MS_LOG("_can_collapse rejecting edge from " + itos(deleted) + " to " + itos(kept) + " - _is_triangle_degenerate_from_positions");
		return false;
	}

	// Strong normal protection
	Vector3_64 v1b = Vector3_64(old_c[1] - old_c[0]);
	Vector3_64 v2b = Vector3_64(old_c[2] - old_c[0]);
	Vector3_64 before = v1b.cross(v2b);

	Vector3_64 v1a = Vector3_64(new_c[1] - new_c[0]);
	Vector3_64 v2a = Vector3_64(new_c[2] - new_c[0]);
	Vector3_64 after = v1a.cross(v2a);

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

	return true;
}

bool MeshSimplify::_can_collapse(uint32_t kept, uint32_t deleted) const {
	const Vert &vert_to_delete = data.verts[deleted];
	if (!vert_to_delete.active) {
		return false;
	}

	// Use collapsable matrix.
	const Vert &vert_to_keep = data.verts[kept];

	if (!Vert::can_collapse[(uint32_t)vert_to_delete.type][(uint32_t)vert_to_keep.type]) {
		return false;
	}

	// Only check triangles that touch the deleted vertex.
	const LocalVector<uint32_t> &tri_list = vert_to_delete.tris;

	// Disallow seams for now.
#if 0
	if (data.verts[deleted].is_seam_or_boundary) {
		return false;
	}
#endif

#define CAN_COLLAPSE_OPTIMIZED
#ifdef CAN_COLLAPSE_OPTIMIZED
	for (uint32_t n = 0; n < tri_list.size(); n++) {
		if (!_can_collapse_test_tri(kept, deleted, tri_list[n])) {
			return false;
		}
	}
#else
	// Reference.
	for (uint32_t n = 0; n < data.tris.size(); n++) {
		if (!_can_collapse_test_tri(kept, deleted, n)) {
			return false;
		}
	}
#endif

	return true;
}

void MeshSimplify::_validate_and_rebuild() {
#if 0
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
	print_line("Validation: active tris = " + itos(active_count));
#endif
}

void MeshSimplify::_debug_log_input_data() {
#ifdef MESH_SIMPLIFY_DEBUG_LOGGING
	Span<Vector3> in_verts = Span<Vector3>(input_data.positions);
	Span<Vector2> in_uvs = Span<Vector2>(input_data.uvs);

	MS_LOG("IN_VERTS\n");
	for (uint32_t n = 0; n < in_verts.size(); n++) {
		String sz = itos(n) + " : " + String(Variant(in_verts[n]));
		if (in_uvs.size())
			sz += ",\tuv " + String(Variant(in_uvs[n]));
		MS_LOG(sz);
	}
#endif
}

bool MeshSimplify::prepare(MeshDeduplicator &r_dd) {
	// Can't simplify when no indices.
	if (!input_data.indices.size()) {
		return false;
	}

	// Duduplicate.

	LocalVector<uint32_t> inds;
	LocalVector<Vector3> verts;
	LocalVector<Vector3> normals;
	LocalVector<Vector2> uvs;
	LocalVector<Vector2> uv2s;
	LocalVector<float> floats;

	uint32_t num_streams = 1;
	if (input_data.uvs.size()) {
		num_streams++;
	}
	if (input_data.uv2s.size()) {
		num_streams++;
	}
	if (input_data.normals.size()) {
		num_streams++;
	}
	if (input_data.colors.size()) {
		num_streams++;
	}
	if (input_data.floats.size()) {
		num_streams++;
	}

	r_dd.set_num_attribute_streams(num_streams);

	uint32_t fill_stream = 0;

	MeshAttributeStream &as_pos = r_dd.get_input_attribute_stream(fill_stream++);
	//as_pos.set_type(MeshAttributeStream::ATTR_POSITION, 0.1f);
	as_pos.set_type(MeshAttributeStream::ATTR_POSITION, 1.0f);
	as_pos.vec3 = input_data.positions;

	if (input_data.uvs.size()) {
		MeshAttributeStream &as = r_dd.get_input_attribute_stream(fill_stream++);
		as.set_type(MeshAttributeStream::ATTR_UV);
		as.vec2 = input_data.uvs;
	}

	if (input_data.uv2s.size()) {
		MeshAttributeStream &as = r_dd.get_input_attribute_stream(fill_stream++);
		as.set_type(MeshAttributeStream::ATTR_UV);
		as.vec2 = input_data.uv2s;
	}

	if (input_data.normals.size()) {
		MeshAttributeStream &as = r_dd.get_input_attribute_stream(fill_stream++);
		as.set_type(MeshAttributeStream::ATTR_NORMAL);
		as.vec3 = input_data.normals;
	}

	if (input_data.colors.size()) {
		MeshAttributeStream &as = r_dd.get_input_attribute_stream(fill_stream++);
		as.set_type(MeshAttributeStream::ATTR_COLOR);
		as.color = input_data.colors;
	}

	if (input_data.floats.size()) {
		MeshAttributeStream &as = r_dd.get_input_attribute_stream(fill_stream++);
		as.set_type(MeshAttributeStream::ATTR_FLOAT);
		as.float_input = input_data.floats;
	}

	if (!r_dd.process(input_data.indices, inds)) {
		return false;
	}
	verts = r_dd.get_output_attribute_stream(0).vec3;

	// Save the deduplicated data.
	input_data.indices = inds;
	input_data.positions = verts;

	uint32_t uv_stream = 1;
	if (input_data.uvs.size()) {
		const MeshAttributeStream &as = r_dd.get_output_attribute_stream(uv_stream++);
		DEV_ASSERT(as.get_type() == MeshAttributeStream::ATTR_UV);
		input_data.uvs = as.vec2;
	}
	if (input_data.uv2s.size()) {
		const MeshAttributeStream &as = r_dd.get_output_attribute_stream(uv_stream++);
		DEV_ASSERT(as.get_type() == MeshAttributeStream::ATTR_UV);
		input_data.uv2s = as.vec2;
	}
	if (input_data.normals.size()) {
		const MeshAttributeStream &as = r_dd.get_output_attribute_stream(uv_stream++);
		DEV_ASSERT(as.get_type() == MeshAttributeStream::ATTR_NORMAL);
		input_data.normals = as.vec3;
	}
	if (input_data.colors.size()) {
		const MeshAttributeStream &as = r_dd.get_output_attribute_stream(uv_stream++);
		DEV_ASSERT(as.get_type() == MeshAttributeStream::ATTR_COLOR);
		input_data.colors = as.color;
	}
	if (input_data.floats.size()) {
		const MeshAttributeStream &as = r_dd.get_output_attribute_stream(uv_stream++);
		DEV_ASSERT(as.get_type() == MeshAttributeStream::ATTR_FLOAT);
		input_data.floats = as.float_input;
	}

	// Basic requirements to do anything, is we need indices and positions.
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

	return true;
}

void MeshSimplify::_delete_triangle(uint32_t p_tri_id) {
	_debug_sanity_check();

	Tri &t = data.tris[p_tri_id];

	// Check the logic we aren't deleting an already deleted.
	DEV_ASSERT(t.active);
	t.active = false;

	// Reduce the triangle count of any edges references by this triangle, as we are deleting it.
	for (uint32_t n = 0; n < 3; n++) {
		data.edges[t.edge_ids[n]].triangle_count--;
	}

	// Remove any links to this triangle on the vertices.
	for (uint32_t n = 0; n < 3; n++) {
		LocalVector<uint32_t> &list = data.verts[t.corn[n]].tris;
		bool result = list.erase_unordered(p_tri_id);
#ifdef DEV_ENABLED
		if (!result) {
			print_line("ERROR: tri " + itos(p_tri_id) + " missing from vertex list");
			for (uint32_t i = 0; i < list.size(); i++) {
				print_line("\t" + itos(list[i]));
			}

			DEV_ASSERT(result);
		}
#endif
	}

	_debug_sanity_check();
}

bool MeshSimplify::simplify_mesh() {
	MeshDeduplicator dd;

	uint64_t time_before = OS::get_singleton()->get_ticks_msec();

	if (!prepare(dd)) {
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

	uint32_t target = MESH_SIMPLIFY_FACTOR(before_triangle_count);

	uint32_t edges_to_collapse = UINT32_MAX;
	if (MESH_NUM_EDGES_TO_COLLAPSE != 0) {
		edges_to_collapse = MESH_NUM_EDGES_TO_COLLAPSE;
		target = 1;
	}

	//	const uint32_t REFRESH_EVERY = 1; // Tune: 16-64 is usually fine
	//	uint32_t collapse_counter = 0;

	LocalVector<uint32_t> altered_edges;
	LocalVector<uint32_t> touched_edges; // Superset of altered_edges: every edge whose
										 // triangle_count changed this collapse, whether or not its (a,b) ids changed,
										 // plus every edge now incident to `kept` (see below for why).
	LocalVector<uint32_t> affected_tris;

	while ((current_triangle_count > target && !queue.empty()) && current_triangle_count > 1) {
		break;
		//		if ((++collapse_counter % REFRESH_EVERY) == 0) {
		{
			//_detect_seam_edges();
			//_rebuild_triangle_edge_ids();
			//_build_vertex_triangle_links();
		}
		_debug_sanity_check();

		SortedEdge se = queue.top();
		queue.pop();

		if (se.edge_id >= data.edges.size())
			continue;

		Edge &edge = data.edges[se.edge_id];

		if (!edge.active || se.version != edge.version || se.cost != edge.cost)
			continue;
		if (!data.verts[edge.a].active || !data.verts[edge.b].active)
			continue;

		uint32_t kept = edge.vertex_to_collapse_to;
		uint32_t deleted = (kept == edge.a ? edge.b : edge.a);

		// Collapse to one of the original endpoints only
		Vert &kept_vert = data.verts[kept];
		Vert &deleted_vert = data.verts[deleted];

		// Disallow collapsing edges for now.
#ifdef MESH_SIMPLIFY_DISALLOW_SEAMS
		if (deleted_vert.is_seam_or_boundary) {
			edge.active = false;
			continue;
		}
#endif

		// Strong safety check - this is the main fix for bad visuals on the shark.
		//
		// If this fails, it means something relevant changed since this edge was
		// last evaluated without its version being bumped (e.g. a vertex a few
		// hops away got reclassified, which can change what _can_collapse allows
		// here without this specific edge being in that collapse's touched_edges
		// set). Rather than permanently deactivating the edge -- which would
		// throw away an edge that might be perfectly collapsible once its cost
		// and direction are recomputed against current topology -- re-evaluate it
		// and put it back in the queue. _evaluate_edge_collapse() already
		// deactivates the edge itself if both directions are genuinely
		// uncollapsable, so this can't loop forever.
		if (!_can_collapse(kept, deleted)) {
			_evaluate_edge_collapse(se.edge_id);
			edge.version++;
			queue.push(SortedEdge(se.edge_id, edge.cost, edge.version));
			continue;
		}

#ifdef MESH_SIMPLIFY_DEBUG_LOGGING
		String sz = "collapsing edge " + itos(se.edge_id) + " : " + itos(edge.get_collapse_from()) + " to " + itos(edge.vertex_to_collapse_to);
		if (edge.is_seam_or_boundary) {
			sz += "\tSEAM";
		}
		sz += " cost : " + itos(edge.get_readable_cost());
		MS_LOG(sz);
#endif

#ifdef DEV_ENABLED
		// Evaluate once more to allow us to debug through (don't do on release, this is just to see what is happening).
		// _evaluate_edge_collapse(se.edge_id);
#endif

		_debug_sanity_check();

		kept_vert.Q = kept_vert.Q + deleted_vert.Q;
		deleted_vert.active = false;
		edge.active = false;

		altered_edges.clear();
		touched_edges.clear();

		// Make a copy here, as the tris may be deleted from the original LocalVector,
		// and we could get sync bugs.
		affected_tris = data.verts[deleted].tris;

		// We only need to consider the triangles that touch the vertex to delete.
		// We don't need to iterate all tris for a single collapse.
		for (uint32_t n = 0; n < affected_tris.size(); n++) {
			uint32_t tri_id = affected_tris[n];
			Tri &t = data.tris[tri_id];
			if (!t.active)
				continue;

			// First pass: update corners
			bool modified = false;
			int kept_count = 0;

			uint32_t new_corn[3];
			new_corn[0] = t.corn[0];
			new_corn[1] = t.corn[1];
			new_corn[2] = t.corn[2];

			for (int i = 0; i < 3; i++) {
				if (new_corn[i] == deleted) {
					new_corn[i] = kept;
					modified = true;
				}
				if (new_corn[i] == kept)
					kept_count++;
			}

			if (modified) {
				if (kept_count > 1 || _is_triangle_degenerate(new_corn)) {
					// BUG FIX: this triangle is being discarded outright (it's the
					// "flap" straddling the collapsed edge), not reassigned -- but its
					// other two edges (kept<->c and deleted<->c) still lose a triangle
					// user here. They were previously never added to altered_edges, so
					// if this was their last remaining triangle they'd end up with
					// triangle_count == 0 while staying "active" forever. Register them
					// so the refresh pass below can retire/reclassify them correctly.
					for (uint32_t i = 0; i < 3; i++) {
						touched_edges.push_back_if_not_present(t.edge_ids[i]);
					}
					_delete_triangle(tri_id);
					current_triangle_count--;
					continue; // corners/edges of this tri don't need reassigning
				}

				// If not deleted, we can assign the new corners.
				for (int i = 0; i < 3; i++) {
					if (t.corn[i] != new_corn[i]) {
						// Remove tri from old verex.
						data.verts[t.corn[i]].tris.erase_unordered(tri_id);

						// Check the triangle not on the new vertex already...
						DEV_ASSERT(data.verts[new_corn[i]].tris.find(tri_id) == -1);

						// Add tri to new vertex list.
						data.verts[new_corn[i]].tris.push_back(tri_id);
						t.corn[i] = new_corn[i];
					}
				}

				// Re-assign edges for this triangle (this is the key change)
				for (uint32_t i = 0; i < 3; i++) {
					uint32_t old_edge_id = t.edge_ids[i];
					uint32_t new_edge_id = _get_or_create_edge(t.corn[i], t.corn[(i + 1) % 3], tri_id, old_edge_id);
					if (new_edge_id != t.edge_ids[i]) {
						// Both the old edge id and the new edge id are changed,
						// and need to be re-evaluated later.
						altered_edges.push_back_if_not_present(old_edge_id);
						altered_edges.push_back_if_not_present(new_edge_id);
						t.edge_ids[i] = new_edge_id;

						// Reduce the triangle count on the old edge, increase tri count on the new edge.
						data.edges[old_edge_id].triangle_count--;
						data.edges[new_edge_id].triangle_count++;
					}
				}
			}
		}

		// BUG FIX: kept_vert.Q was merged with deleted_vert.Q above, which changes
		// the true cost of EVERY edge incident to `kept` -- not just the ones whose
		// corner ids literally changed this round (altered_edges). Previously, an
		// edge from kept to an untouched neighbour kept its pre-merge cached cost
		// indefinitely, letting the queue make decisions on stale data. Pull in
		// kept's full current incident-edge set so the refresh pass below catches
		// all of them.
		_get_edges_touching_vertex(kept, touched_edges);

		_debug_sanity_check();

		// Do NOT deactivate all edges of deleted vertex blindly.
		// Instead, update the ones that should survive (those connected to kept)
		for (uint32_t n = 0; n < altered_edges.size(); n++) {
			uint32_t e_idx = altered_edges[n];
			Edge &e = data.edges[e_idx];
			if (!e.active)
				continue;

			// If any edge has reached triangle count zero
			// (i.e. no more triangles reference it),
			// then delete.
			if (e.triangle_count == 0) {
				e.active = false;
				continue;
			}

			// Update edge to point to kept instead of deleted
			if (e.a == deleted) {
				e.a = kept;
				e.sort();
			}
			if (e.b == deleted) {
				e.b = kept;
				e.sort();
			}
			// Remove invalid edges.
			if (e.a == e.b) {
				e.active = false;
			}
		}

		_debug_sanity_check();

		// Refresh every edge touched by this collapse -- not just the ones whose
		// vertex ids structurally changed (altered_edges). See the two BUG FIX
		// comments above for why touched_edges is the superset we need here.
		for (uint32_t n = 0; n < touched_edges.size(); n++) {
			uint32_t e_idx = touched_edges[n];

			// Derive is_seam_or_boundary from the now-correct triangle_count, and
			// retire the edge if it no longer touches any triangle at all.
			_refresh_edge_seam_flag(e_idx);

			Edge &e = data.edges[e_idx];
			if (!e.active)
				continue;

			// Vertex classification (MANIFOLD/BORDER/SEAM/COMPLEX/LOCKED) depends on
			// the seam flags just refreshed above, so reclassify after, not before.
			_reclassify_vertex(e.a);
			_reclassify_vertex(e.b);

			_evaluate_edge_collapse(e_idx);
			e.version++;
			queue.push(SortedEdge(e_idx, e.cost, e.version));
		}

		// Ideally we should rebuild these incrementally, but doing the whole lot at each collapse
		// is good for reference.
		//_build_vertex_triangle_links();

		_debug_sanity_check();

		// ... after triangle update and adjacency rebuild ...
		//_validate_and_rebuild();

#ifdef MESH_SIMPLIFY_ONE_AT_A_TIME
		break;
#endif

		edges_to_collapse--;
		if (edges_to_collapse <= 0) {
			break;
		}
	}

	// Final cleanup of any new degenerates
	for (uint32_t n = 0; n < data.tris.size(); n++) {
		Tri &t = data.tris[n];
		if (t.active && _is_triangle_degenerate(t.corn)) {
			_delete_triangle(n);
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

#if 0
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

	uint64_t time_after = OS::get_singleton()->get_ticks_msec();

	// Debug
	print_line("simplify before_triangle_count: " + itos(before_triangle_count) + ", after_triangle_count: " + itos(current_triangle_count));

	print_line("\nTook " + itos(time_after - time_before) + " milliseconds.");
	print_line(itos(data.edges.size()) + " max edges.");
	print_line(itos(data.tris.size()) + " max tris.");

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

uint32_t MeshSimplify::_get_or_create_edge(uint32_t p_corn_a, uint32_t p_corn_b, uint32_t p_triangle_id, uint32_t p_first_check_edge) {
	Edge e;
	e.a = p_corn_a;
	e.b = p_corn_b;
	e.sort();

	// If there is an existing edge, check this first.
	if (p_first_check_edge != UINT32_MAX) {
		const Edge &edge = data.edges[p_first_check_edge];
		if ((edge == e) && edge.active) {
			return p_first_check_edge;
		}
	}

	for (uint32_t n = 0; n < data.edges.size(); n++) {
		const Edge &edge = data.edges[n];
		if ((edge == e) && edge.active) {
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

void MeshSimplify::_test_quadrics() {
	print_line("=== QUADRIC UNIT TEST ===");

	// Create a simple flat triangle
	Vector3_64 p0(0, 0, 0);
	Vector3_64 p1(1, 0, 0);
	Vector3_64 p2(0, 1, 0);

	Vector3_64 p3(0, 0, 1);
	Vector3_64 p4(1, 0, 1);
	Vector3_64 p5(0, 1, 1);

	// Test two orientations
	Plane_64 plane1(p0, p1, p2);
	Plane_64 plane2(p0, p2, p1); // reversed winding
	Plane_64 plane3(p3, p4, p5);

	print_line("Distance from p3 to plane3: " + rtos(plane3.normal.dot(p3) + plane3.d));
	print_line("Distance from p4 to plane3: " + rtos(plane3.normal.dot(p4) + plane3.d));
	print_line("Distance from p5 to plane3: " + rtos(plane3.normal.dot(p5) + plane3.d));

	print_line("Plane1: " + String(plane1.normal) + " d=" + rtos(plane1.d));
	print_line("Plane2: " + String(plane2.normal) + " d=" + rtos(plane2.d));
	print_line("Plane3: " + String(plane3.normal) + " d=" + rtos(plane3.d));

	// Build Kp for both
	Quadric Kp1(plane1);
	Quadric Kp2(plane2);
	Quadric Kp3(plane3);

	// Evaluate at a point on the plane
	Vector3i test_pos(0, 0, 0);
	Vector3i test_pos3(0, 0, 1); // on plane3

	print_line("d = " + rtos(plane3.d) + ", Kp3[3][3] = " + rtos(Kp3.m[3][3]));
	Vector3_64 test_p(test_pos3.x, test_pos3.y, test_pos3.z);
	print_line("test point dot normal + d = " + rtos(plane3.normal.dot(test_p) + plane3.d));

	double error1 = _compute_quadric_error(test_pos, Kp1);
	double error2 = _compute_quadric_error(test_pos, Kp2);
	double error3 = _compute_quadric_error(test_pos3, Kp3);

	print_line("Error at point for plane1: " + rtos(error1));
	print_line("Error at point for plane2: " + rtos(error2));
	print_line("Error at point for plane3: " + rtos(error3));

	// The errors should be near zero for points on the plane
	if (Math::abs(error1) > 1e-5 || Math::abs(error2) > 1e-5 || Math::abs(error3) > 1e-5) {
		print_line("WARNING: Quadric error not zero on plane!");
	}
}

void MeshSimplify::_test_attribute_quadrics() {
	print_line("=== ATTRIBUTE QUADRIC UNIT TEST (Gradient) ===");

	Vector3_64 p0(0, 0, 0);
	Vector3_64 p1(1, 0, 0);
	Vector3_64 p2(0, 1, 0);

	// UVs chosen so u = x + 5 exactly
	double u0 = 5.0, u1 = 6.0, u2 = 5.0;

	Vector3_64 edge1 = p1 - p0;
	Vector3_64 edge2 = p2 - p0;
	Vector3_64 cross = edge1.cross(edge2);
	double normal_length = cross.length();
	Vector3_64 normal = cross / normal_length;

	Vector4_64 gradient = _solve_attribute_gradient(p0, p1, p2, normal, u0, u1, u2);

	print_line("Solved gradient = [" + rtos(gradient.x) + ", " + rtos(gradient.y) + ", " + rtos(gradient.z) + "], c = " + rtos(gradient.w));

	Vector3i test_pos(1, 0, 0); // should have UV = 6.0

	Quadric Q_correct = _build_attribute_quadric(gradient, 6.0);
	Quadric Q_wrong = _build_attribute_quadric(gradient, 60.0);

	double err_correct = _compute_quadric_error(test_pos, Q_correct);
	double err_wrong = _compute_quadric_error(test_pos, Q_wrong);

	print_line("Error with CORRECT target (6.0): " + rtos(err_correct));
	print_line("Error with WRONG target (60.0):   " + rtos(err_wrong));

	if (Math::abs(err_correct) < 1e-5) {
		print_line("SUCCESS: Attribute error ~0 for matching target.");
	} else {
		print_line("WARNING: Attribute error NOT zero!");
	}
}

MeshSimplify::Quadric::Quadric(const Plane_64 &p_plane) {
	// 1. STANDARD POSITION GEOMETRY QUADRIC
	// Beware - the standard Garland Quadric assumes planes pass through the origin,
	// whereas ours pass through the triangle.
	// We need  to account for this and use non-standard math here.

	// Kp for plane ax + by + cz + d = 0
	double a = p_plane.normal.x;
	double b = p_plane.normal.y;
	double c = p_plane.normal.z;
	double d = -p_plane.d; // FLIP THE SIGN, Garland expects plane in reverse polarity to Godot standard.

	m[0][0] = a * a;
	m[0][1] = a * b;
	m[0][2] = a * c;
	m[0][3] = a * d;

	m[1][0] = a * b;
	m[1][1] = b * b;
	m[1][2] = b * c;
	m[1][3] = b * d;

	m[2][0] = a * c;
	m[2][1] = b * c;
	m[2][2] = c * c;
	m[2][3] = c * d;

	m[3][0] = a * d;
	m[3][1] = b * d;
	m[3][2] = c * d;
	m[3][3] = d * d;
}

// General outer-product quadric for an arbitrary 4-vector (a, b, c, d),
// representing the squared-error functional (a*x + b*y + c*z + d)^2.
// This is the same math as Quadric(const Plane_64&) above, just without
// being restricted to vectors that come from a position plane -- it lets
// us build quadrics for other linear error functions (attribute error)
// that sum across triangles the same correct way position quadrics do.
MeshSimplify::Quadric::Quadric(const Vector4_64 &p_v) {
	double a = p_v.x;
	double b = p_v.y;
	double c = p_v.z;
	double d = p_v.w;

	m[0][0] = a * a;
	m[0][1] = a * b;
	m[0][2] = a * c;
	m[0][3] = a * d;

	m[1][0] = a * b;
	m[1][1] = b * b;
	m[1][2] = b * c;
	m[1][3] = b * d;

	m[2][0] = a * c;
	m[2][1] = b * c;
	m[2][2] = c * c;
	m[2][3] = c * d;

	m[3][0] = a * d;
	m[3][1] = b * d;
	m[3][2] = c * d;
	m[3][3] = d * d;
}

Vector4_64 MeshSimplify::_solve_attribute_gradient(const Vector3_64 &p0, const Vector3_64 &p1, const Vector3_64 &p2,
		const Vector3_64 &normal, double u0, double u1, double u2) {
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
	if (Math::abs(main_det) < 1e-7)
		return Vector4_64();

	double targets[4] = { u0, u1, u2, 0 };
	double coeff[4] = { 0 };

	for (int col = 0; col < 4; ++col) {
		double temp[4][4];
		for (int r = 0; r < 4; ++r)
			for (int c = 0; c < 4; ++c)
				temp[r][c] = base_matrix[r][c];
		for (int row = 0; row < 4; ++row)
			temp[row][col] = targets[row];
		coeff[col] = det_4x4(temp) / main_det;
	}

	return Vector4_64(coeff[0], coeff[1], coeff[2], coeff[3]);
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

#if 0
		// In _triangle_calculate_plane or when building Kp:
		double area = 0.5 * (p1.pos() - p0.pos()).cross(p2.pos() - p0.pos()).length(); // or use unnormalized for speed
		// Then scale Kp by area before adding.
#endif

		double area = 0.5 * normal_length;

		// 1. STANDARD POSITION GEOMETRY QUADRIC
		Quadric Kp(t.plane);

		// Apply area weighting to position quadrics.
		Kp = Kp * area;

		// Step B: Accumulate this plane's matrix into its three corner vertices
		for (uint32_t i = 0; i < 3; i++) {
			Vert &v = data.verts[t.corn[i]];
			v.Q = v.Q + Kp;

			// === DIAGNOSTIC ===
#if 0
			double self_error = _compute_quadric_error(v.position, v.Q);
			print_line("Vert " + itos(t.corn[i]) + " self-error after tri " + itos(n) + ": " + rtos(self_error));
#endif
		}

		// ATTRIBUTE QUADRIC
		// Fit a single linear UV function per triangle (gu/gv, exact at all 3
		// corners), then fold it into each corner's OWN attribute quadric using
		// that corner's own recorded UV as the target. Each corner's quadric is
		// exactly zero when evaluated at that corner's own position (since the
		// gradient reproduces its own UV exactly by construction), and summing
		// quadrics across triangles before evaluating -- rather than summing
		// gradient vectors and evaluating once -- keeps that property true
		// regardless of how many triangles touch the vertex.
		if (input_data.uvs.size()) {
			Vector3_64 normal = cross / normal_length;

			Vector4_64 gu = _solve_attribute_gradient(p0, p1, p2, normal, v0.uv.x, v1.uv.x, v2.uv.x);
			Vector4_64 gv = _solve_attribute_gradient(p0, p1, p2, normal, v0.uv.y, v1.uv.y, v2.uv.y);

			for (uint32_t i = 0; i < 3; i++) {
				Vert &v = data.verts[t.corn[i]];
				Quadric Qu_face = _build_attribute_quadric(gu, v.uv.x);
				Quadric Qv_face = _build_attribute_quadric(gv, v.uv.y);
				v.Qu = v.Qu + (Qu_face * area);
				v.Qv = v.Qv + (Qv_face * area);
			}
		}
	}

#if 0
	_test_quadrics();
	_test_attribute_quadrics();
#endif
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

// Builds one triangle's contribution to a vertex's attribute quadric.
//
// p_gradient is the linear function (fit exactly through this triangle's 3
// corners) that predicts the attribute value from position: predicted(p) =
// gradient . (p.x, p.y, p.z, 1). p_target is the attribute value we actually
// want preserved at evaluation time (the vertex's own recorded UV).
//
// The squared error (gradient . p - p_target)^2 can be written as
// ((gx, gy, gz, gw - p_target) . (x, y, z, 1))^2 -- exactly the same
// "(a*x+b*y+c*z+d)^2" form as a position-plane quadric. That means we can
// build it as an outer-product Quadric and accumulate/evaluate it with the
// exact same machinery (operator+, _compute_quadric_error) as the geometry
// quadric Q. Crucially, summing quadrics from multiple triangles *before*
// evaluating gives the correct sum of per-triangle squared errors, unlike
// summing the raw gradient vectors first and squaring once (which is what
// this code used to do, and which scaled up incorrectly with vertex valence).
MeshSimplify::Quadric MeshSimplify::_build_attribute_quadric(const Vector4_64 &p_gradient, double p_target) const {
	return Quadric(Vector4_64(p_gradient.x, p_gradient.y, p_gradient.z, p_gradient.w - p_target));
}

void MeshSimplify::_evaluate_edge_collapse(uint32_t p_edge_id) {
	Edge &edge = data.edges[p_edge_id];
	const Vert &a = data.verts[edge.a];
	const Vert &b = data.verts[edge.b];

	Quadric Q_new = a.Q + b.Q;
	//Quadric Qu_new = a.Qu + b.Qu;
	//Quadric Qv_new = a.Qv + b.Qv;

	// User defined weighting balancing factor. Tune this to your preference.
	// 1.0 to 10.0 handles texture preservation nicely without stalling geometry changes.
	const double beta = 150;

	// May need weighting relative to the rest.
	double distance_cost = (a.pos() - b.pos()).length();

	const double VERY_HIGH_COST = 1e30;
	double total_a = VERY_HIGH_COST;
	double total_b = VERY_HIGH_COST;

	// Test both directions.
	bool can_collapse_to_a = _can_collapse(edge.a, edge.b); // keep a, delete b
	bool can_collapse_to_b = _can_collapse(edge.b, edge.a); // keep b, delete a

	// Don't even attempt this edge.
	if (!can_collapse_to_b && !can_collapse_to_a) {
		edge.active = false;
		edge.cost = VERY_HIGH_COST;
		return;
	}

	if (can_collapse_to_a) {
		double geom_cost_a = _compute_quadric_error(a.position, Q_new);

		// Only compute if UVs exist
		double attr_a = 0;
		if (input_data.uvs.size()) { // were these declared?
			// Combine both endpoints' attribute quadrics before evaluating, the
			// same way Q_new combines both endpoints' geometry quadrics -- this
			// way the cost of collapsing to 'a' also accounts for how much 'b's
			// neighbourhood would stretch by adopting a's position, not just a's
			// own pre-existing error.
			Quadric Qu_new = a.Qu + b.Qu;
			Quadric Qv_new = a.Qv + b.Qv;
			attr_a = _compute_quadric_error(a.position, Qu_new) + _compute_quadric_error(a.position, Qv_new);
		}
		total_a = distance_cost + geom_cost_a + beta * attr_a;

		// Add penalty for collapsing from a seam.
		if (b.is_seam_or_boundary) {
			bool breaks_line = true;

			if (b.seam_neighbour_verts.size() == 2) {
				for (uint32_t n = 0; n < b.seam_neighbour_verts.size(); n++) {
					if (b.seam_neighbour_verts[n] == edge.a) {
						breaks_line = false; // direct neighbour on scene line
						break;
					}
				}
			}

			if (breaks_line) {
				total_a *= 60;
				total_a = MAX(total_a, 60.0);
			} else {
				// Normalized deviation from straight line
				DEV_ASSERT(b.seam_neighbour_verts.size() == 2);
				const Vector3i &v_prev = data.verts[b.seam_neighbour_verts[0]].position;
				const Vector3i &v_next = data.verts[b.seam_neighbour_verts[1]].position;

				// Use normalized vectors to make it scale-invariant
				Vector3_64 dir1 = Vector3_64(b.position - v_prev).normalized();
				Vector3_64 dir2 = Vector3_64(v_next - b.position).normalized();
				double angle_cos = dir1.dot(dir2);

				// Penalty based on how much it deviates from 180 degrees (straight line)
				double deviation = 1.0 - angle_cos; // 0 = perfectly straight, 2 = 180 degree turn

				total_a += deviation * 30.0; // tune this constant
			}
		}
	}

	if (can_collapse_to_b) {
		double geom_cost_b = _compute_quadric_error(b.position, Q_new);

		// Only compute if UVs exist
		double attr_b = 0;
		if (input_data.uvs.size()) { // were these declared?
			Quadric Qu_new = a.Qu + b.Qu;
			Quadric Qv_new = a.Qv + b.Qv;
			attr_b = _compute_quadric_error(b.position, Qu_new) + _compute_quadric_error(b.position, Qv_new);
		}

		total_b = distance_cost + geom_cost_b + beta * attr_b;

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
			}

			if (breaks_line) {
				total_b *= 60;
				total_b = MAX(total_b, 60.0);
			} else {
				// Normalized deviation from straight line
				DEV_ASSERT(a.seam_neighbour_verts.size() == 2);
				const Vector3i &v_prev = data.verts[a.seam_neighbour_verts[0]].position;
				const Vector3i &v_next = data.verts[a.seam_neighbour_verts[1]].position;

				// Use normalized vectors to make it scale-invariant
				Vector3_64 dir1 = Vector3_64(a.position - v_prev).normalized();
				Vector3_64 dir2 = Vector3_64(v_next - a.position).normalized();
				double angle_cos = dir1.dot(dir2);

				// Penalty based on how much it deviates from 180 degrees (straight line)
				double deviation = 1.0 - angle_cos; // 0 = perfectly straight, 2 = 180 degree turn

				total_b += deviation * 30.0; // tune this constant
			}
		}
	}

#ifdef MESH_SIMPLIFY_DEBUG_LOGGING
	double from_cost = 0;
#endif
	if (total_a < total_b) {
		edge.vertex_to_collapse_to = edge.a;
		edge.cost = total_a;
#ifdef MESH_SIMPLIFY_DEBUG_LOGGING
		from_cost = total_b;
#endif
	} else {
		edge.vertex_to_collapse_to = edge.b;
		edge.cost = total_b;
#ifdef MESH_SIMPLIFY_DEBUG_LOGGING
		from_cost = total_a;
#endif
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
	print_line("\tevaluating edge " + itos(edge.get_collapse_from()) + " to " + itos(edge.vertex_to_collapse_to) + " ... cost " + itos(edge.get_readable_cost()) + " .. from cost " + itos(edge.translate_readable_cost(from_cost)));
#endif
}

void MeshSimplify::_update_edge_seam_status(uint32_t p_edge_id) {
	if (p_edge_id >= data.edges.size())
		return;
	Edge &e = data.edges[p_edge_id];
	if (!e.active) {
		e.is_seam_or_boundary = false;
		e.triangle_count = 0;
		return;
	}

	// Re-count active triangles using this edge (simple but correct)
	uint32_t count = 0;
	for (uint32_t t = 0; t < data.tris.size(); ++t) {
		const Tri &tri = data.tris[t];
		if (!tri.active)
			continue;
		if (tri.edge_ids[0] == p_edge_id ||
				tri.edge_ids[1] == p_edge_id ||
				tri.edge_ids[2] == p_edge_id) {
			count++;
		}
	}

	e.triangle_count = count;
	bool was_seam = e.is_seam_or_boundary;
	e.is_seam_or_boundary = (count <= 1);

	// Update vertex flags
	if (e.is_seam_or_boundary) {
		data.verts[e.a].is_seam_or_boundary = true;
		data.verts[e.b].is_seam_or_boundary = true;
	} else if (was_seam) {
		// Could clear vertex seam flag if no other seams touch it, but it's harmless to leave it set
	}
}

// Collects every currently-active edge incident to p_vert_id, by walking
// that vertex's (already-current) triangle list. Cheap and local -- O(valence),
// not O(mesh) -- so it's safe to call once or twice per collapse.
void MeshSimplify::_get_edges_touching_vertex(uint32_t p_vert_id, LocalVector<uint32_t> &r_edges) const {
	const Vert &v = data.verts[p_vert_id];
	if (!v.active) {
		return;
	}

	for (uint32_t n = 0; n < v.tris.size(); n++) {
		const Tri &t = data.tris[v.tris[n]];
		if (!t.active) {
			continue;
		}
		for (uint32_t i = 0; i < 3; i++) {
			uint32_t eid = t.edge_ids[i];
			const Edge &e = data.edges[eid];
			if (e.a == p_vert_id || e.b == p_vert_id) {
				r_edges.push_back_if_not_present(eid);
			}
		}
	}
}

// Cheap sibling of _update_edge_seam_status(): that function is correct but
// recounts triangles for the edge by scanning the ENTIRE triangle array, which
// is too expensive to call once or twice per collapse. Here, e.triangle_count
// is already being kept accurate incrementally by the caller (increment/decrement
// on triangle add/remove) -- we just need to derive is_seam_or_boundary from it,
// and retire the edge if it no longer touches any triangle at all.
void MeshSimplify::_refresh_edge_seam_flag(uint32_t p_edge_id) {
	Edge &e = data.edges[p_edge_id];
	if (!e.active) {
		return;
	}

	// BUG FIX: previously an edge could reach triangle_count == 0 (e.g. its last
	// user was a "flap" triangle removed via the kept_count > 1 / degenerate path
	// in simplify_mesh(), which never touched this edge) and simply stay active
	// forever with stale a/b -- retire it here instead.
	if (e.triangle_count == 0) {
		e.active = false;
		e.is_seam_or_boundary = false;
		return;
	}

	// BUG FIX: previously this flag was only ever set once, in _detect_seam_edges(),
	// before the collapse loop started, and never updated again -- so an edge that
	// transitioned from interior (2 tris) to boundary (1 tri) mid-simplification
	// kept reporting itself as interior for the rest of the run.
	e.is_seam_or_boundary = (e.triangle_count == 1) || (e.triangle_count > 2);

	if (e.is_seam_or_boundary) {
		data.verts[e.a].is_seam_or_boundary = true;
		data.verts[e.b].is_seam_or_boundary = true;
	}
}

// Local, incremental version of the vertex-classification tail of
// _detect_seam_edges() (MANIFOLD / BORDER / SEAM / COMPLEX / LOCKED + the
// seam "twin" pairing), scoped to a single vertex using its CURRENT incident
// edges instead of rescanning the whole mesh.
//
// Caveat: the "has_twin" search below looks at seam_neighbour_verts of other
// members of this vertex's wedge, which is only accurate for members that have
// themselves been reclassified since their neighbourhood last changed. In
// practice this converges fine (any vertex whose neighbourhood changes gets
// reclassified via the edges it touches), but for extra safety on seam-heavy
// meshes consider periodically calling the full _detect_seam_edges() every
// few hundred collapses as a resync, in addition to this incremental version.
void MeshSimplify::_reclassify_vertex(uint32_t p_vert_id) {
	Vert &vert = data.verts[p_vert_id];
	if (!vert.active) {
		return;
	}

	LocalVector<uint32_t> touching_edges;
	_get_edges_touching_vertex(p_vert_id, touching_edges);

	uint32_t true_border_count = 0;
	uint32_t seam_pair_count = 0;
	uint32_t nonmanifold_count = 0;

	vert.seam_neighbour_verts.clear();
	vert.is_seam_or_boundary = false;

	for (uint32_t i = 0; i < touching_edges.size(); i++) {
		const Edge &e = data.edges[touching_edges[i]];
		if (!e.active) {
			continue;
		}

		if (e.triangle_count > 2) {
			nonmanifold_count++;
			continue;
		}
		if (e.triangle_count != 1) {
			continue; // interior edge, not a boundary from this vertex.
		}

		vert.is_seam_or_boundary = true;
		uint32_t other = (e.a == p_vert_id) ? e.b : e.a;
		vert.seam_neighbour_verts.push_back(other);

		// Look for another open edge connecting a different member of our wedge
		// to a different member of the neighbour's wedge -- that's the "other
		// side" of a seam (same test as in _detect_seam_edges()).
		uint32_t wedge_self = vert.wedge;
		uint32_t wedge_other = data.verts[other].wedge;
		bool has_twin = false;

		const Wedge &w_self = data.wedges[wedge_self];
		for (uint32_t x = 0; x < w_self.verts.size() && !has_twin; x++) {
			uint32_t v2 = w_self.verts[x];
			if (v2 == p_vert_id || !data.verts[v2].active) {
				continue;
			}
			const LocalVector<uint32_t> &nbrs = data.verts[v2].seam_neighbour_verts;
			for (uint32_t s = 0; s < nbrs.size(); s++) {
				if (data.verts[nbrs[s]].wedge == wedge_other) {
					has_twin = true;
					break;
				}
			}
		}

		if (has_twin) {
			seam_pair_count++;
		} else {
			true_border_count++;
		}
	}

	const Wedge &wedge = data.wedges[vert.wedge];

	if (nonmanifold_count > 0) {
		vert.type = Vert::Type::LOCKED;
	} else if (true_border_count > 0) {
		vert.type = (true_border_count == 2 && seam_pair_count == 0 && wedge.verts.size() == 1)
				? Vert::Type::BORDER
				: Vert::Type::COMPLEX;
	} else if (seam_pair_count > 0) {
		vert.type = (seam_pair_count == 2 && wedge.verts.size() == 2)
				? Vert::Type::SEAM
				: Vert::Type::COMPLEX;
	} else if (wedge.verts.size() > 1) {
		vert.type = Vert::Type::COMPLEX;
	} else {
		vert.type = Vert::Type::MANIFOLD;
	}
}

void MeshSimplify::_rebuild_triangle_edge_ids() {
	for (uint32_t t = 0; t < data.tris.size(); ++t) {
		Tri &tri = data.tris[t];
		if (!tri.active)
			continue;

		tri.edge_ids[0] = _get_or_create_edge(tri.corn[0], tri.corn[1], t);
		tri.edge_ids[1] = _get_or_create_edge(tri.corn[1], tri.corn[2], t);
		tri.edge_ids[2] = _get_or_create_edge(tri.corn[2], tri.corn[0], t);
	}
}

void MeshSimplify::_detect_seam_edges() {
	// Reset
	for (uint32_t i = 0; i < data.edges.size(); i++) {
		data.edges[i].is_seam_or_boundary = false;
		data.edges[i].triangle_count = 0;
	}

	for (uint32_t i = 0; i < data.verts.size(); i++) {
		data.verts[i].seam_neighbour_verts.clear();
		data.verts[i].type = Vert::Type::MANIFOLD; // default
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
#ifdef MESH_SIMPLIFY_DEBUG_LOGGING
	int seam_count = 0;
#endif
	for (uint32_t i = 0; i < data.edges.size(); i++) {
		Edge &e = data.edges[i];
		if (!e.active) {
			continue;
		}

		if (e.triangle_count <= 1) {
			e.is_seam_or_boundary = true;
#ifdef MESH_SIMPLIFY_DEBUG_LOGGING
			seam_count++;
#endif
			// Can 0 happen?
			DEV_ASSERT(e.triangle_count > 0);
		} else if (e.triangle_count > 2) {
			// Non-manifold edge - also protect
			e.is_seam_or_boundary = true;
#ifdef MESH_SIMPLIFY_DEBUG_LOGGING
			seam_count++;
#endif
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

#define MESH_SIMPLIFY_COUNT_VERT_TYPES
#ifdef MESH_SIMPLIFY_COUNT_VERT_TYPES
	uint32_t vertex_type_count[(uint32_t)Vert::Type::MAX] = {};
#endif

	// Requires: wedges built, seam_neighbour_verts populated (both already true
	// at this point in _detect_seam_edges).

	LocalVector<uint32_t> true_border_count;
	LocalVector<uint32_t> seam_pair_count;
	LocalVector<uint32_t> nonmanifold_count;
	true_border_count.resize(data.verts.size());
	seam_pair_count.resize(data.verts.size());
	nonmanifold_count.resize(data.verts.size());
	for (uint32_t n = 0; n < data.verts.size(); n++) {
		true_border_count[n] = 0;
		seam_pair_count[n] = 0;
		nonmanifold_count[n] = 0;
	}

	for (uint32_t i = 0; i < data.edges.size(); i++) {
		const Edge &e = data.edges[i];
		if (!e.active)
			continue;

		if (e.triangle_count > 2) {
			nonmanifold_count[e.a]++;
			nonmanifold_count[e.b]++;
			continue;
		}
		if (e.triangle_count != 1)
			continue; // interior edge

		uint32_t wedge_a = data.verts[e.a].wedge;
		uint32_t wedge_b = data.verts[e.b].wedge;
		bool has_twin = false;

		// Look for another open edge connecting a different member of a's wedge
		// to a different member of b's wedge -- that's the "other side" of a seam.
		const Wedge &wa = data.wedges[wedge_a];
		for (uint32_t x = 0; x < wa.verts.size() && !has_twin; x++) {
			uint32_t a2 = wa.verts[x];
			if (a2 == e.a)
				continue;
			const LocalVector<uint32_t> &nbrs = data.verts[a2].seam_neighbour_verts;
			for (uint32_t s = 0; s < nbrs.size(); s++) {
				if (data.verts[nbrs[s]].wedge == wedge_b) {
					has_twin = true;
					break;
				}
			}
		}

		if (has_twin) {
			seam_pair_count[e.a]++;
			seam_pair_count[e.b]++;
		} else {
			true_border_count[e.a]++;
			true_border_count[e.b]++;
		}
	}

	for (uint32_t n = 0; n < data.verts.size(); n++) {
		Vert &vert = data.verts[n];
		if (!vert.active) {
			continue;
		}
		const Wedge &wedge = data.wedges[vert.wedge];

		uint32_t tb = true_border_count[n];
		uint32_t sp = seam_pair_count[n];
		uint32_t nm = nonmanifold_count[n];

		if (nm > 0) {
			vert.type = Vert::Type::LOCKED;
		} else if (tb > 0) {
			vert.type = (tb == 2 && sp == 0 && wedge.verts.size() == 1)
					? Vert::Type::BORDER
					: Vert::Type::COMPLEX;
		} else if (sp > 0) {
			vert.type = (sp == 2 && wedge.verts.size() == 2)
					? Vert::Type::SEAM
					: Vert::Type::COMPLEX;
		} else if (wedge.verts.size() > 1) {
			vert.type = Vert::Type::COMPLEX; // shares a position but no open edges — rare, be conservative
		} else {
			vert.type = Vert::Type::MANIFOLD;
		}

#ifdef MESH_SIMPLIFY_COUNT_VERT_TYPES
		vertex_type_count[(uint32_t)vert.type] += 1;
#endif
	}

#ifdef MESH_SIMPLIFY_COUNT_VERT_TYPES
	print_line("manifold : " + itos(vertex_type_count[(uint32_t)Vert::Type::MANIFOLD]));
	print_line("border : " + itos(vertex_type_count[(uint32_t)Vert::Type::BORDER]));
	print_line("seam : " + itos(vertex_type_count[(uint32_t)Vert::Type::SEAM]));
	print_line("complex : " + itos(vertex_type_count[(uint32_t)Vert::Type::COMPLEX]));
	print_line("locked : " + itos(vertex_type_count[(uint32_t)Vert::Type::LOCKED]));

#endif

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

	print_line("Detected " + itos(seam_count) + " seam/boundary edges out of " + itos(data.edges.size()));
#endif
}

void MeshSimplify::_debug_sanity_check() {
	return;
	// This is SLOOWWWW .. only do in debug for testing.
#ifdef DEV_ENABLED
	LocalVector<LocalVector<uint32_t>> vert_tri_lists;
	vert_tri_lists.resize(data.verts.size());

	for (uint32_t t = 0; t < data.tris.size(); ++t) {
		const Tri &tri = data.tris[t];
		if (!tri.active)
			continue;

		for (int i = 0; i < 3; ++i) {
			uint32_t vert_id = tri.corn[i];
			DEV_ASSERT(vert_id < data.verts.size());
			vert_tri_lists[vert_id].push_back(t);
		}
	}

	// Now check...
	for (uint32_t n = 0; n < data.verts.size(); n++) {
		if (!data.verts[n].active) {
			continue;
		}

		const LocalVector<uint32_t> &true_list = vert_tri_lists[n];
		const LocalVector<uint32_t> &check_list = data.verts[n].tris;

		DEV_ASSERT(true_list.size() == check_list.size());

		// Check each...
		for (uint32_t i = 0; i < true_list.size(); i++) {
			DEV_ASSERT(check_list.find(true_list[i]) != -1);
		}
	}

#endif
}

void MeshSimplify::_rebuild_vertex_wedges() {
	data.wedges.clear();

	// Reserve to the max possible size, this prevents excessive resizing during population.
	data.wedges.reserve(data.verts.size());

	const uint32_t NUM_BUCKETS = 1024 * 5;
	LocalVector<LocalVector<uint32_t>> wedge_buckets;
	wedge_buckets.resize(NUM_BUCKETS);

	for (uint32_t n = 0; n < data.verts.size(); n++) {
		Vert &vert = data.verts[n];

		// Don't count vertices in degenerate triangles etc in the wedge count.
		if (!vert.active) {
			continue;
		}

		uint32_t hash = vert.position.hash() % NUM_BUCKETS;
		LocalVector<uint32_t> &bucket = wedge_buckets[hash];

		bool found = false;

		for (uint32_t w = 0; w < bucket.size(); w++) {
			uint32_t wedge_id = bucket[w];
			Wedge &wedge = data.wedges[wedge_id];

			if (wedge.position == vert.position) {
				// Record wedge ID in the vertex.
				vert.wedge = wedge_id;

				// Record the vertex ID in the wedge.
				wedge.verts.push_back(n);
				found = true;
				break;
			}
		}

		if (!found) {
			// Create a new wedge.
			uint32_t new_wedge_id = data.wedges.size();
			data.wedges.resize(data.wedges.size() + 1);
			Wedge &wedge = data.wedges[new_wedge_id];

			// Store on the vertex.
			vert.wedge = new_wedge_id;

			// Add to the hash table bucket.
			wedge_buckets[hash].push_back(new_wedge_id);

			// Add the vertex and position to the wedge.
			wedge.position = vert.position;
			wedge.verts.push_back(n);
		}
	}

	print_line("Built wedges... found " + itos(data.verts.size()) + " verts, " + itos(data.wedges.size()) + " wedges.");
}

void MeshSimplify::_build_vertex_triangle_links() {
	for (uint32_t n = 0; n < data.verts.size(); n++) {
		data.verts[n].tris.clear();
	}

	for (uint32_t t = 0; t < data.tris.size(); ++t) {
		const Tri &tri = data.tris[t];
		if (!tri.active)
			continue;

		for (int i = 0; i < 3; ++i) {
			uint32_t vert_id = tri.corn[i];
			DEV_ASSERT(vert_id < data.verts.size());
			DEV_ASSERT(data.verts[vert_id].active);
			data.verts[vert_id].tris.push_back(t);
			data.verts[vert_id].active = true;
		}
	}
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

		tri.edge_ids[0] = _get_or_create_edge(tri.corn[0], tri.corn[1], valid_tri_count);
		tri.edge_ids[1] = _get_or_create_edge(tri.corn[1], tri.corn[2], valid_tri_count);
		tri.edge_ids[2] = _get_or_create_edge(tri.corn[2], tri.corn[0], valid_tri_count);

		// Add the tri to the verts.
		for (uint32_t c = 0; c < 3; c++) {
			Vert &v = data.verts[tri.corn[c]];
			v.active = true;
		}

		// Tri was valid.
		valid_tri_count++;
	}

#if 0
	// Build vertex / edge adjacency.
	for (uint32_t e = 0; e < data.edges.size(); ++e) {
		const Edge &edge = data.edges[e];
		data.verts[edge.a].edges.push_back(e);
		data.verts[edge.b].edges.push_back(e);
	}
#endif

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

	_build_vertex_triangle_links();
	_rebuild_vertex_wedges();
	_detect_seam_edges();

	_initialize_vertex_quadrics();
}
