#include "mesh_deduplicator.h"

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

	ERR_FAIL_COND_V(data.attributes[data.position_attribute_id].type != ATTR_POSITION, false);
	ERR_FAIL_COND_V(!data.attributes[data.position_attribute_id].vec3.size(), false);

	data.out_attributes.clear();
	data.out_indices.clear();

	// Setup attribute internal sum indices (where on the vert to store the sums).
	memset(data.sum_index_count, 0, sizeof(data.sum_index_count));
	for (uint32_t a = 1; a < data.attributes.size(); a++) {
		AttributeStream &as = data.attributes[a];

		// Precalculate.
		as.internal_epsilon_squared = as.epsilon * as.epsilon;

		switch (as.type) {
			case ATTR_COLOR: {
				as.internal_vert_sum_index = data.sum_index_count[ATTR_COLOR]++;
			} break;
			case ATTR_FLOAT: {
				as.internal_vert_sum_index = data.sum_index_count[ATTR_FLOAT]++;
			} break;
			case ATTR_NORMAL: {
				as.internal_vert_sum_index = data.sum_index_count[ATTR_NORMAL]++;
			} break;
			case ATTR_POSITION: {
				as.internal_vert_sum_index = data.sum_index_count[ATTR_POSITION]++;
			} break;
			case ATTR_UV: {
				as.internal_vert_sum_index = data.sum_index_count[ATTR_UV]++;
			} break;
			default: {
				DEV_ASSERT(0 && "attribute not supported.");
			} break;
		}
	}

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
	LocalVector<uint32_t> index_remap;
	index_remap.resize(in_verts.size());

	for (uint32_t n = 0; n < in_verts.size(); n++) {
		const Vector3 &in_pos = in_verts[n];
		Vector3i grid_pos = grid.find_grid_pos(in_pos);

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
								const AttributeStream &as = data.attributes[a];
								switch (as.type) {
									case ATTR_COLOR: {
										//matching_vert->sum_colors[as.internal_vert_sum_index] += as.color[n];
									} break;
									case ATTR_FLOAT: {
										//matching_vert->sum_floats[as.internal_vert_sum_index] += as.float_input[n];
									} break;
									case ATTR_NORMAL: {
										//matching_vert->sum_normals[as.internal_vert_sum_index] += as.vec3[n];
									} break;
									case ATTR_POSITION: {
										if (as.vec3[n].distance_squared_to(as.vec3[sid]) > as.internal_epsilon_squared) {
											reject_merge = true;
											break;
										}
									} break;
									case ATTR_UV: {
										//matching_vert->sum_uvs[as.internal_vert_sum_index] += as.vec2[n];
									} break;
									default: {
										DEV_ASSERT(0 && "attribute not supported.");
									} break;
								}
							} // for a
							if (reject_merge) {
								break;
							}

						} // for h

						//						if (test_vert.average_pos.distance_squared_to(in_pos) > in_verts_epsilon) {
						//							continue;
						//						}

						// Close enough to merge.
						// Check other attributes NYI.
						matching_vert = &test_vert;
						index_remap[n] = test_vert_id;
					}
				}
			}
		}

		if (matching_vert) {
			// Can be combined.
			matching_vert->source_vert_ids.push_back(n);
#if 0
			matching_vert->sum_pos += in_pos;
			matching_vert->count += 1;
			matching_vert->average_pos = matching_vert->sum_pos / matching_vert->count;

			// Sum attributes.
			for (uint32_t a = 1; a < data.attributes.size(); a++) {
				const AttributeStream &as = data.attributes[a];
				switch (as.type) {
					case ATTR_COLOR: {
						matching_vert->sum_colors[as.internal_vert_sum_index] += as.color[n];
					} break;
					case ATTR_FLOAT: {
						matching_vert->sum_floats[as.internal_vert_sum_index] += as.float_input[n];
					} break;
					case ATTR_NORMAL: {
						matching_vert->sum_normals[as.internal_vert_sum_index] += as.vec3[n];
					} break;
					case ATTR_POSITION: {
						matching_vert->sum_positions[as.internal_vert_sum_index] += as.vec3[n];
					} break;
					case ATTR_UV: {
						matching_vert->sum_uvs[as.internal_vert_sum_index] += as.vec2[n];
					} break;
					default: {
						DEV_ASSERT(0 && "attribute not supported.");
					} break;
				}
			}
#endif
			// print_line("Combining with vert in bucket " + itos(bucket));
		} else {
			// If it can't be combined, create new one...
			uint32_t new_vert_id = grid.verts.size();

			grid.verts.resize(new_vert_id + 1);
			Vert &new_vert = grid.verts[new_vert_id];
			new_vert.source_vert_ids.push_back(n);

#if 0			
			new_vert.sum_pos = in_pos;
			new_vert.average_pos = in_pos;
			new_vert.count = 1;

			// Attributes
			new_vert.sum_colors.resize(data.sum_index_count[ATTR_COLOR]);
			new_vert.sum_floats.resize(data.sum_index_count[ATTR_FLOAT]);
			new_vert.sum_normals.resize(data.sum_index_count[ATTR_NORMAL]);
			new_vert.sum_positions.resize(data.sum_index_count[ATTR_POSITION]);
			new_vert.sum_uvs.resize(data.sum_index_count[ATTR_UV]);

			for (uint32_t a = 1; a < data.attributes.size(); a++) {
				const AttributeStream &as = data.attributes[a];
				switch (as.type) {
					case ATTR_COLOR: {
						new_vert.sum_colors[as.internal_vert_sum_index] = as.color[n];
					} break;
					case ATTR_FLOAT: {
						new_vert.sum_floats[as.internal_vert_sum_index] = as.float_input[n];
					} break;
					case ATTR_NORMAL: {
						new_vert.sum_normals[as.internal_vert_sum_index] = as.vec3[n];
					} break;
					case ATTR_POSITION: {
						new_vert.sum_positions[as.internal_vert_sum_index] = as.vec3[n];
					} break;
					case ATTR_UV: {
						new_vert.sum_uvs[as.internal_vert_sum_index] = as.vec2[n];
					} break;
					default: {
						DEV_ASSERT(0 && "attribute not supported.");
					} break;
				}
			}
#endif
			// print_line("Adding vert to bucket " + itos(bucket));
			grid.buckets[bucket].vert_ids.push_back(new_vert_id);

			index_remap[n] = new_vert_id;
		}
	}

	//////////////////////////////////////////////////
	// Store unique verts.

	// Prepare output attributes.
	data.out_attributes.resize(data.attributes.size());

	for (uint32_t a = 0; a < data.attributes.size(); a++) {
		// Input stream, output stream.
		const AttributeStream &is = data.attributes[a];
		AttributeStream &os = data.out_attributes[a];
		os.type = is.type;
		os.name = is.name;

		uint32_t num_grid_verts = grid.verts.size();

		// Most of these will be zero, but the used one will be non-zero.
		os.color.resize(is.color.size() ? num_grid_verts : 0);
		os.float_input.resize(is.float_input.size() ? num_grid_verts : 0);
		os.vec2.resize(is.vec2.size() ? num_grid_verts : 0);
		os.vec3.resize(is.vec3.size() ? num_grid_verts : 0);
	}

	//AttributeStream &out_positions = data.out_attributes[0];

	for (uint32_t n = 0; n < grid.verts.size(); n++) {
		const Vert &read_vert = grid.verts[n];
		uint32_t num_merged_verts = read_vert.source_vert_ids.size();

		// Combine the attributes of each merged vert.
		for (uint32_t m = 0; m < num_merged_verts; m++) {
			// Source merged vert id.
			uint32_t sid = read_vert.source_vert_ids[m];
			for (uint32_t a = 1; a < data.attributes.size(); a++) {
				// Input stream, output stream.
				const AttributeStream &is = data.attributes[a];
				AttributeStream &os = data.out_attributes[a];

				// Watch for precision issues with the divide.
				// We could multiply, or do a single divide at the end.
				switch (is.type) {
					case ATTR_COLOR: {
						os.color[n] += is.color[sid] / num_merged_verts;
					} break;
					case ATTR_FLOAT: {
						os.float_input[n] += is.float_input[sid] / num_merged_verts;
					} break;
					case ATTR_NORMAL: {
						os.vec3[n] += is.vec3[sid] / num_merged_verts;
					} break;
					case ATTR_POSITION: {
						os.vec3[n] += is.vec3[sid] / num_merged_verts;
					} break;
					case ATTR_UV: {
						os.vec2[n] += is.vec2[sid] / num_merged_verts;
					} break;
					default: {
						DEV_ASSERT(0 && "attribute not supported.");
					} break;
				} // for a
			} // for m

#if 0
		out_positions.vec3[n] = read_vert.average_pos;

		// Store unique vert attributes.
		for (uint32_t a = 1; a < data.attributes.size(); a++) {
			// Input stream, output stream.
			const AttributeStream &is = data.attributes[a];
			AttributeStream &os = data.out_attributes[a];

			switch (is.type) {
				case ATTR_COLOR: {
					os.color[n] = read_vert.sum_colors[is.internal_vert_sum_index] / read_vert.count;
				} break;
				case ATTR_FLOAT: {
					os.float_input[n] = read_vert.sum_floats[is.internal_vert_sum_index] / read_vert.count;
				} break;
				case ATTR_NORMAL: {
					os.vec3[n] = read_vert.sum_normals[is.internal_vert_sum_index] / read_vert.count;
				} break;
				case ATTR_POSITION: {
					os.vec3[n] = read_vert.sum_positions[is.internal_vert_sum_index] / read_vert.count;
				} break;
				case ATTR_UV: {
					os.vec2[n] = read_vert.sum_uvs[is.internal_vert_sum_index] / read_vert.count;
				} break;
				default: {
					DEV_ASSERT(0 && "attribute not supported.");
				} break;
			}
#endif
		}
	}

	// Store the final indices now referring to the unique verts.
	data.out_indices.resize(p_indices.size());
	for (uint32_t n = 0; n < p_indices.size(); n++) {
		data.out_indices[n] = index_remap[p_indices[n]];
	}

	print_line("Verts before " + itos(in_verts.size()) + ", after " + itos(data.out_attributes[data.position_attribute_id].vec3.size()));

	return true;
}

#if 0
bool MeshDeduplicator::deduplicate_verts(const Span<uint32_t> &p_in_inds, const Span<Vector3> &p_in_verts, LocalVector<Vector3> &r_out_verts, LocalVector<uint32_t> &r_out_inds, real_t p_epsilon) {
	if (p_in_verts.is_empty() || p_in_inds.is_empty()) {
		return true;
	}

	// Find bounds.
	grid.bound.position = p_in_verts[0];
	grid.bound.size = Vector3();

	for (uint32_t n = 1; n < p_in_verts.size(); n++) {
		grid.bound.expand_to(p_in_verts[n]);
	}

	// Create in verts, and find their grid pos.
	grid.in_verts.resize(p_in_verts.size());
	grid.prepare();

	for (uint32_t n = 0; n < p_in_verts.size(); n++) {
		Vert &v = grid.in_verts[n];
		v.pos = p_in_verts[n];
		v.grid_pos = grid.find_grid_pos(v.pos);

		// Assign in verts to buckets.
		uint32_t bucket = grid.hash_grid_pos(v.grid_pos);
		grid.buckets[bucket].vert_ids.push_back(n);

		print_line("Adding vert to bucket " + itos(bucket));
	}

	return true;
}
#endif
