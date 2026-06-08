#include "mesh_simplify.h"
#include "mesh_deduplicator.h"

Vector3i MeshSimplify::Data::find_grid_pos(const Vector3 &p_pos) const {
	Vector3i res;

	for (uint32_t n = 0; n < 3; n++) {
		double d = (p_pos[n] - bound.position[n]) / bound_extent;
		res[n] = d * grid_size;
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

void MeshSimplify::declare_positions(const Span<Vector3> &p_positions) {
	input_data.positions = LocalVector<Vector3>(p_positions);
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

	dd.set_num_attribute_streams(1);
	MeshAttributeStream &as = dd.get_input_attribute_stream(0);
	as.set_type(MeshAttributeStream::ATTR_POSITION);
	as.vec3 = input_data.positions;

	if (!dd.process(input_data.indices, inds)) {
		return false;
	}
	verts = dd.get_output_attribute_stream(0).vec3;

	// Save the deduplicated data.
	input_data.indices = inds;
	input_data.positions = verts;

	ERR_FAIL_COND_V(!input_data.indices.size(), false);
	ERR_FAIL_COND_V(!input_data.positions.size(), false);

	// Find world bound.
	Span<Vector3> in_verts = Span<Vector3>(input_data.positions);

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
	}

	_create_tris();

	// No valid tris to simplify.
	if (!data.tris.size()) {
		return false;
	}

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
			data.edges[n].link_tri(p_triangle_id);
			return n;
		}
	}

	e.link_tri(p_triangle_id);
	data.edges.push_back(e);
	return data.edges.size() - 1;
}

void MeshSimplify::_triangle_calculate_plane(uint32_t p_tri_id) {
	Tri &tri = data.tris[p_tri_id];
	Vert &p0 = data.verts[tri.corn[0]];
	Vert &p1 = data.verts[tri.corn[1]];
	Vert &p2 = data.verts[tri.corn[2]];

	tri.plane = Plane(p0.pos(), p1.pos(), p2.pos());
}

void MeshSimplify::_initialize_vertex_quadrics() {
	// Step A: Calculate the quadric matrix for every triangle plane
	for (uint32_t n = 0; n < data.tris.size(); n++) {
		Tri &t = data.tris[n];

		// Make sure plane is up to date... (should be?)

		// Create the fundamental error matrix Kp = p * p^T
		Quadric Kp;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				Kp.m[i][j] = plane_coord(t.plane, i) * plane_coord(t.plane, j);
			}
		}

		// Step B: Accumulate this plane's matrix into its three corner vertices
		for (uint32_t i = 0; i < 3; i++) {
			Vert &v = data.verts[t.corn[i]];
			v.Q = v.Q + Kp;
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

void MeshSimplify::_evaluate_edge_collapse(uint32_t p_edge_id) {
	//	void MeshSimplify::evaluate_edge_collapse(int u_idx, int v_idx, const std::vector<Vertex>& vertices) {
	Edge &edge = data.edges[p_edge_id];

	const Vert &u = data.verts[edge.a];
	const Vert &v = data.verts[edge.b];

	// Combine the quadric error histories
	Quadric Q_new = u.Q + v.Q;

	// Evaluate the cost if we collapse EVERYTHING down to vertex U's position
	double cost_at_u = _compute_quadric_error(u.position, Q_new);

	// Evaluate the cost if we collapse EVERYTHING down to vertex V's position
	double cost_at_v = _compute_quadric_error(v.position, Q_new);

	// Pick the endpoint that preserves the local geometry best (lowest error)
	if (cost_at_u < cost_at_v) {
		edge.vertex_to_collapse_to = edge.a;
		edge.cost = cost_at_u;
	} else {
		edge.vertex_to_collapse_to = edge.b;
		edge.cost = cost_at_v;
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

		tri.edge[0] = _create_edge(tri.corn[0], tri.corn[1], valid_tri_count);
		tri.edge[1] = _create_edge(tri.corn[1], tri.corn[2], valid_tri_count);
		tri.edge[2] = _create_edge(tri.corn[2], tri.corn[0], valid_tri_count);

		// Add the tri to the verts.
		for (uint32_t c = 0; c < 3; c++) {
			Vert &v = data.verts[tri.corn[c]];
			v.active = true;
			v.link_tri(valid_tri_count);
		}

		// Tri was valid.
		valid_tri_count++;
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

	_initialize_vertex_quadrics();
}
