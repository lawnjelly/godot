#include "lsong.h"
#include "gui/pattern_view.h"
#include "lbus.h"
#include "lson/lson.h"
#include "midi/lmidi_file.h"

Song *Song::_current_song = nullptr;

LApp g_lapp;

void LSong::calculate_song_length() {
	int32_t song_end = 0;
	for (uint32_t n = 0; n < g_lapp.pattern_instances.size(); n++) {
		const LPatternInstance &pi = g_lapp.pattern_instances.get_active(n);
		//if (_tracks.tracks[pi.data.track].active) {
		int32_t pattern_end = pi.get_tick_end();
		song_end = MAX(song_end, pattern_end);
		//}
	}

	print_line("Song is " + itos(song_end) + " ticks.");
	_timing.song_length_ticks = song_end;
}

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

	PATTERN_BIND(time_sig_micro);
	PATTERN_BIND(time_sig_minor);
	PATTERN_BIND(time_sig_major);

	ClassDB::bind_method(D_METHOD("pattern_calculate_length"), &Song::pattern_calculate_length);

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
	ClassDB::bind_method(D_METHOD("song_export_wav", "filename"), &Song::song_export_wav);

	ClassDB::bind_method(D_METHOD("instruments_load", "filename"), &Song::instruments_load);
	ClassDB::bind_method(D_METHOD("instruments_save", "filename"), &Song::instruments_save);

	ClassDB::bind_method(D_METHOD("transport_set_left_tick", "tick"), &Song::transport_set_left_tick);
	ClassDB::bind_method(D_METHOD("transport_set_right_tick", "tick"), &Song::transport_set_right_tick);
	ClassDB::bind_method(D_METHOD("transport_get_left_tick"), &Song::transport_get_left_tick);
	ClassDB::bind_method(D_METHOD("transport_get_right_tick"), &Song::transport_get_right_tick);

	ClassDB::bind_method(D_METHOD("song_load", "filename"), &Song::song_load);
	ClassDB::bind_method(D_METHOD("song_save", "filename"), &Song::song_save);
	ClassDB::bind_method(D_METHOD("song_clear"), &Song::song_clear);
	ClassDB::bind_method(D_METHOD("song_get_length"), &Song::song_get_length);
	ClassDB::bind_method(D_METHOD("song_get_samples_per_tick"), &Song::song_get_samples_per_tick);

	ADD_SIGNAL(MethodInfo("update_inspector"));
	ADD_SIGNAL(MethodInfo("update_byteview"));
	ADD_SIGNAL(MethodInfo("update_patternview"));
	ADD_SIGNAL(MethodInfo("update_trackview"));
	ADD_SIGNAL(MethodInfo("update_playerview"));
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

void Song::song_clear() {
	_select_pattern(nullptr);

	// delete gui patterns ..
	// this will automatically clear LPatternInstances and LPatterns
	int32_t num_children = _pattern_view->get_child_count();
	for (int32_t n = num_children - 1; n >= 0; n--) {
		Pattern *pat = Object::cast_to<Pattern>(_pattern_view->get_child(n));
		_pattern_delete_internal(pat);
	}

	//_song._players.clear_players();
	_song._timing.reset();
	_song._tracks.reset();
}

LPattern *Song::_create_lpattern(LHandle &r_handle) {
	LPattern *lpat = g_lapp.patterns.request(r_handle);
	lpat->reference();

	lpat->data.handle = r_handle;
	return lpat;
}

LPatternInstance *Song::_create_lpattern_instance_and_pattern(const LHandle &p_pattern_handle, bool p_select_pattern) {
	LHandle hpi;
	LPatternInstance *pi = g_lapp.pattern_instances.request(hpi);

	// add to the song patterns
	_song._pattern_instances.push_back(hpi);

	pi->set_pattern(p_pattern_handle);
	pi->data.tick_start = 64;

	Pattern *pat = memnew(Pattern);
	pat->set_pattern_instance(hpi);

	_pattern_view->add_child(pat);
	pat->set_owner(_pattern_view->get_owner());

	if (p_select_pattern)
		_select_pattern(pat);

	return pi;
}

