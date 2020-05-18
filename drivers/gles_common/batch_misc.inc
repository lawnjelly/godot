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
