private:
//////////////////////////////////////////////////////////////
// TEMPLATE FUNCS

void _translate_batches_to_vertex_colored_FVF() {
	// zeros the size and sets up how big each unit is
	bdata.unit_vertices.prepare(sizeof(BatchVertexColored));

	const BatchColor *source_vertex_colors = &bdata.vertex_colors[0];
	CRASH_COND(bdata.vertex_colors.size() != bdata.vertices.size());

	int num_verts = bdata.vertices.size();

	for (int n = 0; n < num_verts; n++) {
		const BatchVertex &bv = bdata.vertices[n];

		BatchVertexColored *cv = (BatchVertexColored *)bdata.unit_vertices.request();

		cv->pos = bv.pos;
		cv->uv = bv.uv;
		cv->col = *source_vertex_colors++;
	}
}

// Translation always involved adding color to the FVF, which enables
// joining of batches that have different colors.
// There is a trade off. Non colored verts are smaller so work faster, but
// there comes a point where it is better to just use colored verts to avoid lots of
// batches.
// In addition this can optionally add light angles to the FVF, necessary for normal mapping.
template <class BATCH_VERTEX_TYPE, bool INCLUDE_LIGHT_ANGLES, bool INCLUDE_MODULATE, bool INCLUDE_LARGE>
void _translate_batches_to_larger_FVF() {

	// zeros the size and sets up how big each unit is
	bdata.unit_vertices.prepare(sizeof(BATCH_VERTEX_TYPE));
	bdata.batches_temp.reset();

	// As the vertices_colored and batches_temp are 'mirrors' of the non-colored version,
	// the sizes should be equal, and allocations should never fail. Hence the use of debug
	// asserts to check program flow, these should not occur at runtime unless the allocation
	// code has been altered.
#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
	CRASH_COND(bdata.unit_vertices.max_size() != bdata.vertices.max_size());
	CRASH_COND(bdata.batches_temp.max_size() != bdata.batches.max_size());
#endif

	Color curr_col(-1.0, -1.0, -1.0, -1.0);

	Batch *dest_batch = 0;

	const float *source_light_angles = &bdata.light_angles[0];
	const BatchColor *source_vertex_modulates = &bdata.vertex_modulates[0];
	const BatchTransform *source_vertex_transforms = &bdata.vertex_transforms[0];

	// translate the batches into vertex colored batches
	for (int n = 0; n < bdata.batches.size(); n++) {
		const Batch &source_batch = bdata.batches[n];

		// does source batch use light angles?
		const BatchTex &btex = bdata.batch_textures[source_batch.batch_texture_id];
		bool source_batch_uses_light_angles = btex.RID_normal != RID();

		bool needs_new_batch = true;

		if (dest_batch) {
			if (dest_batch->type == source_batch.type) {
				if (source_batch.type == RasterizerStorageCommon::BT_RECT) {
					if (dest_batch->batch_texture_id == source_batch.batch_texture_id) {
						// add to previous batch
						dest_batch->num_commands += source_batch.num_commands;
						needs_new_batch = false;

						// create the colored verts (only if not default)
						//int first_vert = source_batch.first_quad * 4;
						//int end_vert = 4 * (source_batch.first_quad + source_batch.num_commands);
						int first_vert = source_batch.first_vert;
						int end_vert = first_vert + (4 * source_batch.num_commands);

						for (int v = first_vert; v < end_vert; v++) {
							const BatchVertex &bv = bdata.vertices[v];
							BATCH_VERTEX_TYPE *cv = (BATCH_VERTEX_TYPE *)bdata.unit_vertices.request();
#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
							CRASH_COND(!cv);
#endif
							cv->pos = bv.pos;
							cv->uv = bv.uv;
							cv->col = source_batch.color;

							if (INCLUDE_LIGHT_ANGLES) {
								// this is required to allow compilation with non light angle vertex.
								// it should be compiled out.
								BatchVertexLightAngled *lv = (BatchVertexLightAngled *)cv;
								if (source_batch_uses_light_angles)
									lv->light_angle = *source_light_angles++;
								else
									lv->light_angle = 0.0f; // dummy, unused in vertex shader (could possibly be left uninitialized, but probably bad idea)
							} // if including light angles

							if (INCLUDE_MODULATE) {
								BatchVertexModulated *mv = (BatchVertexModulated *)cv;
								mv->modulate = *source_vertex_modulates++;
							} // including modulate

							if (INCLUDE_LARGE) {
								BatchVertexLarge *lv = (BatchVertexLarge *)cv;
								lv->transform = *source_vertex_transforms++;
							} // if including large
						}
					} // textures match
				} else {
					// default
					// we can still join, but only under special circumstances
					// does this ever happen? not sure at this stage, but left for future expansion
					uint32_t source_last_command = source_batch.first_command + source_batch.num_commands;
					if (source_last_command == dest_batch->first_command) {
						dest_batch->num_commands += source_batch.num_commands;
						needs_new_batch = false;
					} // if the commands line up exactly
				}
			} // if both batches are the same type

		} // if dest batch is valid

		if (needs_new_batch) {
			dest_batch = bdata.batches_temp.request();
#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
			CRASH_COND(!dest_batch);
#endif

			*dest_batch = source_batch;

			// create the colored verts (only if not default)
			if (source_batch.type != RasterizerStorageCommon::BT_DEFAULT) {
				//					int first_vert = source_batch.first_quad * 4;
				//					int end_vert = 4 * (source_batch.first_quad + source_batch.num_commands);
				int first_vert = source_batch.first_vert;
				int end_vert = first_vert + (4 * source_batch.num_commands);

				for (int v = first_vert; v < end_vert; v++) {
					const BatchVertex &bv = bdata.vertices[v];
					BATCH_VERTEX_TYPE *cv = (BATCH_VERTEX_TYPE *)bdata.unit_vertices.request();
#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
					CRASH_COND(!cv);
#endif
					cv->pos = bv.pos;
					cv->uv = bv.uv;
					cv->col = source_batch.color;

					if (INCLUDE_LIGHT_ANGLES) {
						// this is required to allow compilation with non light angle vertex.
						// it should be compiled out.
						BatchVertexLightAngled *lv = (BatchVertexLightAngled *)cv;
						if (source_batch_uses_light_angles)
							lv->light_angle = *source_light_angles++;
						else
							lv->light_angle = 0.0f; // dummy, unused in vertex shader (could possibly be left uninitialized, but probably bad idea)
					} // if using light angles

					if (INCLUDE_MODULATE) {
						BatchVertexModulated *mv = (BatchVertexModulated *)cv;
						mv->modulate = *source_vertex_modulates++;
					} // including modulate

					if (INCLUDE_LARGE) {
						BatchVertexLarge *lv = (BatchVertexLarge *)cv;
						lv->transform = *source_vertex_transforms++;
					} // if including large
				}
			}
		}
	}

	// copy the temporary batches to the master batch list (this could be avoided but it makes the code cleaner)
	bdata.batches.copy_from(bdata.batches_temp);
}

