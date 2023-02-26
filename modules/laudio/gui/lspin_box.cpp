#include "lspin_box.h"
#include "core/math/expression.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/spin_box.h"

void LSpinBox::Box::set_value(int32_t p_value) {
	value = p_value % max_value;
	line_edit->set_text(itos(value));
}

void LSpinBox::_adjust_widths() {
	float total_width = 0;
	//	float box_width	= 24;
	//	float box_height = 12;

	for (int n = 0; n < _num_boxes; n++) {
		LineEdit *le = _boxes[n].line_edit;
		//le->set_custom_minimum_size(Size2(16, 24));
		le->set_position(Vector2(total_width, 0));
		//le->set_size(Vector2(box_width, box_height));
		//le->set_margin(MARGIN_RIGHT, -16);
		float seg_width = le->get_size().x + 2;
		//		float seg_width = box_width + 2;

		// icon width
		if (n == _num_boxes - 1) {
			seg_width += 16;
		}
		total_width += seg_width;
		_boxes[n].icon_x = total_width - 16;
	}

	//set_size(Vector2(total_width, 24));
	set_custom_minimum_size(Vector2(total_width, 24));
}

void LSpinBox::_adjust_width_for_icon(const Ref<Texture> &icon) {
	//	int w = icon->get_width();

	//	if (w != _last_w) {
	//		for (uint32_t n=0; n<_num_boxes; n++)
	//		{
	//			line_edit[n]->set_position(
	//		}

	//		if (line_edit)
	//			line_edit->set_margin(MARGIN_RIGHT, -w);
	//		_last_w = w;
	//	}
}

void LSpinBox::_gui_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	//	if (!is_editable()) {
	//		return;
	//	}

	Ref<InputEventMouseButton> mb = p_event;

	int32_t step = _step[0];
	//double step = get_custom_arrow_step() != 0.0 ? get_custom_arrow_step() : get_step();

	if (mb.is_valid() && mb->is_pressed()) {
		bool up = mb->get_position().y < (get_size().height / 2);

		switch (mb->get_button_index()) {
			case BUTTON_LEFT: {
				//grab_focus();
				//line_edit->grab_focus();

				set_value_with_signal(get_value() + (up ? step : -step));

				//				range_click_timer->set_wait_time(0.6);
				//				range_click_timer->set_one_shot(true);
				//				range_click_timer->start();

				//				drag.allowed = true;
				//				drag.capture_pos = mb->get_position();
			} break;
			case BUTTON_RIGHT: {
				//								line_edit->grab_focus();
				//								set_value((up ? get_max() : get_min()));
				set_value_with_signal(get_value() + (up ? 1 : -1));
			} break;
			case BUTTON_WHEEL_UP: {
				//				if (line_edit->has_focus()) {
				set_value_with_signal(get_value() + step * mb->get_factor());
				accept_event();
				//				}
			} break;
			case BUTTON_WHEEL_DOWN: {
				//				if (line_edit->has_focus()) {
				set_value_with_signal(get_value() - step * mb->get_factor());
				accept_event();
				//				}
			} break;
		}
	}

	/*
	if (mb.is_valid() && !mb->is_pressed() && mb->get_button_index() == BUTTON_LEFT) {
		//set_default_cursor_shape(CURSOR_ARROW);
		range_click_timer->stop();
		_release_mouse();
		drag.allowed = false;
	}

	Ref<InputEventMouseMotion> mm = p_event;

	if (mm.is_valid() && mm->get_button_mask() & BUTTON_MASK_LEFT) {
		if (drag.enabled) {
			drag.diff_y += mm->get_relative().y;
			double diff_y = -0.01 * Math::pow(ABS(drag.diff_y), 1.8f) * SGN(drag.diff_y);
			set_value(CLAMP(drag.base_val + step * diff_y, get_min(), get_max()));
		} else if (drag.allowed && drag.capture_pos.distance_to(mm->get_position()) > 2) {
			Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_CAPTURED);
			drag.enabled = true;
			drag.base_val = get_value();
			drag.diff_y = 0;
		}
	}
	*/
}

