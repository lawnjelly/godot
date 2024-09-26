#pragma once

namespace NavPhysics {

#include "navphysics_vector.h"

template <class T, class U = u32, bool force_trivial = false, bool zero_on_first_request = false>
class PooledList {
	NavPhysics::Vector<T, U, force_trivial> list;
	NavPhysics::Vector<U, U, true> freelist;

	// not all list members are necessarily used
	U _used_size;

public:
	PooledList() {
		_used_size = 0;
	}

	// Use with care, in most cases you should make sure to
	// free all elements first (i.e. _used_size would be zero),
	// although it could also be used without this as an optimization
	// in some cases.
	void clear() {
		list.clear();
		freelist.clear();
		_used_size = 0;
	}

	u64 estimate_memory_use() const {
		return ((u64)list.size() * sizeof(T)) + ((u64)freelist.size() * sizeof(U));
	}

	const T &operator[](U p_index) const {
		return list[p_index];
	}
	T &operator[](U p_index) {
		return list[p_index];
	}

	// To be explicit in a pool there is a distinction
	// between the number of elements that are currently
	// in use, and the number of elements that have been reserved.
	// Using size() would be vague.
	U used_size() const { return _used_size; }
	U reserved_size() const { return list.size(); }

	T *request(U &r_id) {
		_used_size++;

		if (freelist.size()) {
			// pop from freelist
			int new_size = freelist.size() - 1;
			r_id = freelist[new_size];
			freelist.resize(new_size);

			return &list[r_id];
		}

		r_id = list.size();
		list.resize(r_id + 1);

		static_assert((!zero_on_first_request) || force_trivial || (__is_pod(T)), "zero_on_first_request requires trivial type");
		if (zero_on_first_request && (force_trivial || __is_pod(T))) {
			list[r_id] = {};
		}

		return &list[r_id];
	}
	void free(const U &p_id) {
		// should not be on free list already
		NP_NP_ERR_FAIL_UNSIGNED_INDEX(p_id, list.size());
		freelist.push_back(p_id);
		NP_NP_ERR_FAIL_COND_MSG(!_used_size, "_used_size has become out of sync, have you f64 freed an item?");
		_used_size--;
	}
};

// a pooled list which automatically keeps a list of the active members
template <class T, class U = u32, bool force_trivial = false, bool zero_on_first_request = false>
class TrackedPooledList {
public:
	U pool_used_size() const { return _pool.used_size(); }
	U pool_reserved_size() const { return _pool.reserved_size(); }
	U active_size() const { return _active_list.size(); }

	// use with care, see the earlier notes in the PooledList clear()
	void clear() {
		_pool.clear();
		_active_list.clear();
		_active_map.clear();
	}

	U get_active_id(U p_index) const {
		return _active_list[p_index];
	}

	const T &get_active(U p_index) const {
		return _pool[get_active_id(p_index)];
	}

	T &get_active(U p_index) {
		return _pool[get_active_id(p_index)];
	}

	const T &operator[](U p_index) const {
		return _pool[p_index];
	}
	T &operator[](U p_index) {
		return _pool[p_index];
	}

	T *request(U &r_id) {
		T *item = _pool.request(r_id);

		// add to the active list
		U active_list_id = _active_list.size();
		_active_list.push_back(r_id);

		// expand the active map (this should be in sync with the pool list
		if (_pool.used_size() > _active_map.size()) {
			_active_map.resize(_pool.used_size());
		}

		// store in the active map
		_active_map[r_id] = active_list_id;

		return item;
	}

	void free(const U &p_id) {
		_pool.free(p_id);

		// remove from the active list.
		U list_id = _active_map[p_id];

		// zero the _active map to detect bugs (only in debug?)
		_active_map[p_id] = -1;

		_active_list.remove_unordered(list_id);

		// keep the replacement in sync with the correct list Id
		if (list_id < _active_list.size()) {
			// which pool id has been replaced in the active list
			U replacement_id = _active_list[list_id];

			// keep that replacements map up to date with the new position
			_active_map[replacement_id] = list_id;
		}
	}

	const NavPhysics::Vector<U, U> &get_active_list() const { return _active_list; }

private:
	NavPhysics::PooledList<T, U, force_trivial, zero_on_first_request> _pool;
	NavPhysics::Vector<U, U> _active_map;
	NavPhysics::Vector<U, U> _active_list;
};

} //namespace NavPhysics
