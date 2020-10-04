///////////////////////////////////////////
// curiously recurring template pattern
T_STORAGE *get_storage() {
	return static_cast<const T *>(this)->storage;
}

const T_STORAGE *get_storage() const {
	return static_cast<const T *>(this)->storage;
}

T *get_this() {
	return static_cast<T *>(this);
}

const T *get_this() const {
	return static_cast<const T *>(this);
}
///////////////////////////////////////////

Batch *_batch_request_new(bool p_blank = true) {
	Batch *batch = bdata.batches.request();
	if (!batch) {
		// grow the batches
		bdata.batches.grow();

		// and the temporary batches (used for color verts)
		bdata.batches_temp.reset();
		bdata.batches_temp.grow();

		// this should always succeed after growing
		batch = bdata.batches.request();
#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
		CRASH_COND(!batch);
#endif
	}

	if (p_blank)
		memset(batch, 0, sizeof(Batch));

	return batch;
}

BatchVertex *_batch_vertex_request_new() {
	return bdata.vertices.request();
}

private:
bool prefill_line(RasterizerCanvas::Item::CommandLine *p_line, FillState &r_fill_state, int &r_command_start, int command_num, int command_count, RasterizerCanvas::Item *p_item, bool multiply_final_modulate) {
	bool change_batch = false;

	// we have separate batch types for non and anti aliased lines.
	// You can't batch the different types together.
	RasterizerStorageCommon::BatchType line_batch_type = RasterizerStorageCommon::BT_LINE;
	uint32_t line_batch_flags = RasterizerStorageCommon::BTF_LINE;
#ifdef GLES_OVER_GL
	if (p_line->antialiased) {
		line_batch_type = RasterizerStorageCommon::BT_LINE_AA;
		line_batch_flags = RasterizerStorageCommon::BTF_LINE_AA;
	}
#endif

	// conditions for creating a new batch
	if (r_fill_state.curr_batch->type != line_batch_type) {
		if (r_fill_state.sequence_batch_type_flags & (~line_batch_flags)) {
			// don't allow joining to a different sequence type
			r_command_start = command_num;
			return true;
		}
		r_fill_state.sequence_batch_type_flags |= line_batch_flags;

		change_batch = true;
	}

	// get the baked line color
	Color col = p_line->color;

	if (multiply_final_modulate)
		col *= r_fill_state.final_modulate;

	BatchColor bcol;
	bcol.set(col);

	// if the color has changed we need a new batch
	// (only single color line batches supported so far)
	if (r_fill_state.curr_batch->color != bcol)
		change_batch = true;

	// not sure if needed
	r_fill_state.batch_tex_id = -1;

	// try to create vertices BEFORE creating a batch,
	// because if the vertex buffer is full, we need to finish this
	// function, draw what we have so far, and then start a new set of batches

	// request multiple vertices at a time, this is more efficient
	BatchVertex *bvs = bdata.vertices.request(2);
	if (!bvs) {
		// run out of space in the vertex buffer .. finish this function and draw what we have so far
		// return where we got to
		r_command_start = command_num;
		return true;
	}

	if (change_batch) {

		// open new batch (this should never fail, it dynamically grows)
		r_fill_state.curr_batch = _batch_request_new(false);

		r_fill_state.curr_batch->type = line_batch_type;
		r_fill_state.curr_batch->color = bcol;
		r_fill_state.curr_batch->batch_texture_id = -1;
		r_fill_state.curr_batch->first_command = command_num;
		r_fill_state.curr_batch->num_commands = 1;
		//r_fill_state.curr_batch->first_quad = bdata.total_quads;
		r_fill_state.curr_batch->first_vert = bdata.total_verts;
	} else {
		// we could alternatively do the count when closing a batch .. perhaps more efficient
		r_fill_state.curr_batch->num_commands++;
	}

	// fill the geometry
	Vector2 from = p_line->from;
	Vector2 to = p_line->to;

	if (r_fill_state.transform_mode != TM_NONE) {
		_software_transform_vertex(from, r_fill_state.transform_combined);
		_software_transform_vertex(to, r_fill_state.transform_combined);
	}

	bvs[0].pos.set(from);
	bvs[0].uv.set(0, 0); // may not be necessary
	bvs[1].pos.set(to);
	bvs[1].uv.set(0, 0);

	bdata.total_verts += 2;

	return false;
}

