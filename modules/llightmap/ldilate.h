#pragma once

#include "llightimage.h"
#include "llighttypes.h"
#include "lvector.h"
#include <limits.h>

//#define DILATE_VERBOSE

namespace LM {

template <class T>
class Dilate {
public:
	bool dilate_image(LightImage<T> &im, const LightImage<uint32_t> &orig_mask, unsigned int uiMaxDist) {
		print_line("Dilating image");

		int w = im.get_width();
		int h = im.get_height();

		// construct mask
		LightImage<uint8_t> mask;
		mask.create(w, h, 0);
		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {
				if (*orig_mask.get(x, y))
					*mask.get(x, y) = 255;
			}
		}

		LightImage<Vec2_i16> source_pts;
		run(mask, &source_pts, uiMaxDist);

		convert_final_image(im);
		return true;
	}

	// mem vars
	LightImage<uint8_t> _done;
	LightImage<Vec2_i16> *_source_pts;
	LVector<Vec2_i16> _actives;
	int _width;
	int _height;

	void run(LightImage<uint8_t> &mask, LightImage<Vec2_i16> *pSource_pts, unsigned int uiMaxDist) {
		_width = mask.get_width();
		_height = mask.get_height();
		int w = _width;
		int h = _height;

		if ((w * h) == 0)
			return;

		// note which pixels have been done (activated)
		_done.create(w, h);

		// source coordinates for best source
		_source_pts = pSource_pts;
		_source_pts->create(w, h);

		// the active points
		_actives.reserve((w * h) / 8); // just a rough number to start with, the vector can grow

		// first find the edges
		find_edges(mask);

		// required number of iterations
		for (unsigned int i = 0; i < uiMaxDist; i++) {
			// this must be recorded before starting, as we will be adding to the list
			unsigned int uiNumActives = _actives.size();

			for (unsigned int n = 0; n < uiNumActives; n++) {
				process_pixel(n, mask);
			}

			// delete the previous active items
			_actives.delete_items_first(uiNumActives);

			print_actives();
		}
	}

	void convert_final_image(LightImage<T> &image) {
		for (unsigned int n = 0; n < _done.get_num_pixels(); n++) {
			if (!_done.get(n))
				continue;

			// get the x y
			int y = n / _width;
			int x = n % _width;

			const Vec2_i16 &scoords = *_source_pts->get(x, y);
			if ((scoords.x != 0) || (scoords.y != 0))
				image.get_item(x, y) = image.get_item(scoords.x, scoords.y);
		}
	}

	unsigned int square_length(const Vector2i &v) const { return (v.x * v.x) + (v.y * v.y); }

	void process_pixel(unsigned int uiPixel, LightImage<uint8_t> &mask) {
		const Vec2_i16 &loc = _actives[uiPixel];
		int iRange = 3;

		Rect2i rect = Rect2i(loc.x, loc.y, 0, 0);
		rect = rect.grow(iRange);
		rect.size.x += 1;
		rect.size.y += 1;

		// clip
		rect = rect.clip(Rect2i(0, 0, _width, _height));
		if ((rect.size.x == 0) && (rect.size.y == 0))
			return;

		// get loc quick format
		Vector2i iloc = Vector2i(loc.x, loc.y);

		Vec2_i16 bestcoord;
		unsigned int sl_best = UINT_MAX;

		int bottom = rect.position.y + rect.size.y;
		int right = rect.position.x + rect.size.x;

		for (int y = rect.position.y; y < bottom; y++) {
			for (int x = rect.position.x; x < right; x++) {
				//if ((x == iloc.x) && (y == iloc.y))
				//	continue;

				Vec2_i16 coord = *_source_pts->get(x, y);

				// ignore
				if (coord.is_zero())
					continue;

				// calculate distance from HERE to the referenced point
				Vector2i offset = Vector2i(coord.x, coord.y);
				offset -= iloc;
				unsigned int sl = square_length(offset);

				// is this the best fit?
				if (sl < sl_best) {
					sl_best = sl;
					bestcoord = coord;
				}
			}
		}

		// DEV_ASSERT that we have found a coord
		DEV_ASSERT(sl_best < UINT_MAX);
		*_source_pts->get(loc.x, loc.y) = bestcoord;

		// add suitable neighbours to active list
		add_active_neighs(loc.x, loc.y, mask);
	}

