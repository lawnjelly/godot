#pragma once

#include "../lstructs.h"
#include "scene/gui/control.h"

class Pattern;

class PatternView : public Control {
	GDCLASS(PatternView, Control);

	Pattern *_selected_pattern = nullptr;
	//	void _log(String p_sz);

	bool _pattern_dirty = false;

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	virtual bool clips_input() const { return true; }
	//	void set_zoom(int32_t p_zoom);
	//	int32_t get_zoom() const { return _zoom; }
	void refresh_all_patterns();
};
