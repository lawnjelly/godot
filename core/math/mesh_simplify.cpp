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
	data.positions.resize(in_verts.size());

	for (uint32_t n = 0; n < in_verts.size(); n++) {
		data.positions[n] = data.find_grid_pos(in_verts[n]);
	}

	return true;
}
