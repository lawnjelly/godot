#pragma once

#include "canvas_batch_types.h"
#include "rasterizer_array.h"
#include "servers/visual/rasterizer.h"

// In order to sensibly handle batches across multiple calls to canvas_render_items,
// we translate the incoming Item::Commands into a stream of batches.
// The incoming commands is analagous to OpenGL immediate mode (which was
// deprecated because it is slow, for the same reasons the old Godot renderer was slow).
// Batches can contain groups of primitives, or state changes, such as texture, material, transform
// etc.
// The batch structure is as compact as possible as a lot of batches will be churned. Supplementary
// data is stored in extra arrays.
// Vertices themselves are usually stored in a compact pos, uv format, except in the case where a lot of
// color change batches occur. If this happens, an extra stage will translate the pos, uv verts to
// colored verts (pos, uv, color) and remove the unnecessary color change batches, and join the primitive
// batches.

// There are 2 important improvements that can be made:
// 1) Item::Commands contain only a single primitive. This is inefficient,
// because there is a lot of 'state checking' required for each command.
// Commands could be capable of containing multiple identical primitives,
// removing the need to state check.
// 2) Many canvas items (text, backgrounds etc) will contain static data.
// It is wasteful to upload and process dynamically when this is not necessary.
// This may be more complex to retrofit.

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

	// the size of these 2 arrays should be identical
	RasterizerArray<BVert> vertices;
	RasterizerArray<BVert_colored> vertices_colored;

	// the sizes of these 2 arrays should be identical
	RasterizerArray<Batch> batches;
	RasterizerArray<Batch> batches_temp; // used for translating to colored vertex batches

	// RIDs are non-pod types, they need slower arrays to handle
	RasterizerArray_non_pod<BTex> batch_textures;
	RasterizerArray_non_pod<BMaterial> batch_materials;

	// sofware transformed verts don't require passing a transform.
	// default (old style) primitives and hardware transformed verts do.
	RasterizerArray<BTransform> transforms;

	// vector to contain larger extra info for the batches such as rects and colors
	RasterizerArray<B128> generic_128s;

	// gets set if we decide heuristically to convert to colored vertices
	bool use_colored_vertices;

	// if the (vertex) buffer is full, we need to flush and render all the batches processed so far
	bool buffer_full;

	// counts
	int total_quads;

	// we keep a record of how many color changes caused new batches
	// if the colors are causing an excessive number of batches, we switch
	// to alternate batching method and add color to the vertex format.
	int total_color_changes;
};



} // namespace
