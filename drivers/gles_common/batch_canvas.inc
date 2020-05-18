void batch_canvas_begin()
{
	// diagnose_frame?
	bdata.frame_string = ""; // just in case, always set this as we don't want a string leak in release...
#ifdef DEBUG_ENABLED
	if (bdata.settings_diagnose_frame) {
		bdata.diagnose_frame = false;

		uint32_t tick = OS::get_singleton()->get_ticks_msec();
		uint64_t frame = Engine::get_singleton()->get_frames_drawn();

		if (tick >= bdata.next_diagnose_tick) {
			bdata.next_diagnose_tick = tick + 10000;

			// the plus one is prevent starting diagnosis half way through frame
			bdata.diagnose_frame_number = frame + 1;
		}

		if (frame == bdata.diagnose_frame_number) {
			bdata.diagnose_frame = true;
			bdata.reset_stats();
		}

		if (bdata.diagnose_frame) {
			bdata.frame_string = "canvas_begin FRAME " + itos(frame) + "\n";
		}
	}
#endif
}

void batch_canvas_end()
{
#ifdef DEBUG_ENABLED
	if (bdata.diagnose_frame) {
		bdata.frame_string += "canvas_end\n";
		if (bdata.stats_items_sorted) {
			bdata.frame_string += "\titems reordered: " + itos(bdata.stats_items_sorted) + "\n";
		}
		if (bdata.stats_light_items_joined) {
			bdata.frame_string += "\tlight items joined: " + itos(bdata.stats_light_items_joined) + "\n";
		}

		print_line(bdata.frame_string);
	}
#endif
}

void batch_canvas_render_items_begin(const Color &p_modulate, RasterizerCanvas::Light *p_light, const Transform2D &p_base_transform) {
	// if we are debugging, flash each frame between batching renderer and old version to compare for regressions
	if (bdata.settings_flash_batching) {
		if ((Engine::get_singleton()->get_frames_drawn() % 2) == 0)
			bdata.settings_use_batching = true;
		else
			bdata.settings_use_batching = false;
	}

	if (!bdata.settings_use_batching) {
		return;
	}

	// this only needs to be done when screen size changes, but this should be
	// infrequent enough
	_calculate_scissor_threshold_area();

	// set up render item state for all the z_indexes (this is common to all z_indexes)
	_render_item_state.reset();
	_render_item_state.item_group_modulate = p_modulate;
	_render_item_state.item_group_light = p_light;
	_render_item_state.item_group_base_transform = p_base_transform;
	_render_item_state.light_region.reset();

	// batch break must be preserved over the different z indices,
	// to prevent joining to an item on a previous index if not allowed
	_render_item_state.join_batch_break = false;

	// whether to join across z indices depends on whether there are z ranged lights.
	// joined z_index items can be wrongly classified with z ranged lights.
	bdata.join_across_z_indices = true;

	int light_count = 0;
	while (p_light) {
		light_count++;

		if ((p_light->z_min != VS::CANVAS_ITEM_Z_MIN) || (p_light->z_max != VS::CANVAS_ITEM_Z_MAX)) {
			// prevent joining across z indices. This would have caused visual regressions
			bdata.join_across_z_indices = false;
		}

		p_light = p_light->next_ptr;
	}

	// can't use the light region bitfield if there are too many lights
	// hopefully most games won't blow this limit..
	// if they do they will work but it won't batch join items just in case
	if (light_count > 64) {
		_render_item_state.light_region.too_many_lights = true;
	}
}

void batch_canvas_render_items_end()
{
	if (!bdata.settings_use_batching) {
		return;
	}

	join_sorted_items();

#ifdef DEBUG_ENABLED
	if (bdata.diagnose_frame) {
		bdata.frame_string += "items\n";
	}
#endif

	// batching render is deferred until after going through all the z_indices, joining all the items
	get_this()->canvas_render_items_implementation(0, 0, _render_item_state.item_group_modulate,
			_render_item_state.item_group_light,
			_render_item_state.item_group_base_transform);

	bdata.items_joined.reset();
	bdata.item_refs.reset();
	bdata.sort_items.reset();
}

void batch_canvas_render_items(RasterizerCanvas::Item *p_item_list, int p_z, const Color &p_modulate, RasterizerCanvas::Light *p_light, const Transform2D &p_base_transform)
{
	// stage 1 : join similar items, so that their state changes are not repeated,
	// and commands from joined items can be batched together
	if (bdata.settings_use_batching) {
		record_items(p_item_list, p_z);
		return;
	}

	// only legacy renders at this stage, batched renderer doesn't render until canvas_render_items_end()
	get_this()->canvas_render_items_implementation(p_item_list, p_z, p_modulate, p_light, p_base_transform);
}

/*
void canvas_render_items_implementation(RasterizerCanvas::Item *p_item_list, int p_z, const Color &p_modulate, RasterizerCanvas::Light *p_light, const Transform2D &p_base_transform) {

	// parameters are easier to pass around in a structure
	RenderItemState ris;
	ris.item_group_z = p_z;
	ris.item_group_modulate = p_modulate;
	ris.item_group_light = p_light;
	ris.item_group_base_transform = p_base_transform;

	state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SKELETON, false);

	state.current_tex = RID();
	state.current_tex_ptr = NULL;
	state.current_normal = RID();
	state.canvas_texscreen_used = false;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);

	if (bdata.settings_use_batching) {
		for (int j = 0; j < bdata.items_joined.size(); j++) {
			render_joined_item(bdata.items_joined[j], ris);
		}
	} else {
		while (p_item_list) {

			RasterizerCanvas::Item *ci = p_item_list;
			_canvas_render_item(ci, ris);
			p_item_list = p_item_list->next;
		}
	}

	if (ris.current_clip) {
		glDisable(GL_SCISSOR_TEST);
	}

	state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SKELETON, false);
}
*/
