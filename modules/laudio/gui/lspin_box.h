#pragma once

#include "scene/gui/control.h"

class LineEdit;
class SpinBox;

class LSpinBox : public Control {
	GDCLASS(LSpinBox, Control);

	enum { MAX_BOXES = 4 };

	struct Box {
		LineEdit *line_edit = nullptr;
		int32_t max_value = 100;
		int32_t min_value = 0;
		int32_t icon_x = 0;
		int32_t value = 0;
		int32_t multiplier = 0;
		void set_value(int32_t p_value);
	};

	int32_t _value = 0;
	Box _boxes[MAX_BOXES];
	//SpinBox * spin_box = nullptr;
	int _last_w = 0;
	int _num_boxes = 0;
	int32_t _step[4] = { 6, 4, 12, 48 };

	void _adjust_width_for_icon(const Ref<Texture> &icon);
	void _adjust_widths();

	void _text_entered(const String &p_string);
	void _line_edit_input(const Ref<InputEvent> &p_event);
	void _line_edit_focus_enter(int p_which);
	void _line_edit_focus_exit(int p_which);

	void _recalc_value_from_boxes();
	void _store_value_in_boxes();
	//virtual void _value_changed(double);

	void set_value_with_signal(int32_t p_value);

protected:
	void _gui_input(const Ref<InputEvent> &p_event);
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_num_boxes(uint32_t p_num_boxes);
	void set_box_min_max(uint32_t p_box_id, int32_t p_min, int32_t p_max);
	void set_step(int32_t p_step_a, int32_t p_step_b, int32_t p_step_c, int32_t p_step_d);
	int32_t get_value() const { return _value; }
	void set_value(int32_t p_value);

	void connect_signal(String p_signal_name, Variant p_object, String p_function_name);

	LSpinBox();
};
