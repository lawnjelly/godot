#pragma once

#include "../lvector.h"

namespace LM
{

template <class T> class LightImage
{
public:
	void Create(uint32_t width, uint32_t height)
	{
		m_uiWidth = width;
		m_uiHeight = height;
		m_uiNumPixels = width * height;
		m_Pixels.resize(m_uiNumPixels);
		Blank();
	}

	T * Get(uint32_t p)
	{
		if (p < m_uiNumPixels)
			return &m_Pixels[p];
		return 0;
	}

	const T * Get(uint32_t p) const
	{
		if (p < m_uiNumPixels)
			return &m_Pixels[p];
		return 0;
	}

	T * Get(uint32_t x, uint32_t y)
	{
		uint32_t p = GetPixelNum(x, y);
		if (p < m_uiNumPixels)
			return &m_Pixels[p];
		return 0;
	}
	const T * Get(uint32_t x, uint32_t y) const
	{
		uint32_t p = GetPixelNum(x, y);
		if (p < m_uiNumPixels)
			return &m_Pixels[p];
		return 0;
	}

	const T &GetItem(uint32_t x, uint32_t y) const {return *Get(x, y);}
	T &GetItem(uint32_t x, uint32_t y) {return *Get(x, y);}

	void Fill(const T &val)
	{
		for (uint32_t n=0; n<m_uiNumPixels; n++)
		{
			m_Pixels[n] = val;
		}
	}
	void Blank()
	{
		T val;
		memset(&val, 0, sizeof (T));
		Fill(val);
	}

	uint32_t GetWidth() const {return m_uiWidth;}
	uint32_t GetHeight() const {return m_uiHeight;}
	uint32_t GetNumPixels() const {return m_uiNumPixels;}
	bool IsWithin(int x, int y) const
	{
		if (x < 0) return false;
		if (y < 0) return false;
		if (x >= m_uiWidth) return false;
		if (y >= m_uiHeight) return false;
		return true;
	}

private:
	uint32_t GetPixelNum(uint32_t x, uint32_t y) const
	{
		return (y * m_uiWidth) + x;
	}

	uint32_t m_uiWidth;
	uint32_t m_uiHeight;
	uint32_t m_uiNumPixels;
	LVector<T> m_Pixels;
};

}
