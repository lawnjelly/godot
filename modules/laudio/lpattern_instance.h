#pragma once

#include "lstructs.h"
#include "scene/gui/button.h"
#include <stdint.h>

class LPattern;

class LPatternInstance {
	LHandle _handle_pattern;

public:
	struct Data {
		int32_t tick_start = 0;
	} data;

	String get_name() const;
	LPattern *get_pattern() const;
	void set_pattern(LHandle p_handle) { _handle_pattern = p_handle; }
	int32_t get_tick_length() const {
		LPattern *p = get_pattern();
		return p ? p->data.tick_length : 128;
	}
	int32_t get_tick_end() const { return data.tick_start + get_tick_length(); }

	~LPatternInstance();
};

class Pattern : public Control {
	GDCLASS(Pattern, Control);

	struct Data {
		bool selected = false;
		String text;
		LHandle handle_pattern_instance;
	} data;

	struct DragData {
		bool dragging = false;
		Vector2 drag_origin;
		int32_t orig_tick_start = 0;
		int32_t relative_drag = 0;
	};
	static DragData drag_data;

	void _pattern_pressed();

protected:
	void _notification(int p_what);
	static void _bind_methods();

	void _draw();
	void set_text(String p_text);

	virtual void _gui_input(const Ref<InputEvent> &p_event);

public:
	void set_pattern_instance(LHandle p_handle);
	LPatternInstance *get_pattern_instance() const;
	void pattern_delete();
	void refresh_position();
	void refresh_text();
	void set_selected(bool p_selected);

	Pattern();
};
