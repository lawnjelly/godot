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
	RasterizerStorageGLES2::Material * Playback_Item_ChangeMaterial(Item *ci);
	bool Playback_Item_SetBlendModeAndUniforms(Item * ci, int &blend_mode);
	void Playback_Item_RenderCommandsNormal(Item * ci, bool unshaded, bool &reclip, RasterizerStorageGLES2::Material * material_ptr);
	void Playback_Item_ProcessLights(Item * ci, int &blend_mode, bool &reclip, RasterizerStorageGLES2::Material * material_ptr, bool unshaded);
	void Playback_Item_ProcessLight(Item * ci, Light * light, bool &light_used, VS::CanvasLightMode &mode, bool &reclip, RasterizerStorageGLES2::Material * material_ptr);
	void Playback_Item_ReenableScissor(bool reclip);


	virtual void fill_canvas_item_render_commands(Item *p_item, Item *current_clip, bool &reclip, RasterizerStorageGLES2::Material *p_material) = 0;

};


} // namespace
