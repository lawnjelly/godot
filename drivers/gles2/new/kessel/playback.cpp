#include "playback.h"

namespace Batch {

void Playback::Playback_Change_Item(const Batch &batch)
{
	Item *ci = batch.change_item.m_pItem;

	// alias
	State_ItemGroup &sig = m_State_ItemGroup;
	State_Item &sit = m_State_Item;

	if (sig.current_clip != ci->final_clip_owner) {

		sig.current_clip = ci->final_clip_owner;

		if (sig.current_clip) {
			glEnable(GL_SCISSOR_TEST);
			int y = storage->frame.current_rt->height - (sig.current_clip->final_clip_rect.position.y + sig.current_clip->final_clip_rect.size.y);
			if (storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_VFLIP])
				y = sig.current_clip->final_clip_rect.position.y;
			glScissor(sig.current_clip->final_clip_rect.position.x, y, sig.current_clip->final_clip_rect.size.width, sig.current_clip->final_clip_rect.size.height);
		} else {
			glDisable(GL_SCISSOR_TEST);
		}
	}

	// TODO: copy back buffer

	if (ci->copy_back_buffer) {
		if (ci->copy_back_buffer->full) {
			_copy_texscreen(Rect2());
		} else {
			_copy_texscreen(ci->copy_back_buffer->rect);
		}
	}

	sit.skeleton = NULL;

	{
		//skeleton handling
		if (ci->skeleton.is_valid() && storage->skeleton_owner.owns(ci->skeleton)) {
			sit.skeleton = storage->skeleton_owner.get(ci->skeleton);
			if (!sit.skeleton->use_2d) {
				sit.skeleton = NULL;
			} else {
				state.skeleton_transform = sig.m_pItemGroup->p_base_transform * sit.skeleton->base_transform_2d;
				state.skeleton_transform_inverse = state.skeleton_transform.affine_inverse();
				state.skeleton_texture_size = Vector2(sit.skeleton->size * 2, 0);
			}
		}

		sit.use_skeleton = sit.skeleton != NULL;
		if (sig.prev_use_skeleton != sit.use_skeleton) {
			sig.rebind_shader = true;
			state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SKELETON, sit.use_skeleton);
			sig.prev_use_skeleton = sit.use_skeleton;
		}

		if (sit.skeleton) {
			glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 3);
			glBindTexture(GL_TEXTURE_2D, sit.skeleton->tex_id);
			state.using_skeleton = true;
		} else {
			state.using_skeleton = false;
		}
	}

}


void Playback::Playback_Change_ItemGroup(const Batch &batch)
{
	m_State_ItemGroup.Reset();

	// store a point to the itemgroup data in the itemgroup state
	int id = batch.change_itemgroup.id;
	m_State_ItemGroup.m_pItemGroup = &m_Data.itemgroups[id];

	state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SKELETON, false);
	state.current_tex = RID();
	state.current_tex_ptr = NULL;
	state.current_normal = RID();
	state.canvas_texscreen_used = false;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);
}

void Playback::Playback_Change_ItemGroup_End(const Batch &batch)
{
	if (m_State_ItemGroup.current_clip) {
		glDisable(GL_SCISSOR_TEST);
	}

	state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SKELETON, false);
}




} // namespace
