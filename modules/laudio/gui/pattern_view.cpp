#include "pattern_view.h"
#include "../lpattern.h"

void PatternView::_notification(int p_what) {
	/*
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			set_physics_process_internal(true);
		} break;
		case NOTIFICATION_EXIT_TREE: {
			set_physics_process_internal(false);
		} break;

		case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			if (_pattern_dirty) {
				_pattern_dirty = false;
				if (_selected_pattern) {
					_selected_pattern->refresh_position();
					_selected_pattern->refresh_text();
				}
			}
		} break;
		default: {
		} break;
	}
	*/
}

void PatternView::_bind_methods() {
}
