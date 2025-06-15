#pragma once

// Thread safe, non-relocatable pool.
// This means elements can be accessed from multiple threads simultaneously, lock free.
// Adding / removing elements may require lock and is more expensive.

#include "core/local_vector.h"

// Take no account of handles, has little error checking.
// For error checking, normally access through Pool.
// Max elements = CHUNK_SIZE * GROUP_SIZE * MAX_GROUPS
// Prefer power of 2 for sizes, as it can use bitshift instructions.
template <class T, uint32_t CHUNK_SIZE = 256, uint32_t GROUP_SIZE = 256, uint32_t MAX_GROUPS = 256, bool THREAD_SAFE = true, class U = uint32_t>
class TSPoolBase {
	struct Address {
		uint32_t group_id = 0;
		uint32_t chunk_id = 0;
		uint32_t element_id = 0;
		Address() {
			group_id = 0;
			chunk_id = 0;
			element_id = 0;
		}
		Address(U p_idx) {
			chunk_id = p_idx / CHUNK_SIZE;
			element_id = p_idx % CHUNK_SIZE;
			group_id = chunk_id / GROUP_SIZE;
		}
		U id() const {
			return (group_id * GROUP_SIZE * CHUNK_SIZE) + (chunk_id * CHUNK_SIZE) + element_id;
		}
	};

	class Chunk {
		// No padding at the moment for alignment, and assuming T is a good size
		// alignment wise.
		uint8_t data[CHUNK_SIZE * sizeof(T)] = { 0 };
		// ToDo - change to bitfield.
		uint8_t slots_used[CHUNK_SIZE] = { 0 };

	public:
		T &get(uint32_t p_idx) {
			return *(T *)(&data[p_idx * sizeof(T)]);
		}
		const T &get(uint32_t p_idx) const {
			return *(const T *)(&data[p_idx * sizeof(T)]);
		}

		void debug() {
			print_line("\n---------------------------\n");

			const uint8_t *p = &data[0];
			for (uint32_t n = 0; n < CHUNK_SIZE; n++) {
				String sz = ptos(p) + " : ";
				for (uint32_t i = 0; i < sizeof(T); i++) {
					sz += String::num_uint64(*p++, 16) + " ";
				}
				print_line(sz);
			}
			print_line(ptos(p) + " : end");
		}

		void set_used(uint32_t p_idx, bool p_set) {
			slots_used[p_idx] = p_set ? 255 : 0;
		}
		bool free_if_used(uint32_t p_idx) {
			if (slots_used[p_idx]) {
				slots_used[p_idx] = 0;
				if (!std::is_trivially_destructible<T>::value) {
					T &ele = get(p_idx);
					ele.~T();
				}
				return true;
			}
			return false;
		}

		void free_used() {
			for (uint32_t n = 0; n < CHUNK_SIZE; n++) {
				free_if_used(n);
			}
		}

		~Chunk() {
			free_used();
		}
	};

	struct Group {
		Chunk *chunks[GROUP_SIZE] = { 0 };
		uint32_t used = 0;
		bool is_full() const { return used == GROUP_SIZE; }
		~Group() {
			for (uint32_t n = 0; n < used; n++) {
				DEV_ASSERT(chunks[n]);
				memdelete(chunks[n]);
				chunks[n] = nullptr;
			}
			used = 0;
		}
	};

	struct TableOfContents {
		Group *groups[MAX_GROUPS] = { 0 };
		uint32_t used = 0;
		bool is_full() const { return used == MAX_GROUPS; }

		void free() {
			for (uint32_t n = 0; n < used; n++) {
				DEV_ASSERT(groups[n]);
				memdelete(groups[n]);
				groups[n] = nullptr;
			}
			used = 0;
		}

		~TableOfContents() {
			free();
		}
	} toc;

	LocalVector<U, U, true> freelist;

	// Not all elements are necessarily used.
	U _used_size = 0;
	Mutex _mutex;

	class LockGuard {
	public:
		LockGuard(Mutex &p_mutex, bool p_thread_safe) {
			// will be compiled out if not set in template
			if (p_thread_safe) {
				_mutex = &p_mutex;

				if (_mutex->try_lock() != OK) {
					WARN_PRINT_ONCE("Info : multithread Pool access detected (benign)");
					_mutex->lock();
				}

			} else {
				_mutex = nullptr;
			}
		}
		~LockGuard() {
			// will be compiled out if not set in template
			if (_mutex) {
				_mutex->unlock();
			}
		}

	private:
		Mutex *_mutex;
	};

