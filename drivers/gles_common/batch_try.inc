protected:
bool _disallow_join_if_batch_types_too_different(RenderItemState &r_ris, uint32_t btf_allowed) {
	r_ris.joined_item_batch_type_flags_curr |= btf_allowed;

	bool disallow = false;

	if (r_ris.joined_item_batch_type_flags_prev & (~btf_allowed))
		disallow = true;

	// mark the render item state as having this batch type
	//	r_ris.joined_item_batch_type_flags = btf_allowed;

	return disallow;
}

bool _detect_batch_break(RenderItemState &r_ris, RasterizerCanvas::Item *p_ci, bool &r_batch_break) {
	int command_count = p_ci->commands.size();

	// Any item that contains commands that are default
	// (i.e. not handled by software transform and the batching renderer) should not be joined.

	// ALSO batched types that differ in what the vertex format is needed to be should not be
	// joined.

	// In order to work this out, it does a lookahead through the commands,
	// which could potentially be very expensive. As such it makes sense to put a limit on this
	// to some small number, which will catch nearly all cases which need joining,
	// but not be overly expensive in the case of items with large numbers of commands.

	// It is hard to know what this number should be, empirically,
	// and this has not been fully investigated. It works to join single sprite items when set to 1 or above.
	// Note that there is a cost to increasing this because it has to look in advance through
	// the commands.
	// On the other hand joining items where possible will usually be better up to a certain
	// number where the cost of software transform is higher than separate drawcalls with hardware
	// transform.

	// if there are more than this number of commands in the item, we
	// don't allow joining (separate state changes, and hardware transform)
	// This is set to quite a conservative (low) number until investigated properly.
	// const int MAX_JOIN_ITEM_COMMANDS = 16;

	r_ris.joined_item_batch_type_flags_curr = 0;

	if (command_count > bdata.settings_max_join_item_commands) {
		return true;
	} else {
		RasterizerCanvas::Item::Command *const *commands = p_ci->commands.ptr();

		// run through the commands looking for one that could prevent joining
		for (int command_num = 0; command_num < command_count; command_num++) {

			RasterizerCanvas::Item::Command *command = commands[command_num];
			CRASH_COND(!command);

			switch (command->type) {

				default: {
					//r_batch_break = true;
					return true;
				} break;
				case RasterizerCanvas::Item::Command::TYPE_LINE: {
					// special case, only batches certain lines
					RasterizerCanvas::Item::CommandLine *line = static_cast<RasterizerCanvas::Item::CommandLine *>(command);

					if (line->width > 1) {
						//r_batch_break = true;
						return true;
					}

					if (_disallow_join_if_batch_types_too_different(r_ris, RasterizerStorageCommon::BTF_LINE | RasterizerStorageCommon::BTF_LINE_AA)) {
						return true;
					}
				} break;
				case RasterizerCanvas::Item::Command::TYPE_POLYGON: {
					//return true;
					// only allow polygons to join if they aren't skeleton
					RasterizerCanvas::Item::CommandPolygon *poly = static_cast<RasterizerCanvas::Item::CommandPolygon *>(command);

					//					return true;

					// light angles not yet implemented, treat as default
					if (poly->normal_map != RID())
						return true;

					// we could possibly join polygons that are software skinned? NYI

					if (!get_this()->bdata.settings_use_software_skinning && poly->bones.size())
						return true;

					if (_disallow_join_if_batch_types_too_different(r_ris, RasterizerStorageCommon::BTF_POLY)) {
						//r_batch_break = true;
						return true;
					}
				} break;
				case RasterizerCanvas::Item::Command::TYPE_RECT:
				case RasterizerCanvas::Item::Command::TYPE_NINEPATCH: {
					if (_disallow_join_if_batch_types_too_different(r_ris, RasterizerStorageCommon::BTF_RECT))
						return true;
				} break;
				case RasterizerCanvas::Item::Command::TYPE_TRANSFORM: {
					// compatible with all types
				} break;
			} // switch

		} // for through commands

	} // else

	return false;
}
