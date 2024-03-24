#pragma once

#include "lvector.h"

namespace LM {

template <class T>
class LightImage {
public:
	LightImage() {
		_width = 0;
		_height = 0;
		_num_pixels = 0;
	}

	void reset() {
		create(0, 0);
	}

	void create(uint32_t width, uint32_t height, bool p_blank = true) {
		_width = width;
		_height = height;
		_num_pixels = width * height;
		_pixels.resize(_num_pixels);
		if (p_blank)
			blank();
	}

	T *get(uint32_t p) {
		if (p < _num_pixels)
			return &_pixels[p];
		return 0;
	}

	const T *get(uint32_t p) const {
		if (p < _num_pixels)
			return &_pixels[p];
		return 0;
	}

	T *get(uint32_t x, uint32_t y) {
		uint32_t p = get_pixel_num(x, y);
		if (p < _num_pixels)
			return &_pixels[p];
		return 0;
	}
	const T *get(uint32_t x, uint32_t y) const {
		uint32_t p = get_pixel_num(x, y);
		if (p < _num_pixels)
			return &_pixels[p];
		return 0;
	}

	const T &get_item(uint32_t x, uint32_t y) const { return *get(x, y); }
	T &get_item(uint32_t x, uint32_t y) { return *get(x, y); }

	void fill(const T &val) {
		for (uint32_t n = 0; n < _num_pixels; n++) {
			_pixels[n] = val;
		}
	}
	void blank() {
		T val;
		memset(&val, 0, sizeof(T));
		fill(val);
	}

	uint32_t get_width() const { return _width; }
	uint32_t get_height() const { return _height; }
	uint32_t get_num_pixels() const { return _num_pixels; }
	bool is_within(int x, int y) const {
		if (x < 0)
			return false;
		if (y < 0)
			return false;
		if (x >= (int)_width)
			return false;
		if (y >= (int)_height)
			return false;
		return true;
	}

	void copy_to(LightImage<T> &dest) const {
		assert(dest.get_num_pixels() == get_num_pixels());

		for (unsigned int n = 0; n < _num_pixels; n++) {
			dest._pixels[n] = _pixels[n];
		}
	}

private:
	uint32_t get_pixel_num(uint32_t x, uint32_t y) const {
		return (y * _width) + x;
	}

	uint32_t _width;
	uint32_t _height;
	uint32_t _num_pixels;
	LVector<T> _pixels;
};

} //namespace LM
