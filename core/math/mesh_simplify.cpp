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
	// Duduplicate.
	MeshDeduplicator dd;

	LocalVector<Vector3> verts;
	LocalVector<uint32_t> inds;

	dd.set_num_attribute_streams(1);
	MeshAttributeStream &as = dd.get_input_attribute_stream(0);
	as.set_type(MeshAttributeStream::ATTR_POSITION);
	as.vec3 = input_data.positions;

	dd.process(input_data.indices, inds);
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

	if ((a == b) || (b == c) || (a == c)) {
		return true;
	}

	return false;
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

		// Tri was valid.
		valid_tri_count++;
	}

	// Resize to exact size the tri array.
	// (could possibly be omitted if we store count externally to the LocalVector)
	data.tris.resize(valid_tri_count);
}
