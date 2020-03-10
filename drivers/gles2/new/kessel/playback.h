#pragma once

#include "batcher_state.h"

namespace Batch {

class Playback : public BatcherState
{
public:

protected:
	void Playback_SetDryRun(bool b) {m_bDryRun = b;}

	void Playback_Change_Item(const Batch &batch);
	void Playback_Change_ItemGroup(const Batch &batch);
	void Playback_Change_ItemGroup_End(const Batch &batch);


	// split apart whole spaghetti  canvas_render_items function
	void Playback_Item_ChangeScissorItem(Item * pNewScissorItem);
	void Playback_Item_CopyBackBuffer(Item *ci);
	void Playback_Item_SkeletonHandling(Item *ci);
	void Playback_Item_ChangeMaterial(Item *ci);
	void Playback_Item_SetBlendModeAndUniforms(Item * ci);
	void Playback_Item_RenderCommandsNormal(Item * ci);
	void Playback_Item_ProcessLights(Item * ci);
	void Playback_Item_ProcessLight(Item * ci, Light * light, bool &light_used, VS::CanvasLightMode &mode);
	void Playback_Item_ReenableScissor();


	virtual void fill_canvas_item_render_commands(Item *p_item, Item *current_clip, bool &reclip, RasterizerStorageGLES2::Material *p_material) = 0;

};


} // namespace
