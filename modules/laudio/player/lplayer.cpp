#include "lplayer.h"
#include "../sample/lsampler.h"
#include "core/os/memory.h"

////////////////////////////////

uint32_t LPlayers::load_palette_instrument(Ref<LInstrument> p_instrument) {
	for (uint32_t n = 0; n < MAX_PLAYERS; n++) {
		if (_palette[n].instrument.is_null()) {
			DEV_ASSERT(_palette[n].ref_count == 0);

			_palette[n].instrument = p_instrument;
			_palette[n].ref_count = 1;
			return n;
		}
	}
	return UINT32_MAX;
}

uint32_t LPlayers::find_palette_instrument(Ref<LInstrument> p_instrument) {
	for (uint32_t n = 0; n < MAX_PLAYERS; n++) {
		if (_palette[n].instrument == p_instrument) {
			return n;
		}
	}

	return UINT32_MAX;
}

bool LPlayers::set_player_instrument(uint32_t p_player_id, Ref<LInstrument> p_instrument) {
	// is the id already set on the instrument? (i.e. a noop)
	if (get_player_instrument(p_player_id) == p_instrument.ptr()) {
		return true;
	}

	// unset any current instrument on the player
	clear_player_instrument(p_player_id);

	// does the instrument already exist?
	uint32_t palette_id = find_palette_instrument(p_instrument);

	if (palette_id == UINT32_MAX) {
		// add to palette
		palette_id = load_palette_instrument(p_instrument);
		ERR_FAIL_COND_V(palette_id == UINT32_MAX, false);
	} else {
		// inc ref count
		_palette[palette_id].ref_count++;
	}

	_players[p_player_id].instrument_palette_id = palette_id;

	return true;
}

String LPlayers::get_player_name(uint32_t p_player_id) {
	ERR_FAIL_COND_V(p_player_id >= MAX_PLAYERS, "Out of range");
	LInstrument *inst = get_player_instrument(p_player_id);
	if (inst) {
		return inst->get_name();
	}
	return "-";
}

void LPlayers::clear_players() {
	for (uint32_t n = 0; n < MAX_PLAYERS; n++) {
		clear_player_instrument(n);
	}
}

bool LPlayers::clear_player_instrument(uint32_t p_player_id) {
	ERR_FAIL_COND_V(p_player_id >= MAX_PLAYERS, false);
	uint32_t &id = _players[p_player_id].instrument_palette_id;
	if (id != UINT32_MAX) {
		PaletteInstrument &pi = _palette[id];
		// clear the instrument link
		id = UINT32_MAX;

		// decrement ref and delete if necessary
		if (pi.instrument.is_valid()) {
			if (pi.ref_count) {
				pi.ref_count--;
				if (!pi.ref_count) {
					pi.instrument.unref();
				}
			}
		}
	}

	return true;
}

bool LPlayers::load(String p_filename) {
	LSon::Node root;
	if (!root.load(p_filename)) {
		ERR_PRINT("FAILED loading " + p_filename);
		return false;
	}

	clear_players();

	//	root.save(p_filename + ".resave");

	// INCLUDE PATHS
	LSon::Node *node_paths = root.find_node("paths");
	ERR_FAIL_NULL_V(node_paths, false);

	LocalVector<String> include_paths;

	for (uint32_t n = 0; n < node_paths->children.size(); n++) {
		LSon::Node *node_path = node_paths->get_child(n);
		ERR_FAIL_COND_V(node_path->type != LSon::Node::NT_STRING, false);

		print_line("Include path shortname " + node_path->name + ", path is \"" + node_path->string + "\"");
		include_paths.push_back(node_path->string);
	}

	// INSTRUMENT PALETTE
	LSon::Node *node_instruments = root.find_node("instruments");
	ERR_FAIL_NULL_V(node_instruments, false);

	for (uint32_t n = 0; n < node_instruments->children.size(); n++) {
		LSon::Node *node_inst = node_instruments->get_child(n);
		ERR_FAIL_COND_V(node_inst->name != "instrument", false);

		// this will screw up if array sizes change - FIXME
		uint32_t palette_id = node_inst->val.u64;

		Ref<LInstrument> new_inst;
		for (uint32_t c = 0; c < node_inst->children.size(); c++) {
			LSon::Node *child = node_inst->get_child(c);

			if (child->name == "type") {
				if (child->string == "sampler") {
					new_inst = Ref<LInstrument>(memnew(LSampler));
					_palette[palette_id].instrument = new_inst;
					_palette[palette_id].ref_count = 0;
				}
			}

			if (child->name == "name") {
				if (new_inst.is_valid()) {
					new_inst->set_name(child->string);
				}
			}

			if (child->name == "data") {
				if (new_inst.is_valid()) {
					new_inst->load(child, include_paths);
				}
			}
		}

		/*
		LSon::Node * node_inst_type = node_inst->get_child(0);
		ERR_FAIL_NULL_V(node_inst_type, false);
		ERR_FAIL_COND_V(node_inst_type->name != "type", false);


		if (node_inst_type->string == "sampler")
		{
			Ref<LInstrument> new_inst = memnew(LSampler);
			_palette[palette_id].instrument = new_inst;
			_palette[palette_id].ref_count = 0;
		}
		*/
		// else unrecognised
	}

	// PLAYERS
	LSon::Node *node_players = root.find_node("players");
	ERR_FAIL_NULL_V(node_players, false);

	for (uint32_t n = 0; n < node_players->children.size(); n++) {
		LSon::Node *node_player = node_players->get_child(n);
		ERR_FAIL_COND_V(node_player->name != "player", false);

		uint32_t player_id = node_player->val.u64;
		if (player_id >= MAX_PLAYERS)
			return false;

		LSon::Node *node_player_inst = node_player->get_child(0);
		ERR_FAIL_COND_V(node_player_inst->name != "instrument", false);

		uint32_t inst_palette_id = node_player_inst->val.u64;

		// both ways reference and inc count
		_players[player_id].instrument_palette_id = inst_palette_id;
		_palette[inst_palette_id].ref_count += 1;
	}

	return true;
}

bool LPlayers::save(String p_filename) {
	LSon::Node root;

	//root.set_name("my list");
	root.set_name("players");

	for (uint32_t n = 0; n < MAX_PLAYERS; n++) {
		//_players[n].save(n, root);
	}

	//	LSon::Node *child0 = root.request_child();
	//	//child0->set_name("child 0");
	//	child0->set_string("chicken");

	//	LSon::Node *child1 = root.request_child();
	//	child1->set_name("child 1");
	//	child1->set_s64(-64);

	//	LSon::Node *child2 = root.request_child();
	//	child2->set_name("cats");
	//	child2->set_array();

	//	for (int n = 0; n < 3; n++) {
	//		LSon::Node *child = child2->request_child();
	//		child->set_name("cat " + itos(n));
	//		child->set_s64(n);
	//	}

	root.save(p_filename);

	// try loading
	LSon::Node load_root;
	load_root.load(p_filename);

	load_root.save("players_test2.txt");

	return true;
}
