#pragma once

#include "scene/gui/button.h"
#include <stdint.h>

class LPattern;

class LPatternInstance {
	LPattern *_pattern = nullptr;

public:
	struct Data {
		uint32_t tick_start = 0;
		uint32_t tick_end = 0;
	} _data;
};

class Pattern : public Button {
	GDCLASS(Pattern, Button);

protected:
	void _notification(int p_what);
	static void _bind_methods();
};
