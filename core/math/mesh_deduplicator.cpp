#include "mesh_deduplicator.h"

void MeshAttributeStream::set_type(Type p_type, float p_epsilon) {
	type = p_type;
	if (p_epsilon >= 0) {
		epsilon = p_epsilon;
	} else {
		// Default epsilons.
		switch (type) {
			case ATTR_POSITION: {
				epsilon = 0.001f;
			} break;
			case ATTR_NORMAL: {
				epsilon = 0.01f;
			} break;
			case ATTR_UV: {
				epsilon = 0.0005f;
			} break;
			case ATTR_COLOR: {
			} break;
			case ATTR_FLOAT: {
				epsilon = 0.001f;
			} break;
			default: {
				ERR_FAIL_MSG("set_type type not supported.");
			} break;
		}
	}
	internal_epsilon_squared = epsilon * epsilon;
}

Vector3i MeshDeduplicator::Grid::find_grid_pos(const Vector3 &p_pos) const {
	Vector3i res;

	for (uint32_t n = 0; n < 3; n++) {
		double d = (p_pos[n] - bound.position[n]) / (double)bound.size[n];
		res[n] = d * grid_size;
		res[n] = CLAMP(res[n], 0, grid_size - 1);
	}

	return res;
}

void MeshDeduplicator::Grid::prepare() {
	buckets.clear();
	buckets.resize(num_buckets);
}

uint32_t MeshDeduplicator::Grid::hash_grid_pos(const Vector3i &p_pos) const {
	uint32_t hash = p_pos.hash();
	hash %= num_buckets;
	return hash;
}

