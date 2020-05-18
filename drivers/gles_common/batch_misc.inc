///////////////////////////////////////////
// curiously recurring template pattern
T_STORAGE * get_storage()
{
	return static_cast<const T*>(this)->storage;
}

const T_STORAGE * get_storage() const
{
	return static_cast<const T*>(this)->storage;
}

T * get_this()
{
	return static_cast<T*>(this);
}

const T * get_this() const
{
	return static_cast<const T*>(this);
}
///////////////////////////////////////////

Batch * _batch_request_new(bool p_blank = true) {
	Batch *batch = bdata.batches.request();
	if (!batch) {
		// grow the batches
		bdata.batches.grow();

		// and the temporary batches (used for color verts)
		bdata.batches_temp.reset();
		bdata.batches_temp.grow();

		// this should always succeed after growing
		batch = bdata.batches.request();
#ifdef DEBUG_ENABLED
		CRASH_COND(!batch);
#endif
	}

	if (p_blank)
		memset(batch, 0, sizeof(Batch));

	return batch;
}

BatchVertex *_batch_vertex_request_new()
{
	return bdata.vertices.request();
}

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
#ifdef DEBUG_ENABLED
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
				if (source_batch.type == Batch::BT_RECT) {
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
#ifdef DEBUG_ENABLED
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
#ifdef DEBUG_ENABLED
			CRASH_COND(!dest_batch);
#endif

			*dest_batch = source_batch;

			// create the colored verts (only if not default)
			if (source_batch.type != Batch::BT_DEFAULT) {
				int first_vert = source_batch.first_quad * 4;
				int end_vert = 4 * (source_batch.first_quad + source_batch.num_commands);

				for (int v = first_vert; v < end_vert; v++) {
					const BatchVertex &bv = bdata.vertices[v];
					BatchVertexColored *cv = bdata.vertices_colored.request();
#ifdef DEBUG_ENABLED
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
