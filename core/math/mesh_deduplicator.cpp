#include "mesh_deduplicator.h"

//#define GODOT_MESH_DEDUPLICATOR_DEBUG_LOGGING
#ifdef GODOT_MESH_DEDUPLICATOR_DEBUG_LOGGING
#define GMD_LOG(a) print_line(a)
#else
#define GMD_LOG(a)
#endif

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
	ERR_FAIL_COND_V(data.position_attribute_id >= data.attributes.size(), false);
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

	grid.bound.size.x = MAX(grid.bound.size.x, min_bound);
	grid.bound.size.y = MAX(grid.bound.size.y, min_bound);
	grid.bound.size.z = MAX(grid.bound.size.z, min_bound);

	real_t bound_max_dimension = grid.bound.size.get_axis(grid.bound.size.max_axis());

	// Dynamically compute grid_size based on position epsilon so that
	// the fixed small neighbor search (+/-2) reliably covers the epsilon ball.
	// This makes the position epsilon actually control the weld distance.
	// We clamp to [4, 65535] to preserve the original "65535 integer range accuracy"
	// for fine positioning when epsilon is small.
	const MeshAttributeStream &pos_as = data.attributes[data.position_attribute_id];
	real_t pos_epsilon = MAX(pos_as.epsilon, (real_t)0.001f);

	grid.grid_size = 4;
	grid.grid_size = (uint32_t)(bound_max_dimension / pos_epsilon + 0.5f);
	print_line("DeDuplication selected grid size : " + itos(grid.grid_size));
	grid.grid_size = CLAMP(grid.grid_size, 4, 65535);

	// Create in verts, and find their grid pos.
	grid.verts.clear();
	grid.prepare();

	// We need a map going from the original vertices to the attribute verts,
	// so that a new list of indices can be output.
	LocalVector<uint32_t> vertex_remap;
	vertex_remap.resize(in_verts.size());
	vertex_remap.fill(UINT32_MAX);

	real_t position_epsilon_squared = data.attributes[0].internal_epsilon_squared;

	for (uint32_t n = 0; n < in_verts.size(); n++) {
		const Vector3 &in_pos = in_verts[n];
		Vector3i grid_pos = grid.find_grid_pos(in_pos);

		//print_line("input vertex " + itos(n) + " in_pos is " + in_pos + ", grid_pos is " + grid_pos);

		bool found_match = false;

		// Check we can fit the attribute streams inside the vert..
		// Probably need to deal with this properly at runtime later.
		DEV_ASSERT(data.attributes.size() - 1 < MAX_ATTRIBUTES_PER_VERT);

		// Create an attribute stack ahead of time for this new vertex we want to add.
		AttributeStack stack;
		for (uint32_t a = 1; a < data.attributes.size(); a++) {
			const MeshAttributeStream &as = data.attributes[a];
			stack.a[a - 1].copy_from_stream(as, n);
		}

		// Search 27 neighbouring cells.
		for (int32_t dz = -1; dz <= 1 && !found_match; dz++) {
			for (int32_t dy = -1; dy <= 1 && !found_match; dy++) {
				for (int32_t dx = -1; dx <= 1 && !found_match; dx++) {
					Vector3i test_pos = grid_pos + Vector3i(dx, dy, dz);
					uint32_t test_bucket_id = grid.hash_grid_pos(test_pos);

					const Bucket &bucket = grid.buckets[test_bucket_id];

					for (uint32_t b = 0; b < bucket.wedge_ids.size() && !found_match; b++) {
						uint32_t wedge_id = bucket.wedge_ids[b];
						Wedge &wedge = grid.wedges[wedge_id];

						// Should we test the average position, or the original position?
						// Average position creates drift.

						// Test wedge based on position only.
						// Is it within range?
						if (in_pos.distance_squared_to(wedge.position_orig) <= position_epsilon_squared) {
							// Merge the position into the existing wedge.
							wedge.position_total += in_pos;
							wedge.source_vertex_count += 1;
							wedge.position_average = wedge.position_total / wedge.source_vertex_count;

							bool merged_into_attribute_vert = false;

							// Either merge into an existing attribute Vertex,
							// or create a new one, if too different.
							for (uint32_t v = 0; v < wedge.vert_ids.size(); v++) {
								uint32_t wedge_vert_id = wedge.vert_ids[v];
								Vert &vert = grid.verts[wedge_vert_id];

								bool reject_merge = false;

								// Skip the first position stream, already done with the wedge.
								for (uint32_t a = 1; a < data.attributes.size(); a++) {
									const MeshAttributeStream &as = data.attributes[a];

									uint32_t am = a - 1;

									switch (as.type) {
										case MeshAttributeStream::ATTR_COLOR: {
											if (!vert.averages.a[am].color.is_equal_approx(stack.a[am].color)) {
												reject_merge = true;
											}
										} break;
										case MeshAttributeStream::ATTR_FLOAT: {
											real_t diff = vert.averages.a[am].flt - stack.a[am].flt;
											if (ABS(diff) > as.epsilon) {
												reject_merge = true;
											}
										} break;
										case MeshAttributeStream::ATTR_NORMAL: {
											// Use distance on (assumed unit) normal as approx for angle < epsilon (radians)
											real_t sq = vert.averages.a[am].vec3.distance_squared_to(stack.a[am].vec3);
											if (sq > as.internal_epsilon_squared) {
												reject_merge = true;
											}
										} break;
										case MeshAttributeStream::ATTR_POSITION: {
											real_t sq = vert.averages.a[am].vec3.distance_squared_to(stack.a[am].vec3);
											if (sq > as.internal_epsilon_squared) {
												reject_merge = true;
											}
										} break;
										case MeshAttributeStream::ATTR_UV: {
											real_t sq = vert.averages.a[am].vec2.distance_squared_to(stack.a[am].vec2);
											if (sq > as.internal_epsilon_squared) {
												reject_merge = true;
											}
										} break;
										default: {
											DEV_ASSERT(0 && "attribute not supported.");
										} break;
									}
								} // for a through attribute streams

								if (!reject_merge) {
									// Merge into existing attribute vert!
									merged_into_attribute_vert = true;
									vert.source_vertex_count += 1;

									vert.totals += stack;
									vert.averages = vert.totals;
									vert.averages /= vert.source_vertex_count;

									vert.source_vert_ids.push_back(n);

									vertex_remap[n] = wedge_vert_id;
								}

							} // for v through the verts on a wedge

							// Deal with the case where we need to create a new attribute vert.
							if (!merged_into_attribute_vert) {
								// Create new attribute vertex.
								uint32_t new_vert_id = grid.verts.size();
								wedge.vert_ids.push_back(new_vert_id);

								grid.verts.resize(grid.verts.size() + 1);
								Vert &vert = grid.verts[new_vert_id];
								vert.totals = stack;
								vert.averages = stack;
								vert.source_vert_ids.push_back(n);
								vert.source_vertex_count = 1;
								vert.wedge_id = wedge_id;

								vertex_remap[n] = new_vert_id;
							}

							// Close enough to merge.
							found_match = true;
							break; // stop this bucket
						} // if the wedge was close enough to merge

					} // for b through bucket verts.
				} // dx
			} // dy
		} // dz

		// No match, we need new everything.
		if (!found_match) {
			// New wedge.
			uint32_t wedge_id = grid.wedges.size();
			grid.wedges.resize(grid.wedges.size() + 1);
			Wedge &wedge = grid.wedges[wedge_id];

			wedge.position_orig = in_pos;
			wedge.position_average = in_pos;
			wedge.position_total = in_pos;
			wedge.grid_pos = grid.find_grid_pos(in_pos);

			wedge.source_vertex_count = 1;

			// Add the wedge to a bucket.
			uint32_t bucket_id = grid.hash_grid_pos(wedge.grid_pos);
			grid.buckets[bucket_id].wedge_ids.push_back(wedge_id);

			// New attribute vert.
			uint32_t vert_id = grid.verts.size();
			wedge.vert_ids.push_back(vert_id);

			grid.verts.resize(grid.verts.size() + 1);
			Vert &vert = grid.verts[vert_id];
			vert.totals = stack;
			vert.averages = stack;
			vert.source_vert_ids.push_back(n);
			vert.source_vertex_count = 1;
			vert.wedge_id = wedge_id;

			vertex_remap[n] = vert_id;
		}

		//print_line("\tvertex_remap to " + itos(vertex_remap[n]));
	}

	//////////////////////////////////////////////////
	// Copy mappings
	data.out_mapping.resize(grid.verts.size());
	for (uint32_t n = 0; n < grid.verts.size(); n++) {
		// The mapping to return will be simplified, and just contain the first source vertex.
		uint32_t orig_index = grid.verts[n].source_vert_ids[0];
		DEV_ASSERT(orig_index != UINT32_MAX);
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

		uint32_t num_attr_verts = grid.verts.size();

		// Most of these will be zero, but the used one will be non-zero.
		os.color.resize(is.color.size() ? num_attr_verts : 0);
		os.float_input.resize(is.float_input.size() ? num_attr_verts : 0);
		os.vec2.resize(is.vec2.size() ? num_attr_verts : 0);
		os.vec3.resize(is.vec3.size() ? num_attr_verts : 0);
	}

	for (uint32_t n = 0; n < grid.verts.size(); n++) {
		const Vert &attr_vert = grid.verts[n];
		const Wedge &wedge = grid.wedges[attr_vert.wedge_id];

		GMD_LOG("vert " + itos(n) + " : ");
		// The averaging of the attributes has already been done on the fly,
		// as we added them.
		for (uint32_t a = 0; a < data.attributes.size(); a++) {
			// Input stream, output stream.
			MeshAttributeStream &os = data.out_attributes[a];
			switch (os.type) {
				case MeshAttributeStream::ATTR_POSITION: {
					os.vec3[n] = wedge.position_average;
					GMD_LOG("\tpos " + String(Variant(os.vec3[n])));
				} break;
				case MeshAttributeStream::ATTR_NORMAL: {
					os.vec3[n] = attr_vert.averages.a[a - 1].vec3;
					GMD_LOG("\tnorm " + String(Variant(os.vec3[n])));
				} break;
				case MeshAttributeStream::ATTR_UV: {
					os.vec2[n] = attr_vert.averages.a[a - 1].vec2;
					GMD_LOG("\tuv " + String(Variant(os.vec2[n])));
				} break;
				case MeshAttributeStream::ATTR_COLOR: {
					os.color[n] = attr_vert.averages.a[a - 1].color;
					GMD_LOG("\tcol " + String(Variant(os.color[n])));
				} break;
				case MeshAttributeStream::ATTR_FLOAT: {
					os.float_input[n] = attr_vert.averages.a[a - 1].flt;
					GMD_LOG("\tfloat " + String(Variant(os.float_input[n])));
				} break;
				default: {
					DEV_ASSERT(0 && "attribute not supported.");
				} break;
			}

		} // for a
	}

	// Store the final indices now referring to the unique verts.
	data.out_indices.resize(p_indices.size());
	for (uint32_t n = 0; n < p_indices.size(); n++) {
		uint32_t orig_index = vertex_remap[p_indices[n]];
		DEV_ASSERT(orig_index != UINT32_MAX);
		data.out_indices[n] = orig_index;
		GMD_LOG("index " + itos(n) + " : " + itos(orig_index));
	}

	print_line("Verts before " + itos(in_verts.size()) + ", after " + itos(data.out_attributes[data.position_attribute_id].vec3.size()) + ", indices " + itos(data.out_indices.size()));
	print_line("Deduplication ratio: " + rtos(grid.verts.size() / (float)in_verts.size()));

	p_output_indices = data.out_indices;

	return true;
}

#undef GMD_LOG
