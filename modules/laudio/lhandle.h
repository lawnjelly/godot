#pragma once

#include "core/error_macros.h"
#include "core/pooled_list.h"
#include <stdint.h>

class Handle_24_8 {
	uint32_t val;

public:
	bool operator==(const Handle_24_8 &p_o) const { return p_o.val == val; }
	uint32_t get_value() const { return val; }
	//void set_value(uint32_t p_value) { val = p_value; }

	uint32_t get_id() const { return val & 0xFFFFFF; }
	uint32_t get_revision() const { return (val & 0xFF000000) >> 24; }
	bool is_valid() const { return val != UINT32_MAX; }
	void set_invalid() { val = UINT32_MAX; }
	void set_invalid_increment_revision() { set_id_revision(0xFFFFFF, get_revision() + 1); }

	void set_id(uint32_t p_id) {
		DEV_ASSERT(p_id <= 0xFFFFFF);
		val &= ~0xFFFFFF;
		val |= (p_id & 0xFFFFFF);
	}

	// revision has wraparound
	void set_id_revision(uint32_t p_id, uint32_t p_revision) {
		DEV_ASSERT(p_id <= 0xFFFFFF);
		val = 0;
		val = (p_revision & 0xFF) << 24;
		val |= (p_id & 0xFFFFFF);
	}

	static void wrap_revision(uint32_t &r_revision) {
		r_revision &= 0xFF;
	}

	Handle_24_8() {
		set_invalid();
	}

	Handle_24_8(uint32_t p_value) {
		val = p_value;
	}
};

// Relies on T containing a revision value which can be set and read, and will not be altered in the constructor or destructor.
template <class T, class U = uint32_t, class HANDLE = Handle_24_8>
class PooledList_HandledPOD {
	static_assert((__is_pod(T)), "PooledList_HandledPOD requires trivial type");
	PooledList<T, U, true, true> _list;

public:
	U used_size() const { return _list.used_size(); }
	U reserved_size() const { return _list.reserved_size(); }

	HANDLE request() {
		U id = 0;
		T *e = _list.request(id);
		DEV_ASSERT(e);
		HANDLE h;
		h.set_id_revision(id, e->get_revision());
		return h;
	}
	void free(HANDLE p_handle) {
		T &e = _list[p_handle.get_id()];

		uint32_t stored_revision = e.get_revision();

		if (stored_revision != p_handle.get_revision()) {
			ERR_FAIL_MSG("Revision is incorrect.");
		}

		// increment the revision on free, rather than alloc
		// in order to catch references to deleted objects
		stored_revision++;
		HANDLE::wrap_revision(stored_revision);
		e->set_revision(stored_revision);

		_list.free(p_handle.get_id());
	}
	T *get(HANDLE p_handle) {
		uint32_t id = p_handle.get_id();
		ERR_FAIL_COND_V_MSG(id >= _list.reserved_size(), nullptr, "Handle ID is out of range.");
		T &e = _list[id];
		ERR_FAIL_COND_V_MSG(e.get_revision() != p_handle.get_revision(), nullptr, "Handle revision is incorrect.");
		return &e;
	}
	const T *get(HANDLE p_handle) const {
		uint32_t id = p_handle.get_id();
		ERR_FAIL_COND_V_MSG(id >= _list.reserved_size(), nullptr, "Handle ID is out of range.");
		const T &e = _list[id];
		ERR_FAIL_COND_V_MSG(e.get_revision() != p_handle.get_revision(), nullptr, "Handle revision is incorrect.");
		return &e;
	}
};

template <class T, class U = uint32_t, bool FORCE_TRIVIAL = false, class HANDLE = Handle_24_8>
class HandledPool {
	PooledList<HANDLE, U, true, true> _handles;
	// We need to keep this in sync, to allow renumbering handles
	// when we remove unordered from the _list.
	LocalVector<U> _list_item_handle_ids;

	LocalVector<T, U, FORCE_TRIVIAL> _list;

public:
	U size() const { return _list.size(); }

	T *request(HANDLE &r_handle) {
		uint32_t new_handle_id;
		HANDLE *new_handle = _handles.request(new_handle_id);

		new_handle->set_id_revision(_list.size(), new_handle->get_revision());
		r_handle.set_id_revision(new_handle_id, new_handle->get_revision());

		_list.resize(_list.size() + 1);

		// keep a back record of which handle ID each list item is
		_list_item_handle_ids.push_back(new_handle_id);

		return &_list[_list.size() - 1];
	}

