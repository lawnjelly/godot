#pragma once

#include "llighttypes.h"
#include <atomic>

namespace LM {

class LAtomic {
public:
	enum { NUM_LOCKS = 32 };

	std::atomic_flag _locks[NUM_LOCKS];

	LAtomic() {
		for (int n = 0; n < NUM_LOCKS; n++) {
			_locks[n].clear();
		}
	}

	void atomic_add_col(unsigned int hash, FColor &col, const FColor &add) {
		hash = hash % NUM_LOCKS;

		// acquire lock
		while (_locks[hash].test_and_set(std::memory_order_acquire))
			; // spin

		// do atomic operation
		col += add;

		// release lock
		_locks[hash].clear(std::memory_order_release);
	}
};

} // namespace LM