void LSpinBox::_text_entered(const String &p_string) {
	int which_box = -1;
	// find which box has focus
	for (int n = 0; n < _num_boxes; n++) {
		if (_boxes[n].line_edit->has_focus()) {
			which_box = n;
			break;
		}
	}

	ERR_FAIL_COND(which_box == -1);

	Ref<Expression> expr;
	expr.instance();
	// Ignore the prefix and suffix in the expression
	//	Error err = expr->parse(p_string.trim_prefix(prefix + " ").trim_suffix(" " + suffix));
	Error err = expr->parse(p_string);
	if (err != OK) {
		return;
	}

	Variant value = expr->execute(Array(), nullptr, false);
	if (value.get_type() != Variant::NIL) {
		int32_t new_box_value = value;

		// calculate how much to increment all over value
		Box &box = _boxes[which_box];
		int32_t change = new_box_value - box.value;
		_value += change * box.multiplier;

		//print_line("new calculated total value " + itos(_value));

		//		_value += (int32_t) value
		//		int left = value;
		//		for (int b = which_box; b >= 0; b--)
		//		{
		//			Box &box = _boxes[b];
		//			box.set_value(left % box.max_value);
		//			left = left / box.max_value;

		//		}
		//		value = value % max_value;
		//		_boxes[which_box].set_value(value);
		//set_value(value);
	}
	//	_recalc_value_from_boxes();
	set_value_with_signal(_value);
	//_value_changed(0);
}

void LSpinBox::_line_edit_input(const Ref<InputEvent> &p_event) {
}

void LSpinBox::set_value(int32_t p_value) {
	_value = p_value;
	_store_value_in_boxes();
}

void LSpinBox::set_value_with_signal(int32_t p_value) {
	set_value(p_value);
	emit_signal("value_changed", _value);
}

void LSpinBox::_recalc_value_from_boxes() {
	int32_t v = 0;

	for (int n = 0; n < _num_boxes; n++) {
		int box_id = _num_boxes - n - 1;
		Box &box = _boxes[box_id];
		v += box.value * box.multiplier;
	}
	_value = v;
	emit_signal("value_changed", _value);
}

//void LSpinBox::_value_changed(double)
void LSpinBox::_store_value_in_boxes() {
	int32_t v = get_value();

	for (int n = 0; n < _num_boxes; n++) {
		int box_id = _num_boxes - n - 1;
		Box &box = _boxes[box_id];

		int box_value = v % box.max_value;
		box.set_value(box_value);
		//		box.line_edit->set_text(itos(box_value));

		v /= box.max_value;
	}

	//	String value = String::num(get_value(), Math::range_step_decimals(get_step()));

	//	if (!line_edit->has_focus()) {
	//		if (prefix != "") {
	//			value = prefix + " " + value;
	//		}
	//		if (suffix != "") {
	//			value += " " + suffix;
	//		}
	//	}

	//	line_edit->set_text(value);
}

void LSpinBox::_line_edit_focus_enter(int p_which) {
	//	int col = line_edit->get_cursor_position();
	//	_value_changed(0); // Update the LineEdit's text.
	//	line_edit->set_cursor_position(col);
}

void LSpinBox::_line_edit_focus_exit(int p_which) {
	// discontinue because the focus_exit was caused by right-click context menu
	//	if (line_edit->get_menu()->is_visible()) {
	//		return;
	//	}

	//	_text_entered(line_edit->get_text());
}

void LSpinBox::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_DRAW: {
			Ref<Texture> updown = get_icon("updown", "SpinBox");
			_adjust_width_for_icon(updown);

			RID ci = get_canvas_item();
			Size2i size = get_size();

			int icon_y = (size.height - updown->get_height()) / 2;
			for (int n = 0; n < _num_boxes; n++) {
				//updown->draw(ci, Point2i(size.width - updown->get_width(), (size.height - updown->get_height()) / 2));
				if (n == _num_boxes - 1) {
					updown->draw(ci, Point2i(_boxes[n].icon_x, icon_y));
				}
			}
			//updown->draw(ci, Point2i(5, 5));

			//draw_rect(Rect2(64, 0, 16, 16), Color(1, 0, 0, 1));
			//draw_texture_rect(updown, Rect2(0, 0, 16, 16));
		} break;

		default: {
		} break;
	}
}

