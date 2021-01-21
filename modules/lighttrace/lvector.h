/*************************************************************************/
/*  lvector.h                                                            */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
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

#pragma once

// just a light wrapper around a vector until we get the Godot vector allocation issues sorted
#include "core/vector.h"
#include <assert.h>
#include <vector>

template <class T>
class LVector {
public:
	// array subscript access
	// note this is not available in Godot Vector
	T &operator[](unsigned int ui) {
#ifdef DEBUG_ENABLED
		assert(ui < (unsigned int)_size);
#endif
		return _vec[ui];
	}

	const T &operator[](unsigned int ui) const {
#ifdef DEBUG_ENABLED
		assert(ui < (unsigned int)_size);
#endif
		return _vec[ui];
	}

	void clear(bool bCompact = false) {
		_size = 0;
		if (bCompact)
			compact();
	}

	void blank() {
		if (!_vec.size()) return;
		memset(&_vec[0], 0, sizeof(T) * _vec.size());
	}

	void compact() {
		_vec.resize(_size);
	}

	void reserve(int s) {
		_vec.resize(s);
		_size = 0;
	}

	void resize(int s, bool bCompact = false) {
		// new size
		_size = s;

		// if compacting is not desired, no need to shrink vector
		if (_size < (int)_vec.size()) {
			if (!bCompact) {
				return;
			}
		}

		_vec.resize(s);
	}

	void set(unsigned int ui, const T &t) {
#ifdef DEBUG_ENABLED
		assert(ui < (unsigned int)_size);
#endif
		_vec[ui] = t;
	}

	// efficient unsorted
	void remove_unsorted(unsigned int ui) {
		// just swap the end element and decrement count
		_vec[ui] = _vec[_size - 1];
		_size--;
	}

	void remove_last() {
		if (_size)
			_size--;
	}

	T *request() {
		_size++;
		if (_size >= (int)_vec.size())
			grow();

		return &_vec[_size - 1];
	}

	void grow() {
		int new_size = _vec.size() * 2;
		if (!new_size) new_size = 1;

		int s = _size;
		resize(new_size);
		_size = s;
	}

	void push_back(const T &t) {
		int size_p1 = _size + 1;

		if (size_p1 < (int)_vec.size()) {
			int size = _size;
			_size = size_p1;
			set(size, t);
		} else {
			// need more space
			grow();

			// call recursive
			push_back(t);
		}
	}

	void copy_from(const LVector<T> &o) {
		// make sure enough space
		if (o.size() > (int)_vec.size()) {
			resize(o.size());
		}

		clear();
		_size = o.size();

		for (int n = 0; n < o.size(); n++) {
			set(n, o[n]);
		}
	}

	void copy_from(const Vector<T> &o) {
		// make sure enough space
		if (o.size() > (int)_vec.size()) {
			resize(o.size());
		}

		clear();
		_size = o.size();

		for (int n = 0; n < o.size(); n++) {
			set(n, o[n]);
		}
	}

	void insert(int i, const T &val) {
		_vec.insert(_vec.begin() + i, val);
		_size++;
	}

	int find(const T &val) {
		for (int n = 0; n < size(); n++) {
			if (_vec[n] == val)
				return n;
		}

		return -1; // not found
	}

	void delete_items_first(unsigned int uiNumItems) {
		if (uiNumItems < (unsigned int)size()) {
			unsigned int num_to_move = size() - uiNumItems;

			if (num_to_move) {
				memmove(&_vec[0], &_vec[uiNumItems], num_to_move * sizeof(T));
			}
			_size -= uiNumItems;
		} else {
			if (uiNumItems == (unsigned int)size()) {
				clear();
			} else {
				assert(0 && "delete_items_first : Not enough items");
			}
		}
	}

	LVector() {
		_size = 0;
	}

	int size() const { return _size; }

private:
	std::vector<T> _vec;

	// working size
	int _size;
};
