// This function may be called MULTIPLE TIMES for each item, so needs to record how far it has got
bool prefill_joined_item(FillState &r_fill_state, int &r_command_start, RasterizerCanvas::Item *p_item, RasterizerCanvas::Item *p_current_clip, bool &r_reclip, typename T_STORAGE::Material *p_material) {
	// we will prefill batches and vertices ready for sending in one go to the vertex buffer
	int command_count = p_item->commands.size();
	RasterizerCanvas::Item::Command *const *commands = p_item->commands.ptr();

	// just a local, might be more efficient in a register (check)
	Vector2 texpixel_size = r_fill_state.texpixel_size;

	// checking the color for not being white makes it 92/90 times faster in the case where it is white
	bool multiply_final_modulate = false;
	if (!r_fill_state.use_hardware_transform && (r_fill_state.final_modulate != Color(1, 1, 1, 1))) {
		multiply_final_modulate = true;
	}

	// start batch is a dummy batch (tex id -1) .. could be made more efficient
	if (!r_fill_state.curr_batch) {
		r_fill_state.curr_batch = _batch_request_new();
		r_fill_state.curr_batch->type = Batch::BT_DEFAULT;
		r_fill_state.curr_batch->first_command = r_command_start;
		// should tex_id be set to -1? check this
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

				bool change_batch = false;

				// conditions for creating a new batch
				if (r_fill_state.curr_batch->type != Batch::BT_RECT) {
					change_batch = true;

					// check for special case if there is only a single or small number of rects,
					// in which case we will use the legacy default rect renderer
					// because it is faster for single rects

					// we only want to do this if not a joined item with more than 1 item,
					// because joined items with more than 1, the command * will be incorrect
					// NOTE - this is assuming that use_hardware_transform means that it is a non-joined item!!
					// If that assumption is incorrect this will go horribly wrong.
					if (bdata.settings_use_single_rect_fallback && r_fill_state.use_hardware_transform) {
						bool is_single_rect = false;
						int command_num_next = command_num + 1;
						if (command_num_next < command_count) {
							RasterizerCanvas::Item::Command *command_next = commands[command_num_next];
							if ((command_next->type != RasterizerCanvas::Item::Command::TYPE_RECT) && (command_next->type != RasterizerCanvas::Item::Command::TYPE_TRANSFORM)) {
								is_single_rect = true;
							}
						} else {
							is_single_rect = true;
						}
						// if it is a rect on its own, do exactly the same as the default routine
						if (is_single_rect) {
							_prefill_default_batch(r_fill_state, command_num, *p_item);
							break;
						}
					} // if use hardware transform
				}

				Color col = rect->modulate;
				if (multiply_final_modulate) {
					col *= r_fill_state.final_modulate;
				}

				// instead of doing all the texture preparation for EVERY rect,
				// we build a list of texture combinations and do this once off.
				// This means we have a potentially rather slow step to identify which texture combo
				// using the RIDs.
				int old_batch_tex_id = r_fill_state.batch_tex_id;
				r_fill_state.batch_tex_id = _batch_find_or_create_tex(rect->texture, rect->normal_map, rect->flags & RasterizerCanvas::CANVAS_RECT_TILE, old_batch_tex_id);

				// try to create vertices BEFORE creating a batch,
				// because if the vertex buffer is full, we need to finish this
				// function, draw what we have so far, and then start a new set of batches

				// request FOUR vertices at a time, this is more efficient
				BatchVertex *bvs = bdata.vertices.request(4);
				if (!bvs) {
					// run out of space in the vertex buffer .. finish this function and draw what we have so far
					// return where we got to
					r_command_start = command_num;
					return true;
				}

				// conditions for creating a new batch
				if (old_batch_tex_id != r_fill_state.batch_tex_id) {
					change_batch = true;
				}

				// we need to treat color change separately because we need to count these
				// to decide whether to switch on the fly to colored vertices.
				if (!r_fill_state.curr_batch->color.equals(col)) {
					change_batch = true;
					bdata.total_color_changes++;
				}

				if (change_batch) {
					// put the tex pixel size  in a local (less verbose and can be a register)
					bdata.batch_textures[r_fill_state.batch_tex_id].tex_pixel_size.to(texpixel_size);

					// need to preserve texpixel_size between items
					r_fill_state.texpixel_size = texpixel_size;

					// open new batch (this should never fail, it dynamically grows)
					r_fill_state.curr_batch = _batch_request_new(false);

					r_fill_state.curr_batch->type = Batch::BT_RECT;
					r_fill_state.curr_batch->color.set(col);
					r_fill_state.curr_batch->batch_texture_id = r_fill_state.batch_tex_id;
					r_fill_state.curr_batch->first_command = command_num;
					r_fill_state.curr_batch->num_commands = 1;
					r_fill_state.curr_batch->first_quad = bdata.total_quads;
				} else {
					// we could alternatively do the count when closing a batch .. perhaps more efficient
					r_fill_state.curr_batch->num_commands++;
				}

				// fill the quad geometry
				Vector2 mins = rect->rect.position;

				if (r_fill_state.transform_mode == TM_TRANSLATE) {
					_software_transform_vertex(mins, r_fill_state.transform_combined);
				}

				Vector2 maxs = mins + rect->rect.size;

				// just aliases
				BatchVertex *bA = &bvs[0];
				BatchVertex *bB = &bvs[1];
				BatchVertex *bC = &bvs[2];
				BatchVertex *bD = &bvs[3];

				bA->pos.x = mins.x;
				bA->pos.y = mins.y;

				bB->pos.x = maxs.x;
				bB->pos.y = mins.y;

				bC->pos.x = maxs.x;
				bC->pos.y = maxs.y;

				bD->pos.x = mins.x;
				bD->pos.y = maxs.y;

				if (rect->rect.size.x < 0) {
					SWAP(bA->pos, bB->pos);
					SWAP(bC->pos, bD->pos);
				}
				if (rect->rect.size.y < 0) {
					SWAP(bA->pos, bD->pos);
					SWAP(bB->pos, bC->pos);
				}

				if (r_fill_state.transform_mode == TM_ALL) {
					_software_transform_vertex(bA->pos, r_fill_state.transform_combined);
					_software_transform_vertex(bB->pos, r_fill_state.transform_combined);
					_software_transform_vertex(bC->pos, r_fill_state.transform_combined);
					_software_transform_vertex(bD->pos, r_fill_state.transform_combined);
				}

				// uvs
				Rect2 src_rect = (rect->flags & RasterizerCanvas::CANVAS_RECT_REGION) ? Rect2(rect->source.position * texpixel_size, rect->source.size * texpixel_size) : Rect2(0, 0, 1, 1);

				// 10% faster calculating the max first
				Vector2 pos_max = src_rect.position + src_rect.size;
				Vector2 uvs[4] = {
					src_rect.position,
					Vector2(pos_max.x, src_rect.position.y),
					pos_max,
					Vector2(src_rect.position.x, pos_max.y),
				};

				if (rect->flags & RasterizerCanvas::CANVAS_RECT_TRANSPOSE) {
					SWAP(uvs[1], uvs[3]);
				}

				if (rect->flags & RasterizerCanvas::CANVAS_RECT_FLIP_H) {
					SWAP(uvs[0], uvs[1]);
					SWAP(uvs[2], uvs[3]);
				}
				if (rect->flags & RasterizerCanvas::CANVAS_RECT_FLIP_V) {
					SWAP(uvs[0], uvs[3]);
					SWAP(uvs[1], uvs[2]);
				}

				bA->uv.set(uvs[0]);
				bB->uv.set(uvs[1]);
				bC->uv.set(uvs[2]);
				bD->uv.set(uvs[3]);

				// increment quad count
				bdata.total_quads++;

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

	// only check whether to convert if there are quads (prevent divide by zero)
	// and we haven't decided to prevent color baking (due to e.g. MODULATE
	// being used in a shader)
	if (bdata.total_quads && !(bdata.joined_item_batch_flags & T_STORAGE::Shader::CanvasItem::PREVENT_COLOR_BAKING)) {
		// minus 1 to prevent single primitives (ratio 1.0) always being converted to colored..
		// in that case it is slightly cheaper to just have the color as part of the batch
		float ratio = (float)(bdata.total_color_changes - 1) / (float)bdata.total_quads;

		// use bigger than or equal so that 0.0 threshold can force always using colored verts
		if (ratio >= bdata.settings_colored_vertex_format_threshold) {
			bdata.use_colored_vertices = true;

			// small perf cost versus going straight to colored verts (maybe around 10%)
			// however more straightforward
			_batch_translate_to_colored();
		}
	}

	// send buffers to opengl
	get_this()->_batch_upload_buffers();

	RasterizerCanvas::Item::Command *const *commands = p_first_item->commands.ptr();

#ifdef DEBUG_ENABLED
	if (bdata.diagnose_frame) {
		diagnose_batches(commands);
	}
#endif

	get_this()->render_batches(commands, p_current_clip, r_reclip, p_material);
}


void render_joined_item_commands(const BItemJoined &p_bij, RasterizerCanvas::Item *p_current_clip, bool &r_reclip, typename T_STORAGE::Material *p_material, bool p_lit) {

	RasterizerCanvas::Item *item = 0;
	RasterizerCanvas::Item *first_item = bdata.item_refs[p_bij.first_item_ref].item;

	FillState fill_state;
	fill_state.reset();
	fill_state.use_hardware_transform = p_bij.use_hardware_transform();
	fill_state.extra_matrix_sent = false;

	// in the special case of custom shaders that read from VERTEX (i.e. vertex position)
	// we want to disable software transform of extra matrix
	if (bdata.joined_item_batch_flags & T_STORAGE::Shader::CanvasItem::PREVENT_VERTEX_BAKING) {
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
				fill_state.reset();
			}
		}
	}

	// flush if any left
	flush_render_batches(first_item, p_current_clip, r_reclip, p_material);
}
