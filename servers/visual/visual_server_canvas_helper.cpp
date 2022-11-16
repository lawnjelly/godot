/*************************************************************************/
/*  visual_server_canvas_helper.cpp                                      */
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

#include "visual_server_canvas_helper.h"

#include "servers/visual/rasterizer.h"
#include "servers/visual_server.h"

FixedPool<VisualServerCanvasHelper::MultiRect, uint32_t, 4> VisualServerCanvasHelper::_multirects;
Mutex VisualServerCanvasHelper::_multirects_mutex;

LocalVector<VisualServerCanvasHelper::MultiRect> VisualServerCanvasHelper::_tilemap_multirects;
Mutex VisualServerCanvasHelper::_tilemap_mutex;

uint32_t VisualServerCanvasHelper::_batch_buffer_max_quads = 256;
bool VisualServerCanvasHelper::_multirect_enabled = true;

uint32_t VisualServerCanvasHelper::multirect_begin() {
	// N.B. This could deadlock if something isn't releasing locks
	// by calling multirect_end().
	while (true) {
		if (!_multirects.is_full()) {
			// N.B. it can become full between the call to is full and locking.
			// Attempt a request...
			_multirects_mutex.lock();
			uint32_t id = _multirects.request();
			_multirects_mutex.unlock();

			if (id != UINT32_MAX) {
				return id + 1; // plus one so zero can indicate invalid
			} // else keep trying in a spin
		}
	}
}

void VisualServerCanvasHelper::multirect_end(uint32_t p_multirect_id) {
	p_multirect_id--;

	multirect_flush(p_multirect_id);
	MutexLock lock(_multirects_mutex);
	_multirects.free(p_multirect_id);
}

void VisualServerCanvasHelper::multirect_add_rect(uint32_t p_multirect_id, RID p_canvas_item, const Rect2 &p_rect, RID p_texture, const Rect2 &p_src_rect, const Color &p_modulate, bool p_transpose, RID p_normal_map, bool p_clip_uv) {
	// Don't create multirects if batching is inactive .. there is no path for them in the legacy renderer.
	if (!_multirect_enabled) {
		VisualServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, p_rect, p_texture, p_src_rect, p_modulate, p_transpose, p_normal_map, p_clip_uv);
		return;
	}

	bool new_common_data = true;

	Rect2 rect = p_rect;
	Rect2 source = p_src_rect;

	// To make the rendering code as efficient as possible,
	// a single MultiRect command should have identical flips and transpose etc.
	// If these change, it flushes the previous multirect and starts a new one.
	uint32_t flags = 0;

	if (p_rect.size.x < 0) {
		flags |= RasterizerCanvas::CANVAS_RECT_FLIP_H;
		rect.size.x = -rect.size.x;
	}
	if (source.size.x < 0) {
		flags ^= RasterizerCanvas::CANVAS_RECT_FLIP_H;
		source.size.x = -source.size.x;
	}
	if (p_rect.size.y < 0) {
		flags |= RasterizerCanvas::CANVAS_RECT_FLIP_V;
		rect.size.y = -rect.size.y;
	}
	if (source.size.y < 0) {
		flags ^= RasterizerCanvas::CANVAS_RECT_FLIP_V;
		source.size.y = -source.size.y;
	}

	if (p_transpose) {
		flags |= RasterizerCanvas::CANVAS_RECT_TRANSPOSE;
		SWAP(rect.size.x, rect.size.y);
	}

	if (p_clip_uv) {
		flags |= RasterizerCanvas::CANVAS_RECT_CLIP_UV;
	}

	State state;
	state.item = p_canvas_item;
	state.texture = p_texture;
	state.modulate = p_modulate;
	state.normal_map = p_normal_map;
	state.flags = flags;

	p_multirect_id--; // plus one based index
	MultiRect &mr = _multirects[p_multirect_id];

	if (!mr.is_empty()) {
		if ((mr.state != state) ||
				(mr.rects.size() >= _batch_buffer_max_quads)) {
			multirect_flush(p_multirect_id);
		} else {
			new_common_data = false;
		}
	}

	if (new_common_data) {
		mr.state = state;
	}

	mr.rects.push_back(rect);
	mr.sources.push_back(source);
}

