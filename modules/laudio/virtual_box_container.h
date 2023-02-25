#pragma once

#include "scene/gui/control.h"

class VirtualBoxContainer : public Control {
	GDCLASS(VirtualBoxContainer, Control);

	struct Data {
		uint32_t line_size = 0;
		uint32_t num_lines = 0;
		uint32_t total_height = 0;

		uint32_t num_display_lines = 0;
		int32_t prev_first_line_id = -1;

		Ref<PackedScene> line_scene;

		void calc_total_height() {
			total_height = line_size * num_lines;
		}
	} data;

	void _on_resize();
	void _place_display_lines(bool p_force = false);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_line_scene(const Ref<PackedScene> &p_scene);
	Ref<PackedScene> get_line_scene() const;

	void set_line_size(uint32_t p_size);
	void set_num_lines(uint32_t p_num_lines);
	void update_bounds();

	// returns whether was on screen
	bool refresh_line(uint32_t p_line_id);
	void refresh_all();

	VirtualBoxContainer(bool p_vertical = false);
};

class VirtualVBoxContainer : public VirtualBoxContainer {
	GDCLASS(VirtualVBoxContainer, VirtualBoxContainer);

public:
	VirtualVBoxContainer() :
			VirtualBoxContainer(false) {}
};
