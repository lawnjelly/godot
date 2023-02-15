#pragma once

#include "lhandle.h"
#include "lpattern.h"
#include "lpattern_instance.h"
#include "lstructs.h"

#define LA_STRINGIFY(A) #A
#define LA_TOSTRING(A) LA_STRINGIFY(A)

#define LA_LITCAT_NX(A, B) A##B
#define LA_LITCAT(A, B) LA_LITCAT_NX(A, B)

#define PATTERNI_GET_SET(VAR_NAME, TYPE, FAIL_VALUE)         \
	void LA_LITCAT(patterni_set_, VAR_NAME)(TYPE VAR_NAME) { \
		LPatternInstance *pi = get_patterni();               \
		if (pi) {                                            \
			pi->data.VAR_NAME = VAR_NAME;                    \
			_pattern_dirty = true;                           \
		}                                                    \
	}                                                        \
	TYPE LA_LITCAT(patterni_get_, VAR_NAME)() const {        \
		LPatternInstance *pi = get_patterni();               \
		if (pi) {                                            \
			return pi->data.VAR_NAME;                        \
		}                                                    \
		return FAIL_VALUE;                                   \
	}

#define PATTERN_GET_SET(VAR_NAME, TYPE, FAIL_VALUE)         \
	void LA_LITCAT(pattern_set_, VAR_NAME)(TYPE VAR_NAME) { \
		LPattern *pi = get_pattern();                       \
		if (pi) {                                           \
			pi->data.VAR_NAME = VAR_NAME;                   \
			_pattern_dirty = true;                          \
		}                                                   \
	}                                                       \
	TYPE LA_LITCAT(pattern_get_, VAR_NAME)() const {        \
		LPattern *pi = get_pattern();                       \
		if (pi) {                                           \
			return pi->data.VAR_NAME;                       \
		}                                                   \
		return FAIL_VALUE;                                  \
	}

class LApp {
public:
	HandledPool<LPattern> patterns;
	HandledPool<LPatternInstance> pattern_instances;
};

extern LApp g_lapp;

class LSong {
public:
	//	HandledPool<LPattern> _patterns;
	//	HandledPool<LPatternInstance> _pattern_instances;
	LTiming _timing;

	LSong();
	~LSong();
};

class Song : public Control {
	GDCLASS(Song, Control);
	LSong _song;

	Pattern *_selected_pattern = nullptr;
	void _log(String p_sz);

	static Song *_current_song;

	LPattern *get_pattern() const;
	LPatternInstance *get_patterni() const;
	bool _pattern_dirty = false;

protected:
	void _notification(int p_what);
	static void _bind_methods();

	void _select_pattern(Pattern *p_pattern);

public:
	static Song *get_current_song() { return _current_song; }
	LSong &get_lsong() { return _song; }

	void create_pattern_view();

	void pattern_create();
	void pattern_duplicate();
	void pattern_delete();
	void pattern_select(Node *p_node);

	PATTERNI_GET_SET(tick_start, int32_t, 0)

	PATTERN_GET_SET(name, String, "")
	PATTERN_GET_SET(tick_start, int32_t, 0)
	PATTERN_GET_SET(tick_length, int32_t, 0)

	Song();
	virtual ~Song();
};
