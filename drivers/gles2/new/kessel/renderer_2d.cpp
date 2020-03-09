#include "renderer_2d.h"

namespace Batch {

Renderer2D::Renderer2D()
{
	m_bUseKessel = false;
}


void Renderer2D::canvas_render_items(Item *p_item_list, int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform)
{
	if (!m_bUseKessel)
	{
		Renderer2D_old::canvas_render_items(p_item_list, p_z, p_modulate, p_light, p_base_transform);
		return;
	}

	fill_canvas_render_items(p_item_list, p_z, p_modulate, p_light, p_base_transform);
}




} // namespace