template <bool SEND_LIGHT_ANGLES>
bool prefill_ninepatch(RasterizerCanvas::Item::CommandNinePatch *p_np, FillState &r_fill_state, int &r_command_start, int command_num, int command_count, RasterizerCanvas::Item *p_item, bool multiply_final_modulate) {
	typename T_STORAGE::Texture *tex = get_storage()->texture_owner.getornull(p_np->texture);

	if (!tex) {
		// FIXME: Handle textureless ninepatch gracefully
		WARN_PRINT("NinePatch without texture not supported yet in GLES2 backend, skipping.");
		return false;
	}
	if (tex->width == 0 || tex->height == 0) {
		WARN_PRINT("Cannot set empty texture to NinePatch.");
		return false;
	}

	// first check there are enough verts for this to complete successfully
	if (bdata.vertices.size() + (4 * 9) > bdata.vertices.max_size()) {
		// return where we got to
		r_command_start = command_num;
		return true;
	}

	// create a temporary rect so we can reuse the rect routine
	RasterizerCanvas::Item::CommandRect trect;

	trect.texture = p_np->texture;
	trect.normal_map = p_np->normal_map;
	trect.modulate = Color(1, 1, 1, 1);
	trect.flags = RasterizerCanvas::CANVAS_RECT_REGION;

	//Size2 texpixel_size(1.0f / tex->width, 1.0f / tex->height);

	Rect2 source = p_np->source;
	if (source.size.x == 0 && source.size.y == 0) {
		source.size.x = tex->width;
		source.size.y = tex->height;
	}

	float screen_scale = 1.0;

	if (source.size.x != 0 && source.size.y != 0) {

		screen_scale = MIN(p_np->rect.size.x / source.size.x, p_np->rect.size.y / source.size.y);
		screen_scale = MIN(1.0, screen_scale);
	}

	// temporarily override to prevent single rect fallback
	bool single_rect_fallback = bdata.settings_use_single_rect_fallback;
	bdata.settings_use_single_rect_fallback = false;

	// translate to rects
	Rect2 &rt = trect.rect;
	Rect2 &src = trect.source;

	float x[4];
	x[0] = p_np->rect.position.x;
	x[1] = x[0] + (p_np->margin[MARGIN_LEFT] * screen_scale);
	x[3] = x[0] + (p_np->rect.size.x);
	x[2] = x[3] - (p_np->margin[MARGIN_RIGHT] * screen_scale);

	float y[4];
	y[0] = p_np->rect.position.y;
	y[1] = y[0] + (p_np->margin[MARGIN_TOP] * screen_scale);
	y[3] = y[0] + (p_np->rect.size.y);
	y[2] = y[3] - (p_np->margin[MARGIN_BOTTOM] * screen_scale);

	float u[4];
	u[0] = source.position.x;
	u[1] = u[0] + (p_np->margin[MARGIN_LEFT]);
	u[3] = u[0] + (p_np->source.size.x);
	u[2] = u[3] - (p_np->margin[MARGIN_RIGHT]);

	float v[4];
	v[0] = source.position.y;
	v[1] = v[0] + (p_np->margin[MARGIN_TOP]);
	v[3] = v[0] + (p_np->source.size.y);
	v[2] = v[3] - (p_np->margin[MARGIN_BOTTOM]);

	// TOP LINE
	for (int line = 0; line < 3; line++) {
		rt.position = Vector2(x[0], y[line]);
		rt.size = Vector2(x[1] - x[0], y[line + 1] - y[line]);
		src.position = Vector2(u[0], v[line]);
		src.size = Vector2(u[1] - u[0], v[line + 1] - v[line]);
		prefill_rect<SEND_LIGHT_ANGLES>(&trect, r_fill_state, r_command_start, command_num, command_count, nullptr, p_item, multiply_final_modulate);

		rt.position.x = x[1];
		rt.size.x = x[2] - x[1];
		src.position.x = u[1];
		src.size.x = u[2] - u[1];
		prefill_rect<SEND_LIGHT_ANGLES>(&trect, r_fill_state, r_command_start, command_num, command_count, nullptr, p_item, multiply_final_modulate);

		rt.position.x = x[2];
		rt.size.x = x[3] - x[2];
		src.position.x = u[2];
		src.size.x = u[3] - u[2];
		prefill_rect<SEND_LIGHT_ANGLES>(&trect, r_fill_state, r_command_start, command_num, command_count, nullptr, p_item, multiply_final_modulate);
	}

	// MIDDLE LINE

	// restore single rect fallback
	bdata.settings_use_single_rect_fallback = single_rect_fallback;
	return false;
}

