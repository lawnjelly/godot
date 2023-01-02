#include "lpattern_instance.h"

void Pattern::_notification(int p_what) {
	switch (p_what) {
			/*
				case NOTIFICATION_DRAW: {
					Ref<Texture> checkerboard = get_icon("Checkerboard", "EditorIcons");
					Size2 size = get_size();

					draw_texture_rect(checkerboard, Rect2(Point2(), size), true);
				} break;
				*/
		default: {
		} break;
	}
}

void Pattern::_bind_methods() {
}
