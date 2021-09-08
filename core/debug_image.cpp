#include "debug_image.h"

#include "core/image.h"

bool DebugImage::create(unsigned int width, unsigned int height) {
	reset();
	if (width > 4096)
		return false;
	if (height > 4096)
		return false;

	_width = width;
	_height = height;
	_num_pixels = width * height;
	_data.resize(_num_pixels);
	fill();

	l_begin();
	return true;
}

void DebugImage::fill() {
	for (unsigned int n = 0; n < _data.size(); n++) {
		_data[n] = _fill_col;
	}
}

void DebugImage::reset() {
	_data.clear();
	_width = 0;
	_height = 0;
	_num_pixels = 0;

	_brush_col = Col(255, 255, 255, 255);
	_fill_col = Col(0, 0, 0, 255);
	_mark_col = Col(255, 0, 255, 255);
	_cursor_x = 0;
	_cursor_y = 0;
}

void DebugImage::l_flush(bool clear_logical_viewport) {
	// calculate overall viewport
	_logical_viewport.position = _commands_rect_min;
	_logical_viewport.size = _commands_rect_max - _commands_rect_min;

	// add a little for luck
	_logical_viewport.size *= 1.01;

	for (int n = 0; n < _commands.size(); n++) {
		const Command &c = _commands[n];

		switch (c.type) {
			case CT_MOVE: {
				int x, y;
				l_to_i(c.x, c.y, x, y);
				move(x, y);
			} break;
			case CT_LINE_TO: {
				int x, y;
				l_to_i(c.x, c.y, x, y);
				line_to(x, y);
			} break;
			case CT_DRAW_NUM: {
				draw_num(c.num);
			} break;
			default: {
			} break;
		}
	}

	l_begin(clear_logical_viewport);
}

void DebugImage::l_begin(bool clear_logical_viewport) {
	_commands.clear();

	if (clear_logical_viewport) {
		_commands_rect_min = Vector2(FLT_MAX, FLT_MAX);
		_commands_rect_max = Vector2(-FLT_MAX, -FLT_MAX);
	}
}

void DebugImage::move(int x, int y) {
	_cursor_x = x;
	_cursor_y = y;
	Col c = Col(0, 255, 0, 255);
	mark(4, &c);
}

void DebugImage::pset_bin(int x, int y, int bin) {
	if ((bin >> 3) & 1) {
		pset(x, y);
	}
	if ((bin >> 2) & 1) {
		pset(x + 1, y);
	}
	if ((bin >> 1) & 1) {
		pset(x + 2, y);
	}
	if ((bin)&1) {
		pset(x + 3, y);
	}
}

