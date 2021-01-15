#include "lplanes_pool.h"

using namespace Lawn;

LPlanesPool::LPlanesPool()
{
	reset();

	// preallocate the vectors to a reasonable size
	for (int n=0; n<POOL_MAX; n++)
	{
		_planes[n].resize(32);
	}
}

void LPlanesPool::reset()
{
	for (int n=0; n<POOL_MAX; n++)
	{
		_freelist[n] = POOL_MAX - n - 1;
	}

	_num_free = POOL_MAX;
}

unsigned int LPlanesPool::request()
{
	if (!_num_free)
		return -1;

	_num_free--;
	return _freelist[_num_free];

}

void LPlanesPool::free(unsigned int ui)
{
	assert (ui <= POOL_MAX);
	assert (_num_free < POOL_MAX);

	_freelist[_num_free] = ui;
	_num_free++;
}
