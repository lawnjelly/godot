#include "laudio_player.h"
#include "lsong.h"

#include "servers/audio_server.h"

//#define LAUDIO_TEST

LAudioPlayer::LAudioPlayer() {
	data.num_samples = AudioServer::get_singleton()->thread_get_mix_buffer_size();
	data.output_bus.resize(data.num_samples);
}

void LAudioPlayer::_play_audio(uint32_t p_left, uint32_t p_right, uint32_t p_samples_left) {
	uint32_t &curs = data.cursor_sample;
	LBus &bus = data.output_bus;

	// wrap the cursor
	if (curs < p_left) {
		curs = MAX(curs, p_left);
		bus.set_song_time_start((int32_t)curs - bus.get_offset());
	}
	if (curs >= p_right) {
		curs = p_left;
		bus.set_song_time_start((int32_t)curs - bus.get_offset());
	}

	uint32_t available = p_right - curs;

	// TEST TEST TEST remove this in production
	//available = MIN(available, 345);

	if (available >= p_samples_left) {
		// simple case, fits within
		data.song->song_play(bus, curs, p_samples_left);
		curs += p_samples_left;
	} else {
		// play what we can
		data.song->song_play(bus, curs, available);
		p_samples_left -= available;
		curs += available;

		// we must write into a different part of the bus
		bus.shift_offset(available);
		_play_audio(p_left, p_right, p_samples_left);
	}
}

void LAudioPlayer::_mix_audio() {
	if (!data.playing)
		return;

	if (!data.song)
		return;

	// update cursor
	const LTiming &timing = data.song->get_lsong()._timing;
	uint32_t left = timing.get_transport_left_sample();
	uint32_t right = timing.get_transport_right_sample();

	data.output_bus.get_sample().blank();
	data.output_bus.set_offset(0);
	data.output_bus.set_song_time_start(data.cursor_sample);

	_play_audio(left, right, data.num_samples);

	//	return;

	int bus_index = AudioServer::get_singleton()->thread_find_bus_index("Master");

	AudioFrame *targets[4] = { nullptr, nullptr, nullptr, nullptr };

	targets[0] = AudioServer::get_singleton()->thread_get_channel_mix_buffer(bus_index, 0);
	//	if (AudioServer::get_singleton()->get_speaker_mode() == AudioServer::SPEAKER_MODE_STEREO) {
	//		targets[0] = AudioServer::get_singleton()->thread_get_channel_mix_buffer(bus_index, 0);
	//	} else {
	//		switch (mix_target) {
	//			case MIX_TARGET_STEREO: {
	//				targets[0] = AudioServer::get_singleton()->thread_get_channel_mix_buffer(bus_index, 0);
	//			} break;
	//			case MIX_TARGET_SURROUND: {
	//				for (int i = 0; i < AudioServer::get_singleton()->get_channel_count(); i++) {
	//					targets[i] = AudioServer::get_singleton()->thread_get_channel_mix_buffer(bus_index, i);
	//				}
	//			} break;
	//			case MIX_TARGET_CENTER: {
	//				targets[0] = AudioServer::get_singleton()->thread_get_channel_mix_buffer(bus_index, 1);
	//			} break;
	//		}
	//	}

	for (int c = 0; c < 4; c++) {
		if (!targets[c]) {
			break;
		}

		const LSample &sample = data.output_bus.get_sample();

		for (int32_t i = 0; i < data.num_samples; i++) {
			AudioFrame *af = &targets[c][i];
			af->l += sample.get_f(i, 0) * data.volume;
			af->r += sample.get_f(i, 1) * data.volume;
		}
	}
}

void LAudioPlayer::test() {
#ifndef LAUDIO_TEST
	return;
#endif
	const LTiming &timing = data.song->get_lsong()._timing;
	uint32_t left = timing.get_transport_left_sample();
	uint32_t right = timing.get_transport_right_sample();

	uint32_t test_num_samples = right - left;

	LSample output;
	output.create(4, 2, 44100, test_num_samples);

	int iterations = test_num_samples / data.num_samples;
	iterations++;

	uint32_t dest_start_sample = 0;
	for (int n = 0; n < iterations; n++) {
		data.output_bus.get_sample().blank();
		data.output_bus.set_offset(0);
		data.output_bus.set_song_time_start(data.cursor_sample);
		//		if ((n % 2) == 0)
		_play_audio(left, right, data.num_samples);
		data.output_bus.get_sample().copy_to(output, data.num_samples, dest_start_sample);
		dest_start_sample += data.num_samples;
	}

	output.save_wav("test_live.wav");
}

void LAudioPlayer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
//		set_physics_process_internal(true);
#ifndef LAUDIO_TEST
			if (!Engine::get_singleton()->is_editor_hint())
				AudioServer::get_singleton()->add_callback(_mix_audios, this);
#endif
		} break;
		case NOTIFICATION_EXIT_TREE: {
#ifndef LAUDIO_TEST
			if (!Engine::get_singleton()->is_editor_hint())
				AudioServer::get_singleton()->remove_callback(_mix_audios, this);
#endif
		} break;
		case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			_mix_audio();
		} break;

		default: {
		} break;
	}
}

void LAudioPlayer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_song", "song"), &LAudioPlayer::set_song);
	ClassDB::bind_method(D_METHOD("set_volume", "volume"), &LAudioPlayer::set_volume);
	ClassDB::bind_method(D_METHOD("set_playing", "playing"), &LAudioPlayer::set_playing);
	ClassDB::bind_method(D_METHOD("test"), &LAudioPlayer::test);

	ClassDB::bind_method(D_METHOD("set_transport_cursor", "sample"), &LAudioPlayer::set_transport_cursor);
	ClassDB::bind_method(D_METHOD("get_transport_cursor"), &LAudioPlayer::get_transport_cursor);
}

uint32_t LAudioPlayer::get_transport_cursor() const {
	return data.cursor_sample;
}

void LAudioPlayer::set_transport_cursor(uint32_t p_sample) {
	data.cursor_sample = p_sample;
}

void LAudioPlayer::set_playing(bool p_playing) {
	data.playing = p_playing;
}

void LAudioPlayer::set_song(Node *p_song) {
	Song *song = Object::cast_to<Song>(p_song);
	data.song = song;
}
