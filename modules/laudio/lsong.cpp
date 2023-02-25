#include "lsong.h"
#include "lmidi_file.h"
#include "pattern_view.h"

Song *Song::_current_song = nullptr;

LApp g_lapp;

LSong::LSong() {
}

LSong::~LSong() {
}

//////////////////////////////////

void Song::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			set_physics_process_internal(true);
		} break;
		case NOTIFICATION_EXIT_TREE: {
			set_physics_process_internal(false);
		} break;

		case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			if (_pattern_dirty) {
				_pattern_dirty = false;
				if (_selected_pattern) {
					_selected_pattern->refresh_position();
					_selected_pattern->refresh_text();
				}
			}
			if (_notes_dirty) {
				_notes_dirty = false;
				LPattern *p = get_pattern();
				if (p) {
					_current_note_id = p->sort_notes(_current_note_id);

					// refresh the GUI
					update_byteview();
				}
			}
		} break;
		default: {
		} break;
	}
}

void Song::_bind_methods() {
	ClassDB::bind_method(D_METHOD("pattern_create"), &Song::pattern_create);
	ClassDB::bind_method(D_METHOD("pattern_duplicate"), &Song::pattern_duplicate);
	ClassDB::bind_method(D_METHOD("pattern_delete"), &Song::pattern_delete);

	ClassDB::bind_method(D_METHOD("set_pattern_view", "pattern_view"), &Song::set_pattern_view);

#define PATTERNI_BIND(VAR_NAME)                                                                                                              \
	ClassDB::bind_method(D_METHOD("patterni_set_" LA_TOSTRING(VAR_NAME), LA_TOSTRING(VAR_NAME)), &Song::LA_LITCAT(patterni_set_, VAR_NAME)); \
	ClassDB::bind_method(D_METHOD("patterni_get_" LA_TOSTRING(VAR_NAME)), &Song::LA_LITCAT(patterni_get_, VAR_NAME));

#define PATTERN_BIND(VAR_NAME)                                                                                                             \
	ClassDB::bind_method(D_METHOD("pattern_set_" LA_TOSTRING(VAR_NAME), LA_TOSTRING(VAR_NAME)), &Song::LA_LITCAT(pattern_set_, VAR_NAME)); \
	ClassDB::bind_method(D_METHOD("pattern_get_" LA_TOSTRING(VAR_NAME)), &Song::LA_LITCAT(pattern_get_, VAR_NAME));

#define NOTE_BIND(VAR_NAME)                                                                                                          \
	ClassDB::bind_method(D_METHOD("note_set_" LA_TOSTRING(VAR_NAME), LA_TOSTRING(VAR_NAME)), &Song::LA_LITCAT(note_set_, VAR_NAME)); \
	ClassDB::bind_method(D_METHOD("note_get_" LA_TOSTRING(VAR_NAME)), &Song::LA_LITCAT(note_get_, VAR_NAME));

	PATTERNI_BIND(tick_start);
	PATTERNI_BIND(track);
	PATTERNI_BIND(transpose);

	PATTERN_BIND(name);
	PATTERN_BIND(tick_start);
	PATTERN_BIND(tick_length);
	PATTERN_BIND(player_a);
	PATTERN_BIND(player_b);
	PATTERN_BIND(player_c);
	PATTERN_BIND(player_d);
	PATTERN_BIND(transpose);
	PATTERN_BIND(quantize_a);
	PATTERN_BIND(quantize_b);

	NOTE_BIND(tick_start);
	NOTE_BIND(tick_length);
	NOTE_BIND(note);
	NOTE_BIND(velocity);
	NOTE_BIND(player);

	ClassDB::bind_method(D_METHOD("note_create"), &Song::note_create);
	ClassDB::bind_method(D_METHOD("note_delete"), &Song::note_delete);
	ClassDB::bind_method(D_METHOD("note_select", "note_id"), &Song::note_select);
	ClassDB::bind_method(D_METHOD("note_size"), &Song::note_size);
	ClassDB::bind_method(D_METHOD("note_get_selected"), &Song::note_get_selected);

	ClassDB::bind_method(D_METHOD("player_set_instrument", "player_id", "instrument"), &Song::player_set_instrument);
	ClassDB::bind_method(D_METHOD("player_get_instrument", "player_id"), &Song::player_get_instrument);
	ClassDB::bind_method(D_METHOD("player_clear", "player_id"), &Song::player_clear);
	ClassDB::bind_method(D_METHOD("player_get_name", "player_id"), &Song::player_get_name);

	ClassDB::bind_method(D_METHOD("track_set_name", "track_id", "name"), &Song::track_set_name);
	ClassDB::bind_method(D_METHOD("track_get_name", "track_id"), &Song::track_get_name);
	ClassDB::bind_method(D_METHOD("track_set_active", "track_id", "active"), &Song::track_set_active);
	ClassDB::bind_method(D_METHOD("track_get_active", "track_id"), &Song::track_get_active);

	ClassDB::bind_method(D_METHOD("song_import_midi", "filename"), &Song::song_import_midi);

	ADD_SIGNAL(MethodInfo("update_inspector"));
	ADD_SIGNAL(MethodInfo("update_byteview"));
	ADD_SIGNAL(MethodInfo("update_all"));
}