void VisualServerCanvasHelper::multirect_flush(uint32_t p_multirect_id) {
	MultiRect &mr = _multirects[p_multirect_id];
	if (!mr.is_empty()) {
		VisualServer::get_singleton()->canvas_item_add_texture_multirect_region(mr.state.item, mr.rects, mr.state.texture, mr.sources, mr.state.modulate, mr.state.flags, mr.state.normal_map);

		mr.rects.clear();
		mr.sources.clear();
	}
}

void VisualServerCanvasHelper::tilemap_begin() {
	if (_multirect_enabled) {
		_tilemap_mutex.lock();
	}
}

void VisualServerCanvasHelper::tilemap_add_rect(RID p_canvas_item, const Rect2 &p_rect, RID p_texture, const Rect2 &p_src_rect, const Color &p_modulate, bool p_transpose, RID p_normal_map, bool p_clip_uv) {
	if (!_multirect_enabled) {
		VisualServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, p_rect, p_texture, p_src_rect, p_modulate, p_transpose, p_normal_map, p_clip_uv);
		return;
	}

	Rect2 rect = p_rect;
	Rect2 source = p_src_rect;

	// To make the rendering code as efficient as possible,
	// a single MultiRect command should have identical flips and transpose etc.
	// If these change, it flushes the previous multirect and starts a new one.
	uint32_t flags = 0;

	if (p_rect.size.x < 0) {
		flags |= RasterizerCanvas::CANVAS_RECT_FLIP_H;
		rect.size.x = -rect.size.x;
	}
	if (source.size.x < 0) {
		flags ^= RasterizerCanvas::CANVAS_RECT_FLIP_H;
		source.size.x = -source.size.x;
	}
	if (p_rect.size.y < 0) {
		flags |= RasterizerCanvas::CANVAS_RECT_FLIP_V;
		rect.size.y = -rect.size.y;
	}
	if (source.size.y < 0) {
		flags ^= RasterizerCanvas::CANVAS_RECT_FLIP_V;
		source.size.y = -source.size.y;
	}

	if (p_transpose) {
		flags |= RasterizerCanvas::CANVAS_RECT_TRANSPOSE;
		SWAP(rect.size.x, rect.size.y);
	}

	if (p_clip_uv) {
		flags |= RasterizerCanvas::CANVAS_RECT_CLIP_UV;
	}

	State state;
	state.item = p_canvas_item;
	state.texture = p_texture;
	state.modulate = p_modulate;
	state.normal_map = p_normal_map;
	state.flags = flags;

	// attempt to add to existing multirect
	for (int n = _tilemap_multirects.size() - 1; n >= 0; n--) {
		MultiRect &mr = _tilemap_multirects[n];

		// matches state?
		if (mr.state == state) {
			// add
			mr.rects.push_back(rect);
			mr.sources.push_back(source);
			return;
		}

		// disallow if we overlap a multirect
		if (mr.overlaps(rect)) {
			break;
		}
	}

	// create new multirect
	_tilemap_multirects.resize(_tilemap_multirects.size() + 1);
	MultiRect &mr = _tilemap_multirects[_tilemap_multirects.size() - 1];
	mr.state = state;
	mr.rects.push_back(rect);
	mr.sources.push_back(source);
}

void VisualServerCanvasHelper::tilemap_end() {
	if (!_multirect_enabled) {
		return;
	}

	for (uint32_t n = 0; n < _tilemap_multirects.size(); n++) {
		MultiRect &mr = _tilemap_multirects[n];
		if (!mr.is_empty()) {
			VisualServer::get_singleton()->canvas_item_add_texture_multirect_region(mr.state.item, mr.rects, mr.state.texture, mr.sources, mr.state.modulate, mr.state.flags, mr.state.normal_map);

			mr.rects.clear();
			mr.sources.clear();
		}
	}

	_tilemap_multirects.clear();

	_tilemap_mutex.unlock();
}
