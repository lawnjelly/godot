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

#include "core/error_macros.h"
#include <stdint.h>

// High performance, very simple fixed size templates.
// Constructors / Destructors will only be called once on startup and shutdown,
// NOT on each request / free, so clearing e.g. Vectors should be done manually.
// This is to prevent unnecessary reallocations.

template <class T, class U = uint32_t, U CAPACITY = 8>
class FixedArray {
	U _size;
	T _list[CAPACITY];

public:
	U size() const { return _size; }
	bool is_empty() const { return !_size; }
	bool is_full() const { return _size == CAPACITY; }

	T *request() {
		if (size() < CAPACITY) {
			return &_list[_size++];
		}
		return nullptr;
	}
	void push_back(const T &p_val) {
		T *mem = request();
		ERR_FAIL_NULL(mem);
		*mem = p_val;
	}

	T *pop() {
		if (size()) {
			return &_list[_size--];
		}
		return nullptr;
	}
	void clear() {
		_size = 0;
	}
	void resize(U p_size) {
		_size = p_size;
	}
	void fill(const T &p_val) {
		for (U n = 0; n < CAPACITY; n++) {
			_list[n] = p_val;
		}
	}
	const T &operator[](U p_index) const {
		CRASH_BAD_UNSIGNED_INDEX(p_index, size());
		return _list[p_index];
	}
	T &operator[](U p_index) {
		CRASH_BAD_UNSIGNED_INDEX(p_index, size());
		return _list[p_index];
	}
};

template <class T, class U = uint32_t, U CAPACITY = 8, U INVALID = UINT32_MAX>
class FixedPool {
	FixedArray<uint32_t, uint32_t, CAPACITY> _freelist;
	FixedArray<T, U, CAPACITY> _list;

public:
	FixedPool() {
		_list.resize(CAPACITY);
		// Initialize freelist
		_freelist.resize(CAPACITY);
		for (U n = 0; n < CAPACITY; n++) {
			_freelist[n] = n;
		}
	}
	bool is_full() const { return _freelist.is_empty(); }
	bool is_empty() const { return _freelist.is_full(); }
	U capacity() const { return CAPACITY; }

	U request() {
		if (!_freelist.is_empty()) {
			return *_freelist.pop();
		}
		return INVALID;
	}
	void free(U p_id) {
		ERR_FAIL_COND(p_id >= CAPACITY);
		_freelist.push_back(p_id);
	}
	const T &operator[](U p_index) const {
		return _list[p_index];
	}
	T &operator[](U p_index) {
		return _list[p_index];
	}
};

#endif // FIXED_ARRAY_H