	void find_edges(LightImage<uint8_t> &mask) {
		int w = mask.get_width();
		int h = mask.get_height();

		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {
				// assess this point
				if (mask.get_item(x, y) || _done.get_item(x, y))
					//				if (mask.GetItem(x, y))
					continue;

					// do we have a used neighbour?
					//bool bSuitable = false;

//#define TEST_NEIGH(a, b) if (image.GetItem(a, b)) {m_Actives.Add(Prim::CoPoint2S(a, b)); bSuitable = true; }
#define TEST_NEIGH(a, b)                               \
	if (mask.is_within(a, b) && mask.get_item(a, b)) { \
		_source_pts->get_item(x, y).set(a, b);         \
		goto suitable;                                 \
	}
				//image.GetItem(x, y) = image.GetItem(a, b);

				// closest 4
				TEST_NEIGH(x, y - 1)
				TEST_NEIGH(x - 1, y)
				TEST_NEIGH(x + 1, y)
				TEST_NEIGH(x, y + 1)

				// then the diagnals
				TEST_NEIGH(x - 1, y - 1)
				TEST_NEIGH(x + 1, y - 1)
				TEST_NEIGH(x - 1, y + 1)
				TEST_NEIGH(x + 1, y + 1)
#undef TEST_NEIGH

				// not suitable, just continue to next x
				continue;

			suitable:
#ifdef DILATE_VERBOSE
				print_line("suitable : " + itos(x) + ", " + itos(y));
#endif
				_done.get_item(x, y) = 255;
				//AddActiveNeighs(x, y, mask);
				_actives.push_back(Vec2_i16(x, y));
			} // for x
		} // for y

		print_actives();
	}

	void check_neigh(int a, int b, LightImage<uint8_t> &mask) {
		bool bWithin = mask.is_within(a, b);
		if (!bWithin)
			return;

		bool bSourcePtSet = _source_pts->get_item(a, b).is_non_zero();

		if (!bSourcePtSet) {
			bool bDone = _done.get_item(a, b);
			bool bMaskSet = mask.get_item(a, b);
			if ((!bDone) && (!bMaskSet)) {
#ifdef DILATE_VERBOSE
				print_line("\tadding active " + itos(a) + ", " + itos(b));
#endif
				_actives.push_back(Vec2_i16(a, b));
				_done.get_item(a, b) = 255;
			}
		}
	}

	void add_active_neighs(int x, int y, LightImage<uint8_t> &mask) {
		//#define CHECK_NEIGH(a, b) if ((image.IsWithin(a, b)) && (!m_pSource_pts->GetItem(a, b).IsNonZero()))\
//{\
//if ((!m_Done.GetItem(a, b)) && (!image.GetItem(a, b)))\
//m_Actives.push_back(Vec2_i16(a, b));\
//m_Done.GetItem(a, b) = 255;\
//}

		check_neigh(x - 1, y - 1, mask);
		check_neigh(x, y - 1, mask);
		check_neigh(x + 1, y - 1, mask);
		check_neigh(x - 1, y, mask);
		check_neigh(x + 1, y, mask);
		check_neigh(x - 1, y + 1, mask);
		check_neigh(x, y + 1, mask);
		check_neigh(x + 1, y + 1, mask);

		//#undef CHECK_NEIGH
	}

	bool on_active_list(int x, int y) const {
		for (int n = 0; n < _actives.size(); n++) {
			Vec2_i16 a = _actives[n];
			if ((a.x == x) && (a.y == y))
				return true;
		}
		return false;
	}

	void print_actives() {
#ifdef DILATE_VERBOSE
		for (int y = 0; y < m_iHeight; y++) {
			String sz;
			for (int x = 0; x < m_iWidth; x++) {
				if (m_Done.GetItem(x, y))
					sz += "*";
				else
					sz += ".";

				if (OnActiveList(x, y))
					sz += "a";
				else
					sz += " ";

				Vec2_i16 cd = m_pSource_pts->GetItem(x, y);
				sz += "(" + itos(cd.x) + ", " + itos(cd.y) + ")";
			}
			print_line(sz);
		}

//		print_line (itos(m_Actives.size()) + " Actives:");
//		for (int n=0; n<m_Actives.size(); n++)
//		{
//			Vec2_i16 a = m_Actives[n];
//			print_line("\t\tactive " + itos(a.x) + ", " + itos(a.y));
//		}
#endif
	}
};

} //namespace LM
