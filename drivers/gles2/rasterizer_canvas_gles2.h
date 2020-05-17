/*************************************************************************/
/*  rasterizer_canvas_gles2.h                                            */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
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

#ifndef RASTERIZERCANVASGLES2_H
#define RASTERIZERCANVASGLES2_H

#include "rasterizer_canvas_base_gles2.h"
#include "drivers/gles_common/rasterizer_canvas_batcher.h"

class RasterizerSceneGLES2;

class RasterizerCanvasGLES2 : public RasterizerCanvasBaseGLES2, public RasterizerCanvasBatcher<RasterizerCanvasGLES2, RasterizerStorageGLES2> {

	friend class RasterizerCanvasBatcher;

public:
	virtual void canvas_render_items_begin(const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform);
	virtual void canvas_render_items_end();
	virtual void canvas_render_items(Item *p_item_list, int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform);
	virtual void canvas_begin();
	virtual void canvas_end();

private:
	// legacy codepath .. to remove after testing
	void _canvas_render_item(Item *p_ci, RenderItemState &r_ris);
	void _canvas_item_render_commands(Item *p_item, Item *p_current_clip, bool &r_reclip, RasterizerStorageGLES2::Material *p_material);

	// high level batch funcs
	void canvas_render_items_implementation(Item *p_item_list, int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform);
	void render_joined_item(const BItemJoined &p_bij, RenderItemState &r_ris);
	void record_items(Item *p_item_list, int p_z);
	void join_sorted_items();
	bool try_join_item(Item *p_ci, RenderItemState &r_ris, bool &r_batch_break);
	void render_joined_item_commands(const BItemJoined &p_bij, Item *p_current_clip, bool &r_reclip, RasterizerStorageGLES2::Material *p_material, bool p_lit);
	void render_batches(Item::Command *const *p_commands, Item *p_current_clip, bool &r_reclip, RasterizerStorageGLES2::Material *p_material);
	bool prefill_joined_item(FillState &r_fill_state, int &r_command_start, Item *p_item, Item *p_current_clip, bool &r_reclip, RasterizerStorageGLES2::Material *p_material);
	void flush_render_batches(Item *p_first_item, Item *p_current_clip, bool &r_reclip, RasterizerStorageGLES2::Material *p_material);

	// low level batch funcs
	void _batch_translate_to_colored();
	_FORCE_INLINE_ int _batch_find_or_create_tex(const RID &p_texture, const RID &p_normal, bool p_tile, int p_previous_match);
	RasterizerStorageGLES2::Texture *_get_canvas_texture(const RID &p_texture) const;
	void _batch_upload_buffers();
	void _batch_render_rects(const Batch &p_batch, RasterizerStorageGLES2::Material *p_material);
	BatchVertex *_batch_vertex_request_new() { return bdata.vertices.request(); }
	Batch *_batch_request_new(bool p_blank = true);

	bool _detect_batch_break(Item *p_ci);
	void _software_transform_vertex(BatchVector2 &r_v, const Transform2D &p_tr) const;
	void _software_transform_vertex(Vector2 &r_v, const Transform2D &p_tr) const;
	TransformMode _find_transform_mode(const Transform2D &p_tr) const;
	_FORCE_INLINE_ void _prefill_default_batch(FillState &r_fill_state, int p_command_num, const Item &p_item);

	// funcs used from rasterizer_canvas_batcher template
	void gl_enable_scissor(int p_x, int p_y, int p_width, int p_height) const;
	void gl_disable_scissor() const;

	// no need to compile these in in release, they are unneeded outside the editor and only add to executable size
#ifdef DEBUG_ENABLED
	void diagnose_batches(Item::Command *const *p_commands);
	String get_command_type_string(const Item::Command &p_command) const;
#endif


public:
	void initialize();
	RasterizerCanvasGLES2();
};

//////////////////////////////////////////////////////////////

