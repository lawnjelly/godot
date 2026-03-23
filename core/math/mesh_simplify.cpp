#include "mesh_simplify.h"
#include "mesh_deduplicator.h"

void MeshSimplify::declare_indices(const Span<int> &p_indices) {
	data.indices.resize(p_indices.size());
	if (p_indices.size()) {
		memcpy(data.indices.ptr(), p_indices.ptr(), p_indices.size() * sizeof(uint32_t));
		static_assert(sizeof(uint32_t) == sizeof(int), "Copying assumes int is 32 bit.");
	}
}

void MeshSimplify::declare_positions(const Span<Vector3> &p_positions) {
	data.positions = LocalVector<Vector3>(p_positions);
}

bool MeshSimplify::simplify_mesh() {
	MeshDeduplicator dd;

	LocalVector<Vector3> verts;
	LocalVector<uint32_t> inds;

	dd.set_num_attribute_streams(1);
	MeshDeduplicator::AttributeStream &as = dd.get_input_attribute_stream(0);
	as.type = MeshDeduplicator::ATTR_POSITION;
	as.vec3 = data.positions;

	dd.process(data.indices, inds);
	verts = dd.get_output_attribute_stream(0).vec3;

	return true;
}
