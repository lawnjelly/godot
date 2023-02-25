#pragma once

#include "core/reference.h"

class LInstrument;

class LPlayer {
public:
	Ref<LInstrument> instrument;
	bool is_active() const { return instrument.is_valid(); }
	LInstrument *get_instrument() {
		if (instrument.is_valid()) {
			return instrument.ptr();
		}
		return nullptr;
	}
	const LInstrument *get_instrument() const {
		if (instrument.is_valid()) {
			return instrument.ptr();
		}
		return nullptr;
	}
	Ref<LInstrument> get_instrument_ref() { return instrument; }
	void set_instrument(Ref<LInstrument> p_instrument) {
		instrument = p_instrument;
	}
	void clear() {
		instrument.unref();
	}
	const char *get_name() const {
		const LInstrument *i = get_instrument();
		if (i) {
			return i->get_name();
		}
		return "-";
	}
};

class LPlayers {
public:
	enum { MAX_PLAYERS = 32 };

private:
	LPlayer _players[MAX_PLAYERS];

public:
	LPlayer &get_player(uint32_t p_id) { return _players[p_id]; }
};
