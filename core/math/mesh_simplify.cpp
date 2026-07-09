#include "mesh_simplify.h"
#include "core/os/os.h"
#include "mesh_deduplicator.h"
#include <map>
#include <utility>

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
	return itos(get_collapse_from()) + " to " + itos(wedge_to_collapse_to);
}

Vector3i MeshSimplify::Data::find_grid_pos(const Vector3 &p_pos) const {
	Vector3i res;

	for (uint32_t n = 0; n < 3; n++) {
		double d = (p_pos[n] - bound.position[n]) / bound_extent;
		double scaled = d * grid_size;

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

bool MeshSimplify::_can_collapse_test_tri(uint32_t kept_vert, uint32_t deleted_vert, uint32_t p_tri_id) const {
	const Tri &t = data.tris[p_tri_id];
	if (!t.active)
		return true;

	bool touches_deleted = false;
	Vector3i old_c[3];

	for (int i = 0; i < 3; ++i) {
		old_c[i] = data.verts[t.corn[i]].position;
		if (t.corn[i] == deleted_vert)
			touches_deleted = true;
	}

	if (!touches_deleted)
		return true;

	uint32_t kept_wedge = data.verts[kept_vert].wedge;
	for (int i = 0; i < 3; ++i) {
		if (data.verts[t.corn[i]].wedge == kept_wedge) {
			return true;
		}
	}

	Vector3i new_c[3];
	for (int i = 0; i < 3; ++i) {
		new_c[i] = (t.corn[i] == deleted_vert) ? data.verts[kept_vert].position : old_c[i];
	}

	if (_is_triangle_degenerate_from_positions(new_c)) {
		MS_LOG("_can_collapse rejecting edge from " + itos(deleted_vert) + " to " + itos(kept_vert) + " - _is_triangle_degenerate_from_positions");
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
		MS_LOG("_can_collapse rejecting edge from " + itos(deleted_vert) + " to " + itos(kept_vert) + " - len_a2");
		return false;
	}

	if (len_b2 > 1.0 && len_a2 > 1.0) {
		double cos_angle = before.dot(after) / (Math::sqrt(len_b2) * Math::sqrt(len_a2));
		if (cos_angle < 0.1) { // Very strict - almost no inversion
			MS_LOG("_can_collapse rejecting edge from " + itos(deleted_vert) + " to " + itos(kept_vert) + " - cos_angle");
			return false;
		}
	}

	// Strong perimeter protection for cylinders
	double old_peri = (old_c[1] - old_c[0]).length() + (old_c[2] - old_c[1]).length() + (old_c[0] - old_c[2]).length();
	double new_peri = (new_c[1] - new_c[0]).length() + (new_c[2] - new_c[1]).length() + (new_c[0] - new_c[2]).length();

	if (new_peri < old_peri * 0.85) {
		MS_LOG("_can_collapse rejecting edge from " + itos(deleted_vert) + " to " + itos(kept_vert) + " - peri");
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
		MS_LOG("_can_collapse rejecting edge from " + itos(deleted_vert) + " to " + itos(kept_vert) + " - min_s");
		return false;
	}

	return true;
}

bool MeshSimplify::_can_collapse(uint32_t kept_wedge, uint32_t deleted_wedge, uint32_t p_edge_id) const {
	const Wedge &w_del = data.wedges[deleted_wedge];
	const Wedge &w_kep = data.wedges[kept_wedge];

	// Constraint 1: Original structure compatibility
	if (!Wedge::can_collapse[(uint32_t)w_del.original_type][(uint32_t)w_kep.original_type]) {
		return false;
	}

	// Constraint 2: Dynamic topological compatibility
	if (!Wedge::can_collapse[(uint32_t)w_del.type][(uint32_t)w_kep.type]) {
		return false;
	}

	// Rule 1: A border wedge can only collapse to another border wedge.
	if (w_del.original_is_border && !w_kep.original_is_border) {
		return false;
	}
	if (w_del.is_border && !w_kep.is_border) {
		return false;
	}

	// Rule 2: A seam wedge can only collapse to another seam wedge.
	if (w_del.original_is_seam && !w_kep.original_is_seam) {
		return false;
	}
	if (w_del.is_seam && !w_kep.is_seam) {
		return false;
	}

	// Rule 3: Topology-preserving boundary edge collapse check (Link Condition).
	if ((w_del.original_is_border && w_kep.original_is_border) || (w_del.is_border && w_kep.is_border)) {
		bool found_border_edge = false;
		if (p_edge_id != UINT32_MAX && p_edge_id < data.edges.size()) {
			const Edge &e = data.edges[p_edge_id];
			if (e.active && e.triangle_count == 1) {
				found_border_edge = true;
			}
		} else {
			for (uint32_t i = 0; i < data.edges.size(); i++) {
				const Edge &e = data.edges[i];
				if (e.active && ((e.a == kept_wedge && e.b == deleted_wedge) || (e.a == deleted_wedge && e.b == kept_wedge))) {
					if (e.triangle_count == 1) {
						found_border_edge = true;
					}
					break;
				}
			}
		}
		if (!found_border_edge) {
			return false; // Prevent pinching across interior manifold structures
		}
	}

	// For each active vertex in the deleted wedge, we verify that its triangles can collapse to its matched vertex in the kept wedge.
	for (uint32_t i = 0; i < w_del.verts.size(); i++) {
		uint32_t v_del = w_del.verts[i];
		if (!data.verts[v_del].active) {
			continue;
		}

		uint32_t v_kep = _find_best_matching_vertex_in_wedge(v_del, kept_wedge);
		if (v_kep == UINT32_MAX) {
			return false;
		}

		const LocalVector<uint32_t> &tri_list = data.verts[v_del].tris;

		for (uint32_t n = 0; n < tri_list.size(); n++) {
			if (!_can_collapse_test_tri(v_kep, v_del, tri_list[n])) {
				return false;
			}
		}
	}

	return true;
}

void MeshSimplify::_validate_and_rebuild() {
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
	if (!input_data.indices.size()) {
		return false;
	}

	LocalVector<uint32_t> inds;

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
	as_pos.set_type(MeshAttributeStream::ATTR_POSITION);
	as_pos.vec3 = input_data.positions;

	uint32_t as_id_uvs = UINT32_MAX;
	uint32_t as_id_uv2s = UINT32_MAX;
	uint32_t as_id_normals = UINT32_MAX;
	uint32_t as_id_colors = UINT32_MAX;
	uint32_t as_id_floats = UINT32_MAX;

	if (input_data.uvs.size()) {
		as_id_uvs = fill_stream;
		MeshAttributeStream &as = r_dd.get_input_attribute_stream(fill_stream++);
		as.set_type(MeshAttributeStream::ATTR_UV);
		as.vec2 = input_data.uvs;
	}

	if (input_data.uv2s.size()) {
		as_id_uv2s = fill_stream;
		MeshAttributeStream &as = r_dd.get_input_attribute_stream(fill_stream++);
		as.set_type(MeshAttributeStream::ATTR_UV);
		as.vec2 = input_data.uv2s;
	}

	if (input_data.normals.size()) {
		as_id_normals = fill_stream;
		MeshAttributeStream &as = r_dd.get_input_attribute_stream(fill_stream++);
		as.set_type(MeshAttributeStream::ATTR_NORMAL);
		as.vec3 = input_data.normals;
	}

	if (input_data.colors.size()) {
		as_id_colors = fill_stream;
		MeshAttributeStream &as = r_dd.get_input_attribute_stream(fill_stream++);
		as.set_type(MeshAttributeStream::ATTR_COLOR);
		as.color = input_data.colors;
	}

	if (input_data.floats.size()) {
		as_id_floats = fill_stream;
		MeshAttributeStream &as = r_dd.get_input_attribute_stream(fill_stream++);
		as.set_type(MeshAttributeStream::ATTR_FLOAT);
		as.float_input = input_data.floats;
	}

	if (!r_dd.process(input_data.indices, inds)) {
		return false;
	}

	input_data.indices = inds;
	input_data.positions = r_dd.get_output_attribute_stream(0).vec3;
	if (as_id_uvs != UINT32_MAX) {
		const MeshAttributeStream &as = r_dd.get_output_attribute_stream(as_id_uvs);
		DEV_ASSERT(as.get_type() == MeshAttributeStream::ATTR_UV);
		input_data.uvs = as.vec2;
	}
	if (as_id_uv2s != UINT32_MAX) {
		const MeshAttributeStream &as = r_dd.get_output_attribute_stream(as_id_uv2s);
		DEV_ASSERT(as.get_type() == MeshAttributeStream::ATTR_UV);
		input_data.uv2s = as.vec2;
	}
	if (as_id_normals != UINT32_MAX) {
		const MeshAttributeStream &as = r_dd.get_output_attribute_stream(as_id_normals);
		DEV_ASSERT(as.get_type() == MeshAttributeStream::ATTR_NORMAL);
		input_data.normals = as.vec3;
	}
	if (as_id_colors != UINT32_MAX) {
		const MeshAttributeStream &as = r_dd.get_output_attribute_stream(as_id_colors);
		DEV_ASSERT(as.get_type() == MeshAttributeStream::ATTR_COLOR);
		input_data.colors = as.color;
	}
	if (as_id_floats != UINT32_MAX) {
		const MeshAttributeStream &as = r_dd.get_output_attribute_stream(as_id_floats);
		DEV_ASSERT(as.get_type() == MeshAttributeStream::ATTR_FLOAT);
		input_data.floats = as.float_input;
	}

	ERR_FAIL_COND_V(!input_data.indices.size(), false);
	ERR_FAIL_COND_V(!input_data.positions.size(), false);

	_debug_log_input_data();

	Span<Vector3> in_verts = Span<Vector3>(input_data.positions);

	data.bound.position = in_verts[0];
	data.bound.size = Vector3();

	for (uint32_t n = 1; n < in_verts.size(); n++) {
		data.bound.expand_to(in_verts[n]);
	}

	const real_t min_bound = 1e-4f;

	if (data.bound.size.length() < min_bound) {
		data.bound.size = Vector3(min_bound, min_bound, min_bound);
	}

	data.bound_extent = data.bound.size.coord[data.bound.size.max_axis()];

	data.verts.resize(in_verts.size());

	Span<Vector2> in_uvs = Span<Vector2>(input_data.uvs);
	Span<Vector2> in_uv2s = Span<Vector2>(input_data.uv2s);
	Span<Vector3> in_normals = Span<Vector3>(input_data.normals);
	Span<Color> in_colors = Span<Color>(input_data.colors);
	Span<float> in_floats = Span<float>(input_data.floats);

	for (uint32_t n = 0; n < in_verts.size(); n++) {
		data.verts[n].position = data.find_grid_pos(in_verts[n]);
		data.verts[n].active = true;

		if (in_uvs.size()) {
			data.verts[n].uv = in_uvs[n];
		}
		if (in_uv2s.size()) {
			data.verts[n].uv2 = in_uv2s[n];
		}
		if (in_normals.size()) {
			data.verts[n].normal = in_normals[n];
		}
		if (in_colors.size()) {
			data.verts[n].color = in_colors[n];
		}
		if (in_floats.size()) {
			data.verts[n].flt = in_floats[n];
		}
	}

	_rebuild_vertex_wedges();
	_create_tris();

	if (!data.tris.size()) {
		return false;
	}

	return true;
}

void MeshSimplify::_delete_triangle(uint32_t p_tri_id) {
	_debug_sanity_check();

	Tri &t = data.tris[p_tri_id];

	DEV_ASSERT(t.active);
	t.active = false;

	for (uint32_t n = 0; n < 3; n++) {
		if (data.edges[t.edge_ids[n]].triangle_count > 0) {
			data.edges[t.edge_ids[n]].triangle_count--;
		}
	}

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

uint32_t MeshSimplify::_find_best_matching_vertex_in_wedge(uint32_t p_v_deleted, uint32_t p_wedge_kept) const {
	const Vert &v_d = data.verts[p_v_deleted];
	const Wedge &w_k = data.wedges[p_wedge_kept];

	uint32_t best_v = UINT32_MAX;
	double min_dist = 1e30;

	for (uint32_t i = 0; i < w_k.verts.size(); i++) {
		uint32_t v_k_id = w_k.verts[i];
		if (!data.verts[v_k_id].active) {
			continue;
		}
		const Vert &v_k = data.verts[v_k_id];

		double dist = 0.0;
		if (input_data.uvs.size()) {
			dist += (v_d.uv - v_k.uv).length_squared() * 10.0;
		}
		if (input_data.uv2s.size()) {
			dist += (v_d.uv2 - v_k.uv2).length_squared() * 10.0;
		}
		if (input_data.normals.size()) {
			dist += (v_d.normal - v_k.normal).length_squared();
		}
		if (input_data.colors.size()) {
			Color diff = v_d.color - v_k.color;
			dist += (diff.r * diff.r + diff.g * diff.g + diff.b * diff.b + diff.a * diff.a);
		}
		if (input_data.floats.size()) {
			double diff = v_d.flt - v_k.flt;
			dist += diff * diff;
		}

		if (dist < min_dist) {
			min_dist = dist;
			best_v = v_k_id;
		}
	}

	return best_v;
}

void MeshSimplify::_collapse_wedge_pair(uint32_t p_kept, uint32_t p_deleted, uint32_t p_edge_idx, LocalVector<uint32_t> &r_altered_edges, LocalVector<uint32_t> &r_touched_edges, LocalVector<uint32_t> &r_affected_tris, std::priority_queue<SortedEdge> &r_queue, uint32_t &r_current_triangle_count, int32_t &r_edges_to_collapse) {
	Wedge &kept_wedge = data.wedges[p_kept];
	Wedge &deleted_wedge = data.wedges[p_deleted];

	// print_line(String("collapse_wedge_pair from ") + itos(p_deleted) + " [" + itos((uint32_t)deleted_wedge.type) + "] to " + itos(p_kept) + " [" + itos((uint32_t)kept_wedge.type) + "]");

	DEV_ASSERT(Wedge::can_collapse[(uint32_t)deleted_wedge.original_type][(uint32_t)kept_wedge.original_type]);
	DEV_ASSERT(Wedge::can_collapse[(uint32_t)deleted_wedge.type][(uint32_t)kept_wedge.type]);

	kept_wedge.Q = kept_wedge.Q + deleted_wedge.Q;

	// Pair each active vertex of the deleted wedge with its best-matching partner in the kept wedge
	LocalVector<std::pair<uint32_t, uint32_t>> matched_pairs;
	for (uint32_t i = 0; i < deleted_wedge.verts.size(); i++) {
		uint32_t v_del = deleted_wedge.verts[i];
		if (!data.verts[v_del].active) {
			continue;
		}
		uint32_t v_kep = _find_best_matching_vertex_in_wedge(v_del, p_kept);
		if (v_kep != UINT32_MAX) {
			matched_pairs.push_back(std::make_pair(v_kep, v_del));

			data.verts[v_kep].Q = data.verts[v_kep].Q + data.verts[v_del].Q;
			data.verts[v_kep].Qu = data.verts[v_kep].Qu + data.verts[v_del].Qu;
			data.verts[v_kep].Qv = data.verts[v_kep].Qv + data.verts[v_del].Qv;
		}
		data.verts[v_del].active = false;
	}

	data.edges[p_edge_idx].active = false;

	r_altered_edges.clear();
	r_touched_edges.clear();

	r_affected_tris.clear();
	for (uint32_t i = 0; i < deleted_wedge.verts.size(); i++) {
		uint32_t v_del = deleted_wedge.verts[i];
		const LocalVector<uint32_t> &vt_list = data.verts[v_del].tris;
		for (uint32_t j = 0; j < vt_list.size(); j++) {
			r_affected_tris.push_back_if_not_present(vt_list[j]);
		}
	}

	for (uint32_t n = 0; n < r_affected_tris.size(); n++) {
		uint32_t tri_id = r_affected_tris[n];
		Tri &t = data.tris[tri_id];
		if (!t.active)
			continue;

		bool modified = false;

		uint32_t new_corn[3];
		new_corn[0] = t.corn[0];
		new_corn[1] = t.corn[1];
		new_corn[2] = t.corn[2];

		for (int i = 0; i < 3; i++) {
			uint32_t v = t.corn[i];
			if (data.verts[v].wedge == p_deleted) {
				uint32_t mapped_v = UINT32_MAX;
				for (uint32_t j = 0; j < matched_pairs.size(); j++) {
					if (matched_pairs[j].second == v) {
						mapped_v = matched_pairs[j].first;
						break;
					}
				}
				if (mapped_v != UINT32_MAX) {
					new_corn[i] = mapped_v;
					modified = true;
				}
			}
		}

		if (modified) {
			uint32_t w0 = data.verts[new_corn[0]].wedge;
			uint32_t w1 = data.verts[new_corn[1]].wedge;
			uint32_t w2 = data.verts[new_corn[2]].wedge;

			if (w0 == w1 || w1 == w2 || w2 == w0 || _is_triangle_degenerate(new_corn)) {
				for (uint32_t i = 0; i < 3; i++) {
					r_touched_edges.push_back_if_not_present(t.edge_ids[i]);
				}
				_delete_triangle(tri_id);
				r_current_triangle_count--;
				continue;
			}

			for (int i = 0; i < 3; i++) {
				if (t.corn[i] != new_corn[i]) {
					data.verts[t.corn[i]].tris.erase_unordered(tri_id);

					DEV_ASSERT(data.verts[new_corn[i]].tris.find(tri_id) == -1);

					data.verts[new_corn[i]].tris.push_back(tri_id);
					t.corn[i] = new_corn[i];
				}
			}

			for (uint32_t i = 0; i < 3; i++) {
				uint32_t old_edge_id = t.edge_ids[i];
				uint32_t w_a = data.verts[t.corn[i]].wedge;
				uint32_t w_b = data.verts[t.corn[(i + 1) % 3]].wedge;
				uint32_t new_edge_id = _get_or_create_edge(w_a, w_b, tri_id, old_edge_id);
				if (new_edge_id != t.edge_ids[i]) {
					r_altered_edges.push_back_if_not_present(old_edge_id);
					r_altered_edges.push_back_if_not_present(new_edge_id);
					t.edge_ids[i] = new_edge_id;

					if (data.edges[old_edge_id].triangle_count > 0) {
						data.edges[old_edge_id].triangle_count--;
					}
					data.edges[new_edge_id].triangle_count++;
				}
			}
		}
	}

	for (uint32_t n = 0; n < r_altered_edges.size(); n++) {
		uint32_t e_idx = r_altered_edges[n];
		Edge &e = data.edges[e_idx];
		if (!e.active)
			continue;

		if (e.triangle_count == 0) {
			e.active = false;
			continue;
		}

		if (e.a == p_deleted) {
			e.a = p_kept;
			e.sort();
		}
		if (e.b == p_deleted) {
			e.b = p_kept;
			e.sort();
		}
		if (e.a == e.b) {
			e.active = false;
		} else {
			r_touched_edges.push_back_if_not_present(e_idx);
		}
	}

	_get_edges_touching_wedge(p_kept, r_touched_edges);

	_debug_sanity_check();

	for (uint32_t n = 0; n < r_touched_edges.size(); n++) {
		uint32_t e_idx = r_touched_edges[n];

		_refresh_edge_seam_flag(e_idx);

		Edge &e = data.edges[e_idx];
		if (!e.active)
			continue;

		_reclassify_wedge(e.a);
		_reclassify_wedge(e.b);

		_evaluate_edge_collapse(e_idx);
		e.version++;
		r_queue.push(SortedEdge(e_idx, e.cost, e.version));
	}

	_debug_sanity_check();

	r_edges_to_collapse--;
}

bool MeshSimplify::_validate_geometric_boundaries(bool p_initial) {
	return true;

	std::map<std::pair<uint32_t, uint32_t>, int> geom_edge_counts;

	for (uint32_t t = 0; t < data.tris.size(); t++) {
		const Tri &tri = data.tris[t];
		if (!tri.active) {
			continue;
		}

		uint32_t w0 = data.verts[tri.corn[0]].wedge;
		uint32_t w1 = data.verts[tri.corn[1]].wedge;
		uint32_t w2 = data.verts[tri.corn[2]].wedge;

		if (w0 == UINT32_MAX || w1 == UINT32_MAX || w2 == UINT32_MAX) {
			continue;
		}

		if (w0 == w1 || w1 == w2 || w2 == w0) {
			continue;
		}

		geom_edge_counts[std::make_pair(MIN(w0, w1), MAX(w0, w1))]++;
		geom_edge_counts[std::make_pair(MIN(w1, w2), MAX(w1, w2))]++;
		geom_edge_counts[std::make_pair(MIN(w2, w0), MAX(w2, w0))]++;
	}

	if (p_initial) {
		data.initial_boundaries.clear();
		for (std::map<std::pair<uint32_t, uint32_t>, int>::const_iterator it = geom_edge_counts.begin(); it != geom_edge_counts.end(); ++it) {
			if (it->second == 1) {
				data.initial_boundaries.push_back(WedgePair(it->first.first, it->first.second));
			}
		}
		return true;
	}

	bool has_error = false;
	for (std::map<std::pair<uint32_t, uint32_t>, int>::const_iterator it = geom_edge_counts.begin(); it != geom_edge_counts.end(); ++it) {
		if (it->second == 1) {
			WedgePair wp(it->first.first, it->first.second);
			bool found = false;
			for (uint32_t i = 0; i < data.initial_boundaries.size(); i++) {
				if (data.initial_boundaries[i] == wp) {
					found = true;
					break;
				}
			}
			if (!found) {
				print_line(String("HOLE DETECTED! New geometric boundary edge: Wedge ") + itos(it->first.first) + " to Wedge " + itos(it->first.second));
				has_error = true;
			}
		} else if (it->second > 2) {
			print_line(String("NON-MANIFOLD DETECTED! Geometric edge: Wedge ") + itos(it->first.first) + " to Wedge " + itos(it->first.second) + " has count " + itos(it->second));
			has_error = true;
		}
	}

	return !has_error;
}

bool MeshSimplify::simplify_mesh() {
	MeshDeduplicator dd;

	uint64_t time_before = OS::get_singleton()->get_ticks_msec();

	if (!prepare(dd)) {
		return false;
	}

	std::priority_queue<SortedEdge> queue;
	for (uint32_t n = 0; n < data.edges.size(); n++) {
		_evaluate_edge_collapse(n);
		SortedEdge e(n, data.edges[n].cost, 0);
		queue.push(e);
	}

	_validate_geometric_boundaries(true);

	uint32_t current_triangle_count = data.tris.size();
	uint32_t before_triangle_count = current_triangle_count;

	uint32_t target = MESH_SIMPLIFY_FACTOR(before_triangle_count);

	int32_t edges_to_collapse = INT32_MAX;
	if (MESH_NUM_EDGES_TO_COLLAPSE != 0) {
		edges_to_collapse = MESH_NUM_EDGES_TO_COLLAPSE;
		target = 1;
	}

	LocalVector<uint32_t> altered_edges;
	LocalVector<uint32_t> touched_edges;
	LocalVector<uint32_t> affected_tris;

	uint32_t infinite_loop_breaker = 1024 * 10;

	while ((current_triangle_count > target && !queue.empty()) && current_triangle_count > 1) {
		infinite_loop_breaker--;
		if (infinite_loop_breaker == 0) {
			print_line("Infinite loop breaker activated.");
			break;
		}

		_debug_sanity_check();

		SortedEdge se = queue.top();
		queue.pop();

		if (se.edge_id >= data.edges.size())
			continue;

		Edge &edge = data.edges[se.edge_id];

		if (!edge.active || se.version != edge.version || se.cost != edge.cost)
			continue;
		if (_count_active_wedge_verts(edge.a) == 0 || _count_active_wedge_verts(edge.b) == 0)
			continue;

		uint32_t kept = edge.wedge_to_collapse_to;
		uint32_t deleted = (kept == edge.a ? edge.b : edge.a);

#ifdef MESH_SIMPLIFY_DISALLOW_SEAMS
		Wedge &kept_wedge = data.wedges[kept];
		Wedge &deleted_wedge = data.wedges[deleted];

		if (deleted_wedge.is_seam_or_boundary || deleted_wedge.original_is_seam_or_boundary) {
			edge.active = false;
			continue;
		}
#endif

		if (!_can_collapse(kept, deleted, se.edge_id)) {
			_evaluate_edge_collapse(se.edge_id);
			edge.version++;
			queue.push(SortedEdge(se.edge_id, edge.cost, edge.version));
			continue;
		}

#ifdef MESH_SIMPLIFY_DEBUG_LOGGING
		String sz = "collapsing edge " + itos(se.edge_id) + " : " + itos(edge.get_collapse_from()) + " to " + itos(edge.wedge_to_collapse_to);
		if (edge.is_seam_or_boundary) {
			sz += "\tSEAM";
		}
		sz += " cost : " + itos(edge.get_readable_cost());
		MS_LOG(sz);
#endif

		_debug_sanity_check();

		// print_line(String("Step: Collapsing Edge ") + itos(se.edge_id) + " (kept wedge: " + itos(kept) + ", deleted wedge: " + itos(deleted) + ")");

		_collapse_wedge_pair(kept, deleted, se.edge_id, altered_edges, touched_edges, affected_tris, queue, current_triangle_count, edges_to_collapse);

		_validate_geometric_boundaries(false);

		if (edges_to_collapse <= 0) {
			break;
		}
#ifdef MESH_SIMPLIFY_ONE_AT_A_TIME
		break;
#endif
	}

	for (uint32_t n = 0; n < data.tris.size(); n++) {
		Tri &t = data.tris[n];
		if (t.active && _is_triangle_degenerate(t.corn)) {
			_delete_triangle(n);
			current_triangle_count--;
		}
	}

	data.output_remapped_indices.resize(current_triangle_count * 3);
	uint32_t out_ind_count = 0;

	for (uint32_t n = 0; n < data.tris.size(); n++) {
		const Tri &t = data.tris[n];
		if (!t.active)
			continue;

		for (uint32_t c = 0; c < 3; c++) {
			uint32_t orig_index = dd.get_output_vertex_mapping_to_input_vertex(t.corn[c]);
			data.output_remapped_indices[out_ind_count++] = orig_index;
		}
	}

	uint64_t time_after = OS::get_singleton()->get_ticks_msec();

	print_line("simplify before_triangle_count: " + itos(before_triangle_count) + ", after_triangle_count: " + itos(current_triangle_count));
	print_line("\nTook " + itos(time_after - time_before) + " milliseconds.");
	print_line(itos(data.edges.size()) + " max edges.");
	print_line(itos(data.tris.size()) + " max tris.");

	return true;
}

int32_t MeshSimplify::_triangle_which_side(const Vector3i &p_a, const Vector3i &p_b, const Vector3i &p_c, const Vector3i &p_test) const {
	int64_t ux = (int64_t)p_b.x - p_a.x;
	int64_t uy = (int64_t)p_b.y - p_a.y;
	int64_t uz = (int64_t)p_b.z - p_a.z;

	int64_t vx = (int64_t)p_c.x - p_a.x;
	int64_t vy = (int64_t)p_c.y - p_a.y;
	int64_t vz = (int64_t)p_c.z - p_a.z;

	int64_t wx = (int64_t)p_test.x - p_a.x;
	int64_t wy = (int64_t)p_test.y - p_a.y;
	int64_t wz = (int64_t)p_test.z - p_a.z;

	int64_t nx = uy * vz - uz * vy;
	int64_t ny = uz * vx - ux * vz;
	int64_t nz = ux * vy - uy * vx;

	int64_t scalar = nx * wx + ny * wy + nz * wz;

	if (scalar > 0)
		return 1;
	if (scalar < 0)
		return -1;
	return 0;
}

bool MeshSimplify::_is_triangle_degenerate(const uint32_t p_inds[3]) const {
	if ((p_inds[0] == p_inds[1]) || (p_inds[1] == p_inds[2]) || (p_inds[0] == p_inds[2])) {
		return true;
	}

	const Vector3i &a = data.verts[p_inds[0]].position;
	const Vector3i &b = data.verts[p_inds[1]].position;
	const Vector3i &c = data.verts[p_inds[2]].position;

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
		return true;
	}

	return false;
}

uint32_t MeshSimplify::_get_or_create_edge(uint32_t p_wedge_a, uint32_t p_wedge_b, uint32_t p_triangle_id, uint32_t p_first_check_edge) {
	Edge e;
	e.a = p_wedge_a;
	e.b = p_wedge_b;
	e.sort();

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

	Vector3_64 p0(0, 0, 0);
	Vector3_64 p1(1, 0, 0);
	Vector3_64 p2(0, 1, 0);

	Vector3_64 p3(0, 0, 1);
	Vector3_64 p4(1, 0, 1);
	Vector3_64 p5(0, 1, 1);

	Plane_64 plane1(p0, p1, p2);
	Plane_64 plane2(p0, p2, p1);
	Plane_64 plane3(p3, p4, p5);

	print_line("Distance from p3 to plane3: " + rtos(plane3.distance_to(p3)));
	print_line("Distance from p4 to plane3: " + rtos(plane3.distance_to(p4)));
	print_line("Distance from p5 to plane3: " + rtos(plane3.distance_to(p5)));

	print_line("Plane1: " + String(plane1.normal) + " d=" + rtos(plane1.d));
	print_line("Plane2: " + String(plane2.normal) + " d=" + rtos(plane2.d));
	print_line("Plane3: " + String(plane3.normal) + " d=" + rtos(plane3.d));

	Quadric Kp1(plane1);
	Quadric Kp2(plane2);
	Quadric Kp3(plane3);

	Vector3i test_pos(0, 0, 0);
	Vector3i test_pos3(0, 0, 1);

	print_line("d = " + rtos(plane3.d) + ", Kp3[3][3] = " + rtos(Kp3.m[3][3]));
	Vector3_64 test_p(test_pos3.x, test_pos3.y, test_pos3.z);
	print_line("test point distance  = " + rtos(plane3.distance_to(test_p)));

	double error1 = _compute_quadric_error(test_pos, Kp1);
	double error2 = _compute_quadric_error(test_pos, Kp2);
	double error3 = _compute_quadric_error(test_pos3, Kp3);

	print_line("Error at point for plane1: " + rtos(error1));
	print_line("Error at point for plane2: " + rtos(error2));
	print_line("Error at point for plane3: " + rtos(error3));

	if (Math::abs(error1) > 1e-5 || Math::abs(error2) > 1e-5 || Math::abs(error3) > 1e-5) {
		print_line("WARNING: Quadric error not zero on plane!");
	}
}

void MeshSimplify::_test_attribute_quadrics() {
	print_line("=== ATTRIBUTE QUADRIC UNIT TEST (Gradient) ===");

	Vector3_64 p0(0, 0, 0);
	Vector3_64 p1(1, 0, 0);
	Vector3_64 p2(0, 1, 0);

	double u0 = 5.0, u1 = 6.0, u2 = 5.0;

	Vector3_64 edge1 = p1 - p0;
	Vector3_64 edge2 = p2 - p0;
	Vector3_64 cross = edge1.cross(edge2);
	double normal_length = cross.length();
	Vector3_64 normal = cross / normal_length;

	Vector4_64 gradient = _solve_attribute_gradient(p0, p1, p2, normal, u0, u1, u2);

	print_line("Solved gradient = [" + rtos(gradient.x) + ", " + rtos(gradient.y) + ", " + rtos(gradient.z) + "], c = " + rtos(gradient.w));

	Vector3i test_pos(1, 0, 0);

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
	double a = p_plane.normal.x;
	double b = p_plane.normal.y;
	double c = p_plane.normal.z;
	double d = -p_plane.d;

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

	struct DetSolver {
		static double det_4x4(double m[4][4]) {
			double sub0 = m[1][1] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) - m[1][2] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) + m[1][3] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]);
			double sub1 = m[1][0] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) - m[1][2] * (m[2][0] * m[3][3] - m[2][3] * m[3][0]) + m[1][3] * (m[2][0] * m[3][2] - m[2][2] * m[3][0]);
			double sub2 = m[1][0] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) - m[1][1] * (m[2][0] * m[3][3] - m[2][3] * m[3][0]) + m[1][3] * (m[2][0] * m[3][1] - m[2][1] * m[3][0]);
			double sub3 = m[1][0] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]) - m[1][1] * (m[2][0] * m[3][2] - m[2][2] * m[3][0]) + m[1][2] * (m[2][0] * m[3][1] - m[2][1] * m[3][0]);
			return m[0][0] * sub0 - m[0][1] * sub1 + m[0][2] * sub2 - m[0][3] * sub3;
		}
	};

	double main_det = DetSolver::det_4x4(base_matrix);
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
		coeff[col] = DetSolver::det_4x4(temp) / main_det;
	}

	return Vector4_64(coeff[0], coeff[1], coeff[2], coeff[3]);
}