void LSpinBox::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_gui_input"), &LSpinBox::_gui_input);
	ClassDB::bind_method(D_METHOD("_text_entered"), &LSpinBox::_text_entered);
	ClassDB::bind_method(D_METHOD("_line_edit_focus_enter"), &LSpinBox::_line_edit_focus_enter);
	ClassDB::bind_method(D_METHOD("_line_edit_focus_exit"), &LSpinBox::_line_edit_focus_exit);
	ClassDB::bind_method(D_METHOD("_line_edit_input"), &LSpinBox::_line_edit_input);

	ClassDB::bind_method(D_METHOD("set_value", "value"), &LSpinBox::set_value);
	ClassDB::bind_method(D_METHOD("get_value"), &LSpinBox::get_value);

	ClassDB::bind_method(D_METHOD("set_box_min_max", "box", "min", "max"), &LSpinBox::set_box_min_max);

	ClassDB::bind_method(D_METHOD("connect_signal", "name", "object", "func_name"), &LSpinBox::connect_signal);

	ADD_SIGNAL(MethodInfo("value_changed", PropertyInfo(Variant::INT, "value")));
}

void LSpinBox::set_box_min_max(uint32_t p_box_id, int32_t p_min, int32_t p_max) {
	Box &box = _boxes[p_box_id];
	box.min_value = p_min;
	box.max_value = p_max;

	// recalc multipliers
	int32_t multiplier = 1;
	for (int n = 0; n < _num_boxes; n++) {
		int box_id = _num_boxes - n - 1;
		Box &box = _boxes[box_id];

		box.multiplier = multiplier;
		multiplier *= box.max_value;
	}
}

void LSpinBox::set_step(int32_t p_step_a, int32_t p_step_b, int32_t p_step_c, int32_t p_step_d) {
	//Box &box = _boxes[p_box_id];
	_step[0] = p_step_a;
	_step[1] = p_step_b;
	_step[2] = p_step_c;
	_step[3] = p_step_d;
}

void LSpinBox::connect_signal(String p_signal_name, Variant p_object, String p_function_name) {
	connect(p_signal_name, p_object, p_function_name);
	//	for (int n=0; n<_num_boxes; n++)
	//	{
	//		_boxes[n].line_edit->connect(p_signal_name, p_object, p_function_name);
	//	}
}

void LSpinBox::set_num_boxes(uint32_t p_num_boxes) {
	p_num_boxes = MIN(p_num_boxes, MAX_BOXES);
	int diff = p_num_boxes - _num_boxes;

	if (diff > 0) {
		for (int n = 0; n < diff; n++) {
			LineEdit *le = memnew(LineEdit);
			add_child(le);
			_boxes[n + _num_boxes].line_edit = le;

			//le->set_anchors_and_margins_preset(Control::PRESET_WIDE);
			le->set_mouse_filter(MOUSE_FILTER_PASS);
			//connect("value_changed",this,"_value_changed");
			le->connect("text_entered", this, "_text_entered", Vector<Variant>(), CONNECT_DEFERRED);
			le->connect("focus_entered", this, "_line_edit_focus_enter", varray(n), CONNECT_DEFERRED);
			le->connect("focus_exited", this, "_line_edit_focus_exit", varray(n), CONNECT_DEFERRED);
			le->connect("gui_input", this, "_line_edit_input");
		}
	}

	if (diff < 0) {
		for (int n = 0; n < -diff; n++) {
			get_child(_num_boxes - n - 1)->queue_delete();
			_boxes[_num_boxes - n - 1].line_edit = nullptr;
		}
	}

	_num_boxes = p_num_boxes;

	_adjust_widths();
}

LSpinBox::LSpinBox() {
	//	set_size(Size2(64, 24));
	//	set_clip_contents(false);

	//	spin_box = memnew(SpinBox);
	//	add_child(spin_box);
	//	spin_box->set_position(Vector2(0, 32));

	set_num_boxes(4);
	set_box_min_max(0, 0, 4096);
	set_box_min_max(1, 0, 16);
	set_box_min_max(2, 0, 8);
	set_box_min_max(3, 0, 6);
}
