protected:
// This function may be called MULTIPLE TIMES for each item, so needs to record how far it has got
bool prefill_joined_item(FillState &r_fill_state, int &r_command_start, RasterizerCanvas::Item *p_item, RasterizerCanvas::Item *p_current_clip, bool &r_reclip, typename T_STORAGE::Material *p_material) {
	// we will prefill batches and vertices ready for sending in one go to the vertex buffer
	int command_count = p_item->commands.size();
	RasterizerCanvas::Item::Command *const *commands = p_item->commands.ptr();

	// locals, might be more efficient in a register (check)
	//Vector2 texpixel_size = r_fill_state.texpixel_size;
	//const float uv_epsilon = bdata.settings_uv_contract_amount;

	// checking the color for not being white makes it 92/90 times faster in the case where it is white
	bool multiply_final_modulate = false;
	if (!r_fill_state.use_hardware_transform && (r_fill_state.final_modulate != Color(1, 1, 1, 1))) {
		multiply_final_modulate = true;
	}

	// start batch is a dummy batch (tex id -1) .. could be made more efficient
	if (!r_fill_state.curr_batch) {
		// OLD METHOD, but left dangling zero length default batches
		//		r_fill_state.curr_batch = _batch_request_new();
		//		r_fill_state.curr_batch->type = RasterizerStorageCommon::BT_DEFAULT;
		//		r_fill_state.curr_batch->first_command = r_command_start;
		// should tex_id be set to -1? check this

		// allocate dummy batch on the stack, it should always get replaced
		// note that the rest of the structure is uninitialized, this should not matter
		// if the type is checked before anything else.
		r_fill_state.curr_batch = (Batch *)alloca(sizeof(Batch));
		r_fill_state.curr_batch->type = RasterizerStorageCommon::BT_DUMMY;

		// this is assumed to be the case
		//CRASH_COND (r_fill_state.transform_extra_command_number_p1);
	}

	// we need to return which command we got up to, so
	// store this outside the loop
	int command_num;

	// do as many commands as possible until the vertex buffer will be full up
	for (command_num = r_command_start; command_num < command_count; command_num++) {

		RasterizerCanvas::Item::Command *command = commands[command_num];

		switch (command->type) {

			default: {
				_prefill_default_batch(r_fill_state, command_num, *p_item);
			} break;
			case RasterizerCanvas::Item::Command::TYPE_TRANSFORM: {
				// if the extra matrix has been sent already,
				// break this extra matrix software path (as we don't want to unset it on the GPU etc)
				if (r_fill_state.extra_matrix_sent) {
					_prefill_default_batch(r_fill_state, command_num, *p_item);
				} else {
					// Extra matrix fast path.
					// Instead of sending the command immediately, we store the modified transform (in combined)
					// for software transform, and only flush this transform command if we NEED to (i.e. we want to
					// render some default commands)
					RasterizerCanvas::Item::CommandTransform *transform = static_cast<RasterizerCanvas::Item::CommandTransform *>(command);
					const Transform2D &extra_matrix = transform->xform;

					if (r_fill_state.use_hardware_transform) {
						// if we are using hardware transform mode, we have already sent the final transform,
						// so we only want to software transform the extra matrix
						r_fill_state.transform_combined = extra_matrix;
					} else {
						r_fill_state.transform_combined = p_item->final_transform * extra_matrix;
					}
					// after a transform command, always use some form of software transform (either the combined final + extra, or just the extra)
					// until we flush this dirty extra matrix because we need to render default commands.
					r_fill_state.transform_mode = _find_transform_mode(r_fill_state.transform_combined);

					// make a note of which command the dirty extra matrix is store in, so we can send it later
					// if necessary
					r_fill_state.transform_extra_command_number_p1 = command_num + 1; // plus 1 so we can test against zero
				}
			} break;
			case RasterizerCanvas::Item::Command::TYPE_RECT: {

				RasterizerCanvas::Item::CommandRect *rect = static_cast<RasterizerCanvas::Item::CommandRect *>(command);

				// unoptimized - could this be done once per batch / batch texture?
				bool send_light_angles = rect->normal_map != RID();

				bool buffer_full = false;

				// the template params must be explicit for compilation,
				// this forces building the multiple versions of the function.
				if (send_light_angles) {
					buffer_full = prefill_rect<true>(rect, r_fill_state, r_command_start, command_num, command_count, commands, p_item, multiply_final_modulate);
				} else {
					buffer_full = prefill_rect<false>(rect, r_fill_state, r_command_start, command_num, command_count, commands, p_item, multiply_final_modulate);
				}

				if (buffer_full)
					return true;

			} break;
			case RasterizerCanvas::Item::Command::TYPE_NINEPATCH: {

				RasterizerCanvas::Item::CommandNinePatch *np = static_cast<RasterizerCanvas::Item::CommandNinePatch *>(command);

				// unoptimized - could this be done once per batch / batch texture?
				bool send_light_angles = np->normal_map != RID();

				bool buffer_full = false;

				if (send_light_angles)
					buffer_full = prefill_ninepatch<true>(np, r_fill_state, r_command_start, command_num, command_count, p_item, multiply_final_modulate);
				else
					buffer_full = prefill_ninepatch<false>(np, r_fill_state, r_command_start, command_num, command_count, p_item, multiply_final_modulate);

				if (buffer_full)
					return true;

			} break;

			case RasterizerCanvas::Item::Command::TYPE_LINE: {

				RasterizerCanvas::Item::CommandLine *line = static_cast<RasterizerCanvas::Item::CommandLine *>(command);

				if (line->width <= 1) {
					bool buffer_full = prefill_line(line, r_fill_state, r_command_start, command_num, command_count, p_item, multiply_final_modulate);

					if (buffer_full)
						return true;
				} else {
					// not accelerated
					_prefill_default_batch(r_fill_state, command_num, *p_item);
				}
			} break;

			case RasterizerCanvas::Item::Command::TYPE_POLYGON: {

				// not using software skinning?
				//				if (true)
				if (!bdata.settings_use_software_skinning && get_this()->state.using_skeleton) {
					// not accelerated
					_prefill_default_batch(r_fill_state, command_num, *p_item);
				} else {
					RasterizerCanvas::Item::CommandPolygon *polygon = static_cast<RasterizerCanvas::Item::CommandPolygon *>(command);

					// unoptimized - could this be done once per batch / batch texture?
					bool send_light_angles = polygon->normal_map != RID();

					bool buffer_full = false;

					if (send_light_angles) {
						// NYI
						_prefill_default_batch(r_fill_state, command_num, *p_item);
						//buffer_full = prefill_polygon<true>(polygon, r_fill_state, r_command_start, command_num, command_count, p_item, multiply_final_modulate);
					} else
						buffer_full = prefill_polygon<false>(polygon, r_fill_state, r_command_start, command_num, command_count, p_item, multiply_final_modulate);

					if (buffer_full)
						return true;
				} // using software skinning path

			} break;
		}
	}

	// VERY IMPORTANT to return where we got to, because this func may be called multiple
	// times per item.
	// Don't miss out on this step by calling return earlier in the function without setting r_command_start.
	r_command_start = command_num;

	return false;
}

