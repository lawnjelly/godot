/**************************************************************************/
/*  fixed_array.h                                                         */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef FIXED_ARRAY_H
#define FIXED_ARRAY_H

#include "core/error_macros.h"
#include "core/span.h"
#include <type_traits>
#include <utility>

// High performance fixed size array, single threaded.
// Especially useful if you need to create an array on the stack, to
// prevent dynamic allocations (especially in bottleneck code).

template <class T, uint32_t CAPACITY = 8, bool force_trivial = false, uint32_t REQUESTED_ALIGN = 0>
class FixedArray {
	static_assert(CAPACITY > 0, "CAPACITY must be at least 1.");

	// If REQUESTED_ALIGN is 0 or less than the natural alignment, use natural alignment of T.
	const static uint32_t ALIGN = (REQUESTED_ALIGN > alignof(T)) ? REQUESTED_ALIGN : alignof(T);
	const static uint32_t UNIT_SIZE = ((sizeof(T) + ALIGN - 1) / ALIGN * ALIGN);
	const static bool CONSTRUCT = !std::is_trivially_constructible<T>::value && !force_trivial;
	const static bool DESTRUCT = !std::is_trivially_destructible<T>::value && !force_trivial;
	const static bool CAN_MEMCPY = ((ALIGN <= alignof(T) && std::is_trivially_copyable<T>::value) || force_trivial);

	uint32_t _size = 0;

	// alignas ensures the byte buffer is properly aligned in memory, preventing ARM crashes.
	alignas(ALIGN) uint8_t _data[CAPACITY * UNIT_SIZE];

	// Helper pointers to avoid strict aliasing UB.
	const T *get_ptr(uint32_t p_index) const {
		return reinterpret_cast<const T *>(&_data[p_index * UNIT_SIZE]);
	}
	T *get_ptr(uint32_t p_index) {
		return reinterpret_cast<T *>(&_data[p_index * UNIT_SIZE]);
	}

	const T &get(uint32_t p_index) const {
		return *get_ptr(p_index);
	}
	T &get(uint32_t p_index) {
		return *get_ptr(p_index);
	}

public:
	FixedArray() = default;

	~FixedArray() {
		clear();
	}

	uint32_t size() const { return _size; }
	bool is_empty() const { return !_size; }
	bool is_full() const { return _size == CAPACITY; }
	uint32_t capacity() const { return CAPACITY; }

	T *request() _LIFETIME_BOUND_ {
		if (size() < CAPACITY) {
			T *ele = get_ptr(_size++);
			if (CONSTRUCT) {
				memnew_placement(ele, T);
			} else {
				new (ele) T();
			}
			return ele;
		}
		return nullptr;
	}

	T *request_uninitialized() _LIFETIME_BOUND_ {
		if (size() < CAPACITY) {
			T *ele = get_ptr(_size++);
			return ele;
		}
		return nullptr;
	}

	void push_back(const T &p_val) {
		ERR_FAIL_COND(size() >= CAPACITY);
		T *ele = get_ptr(_size++);
		new (ele) T(p_val);
	}

	void push_back(T &&p_val) {
		ERR_FAIL_COND(size() >= CAPACITY);
		T *ele = get_ptr(_size++);
		new (ele) T(std::move(p_val));
	}

	void clear() {
		resize(0);
	}

