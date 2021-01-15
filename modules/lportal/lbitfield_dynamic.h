#pragma once

#include <assert.h>

namespace Lawn { // namespace start

class LBitField_Dynamic_IT
{
public:
	// construction
	void initialize();
	void terminate();

private:
	// prevent copying (see effective C++ scott meyers)
	// there is no implementation for copy constructor, hence compiler will complain if you try to copy.
	LBitField_Dynamic_IT& operator=(const LBitField_Dynamic_IT&);
public:

	// create automatically blanks
	void create(unsigned int uiNumBits, bool bBlank = true);
	void destroy();

	// public funcs
	inline unsigned int get_num_bits() const {return m_uiNumBits;}
	inline unsigned int get_bit(unsigned int uiBit) const;
	inline void set_bit(unsigned int uiBit, unsigned int bSet);
	bool check_and_set(unsigned int uiBit);
	void blank(bool bSetOrZero = false);
	void invert();
	void copy_from(const LBitField_Dynamic_IT &source);

	// loading / saving
	unsigned char * get_data() {return m_pucData;}
	const unsigned char * get_data() const {return m_pucData;}
	unsigned int get_num_bytes() const {return m_uiNumBytes;}

protected:
	// member funcs
	void initialize_do();
	void terminate_do();

	// member vars
	unsigned char * m_pucData;
	unsigned int m_uiNumBytes;
	unsigned int m_uiNumBits;
};

class LBitField_Dynamic : public LBitField_Dynamic_IT
{
public:
	// call initialize and terminate automatically
	LBitField_Dynamic(unsigned int uiNumBits) {initialize_do(); create(uiNumBits);}
	LBitField_Dynamic() {initialize_do();}
	~LBitField_Dynamic() {terminate_do();}

	// disallow explicit calls
	void initialize();
	void terminate();
};


//////////////////////////////////////////////////////////
inline unsigned int LBitField_Dynamic_IT::get_bit(unsigned int uiBit) const
{
	assert (m_pucData);
	unsigned int uiByteNumber = uiBit >> 3; // divide by 8
	assert (uiByteNumber < m_uiNumBytes);
	unsigned char uc = m_pucData[uiByteNumber];
	unsigned int uiBitSet = uc & (1 << (uiBit & 7));
	return uiBitSet;
}

inline bool LBitField_Dynamic_IT::check_and_set(unsigned int uiBit)
{
	assert (m_pucData);
	unsigned int uiByteNumber = uiBit >> 3; // divide by 8
	assert (uiByteNumber < m_uiNumBytes);
	unsigned char &uc = m_pucData[uiByteNumber];
	unsigned int uiMask = (1 << (uiBit & 7));
	unsigned int uiBitSet = uc & uiMask;
	if (uiBitSet)
		return false;

	// set
	uc = uc | uiMask;
	return true;
}


inline void LBitField_Dynamic_IT::set_bit(unsigned int uiBit, unsigned int bSet)
{
	assert (m_pucData);
	unsigned int uiByteNumber = uiBit >> 3; // divide by 8
	assert (uiByteNumber < m_uiNumBytes);
	unsigned char uc = m_pucData[uiByteNumber];
	unsigned int uiMask = 1 << (uiBit & 7);
	if (bSet)
	{
		uc = uc | uiMask;
	}
	else
	{
		uc &= ~uiMask;
	}
	m_pucData[uiByteNumber] = uc;
}

} // namespace end