void flush_render_batches(RasterizerCanvas::Item *p_first_item, RasterizerCanvas::Item *p_current_clip, bool &r_reclip, typename T_STORAGE::Material *p_material) {

	// some heuristic to decide whether to use colored verts.
	// feel free to tweak this.
	// this could use hysteresis, to prevent jumping between methods
	// .. however probably not necessary
	bdata.use_colored_vertices = false;

	// switch from regular to colored?
	if (bdata.fvf == RasterizerStorageCommon::FVF_REGULAR) {
		// only check whether to convert if there are quads (prevent divide by zero)
		// and we haven't decided to prevent color baking (due to e.g. MODULATE
		// being used in a shader)
		if (bdata.total_quads && !(bdata.joined_item_batch_flags & RasterizerStorageCommon::PREVENT_COLOR_BAKING)) {
			// minus 1 to prevent single primitives (ratio 1.0) always being converted to colored..
			// in that case it is slightly cheaper to just have the color as part of the batch
			float ratio = (float)(bdata.total_color_changes - 1) / (float)bdata.total_quads;

			// use bigger than or equal so that 0.0 threshold can force always using colored verts
			if (ratio >= bdata.settings_colored_vertex_format_threshold) {
				bdata.use_colored_vertices = true;
				bdata.fvf = RasterizerStorageCommon::FVF_COLOR;
			}
		}

		// if we used vertex colors
		if (bdata.vertex_colors.size()) {
			bdata.use_colored_vertices = true;
			bdata.fvf = RasterizerStorageCommon::FVF_COLOR;
		}

		// needs light angles?
		if (bdata.use_light_angles) {
			bdata.fvf = RasterizerStorageCommon::FVF_LIGHT_ANGLE;
		}
	}

	// translate if required to larger FVFs
	switch (bdata.fvf) {
		case RasterizerStorageCommon::FVF_UNBATCHED: // should not happen
			break;
		case RasterizerStorageCommon::FVF_REGULAR: // no change
			break;
		case RasterizerStorageCommon::FVF_COLOR: {
			// special case, where vertex colors are used (polys)
			if (!bdata.vertex_colors.size())
				_translate_batches_to_larger_FVF<BatchVertexColored, false, false, false>();
			else
				// normal, reduce number of batches by baking batch colors
				_translate_batches_to_vertex_colored_FVF();
		} break;
		case RasterizerStorageCommon::FVF_LIGHT_ANGLE:
			_translate_batches_to_larger_FVF<BatchVertexLightAngled, true, false, false>();
			break;
		case RasterizerStorageCommon::FVF_MODULATED:
			_translate_batches_to_larger_FVF<BatchVertexModulated, true, true, false>();
			break;
		case RasterizerStorageCommon::FVF_LARGE:
			_translate_batches_to_larger_FVF<BatchVertexLarge, true, true, true>();
			break;
	}

	// send buffers to opengl
	get_this()->_batch_upload_buffers();

	RasterizerCanvas::Item::Command *const *commands = p_first_item->commands.ptr();

#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
	if (bdata.diagnose_frame) {
		diagnose_batches(commands);
	}
#endif

	get_this()->render_batches(commands, p_current_clip, r_reclip, p_material);
}

