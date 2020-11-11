#include "bitfield.h"

Bitfield::Bitfield()
{
	_num_bits = 0;
}

Bitfield::~Bitfield()
{
}

void Bitfield::blank(bool p_set_or_zero)
{
	int num_bytes = _data.size();

	if (!num_bytes)
		return;

	uint8_t c = 0;
	if (p_set_or_zero)
		c = 255;

	memset(&_data[0], c, num_bytes);
}


void Bitfield::resize(uint32_t p_num_bits, bool p_blank)
{
	_num_bits = p_num_bits;

	// number of bytes needed
	int num_bytes = _num_bits / 8;

	// remainder? needs multiples of 8 bits
	if (_num_bits % 8)
		num_bytes++;

	_data.resize(num_bytes);

	if (p_blank)
	{
		blank();
	}
}

void Bitfield::invert()
{
	int num_bytes = _data.size();

	for (int n=0; n<num_bytes; n++)
	{
		_data[n] = ~_data[n];
	}
}

uint8_t * Bitfield::get_data()
{
	if (_data.size())
	{
		return &_data[0];
	}
	return nullptr;
}

const uint8_t * Bitfield::get_data() const
{
	if (_data.size())
	{
		return &_data[0];
	}
	return nullptr;
}
