#include "lsong.h"

LSong *LSong::_current_song = nullptr;

LSong::LSong() {
	_current_song = this;
}

LSong::~LSong() {
	if (_current_song == this) {
		_current_song = nullptr;
	}
}

//////////////////////////////////

void Song::_notification(int p_what) {
}

void Song::_bind_methods() {
}

void Song::create_pattern_view() {
}