template <bool SEND_LIGHT_ANGLES>
bool prefill_polygon(RasterizerCanvas::Item::CommandPolygon *p_poly, FillState &r_fill_state, int &r_command_start, int command_num, int command_count, RasterizerCanvas::Item *p_item, bool multiply_final_modulate) {
	bool change_batch = false;

	// conditions for creating a new batch
	if (r_fill_state.curr_batch->type != RasterizerStorageCommon::BT_POLY) {

		// don't allow joining to a different sequence type
		if (r_fill_state.sequence_batch_type_flags & (~RasterizerStorageCommon::BTF_POLY)) {
			// don't allow joining to a different sequence type
			r_command_start = command_num;
			return true;
		}
		r_fill_state.sequence_batch_type_flags |= RasterizerStorageCommon::BTF_POLY;

		change_batch = true;
	}

	int num_inds = p_poly->indices.size();

	// nothing to draw?
	if (!num_inds)
		return false;

	// we aren't using indices, so will transform verts more than once .. less efficient.
	// could be done with a temporary vertex buffer
	BatchVertex *bvs = bdata.vertices.request(num_inds);
	if (!bvs) {
		// run out of space in the vertex buffer .. finish this function and draw what we have so far
		// return where we got to
		r_command_start = command_num;
		return true;
	}

	BatchColor *vertex_colors = bdata.vertex_colors.request(num_inds);
	CRASH_COND(!vertex_colors);

	// the modulate is always baked
	Color modulate;
	if (multiply_final_modulate)
		modulate = r_fill_state.final_modulate;
	else
		modulate = Color(1, 1, 1, 1);

	int old_batch_tex_id = r_fill_state.batch_tex_id;
	r_fill_state.batch_tex_id = _batch_find_or_create_tex(p_poly->texture, p_poly->normal_map, false, old_batch_tex_id);

	// conditions for creating a new batch
	if (old_batch_tex_id != r_fill_state.batch_tex_id) {
		change_batch = true;
	}

	// we need to treat color change separately because we need to count these
	// to decide whether to switch on the fly to colored vertices.
	//	if (!r_fill_state.curr_batch->color.equals(modulate)) {
	//		change_batch = true;
	//		bdata.total_color_changes++;
	//	}

	if (change_batch) {
		// put the tex pixel size  in a local (less verbose and can be a register)
		const BatchTex &batchtex = bdata.batch_textures[r_fill_state.batch_tex_id];
		batchtex.tex_pixel_size.to(r_fill_state.texpixel_size);

		if (bdata.settings_uv_contract) {
			r_fill_state.contract_uvs = (batchtex.flags & VS::TEXTURE_FLAG_FILTER) == 0;
		}

		// open new batch (this should never fail, it dynamically grows)
		r_fill_state.curr_batch = _batch_request_new(false);

		r_fill_state.curr_batch->type = RasterizerStorageCommon::BT_POLY;

		// modulate unused except for debugging?
		r_fill_state.curr_batch->color.set(modulate);
		r_fill_state.curr_batch->batch_texture_id = r_fill_state.batch_tex_id;
		r_fill_state.curr_batch->first_command = command_num;
		r_fill_state.curr_batch->num_commands = num_inds;
		//		r_fill_state.curr_batch->num_elements = num_inds;
		r_fill_state.curr_batch->first_vert = bdata.total_verts;
	} else {
		// we could alternatively do the count when closing a batch .. perhaps more efficient
		r_fill_state.curr_batch->num_commands += num_inds;
	}

	// PRECALCULATE THE COLORS (as there may be less colors than there are indices
	// in either hardware or software paths)
	BatchColor vcol;
	int num_verts = p_poly->points.size();

	// in special cases, only 1 color is specified by convention, so we want to preset this
	// to use in all verts.
	if (p_poly->colors.size())
		vcol.set(p_poly->colors[0] * modulate);
	else
		// color is undefined, use modulate color straight
		vcol.set(modulate);

	BatchColor *precalced_colors = (BatchColor *)alloca(num_verts * sizeof(BatchColor));

	// two stage, super efficient setup of precalculated colors
	int num_colors_specified = p_poly->colors.size();

	for (int n = 0; n < num_colors_specified; n++) {
		vcol.set(p_poly->colors[n] * modulate);
		precalced_colors[n] = vcol;
	}
	for (int n = num_colors_specified; n < num_verts; n++) {
		precalced_colors[n] = vcol;
	}

	if (!software_skin_poly(p_poly, p_item, bvs, vertex_colors, r_fill_state, precalced_colors)) {

		for (int n = 0; n < num_inds; n++) {
			int ind = p_poly->indices[n];

			CRASH_COND(ind >= p_poly->points.size());

			if (r_fill_state.transform_mode != TM_NONE) {
				Vector2 pos = p_poly->points[ind];
				_software_transform_vertex(pos, r_fill_state.transform_combined);
				bvs[n].pos.set(pos.x, pos.y);
			} else {
				const Point2 &pos = p_poly->points[ind];
				bvs[n].pos.set(pos.x, pos.y);
			}

			if (ind < p_poly->uvs.size()) {
				const Point2 &uv = p_poly->uvs[ind];
				bvs[n].uv.set(uv.x, uv.y);
			}

			vertex_colors[n] = precalced_colors[ind];
		}
	} // if not software skinning

	// increment total vert count
	bdata.total_verts += num_inds;

	return false;
}