	bool grow() {
		// Special case for first group?
		if (!toc.used || toc.groups[toc.used - 1]->is_full()) {
			// We need a new group.

			// Completely full, very rare.
			if (toc.is_full()) {
				return false;
			}

			toc.groups[toc.used] = memnew(Group);
			toc.used++;
		}

		// The group id should always be valid.
		Address addr;
		addr.group_id = toc.used - 1;

		Group *group = toc.groups[addr.group_id];
		DEV_ASSERT(group);
		DEV_ASSERT(!group->is_full());

		addr.chunk_id = group->used;

		// Must need to expand the group by definition, if the master freelist is empty.
		group->chunks[group->used] = memnew(Chunk);
		group->used++;

		// Expand the freelist.
		U new_ids = addr.id();
		freelist.resize(CHUNK_SIZE);
		for (U n = 0; n < CHUNK_SIZE; n++) {
			freelist[n] = new_ids + n;
		}

		return true;
	}

	bool set_chunk_slot_used(U p_idx, bool p_used) {
		Address addr(p_idx);
		ERR_FAIL_COND_V(addr.group_id >= MAX_GROUPS, false);
		Group *group = toc.groups[addr.group_id];
		ERR_FAIL_NULL_V(group, false);
		Chunk *chunk = group->chunks[addr.chunk_id];
		ERR_FAIL_NULL_V(chunk, false);
		chunk->set_used(addr.element_id, p_used);

		return true;
	}

public:
	U used_size() const { return _used_size; }

	const T *get(U p_idx) const {
		Address addr(p_idx);
		ERR_FAIL_COND_V(addr.group_id >= MAX_GROUPS, nullptr);
		Group *group = toc.groups[addr.group_id];
		ERR_FAIL_NULL_V(group, nullptr);
		Chunk *chunk = group->chunks[addr.chunk_id];
		ERR_FAIL_NULL_V(chunk, nullptr);
		return (const T *)&chunk->get(addr.element_id);
	}

	T *get(U p_idx) {
		Address addr(p_idx);
		ERR_FAIL_COND_V(addr.group_id >= MAX_GROUPS, nullptr);
		Group *group = toc.groups[addr.group_id];
		ERR_FAIL_NULL_V(group, nullptr);
		Chunk *chunk = group->chunks[addr.chunk_id];
		ERR_FAIL_NULL_V(chunk, nullptr);
		return (T *)&chunk->get(addr.element_id);
	}

	Chunk *debug_get_current_fill_chunk() {
		if (!toc.used) {
			return nullptr;
		}

		Group *group = toc.groups[toc.used - 1];
		if (!group->used) {
			return nullptr;
		}

		return group->chunks[group->used - 1];
	}

	// Note that this is not thread safe if freeing memory,
	// by definition, because reading / writing is lock free
	// and doesn't check this mutex..
	// But clear with free should be rare situation.
	void clear(bool p_free_memory = false) {
		LockGuard guard(_mutex, THREAD_SAFE);

		if (p_free_memory) {
			toc.free();
		} else {
			if (!_used_size) {
				// Nothing to do.
				return;
			}

			for (uint32_t n = 0; n < toc.used; n++) {
				Group *group = toc.groups[n];
				DEV_ASSERT(group);

				for (uint32_t c = 0; c < group->used; c++) {
					Chunk *chunk = group->chunks[c];
					chunk->free_used();
				}
			}

			// Reconstruct the free list from scratch, quicker than doing individuall as we free.
			freelist.clear();
			if (toc.used) {
				Address addr_last;
				addr_last.group_id = toc.used - 1;
				addr_last.chunk_id = toc.groups[addr_last.group_id]->used;

				U num_available = addr_last.id();
				freelist.resize(num_available);

				for (U n = 0; n < num_available; n++) {
					freelist[n] = n;
				}
			}
		}
	}

	T *request(U &r_id) {
		LockGuard guard(_mutex, THREAD_SAFE);

		if (freelist.size()) {
			// Simplest case, pop from freelist,
			// no need to grow.
			U new_size = freelist.size() - 1;
			r_id = freelist[new_size];
			freelist.resize(new_size);

			_used_size++;

			T *ele = get(r_id);

			// This may be able to be combined with the get,
			// but the optimizer will likely  catch it.
			set_chunk_slot_used(r_id, true);

			// DO NOT blank out T at this point,
			// Because it will zero any revisions which are still in use
			// if we are using handles.
			// Instead call the constructor, which can zero only the value,
			// and not the revision.
			memnew_placement(ele, T);

			return ele;
		}

		if (!grow()) {
			r_id = -1;
			return nullptr;
		}

		DEV_ASSERT(freelist.size());
		return request(r_id); // Call recursively to save code.
	}

	void free(U p_id) {
		LockGuard guard(_mutex, THREAD_SAFE);

		if (!std::is_trivially_destructible<T>::value) {
			T *ele = get(p_id);
			ERR_FAIL_NULL(ele);

			// This may be able to be combined with the get,
			// but the optimizer will likely  catch it.
			set_chunk_slot_used(p_id, false);

			ele->~T();
		} else {
#ifdef DEV_ENABLED
			T *ele = get(p_id);
			ERR_FAIL_NULL(ele);
#endif
		}

		freelist.push_back(p_id);
		ERR_FAIL_COND_MSG(!_used_size, "_used_size has become out of sync, have you double freed an item?");
		_used_size--;
	}
};

