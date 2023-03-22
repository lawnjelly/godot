#include "pattern_view.h"
#include "../lpattern.h"

//void PatternView::set_zoom(int32_t p_zoom) {
//	if (p_zoom == _zoom)
//		return;

//	_zoom = p_zoom;

//	refresh_zoom();
//}

void PatternView::refresh_all_patterns() {
	for (int n = 0; n < get_child_count(); n++) {
		Pattern *pat = Object::cast_to<Pattern>(get_child(n));
		if (pat)
			pat->update();
	}
}

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