bool software_skin_poly(RasterizerCanvas::Item::CommandPolygon *p_poly, RasterizerCanvas::Item *p_item, BatchVertex *bvs, BatchColor *vertex_colors, const FillState &p_fill_state, const BatchColor *p_precalced_colors) {
	//	if (get_this()->state.using_skeleton)
	if (p_item->skeleton == RID())
		return false;

	//	return false;

	int num_inds = p_poly->indices.size();
	int num_verts = p_poly->points.size();

	RID skeleton = p_item->skeleton;
	int bone_count = RasterizerStorage::base_singleton->skeleton_get_bone_count(skeleton);

	// we want a temporary buffer of positions to transform

	// this is completely stupid, but is a way to get around compiler warnings about using memset on Vector2s...
	// we could alternatively disable compiler warnings but that is less portable...
	//struct pod_Vector2 {real_t x, y;};

	//	pod_Vector2 *pTemps = (pod_Vector2 *)alloca(num_verts * sizeof(pod_Vector2));
	Vector2 *pTemps = (Vector2 *)alloca(num_verts * sizeof(Vector2));
	memset((void *)pTemps, 0, num_verts * sizeof(Vector2));

	//	for (int n=0; n<num_verts; n++)
	//	{
	//		pTemps[n] = p_poly->points[n];
	//	}

	//	const Transform2D &sti = get_this()->state.skeleton_transform;

	// get the bone transforms
	Transform2D *bone_transforms = (Transform2D *)alloca(bone_count * sizeof(Transform2D));
	for (int b = 0; b < bone_count; b++) {
		bone_transforms[b] = RasterizerStorage::base_singleton->skeleton_bone_get_transform_2d(skeleton, b);
	}

	if (num_verts && (p_poly->bones.size() == num_verts * 4) && (p_poly->weights.size() == p_poly->bones.size())) {

		for (int n = 0; n < num_verts; n++) {
			const Vector2 &src_pos = p_poly->points[n];
			//			Vector2 &dst_pos = * ((Vector2 *) &pTemps[n]);
			Vector2 &dst_pos = pTemps[n];

			float total_weight = 0.0f;

			for (int k = 0; k < 4; k++) {
				int bone_id = p_poly->bones[n * 4 + k];
				float weight = p_poly->weights[n * 4 + k];
				if (weight == 0.0f)
					continue;

				total_weight += weight;

				CRASH_COND(bone_id >= bone_count);
				dst_pos += (bone_transforms[bone_id].xform(src_pos)) * weight;
			}

			if (total_weight < 0.01f)
				dst_pos = src_pos;

			//			CRASH_COND(total_weight < 0.99f);
		}

		// software transform with combined matrix?
		if (p_fill_state.transform_mode != TM_NONE) {
			for (int n = 0; n < num_verts; n++) {
				//Vector2 &dst_pos = * ((Vector2 *) &pTemps[n]);
				Vector2 &dst_pos = pTemps[n];
				_software_transform_vertex(dst_pos, p_fill_state.transform_combined);
			}
		}

	} // if bone format matches
	else {
		// not supported
	}

	// output to the batch verts
	for (int n = 0; n < num_inds; n++) {
		int ind = p_poly->indices[n];

		CRASH_COND(ind >= num_verts);
		//const Point2 &pos = * ((Vector2 *) &pTemps[ind]);
		const Point2 &pos = pTemps[ind];
		bvs[n].pos.set(pos.x, pos.y);

		if (ind < p_poly->uvs.size()) {
			const Point2 &uv = p_poly->uvs[ind];
			bvs[n].uv.set(uv.x, uv.y);
		}

		vertex_colors[n] = p_precalced_colors[ind];
	}

	return true;
}

