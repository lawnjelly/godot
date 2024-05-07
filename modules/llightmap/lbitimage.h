#pragma once

#include "core/bitfield_dynamic.h"

namespace LM { // namespace start

class LBitImage {
	BitFieldDynamic _bf;
	uint32_t _width;
	uint32_t _height;

public:
	void create(uint32_t p_width, uint32_t p_height, bool p_blank = true);
	void destroy();

	uint32_t get_num_pixels() const { return _bf.get_num_bits(); }
	uint32_t get_width() const { return _width; }
	uint32_t get_height() const { return _height; }
	bool dimensions_match(uint32_t p_width, uint32_t p_height) const { return (_width == p_width) && (_height == p_height); }

	uint32_t get_pixel_num(uint32_t p_x, uint32_t p_y) const { return (p_y * _width) + p_x; }
	void blank(bool p_set_or_zero = false) { _bf.blank(p_set_or_zero); }
	void invert() { _bf.invert(); }

	bool is_within(int32_t p_x, int32_t p_y) const {
		if ((uint32_t)p_x >= _width)
			return false;
		if ((uint32_t)p_y >= _height)
			return false;
		return true;
	}

	bool get_pixel(int32_t p_x, int32_t p_y, bool p_checked = true) const {
		if (p_checked) {
			ERR_FAIL_COND_V(!is_within(p_x, p_y), false);
		}
		uint32_t p = get_pixel_num(p_x, p_y);
		return _bf.get_bit(p) != 0;
	}
	void set_pixel(int32_t p_x, int32_t p_y, bool p_set, bool p_checked = true) {
		if (p_checked) {
			ERR_FAIL_COND(!is_within(p_x, p_y));
		}
		uint32_t p = get_pixel_num(p_x, p_y);
		_bf.set_bit(p, p_set);
	}

	Error load(String p_filename, uint8_t *r_extra_data = nullptr, uint32_t *r_extra_data_size = nullptr, uint32_t p_max_extra_data_size = 0);
	Error save(String p_filename, const uint8_t *p_extra_data = nullptr, uint32_t p_extra_data_size = 0);

	uint32_t count(bool p_count_set = true) const;
};

} //namespace LM
