#pragma once

#include "lbus.h"
#include "scene/main/node.h"

class Song;

class LAudioPlayer : public Node {
	GDCLASS(LAudioPlayer, Node);

	void _mix_audio();
	void _play_audio(uint32_t p_left, uint32_t p_right, uint32_t p_samples_left);
	static void _mix_audios(void *self) { reinterpret_cast<LAudioPlayer *>(self)->_mix_audio(); }

	struct Data {
		Song *song = nullptr;
		LBus output_bus;
		bool playing = false;
		int32_t num_samples = 0;
		uint32_t cursor_sample = 0;
		float volume = 0.2;
	} data;

	void test();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_song(Node *p_song);
	void set_playing(bool p_playing);
	void set_volume(float p_volume) { data.volume = p_volume; }

	uint32_t get_transport_cursor() const;
	void set_transport_cursor(uint32_t p_sample);
	LAudioPlayer();
};