struct Handle_32_32 {
	struct Data {
		union {
			uint64_t value = 0;
			struct
			{
				uint32_t id;
				uint32_t revision;
			};
		};
	} _data;

	void clear() { _data.value = 0; }
	bool is_valid() const { return _data.value != 0; }
	uint32_t id() const { return _data.id; }
	uint32_t revision() const { return _data.revision; }
	void set_id(uint32_t p_id) { _data.id = p_id; }
	void set_revision(uint32_t p_revision) { _data.revision = p_revision; }
	void increment_revision() { _data.revision++; }

	uint64_t get_value() const { return _data.value; }
	void set_value(uint64_t p_value) { _data.value = p_value; }
};

struct Handle_24_8 {
	struct Data {
		union {
			uint32_t value = 0;
			struct
			{
				uint32_t id : 24;
				uint8_t revision;
			};
		};
	} _data;

	void clear() { _data.value = 0; }
	bool is_valid() const { return _data.value != 0; }
	uint32_t id() const { return _data.id; }
	uint32_t revision() const { return _data.revision; }
	void set_id(uint32_t p_id) { _data.id = p_id; }
	void set_revision(uint32_t p_revision) { _data.revision = p_revision; }
	void increment_revision() { _data.revision++; }

	uint32_t get_value() const { return _data.value; }
	void set_value(uint32_t p_value) { _data.value = p_value; }
};

// Wrapper takes handles, error checking.
template <class T, class HANDLE = Handle_32_32, class U = uint32_t>
class TSPool {
	// R is revision type.
	using REVISION_TYPE = decltype(HANDLE()._data.revision);

	// ToDo: Alignment.
	// At the moment some bytes might be lost here for padding,
	// particularly with 8 bit revision, but it ensures cache friendly,
	// and better (and less complex) than storing revision in a separate list.

	// This revision + padding makes the pool slightly less efficient memory wise for storing
	// small T sizes.
	struct Unit {
		REVISION_TYPE revision;
		T value;
	};

	TSPoolBase<Unit> _pool;

public:
	U used_size() const { return _pool.used_size(); }
	void clear(bool p_free_memory = false) {
		_pool.clear(p_free_memory);
	}

	const T *get(HANDLE p_handle, bool p_silent = false) const {
		ERR_FAIL_COND_V(!p_handle.is_valid(), nullptr);
		const Unit *unit = _pool.get(p_handle.id);
		ERR_FAIL_NULL_V(unit, nullptr);

		// If the revisions don't match, the object is invalid,
		// and we're probably using a dangling handle.
		if (unit->revision != p_handle.revision()) {
			if (!p_silent) {
				ERR_FAIL_COND_V(unit->revision != p_handle.revision, nullptr);
			}
			return nullptr;
		}

		return (const T *)&unit->value;
	}

	T *get(HANDLE p_handle, bool p_silent = false) {
		ERR_FAIL_COND_V(!p_handle.is_valid(), nullptr);
		Unit *unit = _pool.get(p_handle.id());
		ERR_FAIL_NULL_V(unit, nullptr);

		// If the revisions don't match, the object is invalid,
		// and we're probably using a dangling handle.
		if (unit->revision != p_handle.revision()) {
			if (!p_silent) {
				ERR_FAIL_COND_V(unit->revision != p_handle.revision(), nullptr);
			}
			return nullptr;
		}

		return (T *)&unit->value;
	}

	T *request(HANDLE &r_handle) {
		U id;
		Unit *unit = _pool.request(id);
		ERR_FAIL_NULL_V(unit, nullptr);

		r_handle.set_id(id);
		r_handle.set_revision(unit->revision);

		// Handle special case where we have accidentally created invalid handle.
		// This can happen here particularly with the first ID, with its first revision (0).
		if (!r_handle.is_valid()) {
			r_handle.increment_revision();
			unit->revision = r_handle.revision();
		}

		return (T *)&unit->value;
	}

	void free(HANDLE p_handle) {
		ERR_FAIL_COND(!p_handle.is_valid());

		Unit *unit = _pool.get(p_handle.id());
		ERR_FAIL_NULL(unit);

		// If the revisions don't match, the object is invalid,
		// and we're probably using a dangling handle.
		if (unit->revision != p_handle.revision()) {
			ERR_FAIL_COND(unit->revision != p_handle.revision());
		}

		// Increment revision on free, and wraparound.
		p_handle.increment_revision();

		// Handle special case where we have accidentally created invalid handle.
		// ToDo: possible we can only handle this in the allocation?
		if (!p_handle.is_valid()) {
			// Increment once more, avoid that revision for the last ID.
			p_handle.increment_revision();
		}

		// Resave the incremented revision in the chunk.
		unit->revision = p_handle.revision();

		// Revisions match, ok to free.
		_pool.free(p_handle.id());
	}
};