void render_joined_item_commands(const BItemJoined &p_bij, RasterizerCanvas::Item *p_current_clip, bool &r_reclip, typename T_STORAGE::Material *p_material, bool p_lit) {

	RasterizerCanvas::Item *item = 0;
	RasterizerCanvas::Item *first_item = bdata.item_refs[p_bij.first_item_ref].item;

	// fill_state and bdata have once off setup per joined item, and a smaller reset on flush
	FillState fill_state;
	fill_state.reset_joined_item(p_bij.use_hardware_transform());

	bdata.reset_joined_item();

	// should this joined item be using large FVF?
	if (p_bij.flags & RasterizerStorageCommon::USE_MODULATE_FVF) {
		bdata.use_modulate = true;
		bdata.fvf = RasterizerStorageCommon::FVF_MODULATED;
	}
	if (p_bij.flags & RasterizerStorageCommon::USE_LARGE_FVF) {
		bdata.use_modulate = true;
		bdata.use_large_verts = true;
		bdata.fvf = RasterizerStorageCommon::FVF_LARGE;
	}

	// in the special case of custom shaders that read from VERTEX (i.e. vertex position)
	// we want to disable software transform of extra matrix
	if (bdata.joined_item_batch_flags & RasterizerStorageCommon::PREVENT_VERTEX_BAKING) {
		fill_state.extra_matrix_sent = true;
	}

	for (unsigned int i = 0; i < p_bij.num_item_refs; i++) {
		const BItemRef &ref = bdata.item_refs[p_bij.first_item_ref + i];
		item = ref.item;

		if (!p_lit) {
			// if not lit we use the complex calculated final modulate
			fill_state.final_modulate = ref.final_modulate;
		} else {
			// if lit we ignore canvas modulate and just use the item modulate
			fill_state.final_modulate = item->final_modulate;
		}

		int command_count = item->commands.size();
		int command_start = 0;

		// ONCE OFF fill state setup, that will be retained over multiple calls to
		// prefill_joined_item()
		fill_state.transform_combined = item->final_transform;

		// decide the initial transform mode, and make a backup
		// in orig_transform_mode in case we need to switch back
		if (!fill_state.use_hardware_transform) {
			fill_state.transform_mode = _find_transform_mode(fill_state.transform_combined);
		} else {
			fill_state.transform_mode = TM_NONE;
		}
		fill_state.orig_transform_mode = fill_state.transform_mode;

		// keep track of when we added an extra matrix
		// so we can defer sending until we see a default command
		fill_state.transform_extra_command_number_p1 = 0;

		while (command_start < command_count) {
			// fill as many batches as possible (until all done, or the vertex buffer is full)
			bool bFull = get_this()->prefill_joined_item(fill_state, command_start, item, p_current_clip, r_reclip, p_material);

			if (bFull) {
				// always pass first item (commands for default are always first item)
				flush_render_batches(first_item, p_current_clip, r_reclip, p_material);

				// zero all the batch data ready for a new run
				bdata.reset_flush();

				// don't zero all the fill state, some may need to be preserved
				fill_state.reset_flush();
			}
		}
	}

	// flush if any left
	flush_render_batches(first_item, p_current_clip, r_reclip, p_material);

	// zero all the batch data ready for a new run
	bdata.reset_flush();
}

// legacy
void _canvas_item_render_commands(RasterizerCanvas::Item *p_item, RasterizerCanvas::Item *p_current_clip, bool &r_reclip, typename T_STORAGE::Material *p_material) {

	int command_count = p_item->commands.size();

	RasterizerCanvas::Item::Command *const *commands = p_item->commands.ptr();

	// legacy .. just create one massive batch and render everything as before
	bdata.batches.reset();
	Batch *batch = _batch_request_new();
	batch->type = RasterizerStorageCommon::BT_DEFAULT;
	batch->num_commands = command_count;

	get_this()->render_batches(commands, p_current_clip, r_reclip, p_material);
	bdata.reset_flush();
}