bool MeshDeduplicator::process(const Span<uint32_t> &p_indices, LocalVector<uint32_t> &p_output_indices) {
	// The first attribute stream is required to be positions.
	ERR_FAIL_COND_V(!data.attributes.size(), false);
	ERR_FAIL_COND_V(!p_indices.size(), false);

	ERR_FAIL_COND_V(data.attributes[data.position_attribute_id].type != MeshAttributeStream::ATTR_POSITION, false);
	ERR_FAIL_COND_V(!data.attributes[data.position_attribute_id].vec3.size(), false);

	data.out_attributes.clear();
	data.out_indices.clear();

	Span<Vector3> in_verts = Span<Vector3>(data.attributes[data.position_attribute_id].vec3);

	// Find bounds.
	grid.bound.position = in_verts[0];
	grid.bound.size = Vector3();

	for (uint32_t n = 1; n < in_verts.size(); n++) {
		grid.bound.expand_to(in_verts[n]);
	}

	// Use some minimum bound, to prevent float error.
	const real_t min_bound = 1e-4f;

	if (grid.bound.size.length() < min_bound) {
		grid.bound.size = Vector3(min_bound, min_bound, min_bound);
	}

	// Create in verts, and find their grid pos.
	grid.verts.clear();
	grid.prepare();

	// We need a map going from the original vertices to the unique verts,
	// so that a new list of indices can be output.
	LocalVector<uint32_t> vertex_remap;
	vertex_remap.resize(in_verts.size());

	for (uint32_t n = 0; n < in_verts.size(); n++) {
		const Vector3 &in_pos = in_verts[n];
		Vector3i grid_pos = grid.find_grid_pos(in_pos);

		//print_line("input vertex " + itos(n) + " in_pos is " + in_pos + ", grid_pos is " + grid_pos);

		// Assign in verts to buckets.
		uint32_t bucket = grid.hash_grid_pos(grid_pos);

		// Can it be combined with an existing vertex in the bucket? NYI
		Vert *matching_vert = nullptr;

		// Search 27 neighbouring cells.
		for (int32_t dz = -1; dz <= 1; dz++) {
			for (int32_t dy = -1; dy <= 1; dy++) {
				for (int32_t dx = -1; dx <= 1; dx++) {
					Vector3i test_pos = grid_pos + Vector3i(dx, dy, dz);
					uint32_t test_bucket_id = grid.hash_grid_pos(test_pos);

					const Bucket &bucket = grid.buckets[test_bucket_id];

					for (uint32_t b = 0; b < bucket.vert_ids.size(); b++) {
						uint32_t test_vert_id = bucket.vert_ids[b];
						Vert &test_vert = grid.verts[test_vert_id];

						bool reject_merge = false;

						// Test each attribute of each already held vertex in turn.
						for (uint32_t h = 0; h < test_vert.source_vert_ids.size(); h++) {
							// Test source vert id.
							uint32_t sid = test_vert.source_vert_ids[h];

							for (uint32_t a = 0; a < data.attributes.size(); a++) {
								const MeshAttributeStream &as = data.attributes[a];
								switch (as.type) {
									case MeshAttributeStream::ATTR_COLOR: {
										if (!as.color[n].is_equal_approx(as.color[sid])) {
											reject_merge = true;
										}
									} break;
									case MeshAttributeStream::ATTR_FLOAT: {
										float diff = as.float_input[n] - as.float_input[sid];
										if (ABS(diff) > as.epsilon) {
											reject_merge = true;
										}
									} break;
									case MeshAttributeStream::ATTR_NORMAL: {
										//matching_vert->sum_normals[as.internal_vert_sum_index] += as.vec3[n];
									} break;
									case MeshAttributeStream::ATTR_POSITION: {
										if (as.vec3[n].distance_squared_to(as.vec3[sid]) > as.internal_epsilon_squared) {
											reject_merge = true;
										}
									} break;
									case MeshAttributeStream::ATTR_UV: {
										if (as.vec2[n].distance_squared_to(as.vec2[sid]) > as.internal_epsilon_squared) {
											reject_merge = true;
										}
									} break;
									default: {
										DEV_ASSERT(0 && "attribute not supported.");
									} break;
								}
								if (reject_merge) {
									break;
								}
							} // for a
							if (reject_merge) {
								break;
							}

						} // for h

						if (reject_merge) {
							continue;
						}

						// Close enough to merge.
						// Check other attributes NYI.
						matching_vert = &test_vert;
						vertex_remap[n] = test_vert_id;
					} // for b through bucket verts.
				} // dx
			} // dy
		} // dz

		if (matching_vert) {
			// Can be combined.
			matching_vert->source_vert_ids.push_back(n);
			// print_line("Combining with vert in bucket " + itos(bucket));
		} else {
			// If it can't be combined, create new one...
			uint32_t new_vert_id = grid.verts.size();

			grid.verts.resize(new_vert_id + 1);
			Vert &new_vert = grid.verts[new_vert_id];
			new_vert.source_vert_ids.push_back(n);

			// print_line("Adding vert to bucket " + itos(bucket));
			grid.buckets[bucket].vert_ids.push_back(new_vert_id);

			vertex_remap[n] = new_vert_id;
		}

		//print_line("\tvertex_remap to " + itos(vertex_remap[n]));
	}

	//////////////////////////////////////////////////
	// Copy mappings
	data.out_mapping.resize(grid.verts.size());
	for (uint32_t n = 0; n < grid.verts.size(); n++) {
		// The mapping to return will be simplified, and just contain the first source vertex.
		data.out_mapping[n] = grid.verts[n].source_vert_ids[0];
	}

	// Store unique verts.

	// Prepare output attributes.
	data.out_attributes.resize(data.attributes.size());

	for (uint32_t a = 0; a < data.attributes.size(); a++) {
		// Input stream, output stream.
		const MeshAttributeStream &is = data.attributes[a];
		MeshAttributeStream &os = data.out_attributes[a];
		os.type = is.type;
		os.name = is.name;

		uint32_t num_grid_verts = grid.verts.size();

		// Most of these will be zero, but the used one will be non-zero.
		os.color.resize(is.color.size() ? num_grid_verts : 0);
		os.float_input.resize(is.float_input.size() ? num_grid_verts : 0);
		os.vec2.resize(is.vec2.size() ? num_grid_verts : 0);
		os.vec3.resize(is.vec3.size() ? num_grid_verts : 0);
	}

	for (uint32_t n = 0; n < grid.verts.size(); n++) {
		const Vert &read_vert = grid.verts[n];
		uint32_t num_merged_verts = read_vert.source_vert_ids.size();

		// Combine the attributes of each merged vert.
		for (uint32_t m = 0; m < num_merged_verts; m++) {
			// Source merged vert id.
			uint32_t sid = read_vert.source_vert_ids[m];
			for (uint32_t a = 0; a < data.attributes.size(); a++) {
				// Input stream, output stream.
				const MeshAttributeStream &is = data.attributes[a];
				MeshAttributeStream &os = data.out_attributes[a];

				// Watch for precision issues with the divide.
				// We could multiply, or do a single divide at the end.
				switch (is.type) {
					case MeshAttributeStream::ATTR_COLOR: {
						os.color[n] += is.color[sid] / num_merged_verts;
					} break;
					case MeshAttributeStream::ATTR_FLOAT: {
						os.float_input[n] += is.float_input[sid] / num_merged_verts;
					} break;
					case MeshAttributeStream::ATTR_NORMAL: {
						os.vec3[n] += is.vec3[sid] / num_merged_verts;
					} break;
					case MeshAttributeStream::ATTR_POSITION: {
						os.vec3[n] += is.vec3[sid] / num_merged_verts;
					} break;
					case MeshAttributeStream::ATTR_UV: {
						os.vec2[n] += is.vec2[sid] / num_merged_verts;
					} break;
					default: {
						DEV_ASSERT(0 && "attribute not supported.");
					} break;
				} // for a
			} // for m
		}
	}

	// Store the final indices now referring to the unique verts.
	data.out_indices.resize(p_indices.size());
	for (uint32_t n = 0; n < p_indices.size(); n++) {
		data.out_indices[n] = vertex_remap[p_indices[n]];
	}

	print_line("Verts before " + itos(in_verts.size()) + ", after " + itos(data.out_attributes[data.position_attribute_id].vec3.size()) + ", indices " + itos(data.out_indices.size()));
	print_line("Deduplication ratio: " + rtos((float)in_verts.size() / grid.verts.size()));

	p_output_indices = data.out_indices;

	return true;
}
