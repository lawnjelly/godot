#pragma once

#include "lstructs.h"
#include "scene/gui/button.h"
#include <stdint.h>

class LPattern;
class LSong;

class LPatternInstance {
	LHandle _handle_pattern;

public:
	struct Data {
		int32_t tick_start = 0;
		int32_t track = 0;
		int32_t transpose = 0;
		bool snippet = false;
	} data;

	String get_name() const;
	LPattern *get_pattern() const;
	LHandle get_pattern_handle() const { return _handle_pattern; }
	void set_pattern(LHandle p_handle) { _handle_pattern = p_handle; }
	int32_t get_tick_length() const {
		LPattern *p = get_pattern();
		return p ? p->data.tick_length : 128;
	}
	int32_t get_tick_end() const { return data.tick_start + get_tick_length(); }
	bool play(LSong &p_song, uint32_t p_output_bus_handle, uint32_t p_song_sample_from, uint32_t p_num_samples, uint32_t p_samples_per_tick) const;

	bool load(LSon::Node *p_data);

	~LPatternInstance();
};

class Song;

class Pattern : public Control {
	GDCLASS(Pattern, Control);

	struct Data {
		bool selected = false;
		String text;
		LHandle handle_pattern_instance;
		//int32_t zoom = 0;
		//Song *owner_song = nullptr;
		bool snippet = false;
	} data;

	struct DragData {
		bool dragging = false;
		Vector2 drag_origin;
		int32_t orig_tick_start = 0;
		int32_t orig_track = 0;
		int32_t relative_drag_x = 0;
		int32_t relative_drag_y = 0;
	};
	static DragData drag_data;

	void _pattern_pressed();
	int32_t _apply_zoom(int32_t p_value, int32_t p_zoom_multiply = 1) const;

protected:
	void _notification(int p_what);
	static void _bind_methods();

	void _draw();
	void set_text(String p_text);

	virtual void _gui_input(const Ref<InputEvent> &p_event);

public:
	void set_pattern_instance(LHandle p_handle);
	LPatternInstance *get_pattern_instance() const;
	LHandle get_pattern_instance_handle() const { return data.handle_pattern_instance; }
	void pattern_delete();
	void refresh_position();
	void refresh_text();
	void set_selected(bool p_selected);
	//void set_owner_song(Song *p_song) { data.owner_song = p_song; }
	void set_snippet(bool p_snippet);
	bool get_snippet() const { return data.snippet; }
	//	void set_zoom(int32_t p_zoom);

	Pattern();
};
