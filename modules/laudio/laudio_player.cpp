#include "laudio_player.h"

#include "servers/audio_server.h"

void LAudioPlayer::_mix_audio() {
}

void LAudioPlayer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			AudioServer::get_singleton()->add_callback(_mix_audios, this);
		} break;
		case NOTIFICATION_EXIT_TREE: {
			AudioServer::get_singleton()->remove_callback(_mix_audios, this);
		} break;

		default: {
		} break;
	}
}

void LAudioPlayer::_bind_methods() {
}
