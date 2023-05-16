#pragma once

namespace NavPhysics {

#include "navphysics_vector.h"

template <class T, class U = u32, bool force_trivial = false, bool zero_on_first_request = false, u32 SIZE_LIMIT = 0>
class PooledList {
	NavPhysics::Vector<T, U, force_trivial> list;
	NavPhysics::TVector<U, U> freelist;

	// not all list members are necessarily used
	U _used_size;

public:
#ifdef NP_DEV_ENABLED
	bool _debug = false;
#endif

	PooledList() {
		_used_size = 0;
		if (SIZE_LIMIT) {
			list.reserve(SIZE_LIMIT);
		}
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

	bool is_full() const {
		if (SIZE_LIMIT && _used_size == SIZE_LIMIT) {
			return true;
		}
		return false;
	}

	T *request(U &r_id) {
		if (is_full()) {
			r_id = UINT32_MAX;
			return nullptr;
		}

		_used_size++;

		if (freelist.size()) {
			// pop from freelist
			int new_size = freelist.size() - 1;
			r_id = freelist[new_size];
			freelist.resize(new_size);

#ifdef NP_DEV_ENABLED
			if (_debug) {
				log(String("pool add ") + r_id);
			}
#endif

			return &list[r_id];
		}

		r_id = list.size();
		list.resize(r_id + 1);

		static_assert((!zero_on_first_request) || force_trivial || (__is_pod(T)), "zero_on_first_request requires trivial type");
		if (zero_on_first_request && (force_trivial || __is_pod(T))) {
			list[r_id] = {};
		}

#ifdef NP_DEV_ENABLED
		if (_debug) {
			log(String("pool add ") + r_id);
		}
#endif

		return &list[r_id];
	}
	void free(const U &p_id) {
#ifdef NP_DEV_ENABLED
		if (_debug) {
			log(String("pool free ") + p_id);
		}
#endif

		// should not be on free list already
		NP_ERR_FAIL_UNSIGNED_INDEX(p_id, list.size());
		freelist.push_back(p_id);
		NP_DEV_ASSERT(_used_size);
		NP_ERR_FAIL_COND_MSG(!_used_size, "_used_size has become out of sync, have you f64 freed an item?");
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

	const NavPhysics::TVector<U, U> &get_active_list() const { return _active_list; }

private:
	NavPhysics::PooledList<T, U, force_trivial, zero_on_first_request> _pool;
	NavPhysics::TVector<U, U> _active_map;
	NavPhysics::TVector<U, U> _active_list;
};

template <class T, u32 MAX_ELEMENTS>
class QueuedPooledList : public PooledList<T, u32, false, false, MAX_ELEMENTS> {
	// If the requests_queued gets out of sync,
	// the queue may fail to work because it will
	// get stuck waiting for "missing" requesters.
	// Make sure to call "cancel_request"!
	u32 requests_queued = 0;
	u64 requests_issued = 1;

public:
	// Returns our issue turn in the queue.
	// If we are at this issue or more, we are allowed to take a
	u64 make_request() {
		u64 issue = requests_issued + requests_queued;
		requests_queued++;
		return issue;
	}

#if 0
	void cancel_request() {
		if (requests_queued) {
			requests_queued--;

			// We have to count this as issued, so other requests can not be stalled
			// waiting for their issue count to come up.
			//requests_issued++;
		} else {
			NP_WARN_PRINT("NavPhysics: requests queued out of sync");
		}
	}
#endif

	u32 get_requests_queued() const { return requests_queued; }

	T *queued_request(u32 &r_id, u64 p_issue) {
		// Are we allowed to make a request yet?
		if (p_issue > requests_issued) {
			//NP_DEV_ASSERT((p_issue - requests_issued) < requests_queued);
			//log(String("\twaiting for request issue ") + p_issue + ", requests issued : " + requests_issued);
			return nullptr;
		}

		//log(String("Fulfilling request issue ") + p_issue + ", requests issued : " + requests_issued);

		T *res = this->request(r_id);

		if (res) {
			//requests_issued++;
			//cancel_request();
		}

		return res;
	}

	void iterate() {
		requests_issued++;
		if (requests_queued) {
			requests_queued--;
		}
	}

#if 0
	struct Request {
		u32 plan_id = 0;
		u32 *planned_id = nullptr;
	};
	NavPhysics::Vector<Request> _requests;

public:
	bool queue_request(u32 p_plan_id, u32 *r_planned_id) {
		NP_DEV_ASSERT(find_request(p_plan_id) == -1);
		Request r;
		r.plan_id = p_plan_id;
		r.planned_id = r_planned_id;
		_requests.push_back(r);
		return true;
	}

	i32 find_request(u32 p_plan_id) const {
		for (u32 n = 0; n < _requests.size(); n++) {
			if (_requests[n].plan_id == p_plan_id) {
				return n;
			}
		}

		return -1;
	}

	bool notify_remove_request(u32 p_plan_id) {
		i32 which = find_request(p_plan_id);
		if (which >= 0) {
			_requests.remove(which);
			return true;
		}
		return false;
	}

	bool process_one() {
		if (this->is_full() || !_requests.size()) {
			return false;
		}

		// Get a request from the front of the queue.
		const Request &r = _requests[0];
		request(*r.planned_id);

		// Remove the request (this does expensive memory shift).
		_requests.remove(0);
		return true;
	}
#endif
};

} //namespace NavPhysics