void Song::_log(String p_sz) {
	print_line(p_sz);
}

LPattern *Song::get_pattern() const {
	LPatternInstance *pi = get_patterni();
	if (pi) {
		return pi->get_pattern();
	}
	return nullptr;
}

LPatternInstance *Song::get_patterni() const {
	if (!_selected_pattern)
		return nullptr;

	return _selected_pattern->get_pattern_instance();
}

void Song::pattern_create() {
	_log("pattern_create");

	LHandle hpi;
	LPatternInstance *pi = g_lapp.pattern_instances.request(hpi);

	// create a pattern as well as this is an original
	LHandle hp;
	LPattern *lpat = g_lapp.patterns.request(hp);
	lpat->reference();

	lpat->data.handle = hp;

	pi->set_pattern(hp);

	// storing the handle on the instance itself is useful for self deletion etc
	//pi->set_handle(hpi);

	pi->data.tick_start = 64;

	Pattern *pat = memnew(Pattern);
	pat->set_pattern_instance(hpi);

	_pattern_view->add_child(pat);
	pat->set_owner(_pattern_view->get_owner());

	_select_pattern(pat);
}

void Song::pattern_select(Node *p_node) {
	_log("pattern_select");
	Pattern *pat = Object::cast_to<Pattern>(p_node);
	ERR_FAIL_COND(!pat);
	_select_pattern(pat);
}

void Song::_select_pattern(Pattern *p_pattern) {
	if (p_pattern == _selected_pattern)
		return;

	if (_selected_pattern) {
		_selected_pattern->set_selected(false);
	}

	_selected_pattern = p_pattern;

	if (_selected_pattern) {
		_selected_pattern->set_selected(true);
	}

	update_inspector();
	update_byteview();
}

void Song::update_inspector() {
	emit_signal("update_inspector");
}

void Song::update_byteview() {
	emit_signal("update_byteview");
}

void Song::update_all() {
	emit_signal("update_byteview");
}

void Song::pattern_duplicate() {
	_log("pattern_duplicate");
}

void Song::pattern_delete() {
	_log("pattern_delete");
	if (_selected_pattern) {
		_selected_pattern->pattern_delete();
		_select_pattern(nullptr);
	}
}

uint32_t Song::note_create() {
	_notes_dirty = true;
	LPattern *p = get_pattern();
	ERR_FAIL_NULL_V(p, 0);
	_current_note_id = p->create_note(_current_note_id);
	return _current_note_id;
}

void Song::note_delete() {
	_notes_dirty = true;
	LPattern *p = get_pattern();
	ERR_FAIL_NULL(p);
	ERR_FAIL_COND(_current_note_id >= p->notes.size());
	p->notes.remove(_current_note_id);
	if (_current_note_id) {
		_current_note_id--;
	}
}

