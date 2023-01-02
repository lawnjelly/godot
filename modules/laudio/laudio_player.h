#pragma once

#include "scene/main/node.h"

class LAudioPlayer : public Node {
	GDCLASS(LAudioPlayer, Node);

	void _mix_audio();
	static void _mix_audios(void *self) { reinterpret_cast<LAudioPlayer *>(self)->_mix_audio(); }

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
};