void MeshSimplify::_initialize_vertex_quadrics() {
	for (uint32_t i = 0; i < data.wedges.size(); i++) {
		data.wedges[i].Q = Quadric();
	}
	for (uint32_t i = 0; i < data.verts.size(); i++) {
		data.verts[i].Qu = Quadric();
		data.verts[i].Qv = Quadric();
		data.verts[i].Q = Quadric();
	}

	for (uint32_t n = 0; n < data.tris.size(); n++) {
		Tri &t = data.tris[n];

		const Vert &v0 = data.verts[t.corn[0]];
		const Vert &v1 = data.verts[t.corn[1]];
		const Vert &v2 = data.verts[t.corn[2]];

		Vector3_64 p0 = v0.pos();
		Vector3_64 p1 = v1.pos();
		Vector3_64 p2 = v2.pos();

		Vector3_64 edge1 = p1 - p0;
		Vector3_64 edge2 = p2 - p0;
		Vector3_64 cross = edge1.cross(edge2);
		double normal_length = cross.length();

		if (normal_length < 1e-7) {
			continue;
		}

		double area = 0.5 * normal_length;

		Quadric Kp(t.plane);
		Kp = Kp * area;

		for (uint32_t i = 0; i < 3; i++) {
			uint32_t v_id = t.corn[i];
			Vert &v = data.verts[v_id];
			v.Q = v.Q + Kp;
			data.wedges[v.wedge].Q = data.wedges[v.wedge].Q + Kp;
		}

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
}

double MeshSimplify::_compute_quadric_error(const Vector3i &p_pos, const Quadric &Q) {
	double x = p_pos.x;
	double y = p_pos.y;
	double z = p_pos.z;
	double w = 1;

	double rx = Q.m[0][0] * x + Q.m[0][1] * y + Q.m[0][2] * z + Q.m[0][3] * w;
	double ry = Q.m[1][0] * x + Q.m[1][1] * y + Q.m[1][2] * z + Q.m[1][3] * w;
	double rz = Q.m[2][0] * x + Q.m[2][1] * y + Q.m[2][2] * z + Q.m[2][3] * w;
	double rw = Q.m[3][0] * x + Q.m[3][1] * y + Q.m[3][2] * z + Q.m[3][3] * w;

	double error = (x * rx) + (y * ry) + (z * rz) + (w * rw);

	return error;
}

MeshSimplify::Quadric MeshSimplify::_build_attribute_quadric(const Vector4_64 &p_gradient, double p_target) const {
	return Quadric(Vector4_64(p_gradient.x, p_gradient.y, p_gradient.z, p_gradient.w - p_target));
}

void MeshSimplify::_evaluate_edge_collapse(uint32_t p_edge_id) {
	Edge &edge = data.edges[p_edge_id];
	const Wedge &a = data.wedges[edge.a];
	const Wedge &b = data.wedges[edge.b];

	Quadric Q_new = a.Q + b.Q;

	const double beta = 150;

	double distance_cost = (a.pos() - b.pos()).length();

	const double VERY_HIGH_COST = 1e30;
	double total_a = VERY_HIGH_COST;
	double total_b = VERY_HIGH_COST;

	bool can_collapse_to_a = _can_collapse(edge.a, edge.b, p_edge_id);
	bool can_collapse_to_b = _can_collapse(edge.b, edge.a, p_edge_id);

	if (!can_collapse_to_b && !can_collapse_to_a) {
		edge.active = false;
		edge.cost = VERY_HIGH_COST;
		return;
	}

	if (can_collapse_to_a) {
		double geom_cost_a = _compute_quadric_error(a.position, Q_new);

		double attr_a = 0;
		if (input_data.uvs.size()) {
			for (uint32_t i = 0; i < b.verts.size(); i++) {
				uint32_t v_b = b.verts[i];
				if (!data.verts[v_b].active) {
					continue;
				}
				uint32_t v_a = _find_best_matching_vertex_in_wedge(v_b, edge.a);
				if (v_a != UINT32_MAX) {
					const Vert &va_ref = data.verts[v_a];
					const Vert &vb_ref = data.verts[v_b];
					Quadric Qu_new = va_ref.Qu + vb_ref.Qu;
					Quadric Qv_new = va_ref.Qv + vb_ref.Qv;
					attr_a += _compute_quadric_error(va_ref.position, Qu_new) + _compute_quadric_error(va_ref.position, Qv_new);
				}
			}
		}
		total_a = distance_cost + geom_cost_a + beta * attr_a;

		if (b.is_seam_or_boundary || b.original_is_seam_or_boundary) {
			bool breaks_line = true;

			if (b.seam_neighbour_wedges.size() == 2) {
				for (uint32_t n = 0; n < b.seam_neighbour_wedges.size(); n++) {
					if (b.seam_neighbour_wedges[n] == edge.a) {
						breaks_line = false;
						break;
					}
				}
			}

			if (breaks_line) {
				total_a *= 60;
				total_a = MAX(total_a, 60.0);
			} else {
				DEV_ASSERT(b.seam_neighbour_wedges.size() == 2);
				const Vector3i &v_prev = data.wedges[b.seam_neighbour_wedges[0]].position;
				const Vector3i &v_next = data.wedges[b.seam_neighbour_wedges[1]].position;

				Vector3_64 dir1 = Vector3_64(b.position - v_prev).normalized();
				Vector3_64 dir2 = Vector3_64(v_next - b.position).normalized();
				double angle_cos = dir1.dot(dir2);

				double deviation = 1.0 - angle_cos;

				total_a += deviation * 30.0;
			}
		}
	}

	if (can_collapse_to_b) {
		double geom_cost_b = _compute_quadric_error(b.position, Q_new);

		double attr_b = 0;
		if (input_data.uvs.size()) {
			for (uint32_t i = 0; i < a.verts.size(); i++) {
				uint32_t v_a = a.verts[i];
				if (!data.verts[v_a].active) {
					continue;
				}
				uint32_t v_b = _find_best_matching_vertex_in_wedge(v_a, edge.b);
				if (v_b != UINT32_MAX) {
					const Vert &va_ref = data.verts[v_a];
					const Vert &vb_ref = data.verts[v_b];
					Quadric Qu_new = va_ref.Qu + vb_ref.Qu;
					Quadric Qv_new = va_ref.Qv + vb_ref.Qv;
					attr_b += _compute_quadric_error(vb_ref.position, Qu_new) + _compute_quadric_error(vb_ref.position, Qv_new);
				}
			}
		}

		total_b = distance_cost + geom_cost_b + beta * attr_b;

		if (a.is_seam_or_boundary || a.original_is_seam_or_boundary) {
			bool breaks_line = true;

			if (a.seam_neighbour_wedges.size() == 2) {
				for (uint32_t n = 0; n < a.seam_neighbour_wedges.size(); n++) {
					if (a.seam_neighbour_wedges[n] == edge.b) {
						breaks_line = false;
						break;
					}
				}
			}

			if (breaks_line) {
				total_b *= 60;
				total_b = MAX(total_b, 60.0);
			} else {
				DEV_ASSERT(a.seam_neighbour_wedges.size() == 2);
				const Vector3i &v_prev = data.wedges[a.seam_neighbour_wedges[0]].position;
				const Vector3i &v_next = data.wedges[a.seam_neighbour_wedges[1]].position;

				Vector3_64 dir1 = Vector3_64(a.position - v_prev).normalized();
				Vector3_64 dir2 = Vector3_64(v_next - a.position).normalized();
				double angle_cos = dir1.dot(dir2);

				double deviation = 1.0 - angle_cos;

				total_b += deviation * 30.0;
			}
		}
	}

#ifdef MESH_SIMPLIFY_DEBUG_LOGGING
	double from_cost = 0;
#endif
	if (total_a < total_b) {
		edge.wedge_to_collapse_to = edge.a;
		edge.cost = total_a;
#ifdef MESH_SIMPLIFY_DEBUG_LOGGING
		from_cost = total_b;
#endif
	} else {
		edge.wedge_to_collapse_to = edge.b;
		edge.cost = total_b;
#ifdef MESH_SIMPLIFY_DEBUG_LOGGING
		from_cost = total_a;
#endif
	}

#ifdef MESH_SIMPLIFY_DEBUG_LOGGING
	print_line("\tevaluating edge " + itos(edge.get_collapse_from()) + " to " + itos(edge.wedge_to_collapse_to) + " ... cost " + itos(edge.get_readable_cost()) + " .. from cost " + itos(edge.translate_readable_cost(from_cost)));
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
	e.is_seam_or_boundary = (count <= 1);
}

void MeshSimplify::_get_edges_touching_wedge(uint32_t p_wedge_id, LocalVector<uint32_t> &r_edges) const {
	const Wedge &w = data.wedges[p_wedge_id];
	for (uint32_t i = 0; i < w.verts.size(); i++) {
		uint32_t v_id = w.verts[i];
		if (!data.verts[v_id].active) {
			continue;
		}
		for (uint32_t n = 0; n < data.verts[v_id].tris.size(); n++) {
			const Tri &t = data.tris[data.verts[v_id].tris[n]];
			if (!t.active) {
				continue;
			}
			for (uint32_t j = 0; j < 3; j++) {
				uint32_t eid = t.edge_ids[j];
				const Edge &e = data.edges[eid];
				if (e.a == p_wedge_id || e.b == p_wedge_id) {
					r_edges.push_back_if_not_present(eid);
				}
			}
		}
	}
}

void MeshSimplify::_refresh_edge_seam_flag(uint32_t p_edge_id) {
	Edge &e = data.edges[p_edge_id];
	if (!e.active) {
		return;
	}

	if (e.triangle_count == 0) {
		e.active = false;
		e.is_seam_or_boundary = false;
		return;
	}

	e.is_seam_or_boundary = (e.triangle_count == 1) || (e.triangle_count > 2);
}

void MeshSimplify::_reclassify_wedge(uint32_t p_wedge_id) {
	Wedge &wedge = data.wedges[p_wedge_id];
	uint32_t active_wedge_size = _count_active_wedge_verts(p_wedge_id);
	if (active_wedge_size == 0) {
		return;
	}

	LocalVector<uint32_t> touching_edges;
	_get_edges_touching_wedge(p_wedge_id, touching_edges);

	struct WedgeConnection {
		uint32_t other_wedge;
		uint32_t total_triangle_count = 0;
		uint32_t seam_edge_count = 0;
		uint32_t nonmanifold_edge_count = 0;
	};

	std::map<uint32_t, WedgeConnection> connections;

	for (uint32_t i = 0; i < touching_edges.size(); i++) {
		const Edge &e = data.edges[touching_edges[i]];
		if (!e.active) {
			continue;
		}

		uint32_t other_wedge = (e.a == p_wedge_id) ? e.b : e.a;

		WedgeConnection &conn = connections[other_wedge];
		conn.other_wedge = other_wedge;
		conn.total_triangle_count = e.triangle_count;

		if (e.triangle_count > 2) {
			conn.nonmanifold_edge_count++;
		} else {
			// Find active attribute Vert pairs (v_a, v_b) where v_a is in p_wedge_id and v_b is in other_wedge,
			// and count their occurrences across active triangles using this spatial edge to detect seams.
			std::map<std::pair<uint32_t, uint32_t>, uint32_t> vert_pair_counts;

			const Wedge &w = data.wedges[p_wedge_id];
			for (uint32_t v_idx = 0; v_idx < w.verts.size(); v_idx++) {
				uint32_t v_id = w.verts[v_idx];
				if (!data.verts[v_id].active) {
					continue;
				}
				const LocalVector<uint32_t> &v_tris = data.verts[v_id].tris;
				for (uint32_t t_idx = 0; t_idx < v_tris.size(); t_idx++) {
					uint32_t tri_id = v_tris[t_idx];
					const Tri &tri = data.tris[tri_id];
					if (!tri.active) {
						continue;
					}

					bool has_this_edge = false;
					for (uint32_t edge_idx = 0; edge_idx < 3; edge_idx++) {
						if (tri.edge_ids[edge_idx] == touching_edges[i]) {
							has_this_edge = true;
							break;
						}
					}

					if (has_this_edge) {
						uint32_t v_other = UINT32_MAX;
						for (uint32_t c = 0; c < 3; c++) {
							if (data.verts[tri.corn[c]].wedge == other_wedge) {
								v_other = tri.corn[c];
								break;
							}
						}

						if (v_other != UINT32_MAX) {
							uint32_t v_min = MIN(v_id, v_other);
							uint32_t v_max = MAX(v_id, v_other);
							vert_pair_counts[std::make_pair(v_min, v_max)]++;
						}
					}
				}
			}

			for (std::map<std::pair<uint32_t, uint32_t>, uint32_t>::const_iterator vp_it = vert_pair_counts.begin(); vp_it != vert_pair_counts.end(); ++vp_it) {
				if (vp_it->second == 1) {
					conn.seam_edge_count++;
				}
			}
		}
	}

	uint32_t true_border_count = 0;
	uint32_t seam_pair_count = 0;
	uint32_t nonmanifold_count = 0;

	wedge.seam_neighbour_wedges.clear();
	wedge.is_seam_or_boundary = false;

	for (std::map<uint32_t, WedgeConnection>::const_iterator it = connections.begin(); it != connections.end(); ++it) {
		uint32_t other_w = it->first;
		const WedgeConnection &conn = it->second;

		if (conn.total_triangle_count > 2 || conn.nonmanifold_edge_count > 0) {
			nonmanifold_count++;
			wedge.is_seam_or_boundary = true;
		} else if (conn.total_triangle_count == 1) {
			true_border_count++;
			wedge.is_seam_or_boundary = true;
			wedge.seam_neighbour_wedges.push_back(other_w);
		} else if (conn.total_triangle_count == 2) {
			if (conn.seam_edge_count == 2) {
				seam_pair_count++;
				wedge.is_seam_or_boundary = true;
				wedge.seam_neighbour_wedges.push_back(other_w);
			}
		}
	}

	// Dynamic Layout Flags to guarantee topology preservation across types
	wedge.is_border = (true_border_count > 0);
	wedge.is_seam = (seam_pair_count > 0);

	if (nonmanifold_count > 0) {
		_change_wedge_type(p_wedge_id, Wedge::Type::LOCKED);
	} else if (true_border_count > 0) {
		// If a wedge is on the geometric border but has an attribute seam (active_wedge_size > 1),
		// lock it entirely to prevent its collapse from pulling the border into the mesh.
		if (active_wedge_size > 1) {
			_change_wedge_type(p_wedge_id, Wedge::Type::LOCKED);
		} else {
			// Any active wedge on a boundary (true_border_count > 0) is classified as BORDER,
			// preventing boundary endpoints or corners from degrading into COMPLEX.
			_change_wedge_type(p_wedge_id, Wedge::Type::BORDER);
		}
	} else if (seam_pair_count > 0) {
		_change_wedge_type(p_wedge_id, (seam_pair_count == 2 && active_wedge_size == 2) ? Wedge::Type::SEAM : Wedge::Type::COMPLEX);
	} else if (active_wedge_size > 1) {
		_change_wedge_type(p_wedge_id, Wedge::Type::COMPLEX);
	} else {
		_change_wedge_type(p_wedge_id, Wedge::Type::MANIFOLD);
	}
}

void MeshSimplify::_change_wedge_type(uint32_t p_wedge_id, Wedge::Type p_type) {
	// Just a wrapper to allow debugging.
	Wedge &wedge = data.wedges[p_wedge_id];

	if (wedge.type != p_type) {
		// print_line("changing wedge " + itos(p_wedge_id) + " from type [" + itos((uint32_t)wedge.type) + "] to [" + itos((uint32_t)p_type) + "]");
	}

	wedge.type = p_type;
}

void MeshSimplify::_rebuild_triangle_edge_ids() {
	for (uint32_t t = 0; t < data.tris.size(); ++t) {
		Tri &tri = data.tris[t];
		if (!tri.active)
			continue;

		uint32_t w0 = data.verts[tri.corn[0]].wedge;
		uint32_t w1 = data.verts[tri.corn[1]].wedge;
		uint32_t w2 = data.verts[tri.corn[2]].wedge;

		tri.edge_ids[0] = _get_or_create_edge(w0, w1, t);
		tri.edge_ids[1] = _get_or_create_edge(w1, w2, t);
		tri.edge_ids[2] = _get_or_create_edge(w2, w0, t);
	}
}

void MeshSimplify::_detect_seam_edges() {
	for (uint32_t i = 0; i < data.edges.size(); i++) {
		data.edges[i].is_seam_or_boundary = false;
		data.edges[i].triangle_count = 0;
	}

	for (uint32_t i = 0; i < data.wedges.size(); i++) {
		data.wedges[i].seam_neighbour_wedges.clear();
		data.wedges[i].type = Wedge::Type::MANIFOLD;
		data.wedges[i].is_seam_or_boundary = false;
		data.wedges[i].is_border = false;
		data.wedges[i].is_seam = false;
	}

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

	for (uint32_t i = 0; i < data.edges.size(); i++) {
		Edge &e = data.edges[i];
		if (!e.active) {
			continue;
		}

		if (e.triangle_count <= 1 || e.triangle_count > 2) {
			e.is_seam_or_boundary = true;
		}
	}

	for (uint32_t n = 0; n < data.wedges.size(); n++) {
		_reclassify_wedge(n);
	}

	// Capture the original classifications once during generation
	for (uint32_t n = 0; n < data.wedges.size(); n++) {
		data.wedges[n].original_type = data.wedges[n].type;
		data.wedges[n].original_is_border = data.wedges[n].is_border;
		data.wedges[n].original_is_seam = data.wedges[n].is_seam;
		data.wedges[n].original_is_seam_or_boundary = data.wedges[n].is_seam_or_boundary;
	}

#define MESH_SIMPLIFY_COUNT_VERT_TYPES
#ifdef MESH_SIMPLIFY_COUNT_VERT_TYPES
	uint32_t wedge_type_count[(uint32_t)Wedge::Type::MAX] = {};
	for (uint32_t n = 0; n < data.wedges.size(); n++) {
		if (_count_active_wedge_verts(n) > 0) {
			wedge_type_count[(uint32_t)data.wedges[n].type]++;
			print_line("\twedge " + itos(n) + " type is " + itos((uint32_t)data.wedges[n].type));
		}
	}
	print_line("manifold wedges : " + itos(wedge_type_count[(uint32_t)Wedge::Type::MANIFOLD]));
	print_line("border wedges : " + itos(wedge_type_count[(uint32_t)Wedge::Type::BORDER]));
	print_line("seam wedges : " + itos(wedge_type_count[(uint32_t)Wedge::Type::SEAM]));
	print_line("complex wedges : " + itos(wedge_type_count[(uint32_t)Wedge::Type::COMPLEX]));
	print_line("locked wedges : " + itos(wedge_type_count[(uint32_t)Wedge::Type::LOCKED]));
#endif
}

void MeshSimplify::_debug_sanity_check() {
	return;
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

	for (uint32_t n = 0; n < data.verts.size(); n++) {
		if (!data.verts[n].active) {
			continue;
		}

		const LocalVector<uint32_t> &true_list = vert_tri_lists[n];
		const LocalVector<uint32_t> &check_list = data.verts[n].tris;

		DEV_ASSERT(true_list.size() == check_list.size());

		for (uint32_t i = 0; i < true_list.size(); i++) {
			DEV_ASSERT(check_list.find(true_list[i]) != -1);
		}
	}
#endif
}

void MeshSimplify::_rebuild_vertex_wedges() {
	data.wedges.clear();

	data.wedges.reserve(data.verts.size());

	const uint32_t NUM_BUCKETS = 1024 * 5;
	LocalVector<LocalVector<uint32_t>> wedge_buckets;
	wedge_buckets.resize(NUM_BUCKETS);

	for (uint32_t n = 0; n < data.verts.size(); n++) {
		Vert &vert = data.verts[n];

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
				vert.wedge = wedge_id;

				wedge.verts.push_back(n);
				found = true;
				break;
			}
		}

		if (!found) {
			uint32_t new_wedge_id = data.wedges.size();
			data.wedges.resize(data.wedges.size() + 1);
			Wedge &wedge = data.wedges[new_wedge_id];

			vert.wedge = new_wedge_id;

			wedge_buckets[hash].push_back(new_wedge_id);

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

	data.tris.resize(num_orig_tris);

	uint32_t index_count = 0;
	uint32_t valid_tri_count = 0;

	for (uint32_t t = 0; t < num_orig_tris; t++) {
		Tri &tri = data.tris[valid_tri_count];
		tri.corn[0] = input_data.indices[index_count++];
		tri.corn[1] = input_data.indices[index_count++];
		tri.corn[2] = input_data.indices[index_count++];

		if (_is_triangle_degenerate(tri.corn)) {
			continue;
		}

		uint32_t w0 = data.verts[tri.corn[0]].wedge;
		uint32_t w1 = data.verts[tri.corn[1]].wedge;
		uint32_t w2 = data.verts[tri.corn[2]].wedge;

		tri.edge_ids[0] = _get_or_create_edge(w0, w1, valid_tri_count);
		tri.edge_ids[1] = _get_or_create_edge(w1, w2, valid_tri_count);
		tri.edge_ids[2] = _get_or_create_edge(w2, w0, valid_tri_count);

		for (uint32_t c = 0; c < 3; c++) {
			Vert &v = data.verts[tri.corn[c]];
			v.active = true;
		}

		valid_tri_count++;
	}

	if (valid_tri_count) {
		print_line("Simplify valid tris " + itos(valid_tri_count) + ", degenerate " + itos(num_orig_tris - valid_tri_count));
	}

	data.tris.resize(valid_tri_count);

	for (uint32_t n = 0; n < valid_tri_count; n++) {
		_triangle_calculate_plane(n);
	}

	_build_vertex_triangle_links();
	_detect_seam_edges();

	_initialize_vertex_quadrics();
}

uint32_t MeshSimplify::_count_active_wedge_verts(uint32_t p_wedge_id) const {
	if (p_wedge_id >= data.wedges.size()) {
		return 0;
	}
	const Wedge &w = data.wedges[p_wedge_id];
	uint32_t cnt = 0;
	for (uint32_t n = 0; n < w.verts.size(); n++) {
		if (data.verts[w.verts[n]].active) {
			cnt++;
		}
	}
	return cnt;
}
