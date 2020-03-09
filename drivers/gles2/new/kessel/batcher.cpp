#include "batcher.h"

namespace Batch {


void Batcher::fill_canvas_render_items(Item *p_item_list, int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform)
{
	Playback_SetDryRun(true);

	State_SetItemGroup(p_z, p_modulate, p_light, p_base_transform);

	// dry run playback
	Playback_Change_ItemGroup(*m_State_Fill.m_pBatch_ItemGroup);

	while (p_item_list)
	{
		Item *ci = p_item_list;
		State_SetItem(ci);

		// dry run playback
		Playback_Change_Item(*m_State_Fill.m_pBatch_Item);

		//fill_extract_commands(ci);

		p_item_list = p_item_list->next;
	}

	State_SetItemGroup_End();
}


void Batcher::fill_canvas_item_render_commands(Item *p_item, Item *current_clip, bool &reclip, RasterizerStorageGLES2::Material *p_material)
{

}


void Batcher::fill_extract_commands(Item *ci)
{
}


} // namespace
