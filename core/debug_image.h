#pragma once

#include "core/local_vector.h"
#include "core/math/rect2.h"
#include <stdint.h>

// simple 32 bit RGBA image with line drawing funcs
// and saving .. for debugging geometry.

class DebugImage {
public:
	struct Col {
		Col() {}
		Col(uint8_t p_r, uint8_t p_g, uint8_t p_b, uint8_t p_a) {
			r = p_r;
			g = p_g;
			b = p_b;
			a = p_a;
		}
		void set(uint8_t p_r, uint8_t p_g, uint8_t p_b, uint8_t p_a) {
			r = p_r;
			g = p_g;
			b = p_b;
			a = p_a;
		}
		void scale(uint8_t p_scale) {
			r = ((unsigned int)r * p_scale) >> 8;
			g = ((unsigned int)g * p_scale) >> 8;
			b = ((unsigned int)b * p_scale) >> 8;
		}
		uint8_t r, g, b, a;
	};

private:
	enum CommandType {
		CT_MOVE,
		CT_LINE_TO,
		CT_SET_BRUSH_COLOR,
		CT_DRAW_NUM,
	};

	struct Command {
		CommandType type;
		real_t x;
		real_t y;
		Col color;
		int num;
	};
	LocalVectori<Command> _commands;
	Vector2 _commands_rect_min;
	Vector2 _commands_rect_max;

public:
	bool create(unsigned int width, unsigned int height);
	void fill();
	void clear() { fill(); }
	void reset();

	void set_brush_color(const Col &col) { _brush_col = col; }
	void set_fill_color(const Col &col) { _fill_col = col; }

	// inline for speed
	void pset_bin(int x, int y, int bin);
	bool pset(int x, int y) {
		return pset(x, y, _brush_col);
	}
	bool pset(int x, int y, const Col &col) {
		unsigned int pnum;
		if (!get_pixel_num(x, y, pnum))
			return false;

		_data[pnum] = col;
		return true;
	}
	void move(int x, int y);
	void line_to(int x, int y);
	void line(int x_from, int y_from, int x_to, int y_to) {
		move(x_from, y_from);
		line_to(x_to, y_to);
	}
	void mark(int size = 2, const Col *col = nullptr);
	void draw_num(int num);

	bool save_png(String p_filename);

	// viewports
	void l_to_i(real_t x, real_t y, int &r_x, int &r_y) const {
		x -= _logical_viewport.position.x;
		y -= _logical_viewport.position.y;

		if (_logical_viewport.size.x > 0.001) {
			x /= _logical_viewport.size.x;
			r_x = x * _width;
		} else {
			r_x = 0;
		}
		if (_logical_viewport.size.y > 0.001) {
			y /= _logical_viewport.size.y;
			r_y = y * _height;
		} else {
			r_y = 0;
		}
	}

	void l_move(const Vector2 &pt) {
		l_move(pt.x, pt.y);
	}

	void l_line_to(const Vector2 &pt) {
		l_line_to(pt.x, pt.y);
	}

	void l_expand(real_t fx, real_t fy) {
		_commands_rect_min.x = MIN(_commands_rect_min.x, fx);
		_commands_rect_min.y = MIN(_commands_rect_min.y, fy);
		_commands_rect_max.x = MAX(_commands_rect_max.x, fx);
		_commands_rect_max.y = MAX(_commands_rect_max.y, fy);
	}

	void l_draw_num(int num) {
		Command c;
		c.type = CT_DRAW_NUM;
		c.num = num;
		_commands.push_back(c);
	}

	void l_move(real_t fx, real_t fy) {
		fy = -fy;

		Command c;
		c.type = CT_MOVE;
		c.x = fx;
		c.y = fy;
		_commands.push_back(c);
		l_expand(fx, fy);
	}

	void l_line_to(real_t fx, real_t fy) {
		fy = -fy;

		Command c;
		c.type = CT_LINE_TO;
		c.x = fx;
		c.y = fy;
		_commands.push_back(c);
		l_expand(fx, fy);
	}
	void l_line(real_t fx_from, real_t fy_from, real_t fx_to, real_t fy_to) {
		l_move(fx_from, fy_from);
		l_line_to(fx_to, fy_to);
	}

	void l_begin(bool clear_logical_viewport = true);
	void l_flush(bool clear_logical_viewport = true);

	void set_logical_viewport(const Rect2 &rt) {
		_logical_viewport = rt;
	}

private:
	bool get_pixel_num(unsigned int x, unsigned int y, unsigned int &num) const {
		if (x >= _width)
			return false;
		if (y >= _height)
			return false;
		num = (y * _width) + x;
		return true;
	}

	void _draw_digit(int num, int ox, int oy);

	void draw_line_wu(short X0, short Y0, short X1, short Y1,
			short BaseColor, short NumLevels, unsigned short IntensityBits);

	void draw_pixel_wu(int x, int y, short lum) {
		unsigned int pnum;
		if (!get_pixel_num(x, y, pnum))
			return;

		// cap
		lum = CLAMP(lum, 0, 255);
		lum = 255 - lum;

		Col col = _brush_col;
		col.scale(lum);

		_data[pnum] = col;
	}

	Col _brush_col;
	Col _fill_col;
	Col _mark_col;
	int _cursor_x = 0;
	int _cursor_y = 0;

	// allow scaling everything in the viewport to a floating point area
	Rect2 _logical_viewport;

	LocalVector<Col> _data;
	unsigned int _width = 0;
	unsigned int _height = 0;
	unsigned int _num_pixels = 0;
};
