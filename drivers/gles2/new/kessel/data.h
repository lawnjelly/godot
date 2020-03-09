#pragma once

#include "drivers/gles2/new/batch_array.h"
#include "batch.h"

namespace Batch {

class BData
{
public:
	BData();

	void Reset_Flush();
	void Reset_Pass();

	// the sizes of these 2 arrays should be identical
	BArray<Batch> batches;
	BArray<Batch> batches_temp;

	BArray<BItemGroup> itemgroups;


	Batch * request_new_batch(bool p_blank = true);
};

} // namespace
