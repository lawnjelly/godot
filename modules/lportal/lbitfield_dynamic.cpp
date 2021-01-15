#include "lbitfield_dynamic.h"

#include <string.h>


namespace Lawn { // namespace start

void LBitField_Dynamic::initialize() {assert (0 && "LBitField_Dynamic : Does not support Initialize, use IT version");}
void LBitField_Dynamic::terminate() {assert (0 && "LBitField_Dynamic : Does not support Terminate, use IT version");}

 
void LBitField_Dynamic_IT::initialize()
{
	initialize_do();
}

void LBitField_Dynamic_IT::terminate()
{
	terminate_do();
}


void LBitField_Dynamic_IT::initialize_do()
{
	memset (this, 0, sizeof (LBitField_Dynamic));
}

void LBitField_Dynamic_IT::terminate_do()
{
	destroy();
}

void LBitField_Dynamic_IT::copy_from(const LBitField_Dynamic_IT &source)
{
	create(source.get_num_bits(), false);
	memcpy(m_pucData, source.get_data(), source.get_num_bytes());
}


void LBitField_Dynamic_IT::create(unsigned int uiNumBits, bool bBlank)
{
	// first delete any initial
	destroy();

	m_uiNumBits = uiNumBits;
	if (uiNumBits)
	{
		m_uiNumBytes = (uiNumBits / 8) + 1;

		m_pucData = new unsigned char[m_uiNumBytes];

		if (bBlank)
			blank(false);
	}
}

void LBitField_Dynamic_IT::destroy()
{
	if (m_pucData)
	{
		delete[] m_pucData;
		m_pucData = 0;
	}

	memset (this, 0, sizeof (LBitField_Dynamic));
}


void LBitField_Dynamic_IT::blank(bool bSetOrZero)
{
	if (bSetOrZero)
	{
		memset(m_pucData, 255, m_uiNumBytes);
	}
	else
	{
		memset(m_pucData, 0, m_uiNumBytes);
	}
}

void LBitField_Dynamic_IT::invert()
{
	for (unsigned int n=0; n<m_uiNumBytes; n++)
	{
		m_pucData[n] = ~m_pucData[n];
	}
}

////////////////////////////////////////////////////////////////////////////



} // namespace end
