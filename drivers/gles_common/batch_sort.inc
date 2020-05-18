void record_items(RasterizerCanvas::Item *p_item_list, int p_z) {
	while (p_item_list) {
		BSortItem *s = bdata.sort_items.request_with_grow();

		s->item = p_item_list;
		s->z_index = p_z;

		p_item_list = p_item_list->next;
	}
}

void join_sorted_items() {
	sort_items();

	int z = VS::CANVAS_ITEM_Z_MIN;
	_render_item_state.item_group_z = z;

	for (int s = 0; s < bdata.sort_items.size(); s++) {
		const BSortItem &si = bdata.sort_items[s];
		RasterizerCanvas::Item *ci = si.item;

		// change z?
		if (si.z_index != z) {
			z = si.z_index;

			// may not be required
			_render_item_state.item_group_z = z;

			// if z ranged lights are present, sometimes we have to disable joining over z_indices.
			// we do this here.
			// Note this restriction may be able to be relaxed with light bitfields, investigate!
			if (!bdata.join_across_z_indices) {
				_render_item_state.join_batch_break = true;
			}
		}

		bool join;

		if (_render_item_state.join_batch_break) {
			// always start a new batch for this item
			join = false;

			// could be another batch break (i.e. prevent NEXT item from joining this)
			// so we still need to run try_join_item
			// even though we know join is false.
			// also we need to run try_join_item for every item because it keeps the state up to date,
			// if we didn't run it the state would be out of date.
			get_this()->try_join_item(ci, _render_item_state, _render_item_state.join_batch_break);
		} else {
			join = get_this()->try_join_item(ci, _render_item_state, _render_item_state.join_batch_break);
		}

		// assume the first item will always return no join
		if (!join) {
			_render_item_state.joined_item = bdata.items_joined.request_with_grow();
			_render_item_state.joined_item->first_item_ref = bdata.item_refs.size();
			_render_item_state.joined_item->num_item_refs = 1;
			_render_item_state.joined_item->bounding_rect = ci->global_rect_cache;
			_render_item_state.joined_item->z_index = z;
			_render_item_state.joined_item->flags = bdata.joined_item_batch_flags;

			// add the reference
			BItemRef *r = bdata.item_refs.request_with_grow();
			r->item = ci;
			// we are storing final_modulate in advance per item reference
			// for baking into vertex colors.
			// this may not be ideal... as we are increasing the size of item reference,
			// but it is stupidly complex to calculate later, which would probably be slower.
			r->final_modulate = _render_item_state.final_modulate;
		} else {
			CRASH_COND(_render_item_state.joined_item == 0);
			_render_item_state.joined_item->num_item_refs += 1;
			_render_item_state.joined_item->bounding_rect = _render_item_state.joined_item->bounding_rect.merge(ci->global_rect_cache);

			BItemRef *r = bdata.item_refs.request_with_grow();
			r->item = ci;
			r->final_modulate = _render_item_state.final_modulate;
		}

	} // for s through sort items
}


void sort_items() {
	// turned off?
	if (!bdata.settings_item_reordering_lookahead) {
		return;
	}

	for (int s = 0; s < bdata.sort_items.size() - 2; s++) {
		if (sort_items_from(s)) {
#ifdef DEBUG_ENABLED
			bdata.stats_items_sorted++;
#endif
		}
	}
}

bool _sort_items_match(const BSortItem &p_a, const BSortItem &p_b) const {
	const RasterizerCanvas::Item *a = p_a.item;
	const RasterizerCanvas::Item *b = p_b.item;

	if (b->commands.size() != 1)
		return false;

	// tested outside function
	//	if (a->commands.size() != 1)
	//		return false;

	const RasterizerCanvas::Item::Command &cb = *b->commands[0];
	if (cb.type != RasterizerCanvas::Item::Command::TYPE_RECT)
		return false;

	const RasterizerCanvas::Item::Command &ca = *a->commands[0];
	// tested outside function
	//	if (ca.type != Item::Command::TYPE_RECT)
	//		return false;

	const RasterizerCanvas::Item::CommandRect *rect_a = static_cast<const RasterizerCanvas::Item::CommandRect *>(&ca);
	const RasterizerCanvas::Item::CommandRect *rect_b = static_cast<const RasterizerCanvas::Item::CommandRect *>(&cb);

	if (rect_a->texture != rect_b->texture)
		return false;

	return true;
}

bool sort_items_from(int p_start) {
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND_V((p_start + 1) >= bdata.sort_items.size(), false)
#endif

	const BSortItem &start = bdata.sort_items[p_start];
	int start_z = start.z_index;

	// check start is the right type for sorting
	if (start.item->commands.size() != 1) {
		return false;
	}
	const RasterizerCanvas::Item::Command &command_start = *start.item->commands[0];
	if (command_start.type != RasterizerCanvas::Item::Command::TYPE_RECT) {
		return false;
	}

	BSortItem &second = bdata.sort_items[p_start + 1];
	if (second.z_index != start_z) {
		// no sorting across z indices (for now)
		return false;
	}

	// if the neighbours are already a good match
	if (_sort_items_match(start, second)) // order is crucial, start first
	{
		return false;
	}

	// local cached aabb
	Rect2 second_AABB = second.item->global_rect_cache;

	// if the start and 2nd items overlap, can do no more
	if (start.item->global_rect_cache.intersects(second_AABB)) {
		return false;
	}

	// which neighbour to test
	int test_last = 2 + bdata.settings_item_reordering_lookahead;
	for (int test = 2; test < test_last; test++) {
		int test_sort_item_id = p_start + test;

		// if we've got to the end of the list, can't sort any more, give up
		if (test_sort_item_id >= bdata.sort_items.size()) {
			return false;
		}

		BSortItem *test_sort_item = &bdata.sort_items[test_sort_item_id];

		// across z indices?
		if (test_sort_item->z_index != start_z) {
			return false;
		}

		RasterizerCanvas::Item *test_item = test_sort_item->item;

		// if the test item overlaps the second item, we can't swap, AT ALL
		// because swapping an item OVER this one would cause artefacts
		if (second_AABB.intersects(test_item->global_rect_cache)) {
			return false;
		}

		// do they match?
		if (!_sort_items_match(start, *test_sort_item)) // order is crucial, start first
		{
			continue;
		}

		// we can only swap if there are no AABB overlaps with sandwiched neighbours
		bool ok = true;

		// start from 2, no need to check 1 as the second has already been checked against this item
		// in the intersection test above
		for (int sn = 2; sn < test; sn++) {
			BSortItem *sandwich_neighbour = &bdata.sort_items[p_start + sn];
			if (test_item->global_rect_cache.intersects(sandwich_neighbour->item->global_rect_cache)) {
				ok = false;
				break;
			}
		}
		if (!ok) {
			continue;
		}

		// it is ok to exchange them!
		BSortItem temp;
		temp.assign(second);
		second.assign(*test_sort_item);
		test_sort_item->assign(temp);

		return true;
	} // for test

	return false;
}
