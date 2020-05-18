bool _detect_batch_break(RasterizerCanvas::Item *p_ci) {
	int command_count = p_ci->commands.size();

	// Any item that contains commands that are default
	// (i.e. not handled by software transform and the batching renderer) should not be joined.

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

	if (command_count > bdata.settings_max_join_item_commands) {
		return true;
	} else {
		RasterizerCanvas::Item::Command *const *commands = p_ci->commands.ptr();

		// do as many commands as possible until the vertex buffer will be full up
		for (int command_num = 0; command_num < command_count; command_num++) {

			RasterizerCanvas::Item::Command *command = commands[command_num];
			CRASH_COND(!command);

			switch (command->type) {

				default: {
					return true;
				} break;
				case RasterizerCanvas::Item::Command::TYPE_RECT:
				case RasterizerCanvas::Item::Command::TYPE_TRANSFORM: {
				} break;
			} // switch

		} // for through commands

	} // else

	return false;
}
