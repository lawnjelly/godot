#pragma once

#include "batch_types.h"
#include "rasterizer_array.h"
#include "servers/visual/rasterizer.h"

namespace Batch
{

struct BatchData {
	BatchData();
	void reset_pass();
	void reset_flush();

	Batch * request_new_batch(bool p_blank = true);
	BTransform * request_new_transform() {return transforms.request_with_grow();}
	B128 * request_new_B128() {return generic_128s.request_with_grow();}
	int push_back_new_material(const BMaterial &mat);

	uint32_t max_quads;
	uint32_t vertex_buffer_size_units;
	uint32_t vertex_buffer_size_bytes;
	uint32_t index_buffer_size_units;

	RasterizerArray<BVert> vertices;
	RasterizerArray<BVert_colored> vertices_colored;
	RasterizerArray<Batch> batches;
	RasterizerArray<Batch> batches_temp; // used for translating to colored vertex batches
	RasterizerArray_non_pod<BTex> batch_textures; // the only reason this is non-POD is because of RIDs
	RasterizerArray_non_pod<BMaterial> batch_materials; // the only reason this is non-POD is because of RIDs
	RasterizerArray<BTransform> transforms;

	// vector to contain larger extra info for the batches such as rects and colors
	RasterizerArray<B128> generic_128s;


	bool use_colored_vertices;
	bool buffer_full;

	// counts
	int total_quads;

	// we keep a record of how many color changes caused new batches
	// if the colors are causing an excessive number of batches, we switch
	// to alternate batching method and add color to the vertex format.
	int total_color_changes;
};



} // namespace