// Default batches will not occur in software transform only items
// EXCEPT IN THE CASE OF SINGLE RECTS (and this may well not occur, check the logic in prefill_join_item TYPE_RECT)
// but can occur where transform commands have been sent during hardware batch
_FORCE_INLINE_ void RasterizerCanvasGLES2::_prefill_default_batch(FillState &r_fill_state, int p_command_num, const Item &p_item) {
	if (r_fill_state.curr_batch->type == Batch::BT_DEFAULT) {
		// don't need to flush an extra transform command?
		if (!r_fill_state.transform_extra_command_number_p1) {
			// another default command, just add to the existing batch
			r_fill_state.curr_batch->num_commands++;
		} else {
#ifdef DEBUG_ENABLED
			if (r_fill_state.transform_extra_command_number_p1 != p_command_num) {
				WARN_PRINT_ONCE("_prefill_default_batch : transform_extra_command_number_p1 != p_command_num");
			}
#endif
			// we do have a pending extra transform command to flush
			// either the extra transform is in the prior command, or not, in which case we need 2 batches
			r_fill_state.curr_batch->num_commands += 2;

			r_fill_state.transform_extra_command_number_p1 = 0; // mark as sent
			r_fill_state.extra_matrix_sent = true;

			// the original mode should always be hardware transform ..
			// test this assumption
			//CRASH_COND(r_fill_state.orig_transform_mode != TM_NONE);
			r_fill_state.transform_mode = r_fill_state.orig_transform_mode;

			// do we need to restore anything else?
		}
	} else {
		// end of previous different type batch, so start new default batch

		// first consider whether there is a dirty extra matrix to send
		if (r_fill_state.transform_extra_command_number_p1) {
			// get which command the extra is in, and blank all the records as it no longer is stored CPU side
			int extra_command = r_fill_state.transform_extra_command_number_p1 - 1; // plus 1 based
			r_fill_state.transform_extra_command_number_p1 = 0;
			r_fill_state.extra_matrix_sent = true;

			// send the extra to the GPU in a batch
			r_fill_state.curr_batch = _batch_request_new();
			r_fill_state.curr_batch->type = Batch::BT_DEFAULT;
			r_fill_state.curr_batch->first_command = extra_command;
			r_fill_state.curr_batch->num_commands = 1;

			// revert to the original transform mode
			// e.g. go back to NONE if we were in hardware transform mode
			r_fill_state.transform_mode = r_fill_state.orig_transform_mode;

			// reset the original transform if we are going back to software mode,
			// because the extra is now done on the GPU...
			// (any subsequent extras are sent directly to the GPU, no deferring)
			if (r_fill_state.orig_transform_mode != TM_NONE) {
				r_fill_state.transform_combined = p_item.final_transform;
			}

			// can possibly combine batch with the next one in some cases
			// this is more efficient than having an extra batch especially for the extra
			if ((extra_command + 1) == p_command_num) {
				r_fill_state.curr_batch->num_commands = 2;
				return;
			}
		}

		// start default batch
		r_fill_state.curr_batch = _batch_request_new();
		r_fill_state.curr_batch->type = Batch::BT_DEFAULT;
		r_fill_state.curr_batch->first_command = p_command_num;
		r_fill_state.curr_batch->num_commands = 1;
	}
}

_FORCE_INLINE_ void RasterizerCanvasGLES2::_software_transform_vertex(BatchVector2 &r_v, const Transform2D &p_tr) const {
	Vector2 vc(r_v.x, r_v.y);
	vc = p_tr.xform(vc);
	r_v.set(vc);
}

_FORCE_INLINE_ void RasterizerCanvasGLES2::_software_transform_vertex(Vector2 &r_v, const Transform2D &p_tr) const {
	r_v = p_tr.xform(r_v);
}

_FORCE_INLINE_ RasterizerCanvasGLES2::TransformMode RasterizerCanvasGLES2::_find_transform_mode(const Transform2D &p_tr) const {
	// decided whether to do translate only for software transform
	if ((p_tr.elements[0].x == 1.0) &&
			(p_tr.elements[0].y == 0.0) &&
			(p_tr.elements[1].x == 0.0) &&
			(p_tr.elements[1].y == 1.0)) {
		return TM_TRANSLATE;
	}

	return TM_ALL;
}


#endif // RASTERIZERCANVASGLES2_H
