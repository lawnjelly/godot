#pragma once

#include "../lstructs.h"
#include "scene/gui/button.h"

class Pattern;

class PatternView : public Control {
	GDCLASS(PatternView, Control);

	Pattern *_selected_pattern = nullptr;
	//	void _log(String p_sz);

	//	LPattern *get_pattern() const;
	//	LPatternInstance *get_patterni() const;
	bool _pattern_dirty = false;

protected:
	void _notification(int p_what);
	static void _bind_methods();

	//	void _select_pattern(Pattern *p_pattern);

public:
	//	void create_pattern_view();

	//	PatternView();
	//	virtual ~PatternView();
};
