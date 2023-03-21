#pragma once

#include "../lstructs.h"
//#include "scene/gui/button.h"
#include "scene/gui/control.h"
//#include "scene/gui/scroll_container.h"

class Pattern;

class PatternView : public Control {
	GDCLASS(PatternView, Control);
	//class PatternView : public ScrollContainer {
	//	GDCLASS(PatternView, ScrollContainer);

	Pattern *_selected_pattern = nullptr;
	//	void _log(String p_sz);

	//	LPattern *get_pattern() const;
	//	LPatternInstance *get_patterni() const;
	bool _pattern_dirty = false;
	int32_t _zoom = 0;

protected:
	void _notification(int p_what);
	static void _bind_methods();

	//	void _select_pattern(Pattern *p_pattern);

public:
	virtual bool clips_input() const { return true; }
	//	void create_pattern_view();
	void set_zoom(int32_t p_zoom);
	int32_t get_zoom() const { return _zoom; }
	void refresh_zoom();

	//	PatternView();
	//	virtual ~PatternView();
};
