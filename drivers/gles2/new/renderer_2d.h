#pragma once

#include "batcher.h"

namespace Batch {

class Renderer2D : public Batcher
{
public:
	Renderer2D();

	virtual void canvas_render_items(Item *p_item_list, int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform);

private:
	bool m_bUseKessel;
};


} // namespace
