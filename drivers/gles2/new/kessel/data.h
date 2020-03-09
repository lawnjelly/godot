#pragma once

#include "drivers/gles2/new/defines.h"
#include "drivers/gles2/renderer_2d_old.h"
#include "drivers/gles2/new/batch_array.h"
#include "batch.h"

namespace Batch {

class Data : public Renderer2D_old
{
public:

	// the sizes of these 2 arrays should be identical
	BArray<Batch> batches;
	BArray<Batch> batches_temp; // used for translating to colored vertex batches



};

} // namespace