/*
	// convert the stupidly high amount of batches (each with its own color)
	// to larger batches where the color is stored in the verts instead...
	// There is a trade off. Non colored verts are smaller so work faster, but
	// there comes a point where it is better to just use colored verts to avoid lots of
	// batches.
	void _batch_translate_to_colored() {
		bdata.vertices_colored.reset();
		bdata.batches_temp.reset();

		// As the vertices_colored and batches_temp are 'mirrors' of the non-colored version,
		// the sizes should be equal, and allocations should never fail. Hence the use of debug
		// asserts to check program flow, these should not occur at runtime unless the allocation
		// code has been altered.
#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
		CRASH_COND(bdata.vertices_colored.max_size() != bdata.vertices.max_size());
		CRASH_COND(bdata.batches_temp.max_size() != bdata.batches.max_size());
	#endif

		Color curr_col(-1.0, -1.0, -1.0, -1.0);

		Batch *dest_batch = 0;

		// translate the batches into vertex colored batches
		for (int n = 0; n < bdata.batches.size(); n++) {
			const Batch &source_batch = bdata.batches[n];

			bool needs_new_batch = true;

			if (dest_batch) {
				if (dest_batch->type == source_batch.type) {
					if (source_batch.type == RasterizerStorageCommon::BT_RECT) {
						if (dest_batch->batch_texture_id == source_batch.batch_texture_id) {
							// add to previous batch
							dest_batch->num_commands += source_batch.num_commands;
							needs_new_batch = false;

							// create the colored verts (only if not default)
							int first_vert = source_batch.first_quad * 4;
							int end_vert = 4 * (source_batch.first_quad + source_batch.num_commands);

							for (int v = first_vert; v < end_vert; v++) {
								const BatchVertex &bv = bdata.vertices[v];
								BatchVertexColored *cv = bdata.vertices_colored.request();
#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
								CRASH_COND(!cv);
	#endif
								cv->pos = bv.pos;
								cv->uv = bv.uv;
								cv->col = source_batch.color;
							}
						} // textures match
					} else {
						// default
						// we can still join, but only under special circumstances
						// does this ever happen? not sure at this stage, but left for future expansion
						uint32_t source_last_command = source_batch.first_command + source_batch.num_commands;
						if (source_last_command == dest_batch->first_command) {
							dest_batch->num_commands += source_batch.num_commands;
							needs_new_batch = false;
						} // if the commands line up exactly
					}
				} // if both batches are the same type

			} // if dest batch is valid

			if (needs_new_batch) {
				dest_batch = bdata.batches_temp.request();
#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
				CRASH_COND(!dest_batch);
	#endif

				*dest_batch = source_batch;

				// create the colored verts (only if not default)
				if (source_batch.type != RasterizerStorageCommon::BT_DEFAULT) {
					int first_vert = source_batch.first_quad * 4;
					int end_vert = 4 * (source_batch.first_quad + source_batch.num_commands);

					for (int v = first_vert; v < end_vert; v++) {
						const BatchVertex &bv = bdata.vertices[v];
						BatchVertexColored *cv = bdata.vertices_colored.request();
#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
						CRASH_COND(!cv);
	#endif
						cv->pos = bv.pos;
						cv->uv = bv.uv;
						cv->col = source_batch.color;
					}
				}
			}
		}

		// copy the temporary batches to the master batch list (this could be avoided but it makes the code cleaner)
		bdata.batches.copy_from(bdata.batches_temp);
	}
	*/
