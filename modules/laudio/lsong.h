#pragma once

#include "lhandle.h"
#include "lpattern.h"
#include "lpattern_instance.h"
#include "lstructs.h"

class LSong {
	TrackedPooledList_Handled<LPattern> _patterns;
	TrackedPooledList_Handled<LPatternInstance> _pattern_instances;
	LTiming _timing;

	static LSong *_current_song;

public:
	static LSong *get_current() { return _current_song; }

	LSong();
	~LSong();
};

class Song : public Control {
	GDCLASS(Song, Control);
	LSong _song;

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void create_pattern_view();
};
