#pragma once

#include "playback.h"

namespace Batch {

class Batcher : public Playback
{
public:

	void fill_canvas_render_items(Item *p_item_list, int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform);
	void fill_extract_commands(Item *ci);
	virtual void fill_canvas_item_render_commands(Item *p_item, Item *current_clip, bool &reclip, RasterizerStorageGLES2::Material *p_material);



};



} // namespace
