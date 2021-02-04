/*************************************************************************/
/*  bitfield_dynamic.h                                                   */
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

#ifndef BITFIELD_DYNAMIC_H
#define BITFIELD_DYNAMIC_H

#include "core/error_macros.h"

class BitFieldDynamic {
public:
	BitFieldDynamic();
	~BitFieldDynamic() { destroy(); }

private:
	// prevent copying (see effective C++ scott meyers)
	// there is no implementation for copy constructor, hence compiler will complain if you try to copy
	// feel free to add one if needed...
	BitFieldDynamic &operator=(const BitFieldDynamic &);

public:
	// create automatically blanks
	void create(unsigned int p_num_bits, bool bBlank = true);
	void destroy();

	// public funcs
	unsigned int get_num_bits() const { return _num_bits; }
	unsigned int get_bit(unsigned int p_bit) const;
	void set_bit(unsigned int p_bit, unsigned int p_set);
	bool check_and_set(unsigned int p_bit);
	void blank(bool p_set_or_zero = false);
	void invert();
	void copy_from(const BitFieldDynamic &p_source);

	// loading / saving
	unsigned char *get_data() { return _data; }
	const unsigned char *get_data() const { return _data; }
	unsigned int get_num_bytes() const { return _num_bytes; }

protected:
	// member vars
	unsigned char *_data;
	unsigned int _num_bytes;
	unsigned int _num_bits;
};

inline unsigned int BitFieldDynamic::get_bit(unsigned int p_bit) const {
	DEV_ASSERT(_data);
	unsigned int byte_number = p_bit >> 3; // divide by 8
	DEV_ASSERT(byte_number < _num_bytes);
	unsigned char uc = _data[byte_number];
	unsigned int bit_set = uc & (1 << (p_bit & 7));
	return bit_set;
}

inline bool BitFieldDynamic::check_and_set(unsigned int p_bit) {
	DEV_ASSERT(_data);
	unsigned int byte_number = p_bit >> 3; // divide by 8
	DEV_ASSERT(byte_number < _num_bytes);
	unsigned char &uc = _data[byte_number];
	unsigned int mask = (1 << (p_bit & 7));
	unsigned int bit_set = uc & mask;
	if (bit_set) {
		return false;
	}

	// set
	uc = uc | mask;
	return true;
}

inline void BitFieldDynamic::set_bit(unsigned int p_bit, unsigned int p_set) {
	DEV_ASSERT(_data);
	unsigned int byte_number = p_bit >> 3; // divide by 8
	DEV_ASSERT(byte_number < _num_bytes);
	unsigned char uc = _data[byte_number];
	unsigned int mask = 1 << (p_bit & 7);
	if (p_set) {
		uc = uc | mask;
	} else {
		uc &= ~mask;
	}
	_data[byte_number] = uc;
}

#endif
