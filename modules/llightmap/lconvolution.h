#pragma once

#include "core/os/memory.h"
#include "llightimage.h"
#include "llighttypes.h"

namespace LM {

template <class T>
class Convolution {
public:
	enum {
		NUM_LINES = 3
	};

	Convolution() {
		for (int n = 0; n < NUM_LINES; n++) {
			_p_lines_source[n] = nullptr;
			_p_lines_dest[n] = nullptr;
		}
	}

	~Convolution() {
		destroy();
	}

	void destroy() {
		for (int n = 0; n < NUM_LINES; n++) {
			if (_p_lines_source[n]) {
				memdelete_arr(_p_lines_source[n]);
				_p_lines_source[n] = nullptr;
			}
			if (_p_lines_dest[n]) {
				memdelete_arr(_p_lines_dest[n]);
				_p_lines_dest[n] = nullptr;
			}
		}
	}

	void create_lines() {
		CRASH_COND(_p_lines_source[0]);
		CRASH_COND(!_line_length_expanded);

		for (int n = 0; n < NUM_LINES; n++) {
			_p_lines_source[n] = memnew_arr(T, _line_length_expanded);
			_p_lines_dest[n] = memnew_arr(T, _line_length_expanded);
		}
	}

	void copy_source_line(int source_y, int dest_y) {
		const T *pSource = _p_lines_source[source_y];
		T *pDest = _p_lines_source[dest_y];

		for (int x = 0; x < _line_length_expanded; x++) {
			pDest[x] = pSource[x];
		}
	}

	void load_line(int source_y, int dest_y) {
		const T *pSource = &_p_image->get_item(0, source_y);
		T *pDest = _p_lines_source[dest_y];

		// leave a pixel gap at start and end for filtering
		for (int x = 0; x < _line_length_orig; x++) {
			pDest[x + 1] = pSource[x];
		}

		// wrap start and end
		pDest[0] = pDest[1];
		pDest[_line_length_expanded - 1] = pDest[_line_length_expanded - 2];
	}

	void save_line(int source_y, int dest_y) {
		const T *pSource = _p_lines_dest[source_y];
		pSource++;
		T *pDest = &_p_image->get_item(0, dest_y);

		for (int x = 0; x < _line_length_orig; x++) {
			pDest[x] = pSource[x];
		}
	}

	void zero_pixel(float &f) { f = 0.0f; }
	void zero_pixel(FColor &c) { c.set(0.0f); }
	//	void ZeroPixel(Color &c) { c = Color(0.0f, 0.0f, 0.0f, 1.0f);}

	void adjust_centre(float &centre, float average) {
		float diff = average - centre;
		if (fabsf(diff) < _tolerance) {
			centre = _amount * average + (centre * (1.0f - _amount));
		}
	}

	void adjust_centre(FColor &centre, FColor average) {
		FColor diff = average;
		diff -= centre;

		if ((fabsf(diff.r) < _tolerance) &&
				(fabsf(diff.g) < _tolerance) &&
				(fabsf(diff.b) < _tolerance)) {
			centre *= 1.0f - _amount;
			average *= _amount;
			centre += average;
		}
	}

	//	void AdjustCentre(Color &centre, Color average) {
	//		Color diff = average;
	//		diff -= centre;

	//		if ((fabsf(diff.r) < m_fTolerance) &&
	//				(fabsf(diff.g) < m_fTolerance) &&
	//				(fabsf(diff.b) < m_fTolerance)) {
	//			centre *= 1.0f - m_fAmount;
	//			average *= m_fAmount;
	//			centre += average;
	//		}

	//		centre.a = 1.0f;
	//	}

	void convolve_pixel(int x, int top, int mid, int bot) {
		T total;
		zero_pixel(total);

		total += _p_lines_source[top][x - 1];
		total += _p_lines_source[top][x];
		total += _p_lines_source[top][x + 1];

		total += _p_lines_source[mid][x - 1];
		total += _p_lines_source[mid][x + 1];

		total += _p_lines_source[bot][x - 1];
		total += _p_lines_source[bot][x];
		total += _p_lines_source[bot][x + 1];

		T centre = _p_lines_source[mid][x];

		total += centre;

		total *= 1.0f / 9.0f;

		adjust_centre(centre, total);

		// save
		_p_lines_dest[mid][x] = centre;
	}

	void run(LightImage<T> &image, float tolerance = 0.2f, float amount = 1.0f) {
		_p_image = &image;
		_tolerance = tolerance;
		_amount = amount;

		// noop?
		if (amount <= 0.001f)
			return;

		if (image.get_width() < 2)
			return;
		if (image.get_height() < 2)
			return;

		_line_length_orig = image.get_width();
		_line_length_expanded = _line_length_orig + 2;

		create_lines();

		int height = image.get_height();

		// preload for first line
		load_line(0, 1);
		load_line(1, 2);
		copy_source_line(1, 0);

		int top = 0;
		int mid = 1;
		int bot = 2;

		for (int y = 1; y < height + 1; y++) {
			// convolve
			for (int x = 1; x < _line_length_expanded - 1; x++) {
				convolve_pixel(x, top, mid, bot);
			}

			// save out top
			if (y > 1)
				save_line(top, y - 2);

			// shuffle
			int temp = top;
			top = mid;
			mid = bot;
			bot = temp;

			// load bot
			if ((y + 1) < height) {
				load_line(y + 1, bot);
			} else {
				// copy last line
				copy_source_line(mid, bot);
			}
		}

		destroy();
	}

private:
	LightImage<T> *_p_image;
	T *_p_lines_source[NUM_LINES];
	T *_p_lines_dest[NUM_LINES];

	int _line_length_orig;
	int _line_length_expanded;
	float _tolerance;
	float _amount;
};

} // namespace LM