template <bool SEND_LIGHT_ANGLES>
bool prefill_rect(RasterizerCanvas::Item::CommandRect *rect, FillState &r_fill_state, int &r_command_start, int command_num, int command_count, RasterizerCanvas::Item::Command *const *commands, RasterizerCanvas::Item *p_item, bool multiply_final_modulate) {
	bool change_batch = false;

	// conditions for creating a new batch
	if (r_fill_state.curr_batch->type != RasterizerStorageCommon::BT_RECT) {

		// don't allow joining to a different sequence type
		if (r_fill_state.sequence_batch_type_flags & (~RasterizerStorageCommon::BTF_RECT)) {
			// don't allow joining to a different sequence type
			r_command_start = command_num;
			return true;
		}
		r_fill_state.sequence_batch_type_flags |= RasterizerStorageCommon::BTF_RECT;

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
				return false;
			}
		} // if use hardware transform
	}

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

	// are we using large FVF?
	const bool use_large_verts = bdata.use_large_verts;
	const bool use_modulate = bdata.use_modulate;

	Color col = rect->modulate;

	if (!use_large_verts) {
		if (multiply_final_modulate) {
			col *= r_fill_state.final_modulate;
		}
	}

	// instead of doing all the texture preparation for EVERY rect,
	// we build a list of texture combinations and do this once off.
	// This means we have a potentially rather slow step to identify which texture combo
	// using the RIDs.
	int old_batch_tex_id = r_fill_state.batch_tex_id;
	r_fill_state.batch_tex_id = _batch_find_or_create_tex(rect->texture, rect->normal_map, rect->flags & RasterizerCanvas::CANVAS_RECT_TILE, old_batch_tex_id);

	//r_fill_state.use_light_angles = send_light_angles;
	if (SEND_LIGHT_ANGLES) {
		bdata.use_light_angles = true;
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
		const BatchTex &batchtex = bdata.batch_textures[r_fill_state.batch_tex_id];
		batchtex.tex_pixel_size.to(r_fill_state.texpixel_size);

		if (bdata.settings_uv_contract) {
			r_fill_state.contract_uvs = (batchtex.flags & VS::TEXTURE_FLAG_FILTER) == 0;
		}

		// need to preserve texpixel_size between items
		//r_fill_state.texpixel_size = r_fill_state.texpixel_size;

		// open new batch (this should never fail, it dynamically grows)
		r_fill_state.curr_batch = _batch_request_new(false);

		r_fill_state.curr_batch->type = RasterizerStorageCommon::BT_RECT;
		r_fill_state.curr_batch->color.set(col);
		r_fill_state.curr_batch->batch_texture_id = r_fill_state.batch_tex_id;
		r_fill_state.curr_batch->first_command = command_num;
		r_fill_state.curr_batch->num_commands = 1;
		//r_fill_state.curr_batch->first_quad = bdata.total_quads;
		r_fill_state.curr_batch->first_vert = bdata.total_verts;
	} else {
		// we could alternatively do the count when closing a batch .. perhaps more efficient
		r_fill_state.curr_batch->num_commands++;
	}

	// fill the quad geometry
	Vector2 mins = rect->rect.position;

	if (r_fill_state.transform_mode == TM_TRANSLATE) {

		if (!use_large_verts) {
			_software_transform_vertex(mins, r_fill_state.transform_combined);
		}
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

	// possibility of applying flips here for normal mapping .. but they don't seem to be used
	if (rect->rect.size.x < 0) {
		SWAP(bA->pos, bB->pos);
		SWAP(bC->pos, bD->pos);
	}
	if (rect->rect.size.y < 0) {
		SWAP(bA->pos, bD->pos);
		SWAP(bB->pos, bC->pos);
	}

	if (r_fill_state.transform_mode == TM_ALL) {

		if (!use_large_verts) {
			_software_transform_vertex(bA->pos, r_fill_state.transform_combined);
			_software_transform_vertex(bB->pos, r_fill_state.transform_combined);
			_software_transform_vertex(bC->pos, r_fill_state.transform_combined);
			_software_transform_vertex(bD->pos, r_fill_state.transform_combined);
		}
	}

	// uvs
	Vector2 src_min;
	Vector2 src_max;
	if (rect->flags & RasterizerCanvas::CANVAS_RECT_REGION) {
		src_min = rect->source.position;
		src_max = src_min + rect->source.size;

		src_min *= r_fill_state.texpixel_size;
		src_max *= r_fill_state.texpixel_size;

		const float uv_epsilon = bdata.settings_uv_contract_amount;

		// nudge offset for the maximum to prevent precision error on GPU reading into line outside the source rect
		// this is very difficult to get right.
		if (r_fill_state.contract_uvs) {
			src_min.x += uv_epsilon;
			src_min.y += uv_epsilon;
			src_max.x -= uv_epsilon;
			src_max.y -= uv_epsilon;
		}
	} else {
		src_min = Vector2(0, 0);
		src_max = Vector2(1, 1);
	}

	// 10% faster calculating the max first
	Vector2 uvs[4] = {
		src_min,
		Vector2(src_max.x, src_min.y),
		src_max,
		Vector2(src_min.x, src_max.y),
	};

	// for encoding in light angle
	// flips should be optimized out when not being used for light angle.
	bool flip_h = false;
	bool flip_v = false;

	if (rect->flags & RasterizerCanvas::CANVAS_RECT_TRANSPOSE) {
		SWAP(uvs[1], uvs[3]);
	}

	if (rect->flags & RasterizerCanvas::CANVAS_RECT_FLIP_H) {
		SWAP(uvs[0], uvs[1]);
		SWAP(uvs[2], uvs[3]);
		flip_h = !flip_h;
		flip_v = !flip_v;
	}
	if (rect->flags & RasterizerCanvas::CANVAS_RECT_FLIP_V) {
		SWAP(uvs[0], uvs[3]);
		SWAP(uvs[1], uvs[2]);
		flip_v = !flip_v;
	}

	bA->uv.set(uvs[0]);
	bB->uv.set(uvs[1]);
	bC->uv.set(uvs[2]);
	bD->uv.set(uvs[3]);

	// modulate
	if (use_modulate) {
		// store the final modulate separately from the rect modulate
		BatchColor *pBC = bdata.vertex_modulates.request(4);
#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
		CRASH_COND(pBC == nullptr);
#endif
		pBC[0].set(r_fill_state.final_modulate);
		pBC[1] = pBC[0];
		pBC[2] = pBC[0];
		pBC[3] = pBC[0];
	}

	if (use_large_verts) {
		// store the transform separately
		BatchTransform *pBT = bdata.vertex_transforms.request(4);
#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
		CRASH_COND(pBT == nullptr);
#endif

		const Transform2D &tr = r_fill_state.transform_combined;

		pBT[0].translate.set(tr.elements[2]);
		// could do swizzling in shader?
		pBT[0].basis[0].set(tr.elements[0][0], tr.elements[1][0]);
		pBT[0].basis[1].set(tr.elements[0][1], tr.elements[1][1]);

		pBT[1] = pBT[0];
		pBT[2] = pBT[0];
		pBT[3] = pBT[0];
	}

	if (SEND_LIGHT_ANGLES) {
		// we can either keep the light angles in sync with the verts when writing,
		// or sync them up during translation. We are syncing in translation.
		// N.B. There may be batches that don't require light_angles between batches that do.
		float *angles = bdata.light_angles.request(4);
#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
		CRASH_COND(angles == nullptr);
#endif

		float angle = 0.0f;
		const float TWO_PI = Math_PI * 2;

		if (r_fill_state.transform_mode != TM_NONE) {

			const Transform2D &tr = r_fill_state.transform_combined;

			// apply to an x axis
			// the x axis and y axis can be taken directly from the transform (no need to xform identity vectors)
			Vector2 x_axis(tr.elements[0][0], tr.elements[1][0]);

			// have to do a y axis to check for scaling flips
			// this is hassle and extra slowness. We could only allow flips via the flags.
			Vector2 y_axis(tr.elements[0][1], tr.elements[1][1]);

			// has the x / y axis flipped due to scaling?
			float cross = x_axis.cross(y_axis);
			if (cross < 0.0f) {
				flip_v = !flip_v;
			}

			// passing an angle is smaller than a vector, it can be reconstructed in the shader
			angle = x_axis.angle();

			// we don't want negative angles, as negative is used to encode flips.
			// This moves range from -PI to PI to 0 to TWO_PI
			if (angle < 0.0f)
				angle += TWO_PI;

		} // if transform needed

		// if horizontal flip, angle is shifted by 180 degrees
		if (flip_h) {
			angle += Math_PI;

			// mod to get back to 0 to TWO_PI range
			angle = fmodf(angle, TWO_PI);
		}

		// add 1 (to take care of zero floating point error with sign)
		angle += 1.0f;

		// flip if necessary to indicate a vertical flip in the shader
		if (flip_v)
			angle *= -1.0f;

		// light angle must be sent for each vert, instead as a single uniform in the uniform draw method
		// this has the benefit of enabling batching with light angles.
		for (int n = 0; n < 4; n++) {
			angles[n] = angle;
		}
	}

	// increment quad count
	bdata.total_quads++;
	bdata.total_verts += 4;

	return false;
}
