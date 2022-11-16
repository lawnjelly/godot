/*************************************************************************/
/*  visual_server_canvas_helper.h                                        */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef VISUAL_SERVER_CANVAS_HELPER_H
#define VISUAL_SERVER_CANVAS_HELPER_H

#include "core/color.h"
#include "core/fixed_array.h"
#include "core/local_vector.h"
#include "core/math/rect2.h"
#include "core/rid.h"

class VisualServerCanvasHelper {
public:
	struct State {
		RID item;
		RID texture;
		Color modulate;
		RID normal_map;
		uint32_t flags = 0;

		bool operator==(const State &p_state) const {
			return ((item == p_state.item) &&
					(texture == p_state.texture) &&
					(modulate == p_state.modulate) &&
					(normal_map == p_state.normal_map) &&
					(flags == p_state.flags));
		}
		bool operator!=(const State &p_state) const { return !(*this == p_state); }
	};

private:
	// Variable sized array multirect
	struct MultiRectV {
		State state;
		LocalVector<Rect2> rects;
		LocalVector<Rect2> sources;

		bool overlaps(const Rect2 &p_rect) const {
			for (uint32_t n = 0; n < rects.size(); n++) {
				if (rects[n].intersects(p_rect)) {
					return true;
				}
			}
			return false;
		}

		bool is_empty() const { return !rects.size(); }
	};

	// There is a single mutex for tilemaps, only one quadrant can be adding
	// at a time.
	static LocalVector<MultiRectV> _tilemap_multirects;
	static Mutex _tilemap_mutex;

	static uint32_t _batch_buffer_max_quads;

public:
	static void tilemap_begin();
	static void tilemap_add_rect(RID p_canvas_item, const Rect2 &p_rect, RID p_texture, const Rect2 &p_src_rect, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false, RID p_normal_map = RID(), bool p_clip_uv = false);
	static void tilemap_end();

	static void set_batch_buffer_max_quads(uint32_t p_max_quads) { _batch_buffer_max_quads = MIN(p_max_quads, 4096); }

	static bool _multirect_enabled;
};

class MultiRect {
public:
	enum { MAX_RECTS = 512 };

private:
	VisualServerCanvasHelper::State state;
	bool state_set = false;
	uint32_t flip_flags = 0;
	FixedArray<Rect2, MAX_RECTS, true> rects;
	FixedArray<Rect2, MAX_RECTS, true> sources;

	bool overlaps(const Rect2 &p_rect) const {
		for (uint32_t n = 0; n < rects.size(); n++) {
			if (rects[n].intersects(p_rect)) {
				return true;
			}
		}
		return false;
	}

public:
	// Simple API
	void begin();
	void add_rect(RID p_canvas_item, const Rect2 &p_rect, RID p_texture, const Rect2 &p_src_rect, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false, RID p_normal_map = RID(), bool p_clip_uv = false);

	// Efficient API
	void begin(RID p_canvas_item, RID p_texture, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false, RID p_normal_map = RID(), bool p_clip_uv = false);
	bool add(const Rect2 &p_rect, const Rect2 &p_src_rect, bool p_commit_on_flip_change = true);
	bool is_empty() const { return rects.is_empty(); }
	bool is_full() const { return rects.is_full(); }
	void end();

	MultiRect();
	~MultiRect();
};

#endif // VISUAL_SERVER_CANVAS_HELPER_H
