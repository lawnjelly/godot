#include "batcher.h"

namespace Batch {


void Batcher::fill_canvas_render_items(Item *p_item_list, int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform)
{
	State_SetItemGroup(p_z, p_modulate, p_light, p_base_transform);

	while (p_item_list)
	{
		Item *ci = p_item_list;
		State_SetItem(ci);
		p_item_list = p_item_list->next;
	}

	State_SetItemGroup_End();
}


} // namespace
