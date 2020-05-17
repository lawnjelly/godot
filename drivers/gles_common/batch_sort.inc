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