void DebugImage::_draw_digit(int num, int ox, int oy) {
	//	int x = _cursor_x + ox;
	//	int y = _cursor_y + oy;
	int x = ox;
	int y = oy;

	switch (num) {
		default: {
			pset_bin(x, y + 0, 0b0110);
			pset_bin(x, y + 1, 0b1001);
			pset_bin(x, y + 2, 0b1001);
			pset_bin(x, y + 3, 0b1001);
			pset_bin(x, y + 4, 0b0110);
		} break;
		case 1: {
			pset_bin(x, y + 0, 0b0110);
			pset_bin(x, y + 1, 0b0010);
			pset_bin(x, y + 2, 0b0010);
			pset_bin(x, y + 3, 0b0010);
			pset_bin(x, y + 4, 0b0010);
		} break;
		case 2: {
			pset_bin(x, y + 0, 0b0110);
			pset_bin(x, y + 1, 0b1001);
			pset_bin(x, y + 2, 0b0010);
			pset_bin(x, y + 3, 0b0100);
			pset_bin(x, y + 4, 0b1111);
		} break;
		case 3: {
			pset_bin(x, y + 0, 0b1110);
			pset_bin(x, y + 1, 0b0001);
			pset_bin(x, y + 2, 0b0110);
			pset_bin(x, y + 3, 0b0001);
			pset_bin(x, y + 4, 0b1110);
		} break;
		case 4: {
			pset_bin(x, y + 0, 0b0110);
			pset_bin(x, y + 1, 0b1010);
			pset_bin(x, y + 2, 0b1111);
			pset_bin(x, y + 3, 0b0010);
			pset_bin(x, y + 4, 0b0010);
		} break;
		case 5: {
			pset_bin(x, y + 0, 0b1111);
			pset_bin(x, y + 1, 0b1000);
			pset_bin(x, y + 2, 0b1110);
			pset_bin(x, y + 3, 0b0001);
			pset_bin(x, y + 4, 0b1110);
		} break;
		case 6: {
			pset_bin(x, y + 0, 0b0111);
			pset_bin(x, y + 1, 0b1000);
			pset_bin(x, y + 2, 0b1110);
			pset_bin(x, y + 3, 0b1001);
			pset_bin(x, y + 4, 0b0110);
		} break;
		case 7: {
			pset_bin(x, y + 0, 0b1111);
			pset_bin(x, y + 1, 0b0001);
			pset_bin(x, y + 2, 0b0010);
			pset_bin(x, y + 3, 0b0010);
			pset_bin(x, y + 4, 0b0010);
		} break;
		case 8: {
			pset_bin(x, y + 0, 0b0110);
			pset_bin(x, y + 1, 0b1001);
			pset_bin(x, y + 2, 0b0110);
			pset_bin(x, y + 3, 0b1001);
			pset_bin(x, y + 4, 0b0110);
		} break;
		case 9: {
			pset_bin(x, y + 0, 0b0111);
			pset_bin(x, y + 1, 0b1001);
			pset_bin(x, y + 2, 0b0111);
			pset_bin(x, y + 3, 0b0001);
			pset_bin(x, y + 4, 0b0110);
		} break;
	}
}

void DebugImage::draw_num(int num) {
	int digits = 1;
	if (num > 9)
		digits = 2;

	int x = _cursor_x;
	int y = _cursor_y;

	if ((_height - y) < 10) {
		y -= 22;
	}

	if ((_width - x) < 10) {
		x -= 22;
	}

	if (num <= 9) {
		_draw_digit(num % 10, x + 6, y + 8);
	} else {
		_draw_digit(num % 10, x + 12, y + 8);
		_draw_digit((num / 10) % 10, x + 6, y + 8);
	}
}

void DebugImage::mark(int size, const Col *col) {
	Col c = _mark_col;
	if (col) {
		c = *col;
	}

	for (int y = _cursor_y - size; y <= _cursor_y + size; y++) {
		for (int x = _cursor_x - size; x <= _cursor_x + size; x++) {
			pset(x, y, c);
		}
	}
}

void DebugImage::line_to(int x, int y) {
	draw_line_wu(_cursor_x, _cursor_y, x, y, 0, 256, 8);
	_cursor_x = x;
	_cursor_y = y;
	mark();
}

