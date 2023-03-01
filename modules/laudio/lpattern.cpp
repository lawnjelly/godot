#include "lpattern.h"
#include "lsong.h"

void LPattern::calculate_length() {
	if (!notes.size()) {
		data.reset();
		return;
	}

	data.tick_start = INT32_MAX;
	int32_t tick_end = INT32_MIN;
	data.tick_length = 0;

	for (uint32_t n = 0; n < notes.size(); n++) {
		const LNote &note = notes[n];
		int32_t start = note.tick_start;
		int32_t end = start + note.tick_length;
		data.tick_start = MIN(data.tick_start, start);
		tick_end = MAX(tick_end, end);
	}

	if (tick_end > data.tick_start) {
		data.tick_length = tick_end - data.tick_start;
	}
}

bool LPattern::load_notes(LSon::Node *p_data) {
	for (uint32_t c = 0; c < p_data->children.size(); c++) {
		LSon::Node *child_note = p_data->get_child(c);
		if (child_note->name != "note")
			return false;

		notes.resize(notes.size() + 1);
		LNote *note = get_note(notes.size() - 1);

		if (!child_note->get_s64(note->note))
			return false;

		for (uint32_t n = 0; n < child_note->children.size(); n++) {
			LSon::Node *child = child_note->get_child(n);
			if (child->name == "start") {
				if (!child->get_s64(note->tick_start))
					return false;
			}
			if (child->name == "length") {
				if (!child->get_s64(note->tick_length))
					return false;
			}
			if (child->name == "vel") {
				if (!child->get_s64(note->velocity))
					return false;
			}
			if (child->name == "player") {
				if (!child->get_s64(note->player))
					return false;
			}
		}
	}

	return true;
}

bool LPattern::load(LSon::Node *p_data) {
	for (uint32_t c = 0; c < p_data->children.size(); c++) {
		LSon::Node *child = p_data->get_child(c);

		if (child->name == "name") {
			if (!child->get_string(data.name))
				return false;
		}
		if (child->name == "tick_start") {
			if (!child->get_s64(data.tick_start))
				return false;
		}
		if (child->name == "tick_length") {
			if (!child->get_s64(data.tick_length))
				return false;
		}
		if (child->name == "time_sig_micro") {
			if (!child->get_s64(data.time_sig_micro))
				return false;
		}
		if (child->name == "time_sig_minor") {
			if (!child->get_s64(data.time_sig_minor))
				return false;
		}
		if (child->name == "time_sig_major") {
			if (!child->get_s64(data.time_sig_major))
				return false;
		}
		if (child->name == "notes") {
			if (!load_notes(child))
				return false;
		}
	}

	return true;
}

bool LPattern::play(LSong &p_song, uint32_t p_output_bus_handle, uint32_t p_start_sample, uint32_t p_num_samples, uint32_t p_samples_per_tick, uint32_t p_pattern_start_tick) const {
	int32_t player_id = data.player_a;

	LInstrument *inst = p_song._players.get_player_instrument(player_id);
	// no instrument assigned
	if (!inst)
		return false;

	inst->set_output_bus(p_output_bus_handle);

	for (uint32_t n = 0; n < notes.size(); n++) {
		const LNote &note = notes[n];
		int32_t start = note.tick_start + p_pattern_start_tick;
		//int32_t end = start + note.tick_length;

		inst->play(note.note, p_start_sample, p_num_samples, start * p_samples_per_tick, note.tick_length * p_samples_per_tick);
	}

	return true;
}

uint32_t LPattern::sort_notes(uint32_t p_old_selected_note) {
	if (p_old_selected_note >= notes.size()) {
		return 0;
	}

	// keep the current note ID up to date
	uint32_t note_id = p_old_selected_note;

	bool moved = true;
	while (moved) {
		moved = false;

		const LNote &curr = notes[note_id];
		if (note_id > 0) {
			// try moving back
			const LNote &prev = notes[note_id - 1];
			if (prev.tick_start > curr.tick_start) {
				SWAP(notes[note_id], notes[note_id - 1]);
				note_id--;
				moved = true;
				continue;
			}
		}
		if (note_id < notes.size() - 1) {
			// try moving forward
			const LNote &next = notes[note_id + 1];
			if (next.tick_start < curr.tick_start) {
				SWAP(notes[note_id], notes[note_id + 1]);
				note_id++;
				moved = true;
				continue;
			}
		}
	}
	//notes.sort();
	return note_id;
}