void Song::pattern_create() {
	_log("pattern_create");

	// create a pattern as well as this is an original
	LHandle hp;
	_create_lpattern(hp);

	_create_lpattern_instance_and_pattern(hp, true);
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

	_current_note_id = 0;

	update_inspector();
	update_byteview();
}

void Song::pattern_calculate_length() {
	LPattern *pi = get_pattern();
	if (pi) {
		pi->calculate_length();
		_pattern_dirty = true;
	}
}

void Song::update_inspector() {
	emit_signal("update_inspector");
}

void Song::update_byteview() {
	emit_signal("update_byteview");
}

void Song::update_patternview() {
	emit_signal("update_patternview");
}

void Song::update_trackview() {
	emit_signal("update_trackview");
}

void Song::update_playerview() {
	emit_signal("update_playerview");
}

void Song::update_all() {
	emit_signal("update_all");
}

void Song::pattern_duplicate() {
	_log("pattern_duplicate");
}

void Song::_pattern_delete_internal(Pattern *p_pattern) {
	// remove from song pattern instances
	LHandle handle = p_pattern->get_pattern_instance_handle();
	if (handle.is_valid()) {
		int64_t found = _song._pattern_instances.find(handle);
		DEV_ASSERT(found != -1);
		if (found == -1) {
			ERR_PRINT("Pattern instance not found in song.");
		} else {
			_song._pattern_instances.remove_unordered(found);
		}
	}

	p_pattern->pattern_delete();
}

