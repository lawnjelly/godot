/*************************************************************************/
/*  fixed_array.h                                                        */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef FIXED_ARRAY_H
#define FIXED_ARRAY_H

#include "core/local_vector.h"

// High performance, single threaded.
// Especially useful if you need to create an array on the stack, to
// prevent dynamic allocations (especially in bottleneck code).

template <class T, uint32_t CAPACITY = 8, bool force_trivial = false, uint32_t ALIGN = 1>
class FixedArray {
	static_assert(ALIGN > 0, "ALIGN must be at least 1.");
	const static uint32_t UNIT_SIZE = ((sizeof(T) + ALIGN - 1) / ALIGN * ALIGN);
	const static bool CONSTRUCT = !std::is_trivially_constructible<T>::value && !force_trivial;
	const static bool DESTRUCT = !std::is_trivially_destructible<T>::value && !force_trivial;

	uint32_t _size;
	uint8_t _data[CAPACITY * UNIT_SIZE];

	const T &get(uint32_t p_index) const {
		return *(const T *)&_data[p_index * UNIT_SIZE];
	}
	T &get(uint32_t p_index) {
		return *(T *)&_data[p_index * UNIT_SIZE];
	}

public:
	uint32_t size() const { return _size; }
	bool is_empty() const { return !_size; }
	bool is_full() const { return _size == CAPACITY; }
	uint32_t capacity() const { return CAPACITY; }

	T *request(bool p_construct = true) {
		if (size() < CAPACITY) {
			T *ele = &get(_size++);
			if (CONSTRUCT && p_construct) {
				memnew_placement(ele, T);
			}
			return ele;
		}
		return nullptr;
	}
	void push_back(const T &p_val) {
		T *mem = request(false);
		ERR_FAIL_NULL(mem);
		*mem = p_val;
	}
	T *pop() {
		static_assert(!DESTRUCT, "pop() is not supported for non-trivial types.");
		if (size()) {
			return &get(--_size);
		}
		return nullptr;
	}
	void clear() {
		resize(0);
	}
	void remove_unordered(uint32_t p_index) {
		ERR_FAIL_UNSIGNED_INDEX(p_index, _size);

		_size--;
		if (_size > p_index) {
			get(p_index) = get(_size);
		}

		if (DESTRUCT) {
			get(_size).~T();
		}
	}
	void resize(uint32_t p_size) {
		ERR_FAIL_COND(p_size > CAPACITY);

		if (DESTRUCT && (p_size < _size)) {
			for (uint32_t i = p_size; i < _size; i++) {
				get(i).~T();
			}
		}

		if (CONSTRUCT && (p_size > _size)) {
			for (uint32_t i = _size; i < p_size; i++) {
				memnew_placement(&get(i), T);
			}
		}

		_size = p_size;
	}
	void fill(const T &p_val) {
		for (uint32_t n = 0; n < _size; n++) {
			get(n) = p_val;
		}
	}
	const T &operator[](uint32_t p_index) const {
#ifdef TOOLS_ENABLED
		CRASH_BAD_UNSIGNED_INDEX(p_index, size());
#endif
		return get(p_index);
	}
	T &operator[](uint32_t p_index) {
#ifdef TOOLS_ENABLED
		CRASH_BAD_UNSIGNED_INDEX(p_index, size());
#endif
		return get(p_index);
	}

	operator Vector<T>() const {
		Vector<T> ret;
		if (size()) {
			ret.resize(size());
			T *dest = ret.ptrw();
			if (ALIGN <= 1) {
				memcpy(dest, _data, sizeof(T) * _size);
			} else {
				for (uint32_t n = 0; n < _size; n++) {
					dest[n] = get(n);
				}
			}
		}
		return ret;
	}

	operator LocalVector<T>() const {
		LocalVector<T> ret;
		if (size()) {
			ret.resize(size());
			T *dest = ret.ptr();
			if (ALIGN <= 1) {
				memcpy(dest, _data, sizeof(T) * _size);
			} else {
				for (uint32_t n = 0; n < _size; n++) {
					dest[n] = get(n);
				}
			}
		}
		return ret;
	}

	operator PoolVector<T>() const {
		PoolVector<T> pl;
		if (size()) {
			pl.resize(size());
			typename PoolVector<T>::Write w = pl.write();
			T *dest = w.ptr();
			if (ALIGN <= 1) {
				memcpy(dest, _data, sizeof(T) * _size);
			} else {
				for (uint32_t n = 0; n < _size; n++) {
					dest[n] = get(n);
				}
			}
		}
		return pl;
	}
};

//////////////////////////////

// Simple fixed size pool.

// There are 3 modes for constructors / destructors:
// * Force trivial (none called)
// * Regular (called on request and free)
// * One-off (called only on construction and destruction of the pool)

template <class T, uint32_t CAPACITY = 8, bool force_trivial = false, bool one_off_construction = false, uint32_t ALIGN = 1, bool PEDANTIC = true>
class FixedPool {
	static_assert(ALIGN > 0, "ALIGN must be at least 1.");
	const static bool CONSTRUCT = !std::is_trivially_constructible<T>::value && !force_trivial;
	const static bool DESTRUCT = !std::is_trivially_destructible<T>::value && !force_trivial;

	// Only using bools here because enums are a nightmare with templates.
	static_assert(((force_trivial && one_off_construction) == false), "Trivial is not compatible with one off construction.");

	// Force trivial in the backing data, the pool is responsible for calling constructors / destructors
	FixedArray<uint32_t, CAPACITY> _freelist;
	FixedArray<T, CAPACITY, true, ALIGN> _list;

public:
	FixedPool() {
		_list.resize(CAPACITY);
		_freelist.resize(CAPACITY);

		for (uint32_t n = 0; n < CAPACITY; n++) {
			_freelist[n] = n;
		}

		if (CONSTRUCT && one_off_construction) {
			for (uint32_t n = 0; n < CAPACITY; n++) {
				memnew_placement(&_list[n], T);
			}
		}
	}
	~FixedPool() {
		if (DESTRUCT && one_off_construction) {
			for (uint32_t n = 0; n < CAPACITY; n++) {
				_list[n].~T();
			}
		}
	}
	bool is_full() const { return _freelist.is_empty(); }
	bool is_empty() const { return _freelist.is_full(); }
	uint32_t capacity() const { return CAPACITY; }

	uint32_t request() {
		if (!_freelist.is_empty()) {
			uint32_t id = *_freelist.pop();

			if (CONSTRUCT && !one_off_construction) {
				memnew_placement(&_list[id], T);
			}

			return id;
		}
		return UINT32_MAX;
	}
	void free(uint32_t p_id) {
		ERR_FAIL_COND(p_id >= CAPACITY);
#ifdef DEV_ENABLED
		if (PEDANTIC) {
			for (uint32_t n = 0; n < _freelist.size(); n++) {
				if (_freelist[n] == p_id) {
					ERR_FAIL_COND_MSG(_freelist[n] == p_id, "Double free detected.");
				}
			}
		}
#endif
		if (DESTRUCT && !one_off_construction) {
			_list[p_id].~T();
		}

		_freelist.push_back(p_id);
	}
	const T &operator[](uint32_t p_index) const {
		return _list[p_index];
	}
	T &operator[](uint32_t p_index) {
		return _list[p_index];
	}
};

#endif // FIXED_ARRAY_H
