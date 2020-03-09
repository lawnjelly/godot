#pragma once

#include "batcher.h"

namespace Batch {

class Renderer2D : public Batcher
{
public:
	Renderer2D();

	virtual void canvas_begin();
	virtual void canvas_end();
	virtual void canvas_render_items(Item *p_item_list, int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform);
	virtual void canvas_render_items_flush();

	virtual void flush();

private:
	void flush_do();
	void flush_render();

	// mem vars
	bool m_bUseKessel;
	bool m_bKesselFlash;
};


} // namespace
