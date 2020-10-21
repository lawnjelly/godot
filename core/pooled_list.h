#pragma once

#include "core/local_vector.h"

template <class T, bool force_trivial>
class PooledList
{
	LocalVector<T, uint32_t, force_trivial> list;
	LocalVector<uint32_t, uint32_t, true> freelist;

	// not all list members are necessarily used
	int _used_size;
public:
	PooledList()
	{
		_used_size = 0;
	}

	const T &operator[](uint32_t p_index) const {
		return list[p_index];
	}
	T &operator[](uint32_t p_index) {
		return list[p_index];
	}

	int size() const {return _used_size;}

	T * request(uint32_t &r_id)
	{
		_used_size++;

		if (freelist.size())
		{
			// pop from freelist
			int new_size = freelist.size()-1;
			r_id = freelist[new_size];
			freelist.resize(new_size);
			return &list[r_id];
		}

		r_id = list.size();
		list.resize(r_id +1);
		return &list[r_id];
	}
	void free(uint32_t &p_id)
	{
		// should not be on free list already
		CRASH_COND(p_id >= list.size());
		freelist.push_back(p_id);
		_used_size--;
	}

};