	void remove_unordered(uint32_t p_index) {
		ERR_FAIL_UNSIGNED_INDEX(p_index, _size);

		_size--;
		if (_size > p_index) {
			get(p_index) = std::move(get(_size));
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

		if (p_size > _size) {
			for (uint32_t i = _size; i < p_size; i++) {
				if (CONSTRUCT) {
					memnew_placement(get_ptr(i), T);
				} else {
					new (get_ptr(i)) T();
				}
			}
		}

		_size = p_size;
	}

	bool pop() {
		if (!size()) {
			return false;
		}
		resize(size() - 1);
		return true;
	}

private:
	void _insert_shift(uint32_t p_index, uint32_t p_max_size) {
		DEV_ASSERT(p_max_size <= CAPACITY);

		int32_t move_end = (int32_t)_size - 1;

		// Grow if possible (this constructs a default element at the end)
		if (_size < p_max_size) {
			resize(_size + 1);
		}

		move_end = MIN(move_end, (int32_t)p_max_size - 2);

		if (move_end < (int32_t)p_index) {
			return; // Nothing to shift
		}

		// Fast path for trivial types
		if (CAN_MEMCPY) {
			// memmove handles overlapping regions safely
			uint8_t *dest = &_data[(p_index + 1) * UNIT_SIZE];
			uint8_t *src = &_data[p_index * UNIT_SIZE];
			size_t bytes = (move_end - p_index + 1) * UNIT_SIZE;
			memmove(dest, src, bytes);
		} else {
			// Element-wise move for non-trivial types
			for (int32_t n = move_end; n >= (int32_t)p_index; n--) {
				get(n + 1) = std::move(get(n));
			}
		}
	}

public:
	void insert(const T &p_val, uint32_t p_index, uint32_t p_max_size = CAPACITY) {
		ERR_FAIL_UNSIGNED_INDEX(p_index, MIN(p_max_size, _size + 1));
		_insert_shift(p_index, p_max_size);
		get(p_index) = p_val;
	}

	void insert(T &&p_val, uint32_t p_index, uint32_t p_max_size = CAPACITY) {
		ERR_FAIL_UNSIGNED_INDEX(p_index, MIN(p_max_size, _size + 1));
		_insert_shift(p_index, p_max_size);
		get(p_index) = std::move(p_val);
	}

	const T &operator[](uint32_t p_index) const {
		DEV_ASSERT(p_index < size());
		return get(p_index);
	}
	T &operator[](uint32_t p_index) {
		DEV_ASSERT(p_index < size());
		return get(p_index);
	}

	const T &last() const {
		DEV_ASSERT(size());
		return (*this)[size() - 1];
	}
	T &last() {
		DEV_ASSERT(size());
		return (*this)[size() - 1];
	}
	const T &first() const {
		DEV_ASSERT(size());
		return (*this)[0];
	}
	T &first() {
		DEV_ASSERT(size());
		return (*this)[0];
	}

	_FORCE_INLINE_ Span<T> span() const _LIFETIME_BOUND_ { return Span((T *)_data, _size); }
	_FORCE_INLINE_ operator Span<T>() const _LIFETIME_BOUND_ { return span(); }

	// c++ Rule of Five...

	// Copy Constructor.
	FixedArray(const FixedArray &p_other) {
		_size = p_other._size;
		if (CAN_MEMCPY) {
			memcpy(_data, p_other._data, _size * UNIT_SIZE);
		} else {
			for (uint32_t i = 0; i < _size; i++) {
				new (get_ptr(i)) T(p_other.get(i));
			}
		}
	}

	// Move Constructor.
	FixedArray(FixedArray &&p_other) {
		_size = p_other._size;
		if (CAN_MEMCPY) {
			memcpy(_data, p_other._data, _size * UNIT_SIZE);
		} else {
			for (uint32_t i = 0; i < _size; i++) {
				new (get_ptr(i)) T(std::move(p_other.get(i)));
			}
		}
		p_other.clear();
	}

	// Copy Assignment.
	FixedArray &operator=(const FixedArray &p_other) {
		if (this != &p_other) {
			clear();
			_size = p_other._size;
			if (CAN_MEMCPY) {
				memcpy(_data, p_other._data, _size * UNIT_SIZE);
			} else {
				for (uint32_t i = 0; i < _size; i++) {
					new (get_ptr(i)) T(p_other.get(i));
				}
			}
		}
		return *this;
	}

	// Move Assignment.
	FixedArray &operator=(FixedArray &&p_other) {
		if (this != &p_other) {
			clear();
			_size = p_other._size;
			if (CAN_MEMCPY) {
				memcpy(_data, p_other._data, _size * UNIT_SIZE);
			} else {
				for (uint32_t i = 0; i < _size; i++) {
					new (get_ptr(i)) T(std::move(p_other.get(i)));
				}
			}
			p_other.clear();
		}
		return *this;
	}
};

#endif // FIXED_ARRAY_H