uint32_t Song::note_get_selected() const {
	return _current_note_id;
}

void Song::note_select(uint32_t p_note_id) {
	LPattern *p = get_pattern();
	ERR_FAIL_NULL(p);
	ERR_FAIL_COND(p_note_id >= p->notes.size());
	if (p_note_id != _current_note_id) {
		_current_note_id = p_note_id;
		//print_line("note_select " + itos(p_note_id));
	}
}

uint32_t Song::note_size() {
	LPattern *p = get_pattern();
	ERR_FAIL_NULL_V(p, 0);
	return p->notes.size();
}

void Song::player_set_instrument(uint32_t p_player_id, Ref<LInstrument> p_instrument) {
	ERR_FAIL_COND(p_player_id >= LPlayers::MAX_PLAYERS);
	LPlayer &p = get_lsong()._players.get_player(p_player_id);
	p.set_instrument(p_instrument);
}

Ref<LInstrument> Song::player_get_instrument(uint32_t p_player_id) {
	ERR_FAIL_COND_V(p_player_id >= LPlayers::MAX_PLAYERS, Ref<LInstrument>());
	LPlayer &p = get_lsong()._players.get_player(p_player_id);
	return p.get_instrument_ref();
}

void Song::player_clear(uint32_t p_player_id) {
	ERR_FAIL_COND(p_player_id >= LPlayers::MAX_PLAYERS);
	LPlayer &p = get_lsong()._players.get_player(p_player_id);
	p.clear();
}

String Song::player_get_name(uint32_t p_player_id) {
	ERR_FAIL_COND_V(p_player_id >= LPlayers::MAX_PLAYERS, String());
	LPlayer &p = get_lsong()._players.get_player(p_player_id);
	return p.get_name();
}

void Song::set_pattern_view(Node *p_pattern_view) {
	_pattern_view = Object::cast_to<PatternView>(p_pattern_view);
	ERR_FAIL_NULL(_pattern_view);
}

void Song::track_set_name(uint32_t p_track, String p_name) {
	get_lsong()._tracks.tracks[p_track].name = p_name;
}

String Song::track_get_name(uint32_t p_track) const {
	return get_lsong()._tracks.tracks[p_track].name;
}

void Song::track_set_active(uint32_t p_track, bool p_active) {
	get_lsong()._tracks.tracks[p_track].active = p_active;
}

bool Song::track_get_active(uint32_t p_track) const {
	return get_lsong()._tracks.tracks[p_track].active;
}

bool Song::song_import_midi(String p_filename) {
	Error err;
	FileAccess *file = FileAccess::open(p_filename, FileAccess::READ, &err);
	if (!file) {
		return false;
	}

	Sound::LMIDIFile midi;
	bool result = midi.Load(file);

	memdelete(file);

	if (result) {
		for (uint32_t t = 0; t < midi.GetNumTracks(); t++) {
			const Sound::LMIDIFile::LTrack &track = midi.GetTrack(t);
			pattern_create();
			patterni_set_track(t);
			pattern_set_name(track._name);
			track_set_name(t, track._name);

			for (uint32_t n = 0; n < track.GetNumNotes(); n++) {
				//				if (n == 64)
				//					break;

				const Sound::LMIDIFile::LNote *note = track.GetNote(n);
				if (note->m_uiLength != UINT32_MAX) {
					uint32_t note_id = note_create();
					note_set_tick_start(note_id, note->m_uiTime);
					note_set_tick_length(note_id, note->m_uiLength);
					note_set_note(note_id, note->m_Key);
					note_set_velocity(note_id, note->m_Velocity);
				}
			}
		}
	}

	update_all();
	return result;
}

Song::Song() {
	_current_song = this;
	// create test song
	//	LHandle hpi = _song._pattern_instances.request();
	//	LPatternInstance * pi = _song._pattern_instances.get(hpi);
	//	pi->_data.tick_start = 64;
	//	pi->_data.tick_end = 192;

	//	create_pattern_view();
}

Song::~Song() {
	if (_current_song == this) {
		_current_song = nullptr;
	}
}