	void free(HANDLE p_handle) {
		ERR_FAIL_COND(p_handle.get_id() >= _handles.reserved_size());
		HANDLE &h = _handles[p_handle.get_id()];
		ERR_FAIL_COND(h.get_revision() != p_handle.get_revision());

		U list_id = h.get_id();
		ERR_FAIL_COND(list_id >= _list.size());
		DEV_ASSERT(_list.size());

		{
			// we need to renumber the last element as it will be exchanged.
			// this is inefficient, can be optimized.
			U old_list_pos = _list.size() - 1;
			U new_list_pos = list_id;
			_handles[_list_item_handle_ids[old_list_pos]].set_id(new_list_pos);
		}

		_list.remove_unordered(list_id);
		_list_item_handle_ids.remove_unordered(list_id);

		// Make sure the existing handle revision is out of sync
		// to detect errors if we attempt to use the handle after free.
		h.set_invalid_increment_revision();
	}

	// DO NOT hang on to these pointers, as they are only valid until the list has been resized.
	// Access by handle EACH time, and use a mutex if multithreading.
	T *get(HANDLE p_handle) {
		U id = p_handle.get_id();
		ERR_FAIL_COND_V_MSG(id >= _handles.reserved_size(), nullptr, "Handle ID is out of range.");
		const HANDLE &h = _handles[id];
		ERR_FAIL_COND_V_MSG(h.get_revision() != p_handle.get_revision(), nullptr, "Handle revision is incorrect.");
		U list_id = h.get_id();
		ERR_FAIL_UNSIGNED_INDEX_V(list_id, _list.size(), nullptr);
		return &_list[list_id];
	}
	const T *get(HANDLE p_handle) const {
		U id = p_handle.get_id();
		ERR_FAIL_COND_V_MSG(id >= _handles.reserved_size(), nullptr, "Handle ID is out of range.");
		const HANDLE &h = _handles[id];
		ERR_FAIL_COND_V_MSG(h.get_revision() != p_handle.get_revision(), nullptr, "Handle revision is incorrect.");
		U list_id = h.get_id();
		ERR_FAIL_UNSIGNED_INDEX_V(list_id, _list.size(), nullptr);
		return &_list[list_id];
	}

	// order is not guaranteed.
	const T &get_active(U p_index) const {
		return _list[p_index];
	}

	T &get_active(U p_index) {
		return _list[p_index];
	}
};

// Generic Handled PooledList, works with non-trivial types, but not as efficient as PooledList_HandledPOD
// because the revision is stored in a separate list and may cause a cache miss.
template <class T, class U = uint32_t, class HANDLE = Handle_24_8, class REVISION = uint8_t>
class TrackedPooledList_Handled {
	TrackedPooledList<T, U> _list;
	LocalVector<REVISION> _revisions;

public:
	U used_size() const { return _list.pool_used_size(); }
	U reserved_size() const { return _list.pool_reserved_size(); }
	U active_size() const { return _list.active_size(); }

	HANDLE request() {
		U id = 0;
		T *e = _list.request(id);
		DEV_ASSERT(e);
		HANDLE h;

		if (id >= _revisions.size()) {
			// first occurrence of this ID
			_revisions.push_back(0);
			h.set_id_revision(id, 0);
		} else {
			const REVISION &r = _revisions[id];
			h.set_id_revision(id, r);
		}

		return h;
	}
	void free(HANDLE p_handle) {
		uint32_t id = p_handle.get_id();
		ERR_FAIL_COND_MSG(id >= _revisions.size(), "Handle ID is out of range.");
		ERR_FAIL_COND_MSG(_revisions[id] != p_handle.get_revision(), "Revision is incorrect.");

		// increment the revision on free, rather than alloc
		// in order to catch references to deleted objects
		REVISION &r = _revisions[id];
		r++;

		_list.free(p_handle.get_id());
	}
	T *get(HANDLE p_handle) {
		uint32_t id = p_handle.get_id();
		ERR_FAIL_COND_V_MSG(id >= reserved_size(), nullptr, "Handle ID is out of range.");
		DEV_ASSERT(_revisions.size() == reserved_size());
		ERR_FAIL_COND_V_MSG(_revisions[id] != p_handle.get_revision(), nullptr, "Handle revision is incorrect.");
		return &_list[id];
	}
	const T *get(HANDLE p_handle) const {
		uint32_t id = p_handle.get_id();
		ERR_FAIL_COND_V_MSG(id >= reserved_size(), nullptr, "Handle ID is out of range.");
		DEV_ASSERT(_revisions.size() == reserved_size());
		ERR_FAIL_COND_V_MSG(_revisions[id] != p_handle.get_revision(), nullptr, "Handle revision is incorrect.");
		return &_list[id];
	}

	const T &get_active(U p_index) const {
		return _list.get_active(p_index);
	}

	T &get_active(U p_index) {
		return _list.get_active(p_index);
	}
};
