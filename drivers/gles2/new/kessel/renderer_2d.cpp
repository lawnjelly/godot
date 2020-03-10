#include "renderer_2d.h"
#include "core/engine.h"

namespace Batch
{

Renderer2D::Renderer2D()
{
	m_bUseKessel = true;
	m_bKesselFlash = true;
}

void Renderer2D::canvas_begin()
{
	m_Data.Reset_Pass();
	Renderer2D_old::canvas_begin();
}

void Renderer2D::canvas_end()
{
	Renderer2D_old::canvas_end();
}

void Renderer2D::canvas_render_items_flush()
{
	flush();
}

void Renderer2D::canvas_render_items(Item *p_item_list, int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform)
{
	bool use_kessel = m_bUseKessel;
#ifdef KESSEL_FLASH
	if ((Engine::get_singleton()->get_frames_drawn() % 2) == 0)
		use_kessel = false;
#endif

	if (!use_kessel)
	{
		Renderer2D_old::canvas_render_items(p_item_list, p_z, p_modulate, p_light, p_base_transform);
		return;
	}

	fill_canvas_render_items(p_item_list, p_z, p_modulate, p_light, p_base_transform);
}

void Renderer2D::flush_do()
{
	// some heuristic to decide whether to use colored verts.
	// feel free to tweak this.
	// this could use hysteresis, to prevent jumping between methods
	// .. however probably not necessary
	//	if ((bd.total_color_changes * 4) > (bd.total_quads * 1)) {
	//		// small perf cost versus going straight to colored verts (maybe around 10%)
	//		// however more straightforward
	//		_canvas_batcher.batch_translate_to_colored();
	//	}

	//	batch_upload_buffers();

	flush_render();
}

void Renderer2D::flush_render()
{
	// mark that we are rendering rather than filling during playback
	Playback_SetDryRun(false);

	int num_batches = m_Data.batches.size();

	//Item::Command *const *commands = 0;

	for (int batch_num = 0; batch_num < num_batches; batch_num++)
	{
		const Batch &batch = m_Data.batches[batch_num];

		switch (batch.type)
		{
			default:
				break;
			case Batch::BT_CHANGE_ITEM:
			{
				Playback_Change_Item(batch);
			}
			break;
			case Batch::BT_CHANGE_ITEM_GROUP:
			{
				Playback_Change_ItemGroup(batch);
			}
			break;
			case Batch::BT_CHANGE_ITEM_GROUP_END:
			{
				Playback_Change_ItemGroup_End(batch);
			}
			break;
		} // switch
	} // for
}

void Renderer2D::flush()
{
	if (m_Data.batches.size())
	{
		flush_do();
	}
	m_Data.Reset_Flush();
}

} // namespace Batch
