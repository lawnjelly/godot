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
#include "core/pooled_list.h"
#include "core/rid.h"

class VisualServerCanvasHelper {
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

	struct MultiRect {
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

	// A fixed pool is used to keep addresses constant, and to prevent
	// reallocation and the need for locks when adding rects.
	// The capacity should be roughly maximum at the number of CPU cores,
	// but we will use less than something crazy to avoid wasting buffer memory.
	// 4 simultaneous writes should be enough for anybody.
	// If more than this are attempted, it will spin until one becomes free.
	static FixedPool<MultiRect, uint32_t, 4> _multirects;
	static Mutex _multirects_mutex;

	// There is a single mutex for tilemaps, only one quadrant can be adding
	// at a time.
	static LocalVector<MultiRect> _tilemap_multirects;
	static Mutex _tilemap_mutex;

	static uint32_t _batch_buffer_max_quads;
	static bool _multirect_enabled;

	static void multirect_flush(uint32_t p_multirect_id);

public:
	// returns multirect ID
	static uint32_t multirect_begin();
	static void multirect_add_rect(uint32_t p_multirect_id, RID p_canvas_item, const Rect2 &p_rect, RID p_texture, const Rect2 &p_src_rect, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false, RID p_normal_map = RID(), bool p_clip_uv = false);
	static void multirect_end(uint32_t p_multirect_id);

	static void tilemap_begin();
	static void tilemap_add_rect(RID p_canvas_item, const Rect2 &p_rect, RID p_texture, const Rect2 &p_src_rect, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false, RID p_normal_map = RID(), bool p_clip_uv = false);
	static void tilemap_end();

	static void set_batch_buffer_max_quads(uint32_t p_max_quads) { _batch_buffer_max_quads = p_max_quads; }
	static void set_multirect_enabled(bool p_enabled) { _multirect_enabled = p_enabled; }
};

#endif // VISUAL_SERVER_CANVAS_HELPER_H