void Song::pattern_delete() {
	_log("pattern_delete");
	if (_selected_pattern) {
		_pattern_delete_internal(_selected_pattern);
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
	if (!p)
		return 0;
	return p->notes.size();
}

void Song::player_set_instrument(uint32_t p_player_id, Ref<LInstrument> p_instrument) {
	ERR_FAIL_COND(p_player_id >= LPlayers::MAX_PLAYERS);
	//LPlayer &p = get_lsong()._players.get_player(p_player_id);
	//p.set_instrument(p_instrument);
	get_lsong()._players.set_player_instrument(p_player_id, p_instrument);
}

Ref<LInstrument> Song::player_get_instrument(uint32_t p_player_id) {
	ERR_FAIL_COND_V(p_player_id >= LPlayers::MAX_PLAYERS, Ref<LInstrument>());
	return get_lsong()._players.get_player_instrument_ref(p_player_id);
	//	LPlayer &p = get_lsong()._players.get_player(p_player_id);
	//	return p.get_instrument_ref();
}

void Song::player_clear(uint32_t p_player_id) {
	ERR_FAIL_COND(p_player_id >= LPlayers::MAX_PLAYERS);
	get_lsong()._players.clear_player_instrument(p_player_id);
}

String Song::player_get_name(uint32_t p_player_id) {
	ERR_FAIL_COND_V(p_player_id >= LPlayers::MAX_PLAYERS, String());
	return get_lsong()._players.get_player_name(p_player_id);
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

//uint32_t Song::transport_get_cursor() const
//{
//	return 0;
//}

//void Song::transport_set_cursor(uint32_t p_pos)
//{
//}

void Song::transport_set_left_tick(uint32_t p_tick) {
	get_lsong()._timing.transport_tick_left = p_tick;
}

void Song::transport_set_right_tick(uint32_t p_tick) {
	get_lsong()._timing.transport_tick_right = p_tick;
}

uint32_t Song::transport_get_left_tick() const {
	return get_lsong()._timing.transport_tick_left;
}

uint32_t Song::transport_get_right_tick() const {
	return get_lsong()._timing.transport_tick_right;
}

bool Song::instruments_load(String p_filename) {
	bool res = get_lsong()._players.load(p_filename);
	update_all();
	return res;
}

bool Song::instruments_save(String p_filename) {
	return get_lsong()._players.save(p_filename);
}

bool Song::song_load(String p_filename) {
	song_clear();

	LSon::Node node_root;
	if (!node_root.load(p_filename))
		return false;

	LSon::Node *node_timing = node_root.find_node("timing");
	if (node_timing) {
		if (!_song._timing.load(node_timing))
			return false;
	}

	LSon::Node *node_patterns = node_root.find_node("patterns");
	ERR_FAIL_NULL_V(node_patterns, false);

	LocalVector<LHandle> pattern_handles;

	for (uint32_t n = 0; n < node_patterns->children.size(); n++) {
		LSon::Node *node_pattern = node_patterns->get_child(n);
		ERR_FAIL_COND_V(node_pattern->name != "pattern", false);

		LHandle hpat;
		LPattern *pat = _create_lpattern(hpat);

		if (!pat->load(node_pattern))
			return false;

		pattern_handles.push_back(hpat);
	}

	LSon::Node *node_pattern_instances = node_root.find_node("pattern_instances");
	ERR_FAIL_NULL_V(node_pattern_instances, false);

	for (uint32_t n = 0; n < node_pattern_instances->children.size(); n++) {
		LSon::Node *node_pattern_instance = node_pattern_instances->get_child(n);
		ERR_FAIL_COND_V(node_pattern_instance->name != "pattern_instance", false);

		LSon::Node *child = node_pattern_instance->get_child(0);
		ERR_FAIL_COND_V(child->name != "pattern_id", false);

		uint32_t pattern_id;
		if (!child->get_s64(pattern_id))
			return false;

		ERR_FAIL_COND_V(pattern_id >= pattern_handles.size(), false);
		LPatternInstance *pi = _create_lpattern_instance_and_pattern(pattern_handles[pattern_id], false);

		if (!pi->load(node_pattern_instance))
			return false;
	}

	LSon::Node *node_tracks = node_root.find_node("tracks");
	ERR_FAIL_NULL_V(node_tracks, false);

	for (uint32_t n = 0; n < node_tracks->children.size(); n++) {
		LSon::Node *node_track = node_tracks->get_child(n);
		ERR_FAIL_COND_V(node_track->name != "track", false);

		LSon::Node *child = node_track->get_child(0);
		ERR_FAIL_COND_V(child->name != "active", false);

		ERR_FAIL_COND_V(n > LTracks::MAX_TRACKS, false);
		LTrack &track = get_lsong()._tracks.tracks[n];
		if (!child->get_bool(track.active))
			return false;
	}

	get_lsong().calculate_song_length();
	transport_set_left_tick(0);
	transport_set_right_tick(get_lsong()._timing.song_length_ticks);

	update_all();
	return true;
}

bool Song::song_save(String p_filename) {
	bool result = true;

	LSon::Node node_root;
	node_root.set_name("song");

	// save timing
	_song._timing.save(&node_root);

	// FIRST WE NEED A MAPPING OF PATTERN INSTANCES TO PATTERNS
	LocalVector<LHandle> pattern_handles;

	for (uint32_t n = 0; n < _song._pattern_instances.size(); n++) {
		LHandle hpi = _song._pattern_instances[n];
		LPatternInstance *pi = g_lapp.pattern_instances.get(hpi);

		LHandle hpat = pi->get_pattern_handle();
		LPattern *lpat = g_lapp.patterns.get(hpat);

		DEV_ASSERT(pi);
		DEV_ASSERT(lpat);

		// pattern already used?
		int64_t found = -1;
		if (lpat->get_refcount() > 1) {
			found = pattern_handles.find(hpat);
		}
		if (found != -1) {
			lpat->data.save_id = found;
		} else {
			// first occurrence of pattern
			lpat->data.save_id = pattern_handles.size();
			pattern_handles.push_back(hpat);
		}
	}

	// save each pattern
	LSon::Node *node_patterns = node_root.request_child();
	node_patterns->set_name("patterns");
	node_patterns->set_array();
	for (uint32_t n = 0; n < pattern_handles.size(); n++) {
		if (!_save_pattern(node_patterns, n, pattern_handles[n]))
			result = false;
	}

	// save each pattern instance
	LSon::Node *node_pattern_instances = node_root.request_child();
	node_pattern_instances->set_name("pattern_instances");
	node_pattern_instances->set_array();

	for (uint32_t n = 0; n < _song._pattern_instances.size(); n++) {
		LHandle hpi = _song._pattern_instances[n];
		//LPatternInstance *pi = g_lapp.pattern_instances.get(hpi);
		if (!_save_pattern_instance(node_pattern_instances, n, hpi))
			result = false;
	}

	// tracks
	LSon::Node *node_tracks = node_root.request_child();
	node_tracks->set_name("tracks");
	node_tracks->set_array();
	for (uint32_t n = 0; n < LTracks::MAX_TRACKS; n++) {
		if (!_save_track(node_tracks, n))
			result = false;
	}

	if (!node_root.save(p_filename))
		result = false;

	return result;
}

bool Song::_save_pattern(LSon::Node *p_node_patterns, uint32_t p_pattern_id, LHandle p_handle) {
	LSon::Node *node = p_node_patterns->request_child_s64("pattern", p_pattern_id);

	LPattern *lpat = g_lapp.patterns.get(p_handle);
	if (!lpat)
		return false;

	node->request_child_string("name", lpat->data.name);
	node->request_child_s64("tick_start", lpat->data.tick_start);
	node->request_child_s64("tick_length", lpat->data.tick_length);

	node->request_child_s64("time_sig_micro", lpat->data.time_sig_micro);
	node->request_child_s64("time_sig_minor", lpat->data.time_sig_minor);
	node->request_child_s64("time_sig_major", lpat->data.time_sig_major);

	if (lpat->data.transpose != 0)
		node->request_child_s64("transpose", lpat->data.transpose);

	if (lpat->data.player_a != 0)
		node->request_child_s64("player_a", lpat->data.player_a);
	if (lpat->data.player_b != 0)
		node->request_child_s64("player_b", lpat->data.player_b);
	if (lpat->data.player_c != 0)
		node->request_child_s64("player_c", lpat->data.player_c);
	if (lpat->data.player_d != 0)
		node->request_child_s64("player_d", lpat->data.player_d);

	LSon::Node *node_notes = node->request_child_s64("notes", lpat->notes.size());
	node_notes->set_array();

	for (uint32_t n = 0; n < lpat->notes.size(); n++) {
		LNote *note = lpat->get_note(n);

		LSon::Node *node_note = node_notes->request_child_s64("note", note->note);
		node_note->request_child_s64("start", note->tick_start);
		node_note->request_child_s64("length", note->tick_length);
		node_note->request_child_s64("vel", note->velocity);
		if (note->player != 0)
			node_note->request_child_s64("player", note->player);
	}

	return true;
}

bool Song::_save_track(LSon::Node *p_node_tracks, uint32_t p_track_id) {
	LSon::Node *node = p_node_tracks->request_child_s64("track", p_track_id);
	const LTrack &track = get_lsong()._tracks.tracks[p_track_id];

	node->request_child_bool("active", track.active);

	return true;
}

bool Song::_save_pattern_instance(LSon::Node *p_node_pattern_instances, uint32_t p_pattern_instance_id, LHandle p_handle) {
	LSon::Node *node = p_node_pattern_instances->request_child_s64("pattern_instance", p_pattern_instance_id);

	LPatternInstance *pi = g_lapp.pattern_instances.get(p_handle);
	if (!pi)
		return false;

	LPattern *pat = pi->get_pattern();
	if (!pat)
		return false;

	node->request_child_s64("pattern_id", pat->data.save_id);
	node->request_child_s64("tick_start", pi->data.tick_start);
	node->request_child_s64("track", pi->data.track);

	if (pi->data.transpose != 0)
		node->request_child_s64("transpose", pi->data.transpose);

	return true;
}

uint32_t Song::song_get_samples_per_tick() const {
	return get_lsong()._timing.samples_pt;
}

bool Song::song_import_midi(String p_filename) {
	Error err;
	FileAccess *file = FileAccess::open(p_filename, FileAccess::READ, &err);
	if (!file) {
		return false;
	}

	song_clear();

	Sound::LMIDIFile midi;
	bool result = midi.Load(file);

	memdelete(file);

	if (result) {
		uint32_t division = midi.GetDivision();

		print_line("MIDI file division " + itos(division));

		// first calculate the lowest common multiple, to simplify the timings
		for (uint32_t t = 0; t < midi.GetNumTracks(); t++) {
			const Sound::LMIDIFile::LTrack &track = midi.GetTrack(t);
			for (uint32_t n = 0; n < track.GetNumNotes(); n++) {
				const Sound::LMIDIFile::LNote *note = track.GetNote(n);
				division = note->calculate_lowest_tick_multiple(division);
			}
		}

		print_line("Lowest common multiple division " + itos(division));

		for (uint32_t t = 0; t < midi.GetNumTracks(); t++) {
			const Sound::LMIDIFile::LTrack &track = midi.GetTrack(t);
			pattern_create();
			patterni_set_track(t);
			if (track._name.length()) {
				pattern_set_name(track._name);
				track_set_name(t, track._name);
			}
			int32_t track_start, track_end;
			track_start = track.find_track_start_and_end(track_end);
			patterni_set_tick_start(track_start / division);
			int32_t track_length = track_end - track_start;

			pattern_set_tick_length(MIN(track_length / division, 24));

			for (uint32_t n = 0; n < track.GetNumNotes(); n++) {
				//				if (n == 64)
				//					break;

				const Sound::LMIDIFile::LNote *note = track.GetNote(n);
				if (note->m_uiLength != UINT32_MAX) {
					uint32_t note_id = note_create();

					int32_t start = note->m_uiTime - track_start;
					start /= division;
					note_set_tick_start(note_id, start);

					int32_t length = note->m_uiLength;
					length /= division;

					note_set_tick_length(note_id, length);
					note_set_note(note_id, note->m_Key);
					note_set_velocity(note_id, note->m_Velocity);
				}
			}
			pattern_calculate_length();
		}
	}

	get_lsong().calculate_song_length();
	transport_set_left_tick(0);
	transport_set_right_tick(get_lsong()._timing.song_length_ticks);

	update_all();
	return result;
}

uint32_t Song::song_get_length() const {
	return get_lsong()._timing.song_length_ticks;
}

bool Song::song_play(LBus &r_output_bus, int32_t p_song_sample_from, int32_t p_num_samples) {
	uint32_t samples_per_tick = get_lsong()._timing.samples_pt;

	for (uint32_t n = 0; n < g_lapp.pattern_instances.size(); n++) {
		const LPatternInstance &pi = g_lapp.pattern_instances.get_active(n);

		if (get_lsong()._tracks.tracks[pi.data.track].active)
			pi.play(get_lsong(), r_output_bus.get_handle(), p_song_sample_from, p_num_samples, samples_per_tick);
	}

	return true;
}

bool Song::song_export_wav(String p_filename) {
	// calculate song length
	//	for (uint32_t n=0; n<LTracks::MAX_TRACKS; n++)
	//	{

	//	}
	//	get_lsong()._tracks.

	//	LPatternInstance *pi = g_lapp.pattern_instances.request(hpi);
	int32_t song_end = 0;
	for (uint32_t n = 0; n < g_lapp.pattern_instances.size(); n++) {
		const LPatternInstance &pi = g_lapp.pattern_instances.get_active(n);
		if (get_lsong()._tracks.tracks[pi.data.track].active) {
			int32_t pattern_end = pi.get_tick_end();
			song_end = MAX(song_end, pattern_end);
		}
	}

	// for testing limit song length
	//	song_end = MIN(song_end, 192);

	// calculate in samples
	uint32_t samples_per_tick = get_lsong()._timing.samples_pt;
	uint32_t total_samples = samples_per_tick * song_end;
	print_line("total song ticks: " + itos(song_end) + ", total song samples:" + itos(total_samples));

	LBus output_bus;
	output_bus.resize(total_samples);
	LBus *test = g_Buses.get_bus(output_bus.get_handle());
	DEV_ASSERT(test == &output_bus);

	for (uint32_t n = 0; n < g_lapp.pattern_instances.size(); n++) {
		const LPatternInstance &pi = g_lapp.pattern_instances.get_active(n);
		if (get_lsong()._tracks.tracks[pi.data.track].active) {
			pi.play(get_lsong(), output_bus.get_handle(), 0, total_samples, samples_per_tick);
		}
	}

	/*
		// test
		LInstrument * inst = get_lsong()._players.get_player_instrument(2);
		inst->set_output_bus(output_bus.get_handle());
		for (int i=0; i<10; i++)
		{
			int32_t slice = 4410;
			output_bus.set_offset(i*slice);

			inst->play(60, i * slice, slice, 0, 44100);
		}
		*/

	output_bus.get_sample().normalize();
	return output_bus.save(p_filename + ".wav");
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
