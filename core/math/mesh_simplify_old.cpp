/*
bool MeshSimplify::_simplify_vert_primary(uint32_t p_vert_from_id, uint32_t p_vert_to_id) {
	real_t max_displacement = 0.0;

	Vert &vert_from = _verts[p_vert_from_id];
	Vert &vert_to = _verts[p_vert_to_id];

	if (_try_simplify_merge_vert(vert_from, p_vert_from_id, p_vert_to_id, max_displacement)) {
		// special case, if we have a linked vert, that must also do a mirror merge for this to be allowed.
		bool allow = true;
		if (vert_from.linked_verts.size()) {
			allow = false;

			//if (merge_vert.linked_verts.size() == 1) {
			//			if (true) {
			uint32_t link_from = vert_from.linked_verts[0];

			if (link_from != p_vert_to_id) {
				const Vert &link_from_vert = _verts[link_from];

				// find the edge neighbour of the link_from that is also a linked vert
				// of the merged_vert
				uint32_t link_to = 0;
				bool found = false;
				for (int ln = 0; ln < 2; ln++) {
					link_to = link_from_vert.edge_vert_neighs[ln];

					// is link_to on the list?
					for (int lv = 0; lv < vert_to.linked_verts.size(); lv++) {
						if (vert_to.linked_verts[lv] == link_to) {
							// found!
							found = true;
							break;
						}
					}
				}

				// if we didn't find, we are attempting to join a linked vert to a non-linked vert,
				// and this will disrupt the border, so we do not allow
				if (found) {
					//_check_merge_allowed(p_vert_id, test_merge_vert);

					// test and carry out the linked merge, if it can work
					allow = _tri_simplify_linked_merge_vert(link_from, link_to);
					//_check_merge_allowed(p_vert_id, test_merge_vert);
				} else {
					allow = false;
				}

			} // if not trying to link to the merge vert
			//			}
		}

		if (allow) {
			_add_collapse(p_vert_from_id, p_vert_to_id, max_displacement);
			return true;
		}
	}

	return false;
}

bool MeshSimplify::_simplify_vert(uint32_t p_vert_from_id) {
	// get a list of possible verts to merge to
	if (!_find_vert_to_merge_to(p_vert_from_id))
		return false;

	for (int m = 0; m < _merge_vert_ids.size(); m++) {
		uint32_t vert_to_id = _merge_vert_ids[m];

		if (_simplify_vert_primary(p_vert_from_id, vert_to_id)) {
			// special case of mirror verts, try and do the same but mirror merge
			if (_mirror_verts_only) {
				Vert &vert_from = _verts[p_vert_from_id];
				Vert &vert_to = _verts[vert_to_id];

				if ((vert_from.mirror_vert != UINT32_MAX) && (vert_to.mirror_vert != UINT32_MAX)) {
					_simplify_vert_primary(vert_from.mirror_vert, vert_to.mirror_vert);
				}
			}

			return true;
		}
	}

	return false;
}

bool MeshSimplify::_tri_simplify_linked_merge_vert(uint32_t p_vert_from_id, uint32_t p_vert_to_id) {
	Vert &vert_from = _verts[p_vert_from_id];

	real_t max_displacement = 0.0;

	if (_try_simplify_merge_vert(vert_from, p_vert_from_id, p_vert_to_id, max_displacement)) {
		_add_collapse(p_vert_from_id, p_vert_to_id, max_displacement);
		return true;
	}

	return false;
}

bool MeshSimplify::_simplify() {
	uint32_t start_from = 0;

	while (true) {
		uint32_t vert_id = _choose_vert_to_merge(start_from);
		if (vert_id == UINT32_MAX)
			return false;

		// for next loop
		start_from = vert_id + 1;

		if (_simplify_vert(vert_id)) {
#ifdef GODOT_MESH_SIMPLIFY_USE_DIRTY_VERTS
			_verts[vert_id].dirty = false;
#endif
			return true;
		}

		// mark this vertex as no longer dirty
#ifdef GODOT_MESH_SIMPLIFY_USE_DIRTY_VERTS
		_verts[vert_id].dirty = false;
#endif
	}

	return false;
}

*/
