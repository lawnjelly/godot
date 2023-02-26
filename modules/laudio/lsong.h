#pragma once

#include "lhandle.h"
#include "lpattern.h"
#include "lpattern_instance.h"
#include "lstructs.h"
#include "player/lplayer.h"

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

#define PATTERN_GET_SET(VAR_NAME, TYPE, FAIL_VALUE, DIRTY_NOTES) \
	void LA_LITCAT(pattern_set_, VAR_NAME)(TYPE VAR_NAME) {      \
		LPattern *pi = get_pattern();                            \
		if (pi) {                                                \
			pi->data.VAR_NAME = VAR_NAME;                        \
			_pattern_dirty = true;                               \
			if (DIRTY_NOTES) {                                   \
				_notes_dirty = true;                             \
			}                                                    \
		}                                                        \
	}                                                            \
	TYPE LA_LITCAT(pattern_get_, VAR_NAME)() const {             \
		LPattern *pi = get_pattern();                            \
		if (pi) {                                                \
			return pi->data.VAR_NAME;                            \
		}                                                        \
		return FAIL_VALUE;                                       \
	}

#define NOTE_GET_SET(VAR_NAME, TYPE, FAIL_VALUE)                             \
	void LA_LITCAT(note_set_, VAR_NAME)(uint32_t p_note_id, TYPE VAR_NAME) { \
		LPattern *pi = get_pattern();                                        \
		if (pi) {                                                            \
			LNote *_note = pi->get_note(p_note_id);                          \
			if (_note) {                                                     \
				_note->VAR_NAME = VAR_NAME;                                  \
			}                                                                \
			_notes_dirty = true;                                             \
		}                                                                    \
	}                                                                        \
	TYPE LA_LITCAT(note_get_, VAR_NAME)(uint32_t p_note_id) const {          \
		LPattern *pi = get_pattern();                                        \
		if (pi) {                                                            \
			LNote *_note = pi->get_note(p_note_id);                          \
			if (_note) {                                                     \
				return _note->VAR_NAME;                                      \
			}                                                                \
		}                                                                    \
		return FAIL_VALUE;                                                   \
	}

class LApp {
public:
	HandledPool<LPattern> patterns;
	HandledPool<LPatternInstance> pattern_instances;
};

extern LApp g_lapp;

class PatternView;
class LInstrument;

class LSong {
public:
	//	HandledPool<LPattern> _patterns;
	//	HandledPool<LPatternInstance> _pattern_instances;
	LTiming _timing;
	LPlayers _players;
	LTracks _tracks;

	LSong();
	~LSong();
};

class Song : public Node {
	GDCLASS(Song, Node);
	LSong _song;

	Pattern *_selected_pattern = nullptr;
	PatternView *_pattern_view = nullptr;
	uint32_t _current_note_id = 0;

	void _log(String p_sz);

	static Song *_current_song;

	LPattern *get_pattern() const;
	LPatternInstance *get_patterni() const;
	bool _pattern_dirty = false;
	bool _notes_dirty = false;

protected:
	void _notification(int p_what);
	static void _bind_methods();

	void _select_pattern(Pattern *p_pattern);

public:
	static Song *get_current_song() { return _current_song; }
	LSong &get_lsong() { return _song; }
	const LSong &get_lsong() const { return _song; }

	void set_pattern_view(Node *p_pattern_view);

	void pattern_create();
	void pattern_duplicate();
	void pattern_delete();
	void pattern_select(Node *p_node);

	void update_inspector();
	void update_byteview();
	void update_patternview();
	void update_trackview();
	void update_playerview();
	void update_all();

	PATTERNI_GET_SET(tick_start, int32_t, 0)
	PATTERNI_GET_SET(track, int32_t, 0)
	PATTERNI_GET_SET(transpose, int32_t, 0)

	PATTERN_GET_SET(name, String, "", false)
	PATTERN_GET_SET(tick_start, int32_t, 0, false)
	PATTERN_GET_SET(tick_length, int32_t, 0, false)

	PATTERN_GET_SET(player_a, int32_t, 0, false)
	PATTERN_GET_SET(player_b, int32_t, 0, false)
	PATTERN_GET_SET(player_c, int32_t, 0, false)
	PATTERN_GET_SET(player_d, int32_t, 0, false)

	PATTERN_GET_SET(transpose, int32_t, 0, false)

	PATTERN_GET_SET(time_sig_micro, int32_t, 0, true)
	PATTERN_GET_SET(time_sig_minor, int32_t, 0, true)
	PATTERN_GET_SET(time_sig_major, int32_t, 0, true)

	NOTE_GET_SET(tick_start, int32_t, 0)
	NOTE_GET_SET(tick_length, int32_t, 0)
	NOTE_GET_SET(note, int32_t, 60)
	NOTE_GET_SET(velocity, int32_t, 100)
	NOTE_GET_SET(player, int32_t, 0)

	uint32_t note_create();
	void note_delete();
	void note_select(uint32_t p_note_id);
	uint32_t note_size();
	uint32_t note_get_selected() const;

	void player_set_instrument(uint32_t p_player_id, Ref<LInstrument> p_instrument);
	Ref<LInstrument> player_get_instrument(uint32_t p_player_id);
	void player_clear(uint32_t p_player_id);
	String player_get_name(uint32_t p_player_id);

	void track_set_name(uint32_t p_track, String p_name);
	String track_get_name(uint32_t p_track) const;
	void track_set_active(uint32_t p_track, bool p_active);
	bool track_get_active(uint32_t p_track) const;

	bool song_import_midi(String p_filename);

	Song();
	virtual ~Song();
};
