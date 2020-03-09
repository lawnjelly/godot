#include "data.h"

namespace Batch {

Data::Data()
{

}


Batch * Data::request_new_batch(bool p_blank)
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


} // namespace
