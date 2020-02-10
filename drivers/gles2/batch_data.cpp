#include "batch_data.h"

namespace Batch
{

BatchData::BatchData()
{
	reset_pass();
	max_quads = 0;
	vertex_buffer_size_units = 0;
	index_buffer_size_units = 0;
	use_colored_vertices = false;


	// batch buffer
		// the maximum num quads in a batch is limited by GLES2. We can have only 16 bit indices,
		// which means we can address a vertex buffer of max size 65535. 4 vertices are needed per quad.

		// Note this determines the memory use by the vertex buffer vector. max quads (65536/4)-1
		// but can be reduced to save memory if really required (will result in more batches though)
		max_quads = (65536 / 4) - 1;
		// test
		//max_quads = 64;

		uint32_t sizeof_batch_vert = sizeof(BVert);

		buffer_full = false;
		total_quads = 0;
		total_color_changes = 0;

		// 4 verts per quad
		vertex_buffer_size_units = max_quads * 4;

		// the index buffer can be longer than 65535, only the indices need to be within this range
		index_buffer_size_units = max_quads * 6;

		// this comes out at approx 64K for non-colored vertex buffer, and 128K for colored vertex buffer
		vertex_buffer_size_bytes = vertex_buffer_size_units * sizeof_batch_vert;
	//	index_buffer_size_bytes = index_buffer_size_units * 2; // 16 bit inds

		// create equal number of norma and colored verts (as the normal may need to be translated to colored)
		vertices.create(vertex_buffer_size_units); // 512k
		vertices_colored.create(vertices.max_size()); // 1024k

		// num batches will be auto increased dynamically if required
		batches.create(1024);
		batches_temp.create(batches.max_size());

		// batch textures can also be increased dynamically
		batch_textures.create(32);
/*
		// just reserve some space (may not be needed as we are orphaning, but hey ho)
		glGenBuffers(1, &bdata.gl_vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, bdata.gl_vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, bdata.vertex_buffer_size_bytes, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// pre fill index buffer, the indices never need to change so can be static
		glGenBuffers(1, &bdata.gl_index_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bdata.gl_index_buffer);

		Vector<uint16_t> indices;
		indices.resize(bdata.index_buffer_size_units);

		for (int q = 0; q < max_quads; q++) {
			int i_pos = q * 6; //  6 inds per quad
			int q_pos = q * 4; // 4 verts per quad
			indices.set(i_pos, q_pos);
			indices.set(i_pos + 1, q_pos + 1);
			indices.set(i_pos + 2, q_pos + 2);
			indices.set(i_pos + 3, q_pos);
			indices.set(i_pos + 4, q_pos + 2);
			indices.set(i_pos + 5, q_pos + 3);

			// we can only use 16 bit indices in GLES2!
#ifdef DEBUG_ENABLED
			CRASH_COND((q_pos + 3) > 65535);
#endif
		}

		glBufferData(GL_ELEMENT_ARRAY_BUFFER, bdata.index_buffer_size_bytes, &indices[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		*/

}

int BatchData::push_back_new_material(const BMaterial &mat)
{
	int size = batch_materials.size();
	batch_materials.push_back(mat);
	return size;
}

Batch * BatchData::request_new_batch(bool p_blank)
{
	Batch *batch = batches.request();
	if (!batch) {
		// grow the batches
		batches.grow();

		// and the temporary batches (used for color verts)
		batches_temp.reset();
		batches_temp.grow();

		// this should always succeed after growing
		batch = batches.request();
#ifdef DEBUG_ENABLED
		CRASH_COND(!batch);
#endif
	}

	if (p_blank)
		memset(batch, 0, sizeof(Batch));

	return batch;
}

// after each flush
void BatchData::reset_flush()
{
	batches.reset();
	batch_textures.reset();
	vertices.reset();

	buffer_full = false;
	total_quads = 0;
	total_color_changes = 0;
	use_colored_vertices = false;
}

// think in terms of every frame
void BatchData::reset_pass()
{
	// zero all the batch data ready for the next run
	batches.reset();
	batch_textures.reset();
	batch_materials.reset();
	transforms.reset();
	vertices.reset();

	buffer_full = false;
	total_quads = 0;
	total_color_changes = 0;
	use_colored_vertices = false;
}



} // namespace