void DebugImage::draw_line_wu(short X0, short Y0, short X1, short Y1,
		short BaseColor, short NumLevels, unsigned short IntensityBits) {
	unsigned short IntensityShift, ErrorAdj, ErrorAcc;
	unsigned short ErrorAccTemp, Weighting, WeightingComplementMask;
	short DeltaX, DeltaY, Temp, XDir;

	/* Make sure the line runs top to bottom */
	if (Y0 > Y1) {
		Temp = Y0;
		Y0 = Y1;
		Y1 = Temp;
		Temp = X0;
		X0 = X1;
		X1 = Temp;
	}
	/* Draw the initial pixel, which is always exactly intersected by
	   the line and so needs no weighting */
	draw_pixel_wu(X0, Y0, BaseColor);

	if ((DeltaX = X1 - X0) >= 0) {
		XDir = 1;
	} else {
		XDir = -1;
		DeltaX = -DeltaX; /* make DeltaX positive */
	}
	/* Special-case horizontal, vertical, and diagonal lines, which
	   require no weighting because they go right through the center of
	   every pixel */
	if ((DeltaY = Y1 - Y0) == 0) {
		/* Horizontal line */
		while (DeltaX-- != 0) {
			X0 += XDir;
			draw_pixel_wu(X0, Y0, BaseColor);
		}
		return;
	}
	if (DeltaX == 0) {
		/* Vertical line */
		do {
			Y0++;
			draw_pixel_wu(X0, Y0, BaseColor);
		} while (--DeltaY != 0);
		return;
	}
	if (DeltaX == DeltaY) {
		/* Diagonal line */
		do {
			X0 += XDir;
			Y0++;
			draw_pixel_wu(X0, Y0, BaseColor);
		} while (--DeltaY != 0);
		return;
	}
	/* Line is not horizontal, diagonal, or vertical */
	ErrorAcc = 0; /* initialize the line error accumulator to 0 */
	/* # of bits by which to shift ErrorAcc to get intensity level */
	IntensityShift = 16 - IntensityBits;
	/* Mask used to flip all bits in an intensity weighting, producing the
	   result (1 - intensity weighting) */
	WeightingComplementMask = NumLevels - 1;
	/* Is this an X-major or Y-major line? */
	if (DeltaY > DeltaX) {
		/* Y-major line; calculate 16-bit fixed-point fractional part of a
		  pixel that X advances each time Y advances 1 pixel, truncating the
		  result so that we won't overrun the endpoint along the X axis */
		ErrorAdj = ((unsigned long)DeltaX << 16) / (unsigned long)DeltaY;
		/* Draw all pixels other than the first and last */
		while (--DeltaY) {
			ErrorAccTemp = ErrorAcc; /* remember currrent accumulated error */
			ErrorAcc += ErrorAdj; /* calculate error for next pixel */
			if (ErrorAcc <= ErrorAccTemp) {
				/* The error accumulator turned over, so advance the X coord */
				X0 += XDir;
			}
			Y0++; /* Y-major, so always advance Y */
			/* The IntensityBits most significant bits of ErrorAcc give us the
			 intensity weighting for this pixel, and the complement of the
			 weighting for the paired pixel */
			Weighting = ErrorAcc >> IntensityShift;
			draw_pixel_wu(X0, Y0, BaseColor + Weighting);
			draw_pixel_wu(X0 + XDir, Y0,
					BaseColor + (Weighting ^ WeightingComplementMask));
		}
		/* Draw the final pixel, which is
		  always exactly intersected by the line
		  and so needs no weighting */
		draw_pixel_wu(X1, Y1, BaseColor);
		return;
	}
	/* It's an X-major line; calculate 16-bit fixed-point fractional part of a
	   pixel that Y advances each time X advances 1 pixel, truncating the
	   result to avoid overrunning the endpoint along the X axis */
	ErrorAdj = ((unsigned long)DeltaY << 16) / (unsigned long)DeltaX;
	/* Draw all pixels other than the first and last */
	while (--DeltaX) {
		ErrorAccTemp = ErrorAcc; /* remember currrent accumulated error */
		ErrorAcc += ErrorAdj; /* calculate error for next pixel */
		if (ErrorAcc <= ErrorAccTemp) {
			/* The error accumulator turned over, so advance the Y coord */
			Y0++;
		}
		X0 += XDir; /* X-major, so always advance X */
		/* The IntensityBits most significant bits of ErrorAcc give us the
		  intensity weighting for this pixel, and the complement of the
		  weighting for the paired pixel */
		Weighting = ErrorAcc >> IntensityShift;
		draw_pixel_wu(X0, Y0, BaseColor + Weighting);
		draw_pixel_wu(X0, Y0 + 1,
				BaseColor + (Weighting ^ WeightingComplementMask));
	}
	/* Draw the final pixel, which is always exactly intersected by the line
	   and so needs no weighting */
	draw_pixel_wu(X1, Y1, BaseColor);
}

bool DebugImage::save_png(String p_filename) {
	PoolVector<uint8_t> pool_data;
	pool_data.resize(_data.size() * 4);

	int count = 0;
	for (int n = 0; n < _num_pixels; n++) {
		const Col &c = _data[n];
		pool_data.set(count++, c.r);
		pool_data.set(count++, c.g);
		pool_data.set(count++, c.b);
		pool_data.set(count++, c.a);
	}

	Ref<Image> im;
	im.instance();
	im->create(_width, _height, false, Image::FORMAT_RGBA8, pool_data);

	return im->save_png(p_filename) == OK;
}
