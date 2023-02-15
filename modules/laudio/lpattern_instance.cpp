#include "lpattern_instance.h"
#include "lpattern.h"
#include "lsong.h"
#include "scene/main/viewport.h"

String LPatternInstance::get_name() const {
	LPattern *p = get_pattern();
	if (p) {
		return p->get_name();
	}
	return "undefined";
}

LPatternInstance::~LPatternInstance() {
	// This should be taken care of externally
	//DEV_ASSERT(!_pattern);
}

LPattern *LPatternInstance::get_pattern() const {
	return g_lapp.patterns.get(_handle_pattern);
}

/////////////////////////////

LPatternInstance *Pattern::get_pattern_instance() const {
	return g_lapp.pattern_instances.get(data.handle_pattern_instance);
}

void Pattern::set_pattern_instance(LHandle p_handle) {
	data.handle_pattern_instance = p_handle;
}

void Pattern::_gui_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	Ref<InputEventMouseButton> mouse_button = p_event;
	//bool ui_accept = p_event->is_action("ui_accept") && !p_event->is_echo();
	bool ui_accept = false;

	//	bool button_masked = mouse_button.is_valid() && ((1 << (mouse_button->get_button_index() - 1)) & button_mask) > 0;
	bool button_masked = mouse_button.is_valid();
	if (button_masked || ui_accept) {
		//was_mouse_pressed = button_masked;
		//on_action_event(p_event);
		//was_mouse_pressed = false;

		if (mouse_button->is_pressed()) {
			_pattern_pressed();
			//print_line("button pressed");
			return;
		}
	}
}

void Pattern::_pattern_pressed() {
	print_line("Pattern _pressed");
	if (Song::get_current_song()) {
		Song::get_current_song()->pattern_select(this);
	}

	//	pressed();
	//	emit_signal("pressed");
}

void Pattern::set_text(String p_text) {
	data.text = p_text;
	update();
}

void Pattern::set_selected(bool p_selected) {
	data.selected = p_selected;
	//	update();
}

void Pattern::refresh_position() {
	LPatternInstance *pi = get_pattern_instance();
	if (pi) {
		set_position(Point2(pi->data.tick_start, 0));
		set_size(Size2(pi->get_tick_length(), 24));
		set_text(pi->get_name());
	}
}

void Pattern::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			//connect("pressed", this, "_pressed");
		} break;
		case NOTIFICATION_ENTER_TREE: {
			refresh_position();
		} break;
		case NOTIFICATION_EXIT_TREE: {
		} break;
		case NOTIFICATION_FOCUS_ENTER: {
			//print_line("focus enter");
		} break;
		case NOTIFICATION_FOCUS_EXIT: {
			//print_line("focus exit");
		} break;
		case NOTIFICATION_MOUSE_ENTER: {
			//print_line("mouse enter");
		} break;
		case NOTIFICATION_MOUSE_EXIT: {
			//print_line("mouse exit");
		} break;

		case NOTIFICATION_DRAW: {
			_draw();
		} break;
		default: {
		} break;
	}
}

void Pattern::_draw() {
	//	Ref<Texture> checkerboard = get_icon("Checkerboard", "EditorIcons");
	Size2 size = get_size();

	//	draw_texture_rect(checkerboard, Rect2(Point2(), size), true);

	RID ci = get_canvas_item();

	Color col = data.selected ? Color(0.5, 0.2, 0.2, 1) : Color(0.2, 0.2, 0.2, 1);

	draw_rect(Rect2(Point2(), size), col);

	Ref<Font> font = get_font("font");
	float line_height = font->get_height();
	font->draw(ci, Point2(0, line_height), data.text);

	//	Ref<StyleBox> style = get_stylebox("normal");
	//	style->draw(ci, Rect2(Point2(0, 0), size));
}

void Pattern::_bind_methods() {
	//ClassDB::bind_method(D_METHOD("_pressed"), &Pattern::_pressed);

	ClassDB::bind_method(D_METHOD("_gui_input"), &Pattern::_gui_input);
	//ClassDB::bind_method(D_METHOD("_unhandled_input"), &Pattern::_unhandled_input);

	//ClassDB::bind_method("_gui_input", &Pattern::_gui_input);
}

void Pattern::pattern_delete() {
	// does the pattern original need to be deleted too?
	LPatternInstance *pi = get_pattern_instance();
	if (pi) {
		LPattern *pat = pi->get_pattern();
		if (pat) {
			if (pat->release()) {
				g_lapp.patterns.free(pat->data.handle);
			}
			pi->set_pattern(LHandle());
		}
	}

	g_lapp.pattern_instances.free(data.handle_pattern_instance);
	data.handle_pattern_instance.set_invalid();
	queue_delete();
}

Pattern::Pattern() {
	set_focus_mode(FOCUS_ALL);
	set_default_cursor_shape(CURSOR_POINTING_HAND);
}
