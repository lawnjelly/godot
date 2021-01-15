#pragma once

#include "lvector.h"
#include "core/math/plane.h"

namespace Lawn {

// The recursive visibility function needs to allocate lists of planes each time a room is traversed.
// Instead of doing this allocation on the fly we will use a pool which should be much faster and nearer
// constant time.

// Note this simple pool isn't super optimal but should be fine for now.
class LPlanesPool
{
public:
	const static int POOL_MAX = 32;

	void reset();

	unsigned int request();
	void free(unsigned int ui);

	LVector<Plane> &get(unsigned int ui) {return _planes[ui];}

	LPlanesPool();
private:
	LVector<Plane> _planes[	POOL_MAX];

	uint8_t _freelist[POOL_MAX];
	uint32_t _num_free;
};

} // namespace
