#pragma once

#include "legacy.h"

namespace Batch {

class Batcher : public Legacy
{
public:

	void fill_canvas_render_items(Item *p_item_list, int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform);



};



} // namespace
