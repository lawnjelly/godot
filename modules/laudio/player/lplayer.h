#pragma once

#include "../lson/lson.h"
#include "core/reference.h"
#include "linstrument.h"

class LPlayer {
public:
	uint32_t instrument_palette_id = UINT32_MAX;
	//	LInstrument_Holder holder;
	//	bool is_active() const { return holder.is_valid(); }
	//	LInstrument *get_instrument() {
	//		if (holder.is_valid()) {
	//			return holder.get_instrument().ptr();
	//		}
	//		return nullptr;
	//	}
	//	const LInstrument *get_instrument() const {
	//		if (holder.is_valid()) {
	//			return holder.get_instrument().ptr();
	//		}
	//		return nullptr;
	//	}
	//	Ref<LInstrument> get_instrument_ref() { return holder.get_instrument(); }
	//	void set_instrument(Ref<LInstrument> p_instrument) {
	//		holder._instrument = p_instrument;
	//	}
	//	bool load(String p_filename) {
	//		return holder.load(p_filename);
	//	}

	//	bool save(uint32_t p_player_id, LSon::Node &p_root);

	//	void clear() {
	//		holder._instrument.unref();
	//	}
	//	const char *get_name() const {
	//		const LInstrument *i = get_instrument();
	//		if (i) {
	//			return i->get_name();
	//		}
	//		return "-";
	//	}
};

class LPlayers {
public:
	enum { MAX_PLAYERS = 32 };

	struct PaletteInstrument {
		Ref<LInstrument> instrument;
		int32_t ref_count = 0;
	};

private:
	LPlayer _players[MAX_PLAYERS];
	PaletteInstrument _palette[MAX_PLAYERS];

public:
	LPlayer &get_player(uint32_t p_id) { return _players[p_id]; }
	LInstrument *get_player_instrument(uint32_t p_player_id) {
		ERR_FAIL_COND_V(p_player_id >= MAX_PLAYERS, nullptr);
		uint32_t id = _players[p_player_id].instrument_palette_id;
		if (id != UINT32_MAX) {
			if (_palette[id].instrument.is_valid()) {
				return _palette[id].instrument.ptr();
			}
		}
		return nullptr;
	}
	Ref<LInstrument> get_player_instrument_ref(uint32_t p_player_id) {
		ERR_FAIL_COND_V(p_player_id >= MAX_PLAYERS, Ref<LInstrument>());
		uint32_t id = _players[p_player_id].instrument_palette_id;
		if (id != UINT32_MAX) {
			if (_palette[id].instrument.is_valid()) {
				return _palette[id].instrument;
			}
		}
		return Ref<LInstrument>();
	}

	bool clear_player_instrument(uint32_t p_player_id);
	void clear_players();

	bool load(String p_filename);
	bool save(String p_filename);

	uint32_t load_palette_instrument(Ref<LInstrument> p_instrument);
	uint32_t find_palette_instrument(Ref<LInstrument> p_instrument);

	bool set_player_instrument(uint32_t p_player_id, Ref<LInstrument> p_instrument);
	String get_player_name(uint32_t p_player_id);

	LPlayers() {
		//load("players.txt");
		//save("players_test.txt");
	}
};
