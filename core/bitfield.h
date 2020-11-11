#pragma once

#include "local_vector.h"

class Bitfield
{
public:
	Bitfield();
	~Bitfield();

	// use
	void resize(uint32_t p_num_bits, bool p_blank = true);
	void blank(bool p_set_or_zero = false);
	void invert();

	// inlines
	uint32_t get_num_bits() const {return _num_bits;}
	bool get_bit(unsigned int p_bit, bool p_fast = false) const;
	void set_bit(unsigned int p_bit, bool p_set = true, bool p_fast = false);
	bool check_and_set_bit(unsigned int p_bit, bool p_set, bool p_fast = false);
	void clear_bit(unsigned int p_bit, bool p_fast = false) {set_bit(p_bit, false, p_fast);}

	// loading / saving
	uint8_t * get_data();
	const uint8_t * get_data() const;
	int get_num_bytes() const {return _data.size();}

private:
	uint32_t _num_bits;
	LocalVector<uint8_t, uint32_t, true>_data;
};

inline bool Bitfield::get_bit(unsigned int p_bit, bool p_fast) const
{
	unsigned int byte = p_bit >> 3; // divide by 8

	// should get compiled out in release
	if (!p_fast)
	{
		ERR_FAIL_INDEX_V(byte, _data.size(), false);
	}

	uint8_t c = _data[byte];
	unsigned int bit_is_set = c & (1 << (p_bit & 7));

	return bit_is_set != 0;
}

inline void Bitfield::set_bit(unsigned int p_bit, bool p_set, bool p_fast)
{
	unsigned int byte = p_bit >> 3; // divide by 8

	// should get compiled out in release
	if (!p_fast)
	{
		ERR_FAIL_INDEX(byte, _data.size());
	}

	uint8_t c = _data[byte];
	uint8_t mask = 1 << (p_bit & 7);
	if (p_set)
	{
		c |= mask;
	}
	else
	{
		c &= ~mask;
	}
	_data[byte] = c;
}

inline bool Bitfield::check_and_set_bit(unsigned int p_bit, bool p_set, bool p_fast)
{
	unsigned int byte = p_bit >> 3; // divide by 8

	// should get compiled out in release
	if (!p_fast)
	{
		ERR_FAIL_INDEX_V(byte, _data.size(), false);
	}

	uint8_t &c = _data[byte];
	uint8_t mask = 1 << (p_bit & 7);

	unsigned int already_set = c & mask;

	if (p_set)
	{
		if (already_set) return false;
		c |= mask;
	}
	else
	{
		if (!already_set) return false;
		c &= ~mask;
	}

	return true;
}
