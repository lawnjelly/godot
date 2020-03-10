#include "playback.h"
#include "servers/visual/visual_server_raster.h"

namespace Batch
{

void Playback::Playback_Item_ChangeScissorItem(Item *pNewScissorItem)
{
	//State_ItemGroup &sig = m_State_ItemGroup;

	State_SetScissorItem(pNewScissorItem);
	State_SetScissor(pNewScissorItem != 0);

	/* KEEP THIS HERE AS REFERENCE IN CASE OF REGRESSIONS
	if (sig.m_pScissorItem != pNewScissorItem)
	{

		sig.m_pScissorItem = pNewScissorItem;

		if (!m_bDryRun)
		{
			if (sig.m_pScissorItem)
			{
				glEnable(GL_SCISSOR_TEST);
				int y = storage->frame.current_rt->height - (sig.m_pScissorItem->final_clip_rect.position.y + sig.m_pScissorItem->final_clip_rect.size.y);
				if (storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_VFLIP])
					y = sig.m_pScissorItem->final_clip_rect.position.y;
				glScissor(sig.m_pScissorItem->final_clip_rect.position.x, y, sig.m_pScissorItem->final_clip_rect.size.width, sig.m_pScissorItem->final_clip_rect.size.height);
			}
			else
			{
				glDisable(GL_SCISSOR_TEST);
			}
		} // dry run
	}
	*/
}

void Playback::Playback_Item_CopyBackBuffer(Item *ci)
{
	// TODO: copy back buffer

	if (ci->copy_back_buffer)
	{
		if (!m_bDryRun)
		{
			if (ci->copy_back_buffer->full)
			{
				_copy_texscreen(Rect2());
			}
			else
			{
				_copy_texscreen(ci->copy_back_buffer->rect);
			}
		} // dry run
		else
		{
			AddItemChangeFlag(CF_COPY_BACK_BUFFER);
		}
	}
}

void Playback::Playback_Item_SkeletonHandling(Item *ci)
{
	State_Item &sit = m_State_Item;
	State_ItemGroup &sig = m_State_ItemGroup;

	sit.m_pSkeleton = NULL;

	//skeleton handling
	if (ci->skeleton.is_valid() && storage->skeleton_owner.owns(ci->skeleton))
	{
		AddItemChangeFlag(CF_SKELETON);
		sit.m_pSkeleton = storage->skeleton_owner.get(ci->skeleton);
		if (!sit.m_pSkeleton->use_2d)
		{
			sit.m_pSkeleton = NULL;
		}
		else
		{
			if (!m_bDryRun)
			{
				state.skeleton_transform = sig.m_pItemGroup->p_base_transform * sit.m_pSkeleton->base_transform_2d;
				state.skeleton_transform_inverse = state.skeleton_transform.affine_inverse();
				state.skeleton_texture_size = Vector2(sit.m_pSkeleton->size * 2, 0);
			} // dry run
		}
	}

	sit.m_bUseSkeleton = sit.m_pSkeleton != NULL;
	if (sig.m_bPrevUseSkeleton != sit.m_bUseSkeleton)
	{
		AddItemChangeFlag(CF_SKELETON);
		sig.m_bRebindShader = true;
		if (!m_bDryRun)
		{
			state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SKELETON, sit.m_bUseSkeleton);
		}
		sig.m_bPrevUseSkeleton = sit.m_bUseSkeleton;
	}

	if (!m_bDryRun)
	{
		if (sit.m_pSkeleton)
		{
			glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 3);
			glBindTexture(GL_TEXTURE_2D, sit.m_pSkeleton->tex_id);
			state.using_skeleton = true;
		}
		else
		{
			state.using_skeleton = false;
		}
	} // dry run
}

void Playback::Playback_Item_ChangeMaterial(Item *ci)
{
	State_ItemGroup &sig = m_State_ItemGroup;
	State_Item &sit = m_State_Item;

	Item *material_owner;
	if (ci->material_owner)
		material_owner = ci->material_owner;
	else
		material_owner = ci;

	RID material = material_owner->material;
	save_material(storage->material_owner.getornull(material));

	// check whether rebind is always set .. this will muck up batching
	if ((material != sig.m_RID_CanvasLastMaterial) || sig.m_bRebindShader)
	{
		AddItemChangeFlag(CF_MATERIAL);

		RasterizerStorageGLES2::Shader *shader_ptr = NULL;

		if (get_material())
		{
			shader_ptr = get_material()->shader;

			if (shader_ptr && shader_ptr->mode != VS::SHADER_CANVAS_ITEM)
				shader_ptr = NULL; // not a canvas item shader, don't use.
		}

		if (!m_bDryRun)
		{
			if (shader_ptr)
			{
				if (shader_ptr->canvas_item.uses_screen_texture)
				{
					if (!state.canvas_texscreen_used)
					{
						//copy if not copied before
						_copy_texscreen(Rect2());

						// blend mode will have been enabled so make sure we disable it again later on
						//last_blend_mode = last_blend_mode != RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_DISABLED ? last_blend_mode : -1;
					}

					if (storage->frame.current_rt->copy_screen_effect.color)
					{
						glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 4);
						glBindTexture(GL_TEXTURE_2D, storage->frame.current_rt->copy_screen_effect.color);
					}
				}

				if (shader_ptr != sig.m_pCachedShader)
				{

					if (shader_ptr->canvas_item.uses_time)
					{
						VisualServerRaster::redraw_request();
					}

					state.canvas_shader.set_custom_shader(shader_ptr->custom_code_id);
					state.canvas_shader.bind();
				}

				int tc = get_material()->textures.size();
				Pair<StringName, RID> *textures = get_material()->textures.ptrw();

				ShaderLanguage::ShaderNode::Uniform::Hint *texture_hints = shader_ptr->texture_hints.ptrw();

				for (int i = 0; i < tc; i++)
				{

					glActiveTexture(GL_TEXTURE0 + i);

					RasterizerStorageGLES2::Texture *t = storage->texture_owner.getornull(textures[i].second);

					if (!t)
					{

						switch (texture_hints[i])
						{
							case ShaderLanguage::ShaderNode::Uniform::HINT_BLACK_ALBEDO:
							case ShaderLanguage::ShaderNode::Uniform::HINT_BLACK:
							{
								glBindTexture(GL_TEXTURE_2D, storage->resources.black_tex);
							}
							break;
							case ShaderLanguage::ShaderNode::Uniform::HINT_ANISO:
							{
								glBindTexture(GL_TEXTURE_2D, storage->resources.aniso_tex);
							}
							break;
							case ShaderLanguage::ShaderNode::Uniform::HINT_NORMAL:
							{
								glBindTexture(GL_TEXTURE_2D, storage->resources.normal_tex);
							}
							break;
							default:
							{
								glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);
							}
							break;
						}

						continue;
					}

					if (t->redraw_if_visible)
						VisualServerRaster::redraw_request();

					t = t->get_ptr();

#ifdef TOOLS_ENABLED
					if (t->detect_normal && texture_hints[i] == ShaderLanguage::ShaderNode::Uniform::HINT_NORMAL)
					{
						t->detect_normal(t->detect_normal_ud);
					}
#endif
					if (t->render_target)
						t->render_target->used_in_frame = true;

					glBindTexture(t->target, t->tex_id);
				}
			}
			else
			{
				state.canvas_shader.set_custom_shader(0);
				state.canvas_shader.bind();
			}
			state.canvas_shader.use_material((void *)get_material());

		} // dry run

		sig.m_pCachedShader = shader_ptr;

		sig.m_RID_CanvasLastMaterial = material;

		sig.m_bRebindShader = false;
	}
}

void Playback::Playback_Item_SetBlendModeAndUniforms(Item *ci)
{
	State_ItemGroup &sig = m_State_ItemGroup;
	const BItemGroup &big = *sig.m_pItemGroup;
	const Color &p_modulate = big.p_modulate;
	State_Item &sit = m_State_Item;

	// decide new blend mode
	int blend_mode = 0;
	if (sig.m_pCachedShader)
		blend_mode = sig.m_pCachedShader->canvas_item.blend_mode;
	else
		blend_mode = RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_MIX;
//	state_set_blend_mode(blend_mode);

	// is item unshaded?
	sit.m_bUnshaded = sig.m_pCachedShader &&
	((sig.m_pCachedShader->canvas_item.light_mode == RasterizerStorageGLES2::Shader::CanvasItem::LIGHT_MODE_UNSHADED) ||
	((blend_mode != RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_MIX) &&
	(blend_mode != RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_PMALPHA)));

	// last blend mode is never set????
	State_SetBlendMode(blend_mode);
//	state_set_blend_mode(blend_mode);
//	if (sig.m_iLastBlendMode != get_blendmode())
//	{
//		sig.m_iLastBlendMode = get_blendmode();

//		AddItemChangeFlag(CF_BLEND_MODE);
//		if (!m_bDryRun)
//			GL_SetState_BlendMode(get_blendmode());
//	}

	Color col = ci->final_modulate;
	// possibly need a changed flag for unshaded?
	if (!sit.m_bUnshaded)
		col *= p_modulate;
	state_set_final_modulate(col);

	state_set_model_view(ci->final_transform);
	state_set_extra(Transform2D());

	if (!m_bDryRun)
		_set_uniforms();
}

void Playback::Playback_Item_RenderCommandsNormal(Item *ci)
{
	State_ItemGroup &sig = m_State_ItemGroup;
	State_Item &sit = m_State_Item;

	sit.m_bReclip = false;

	if (sit.m_bUnshaded || (state.uniforms.final_modulate.a > 0.001 && (!sig.m_pCachedShader || sig.m_pCachedShader->canvas_item.light_mode != RasterizerStorageGLES2::Shader::CanvasItem::LIGHT_MODE_LIGHT_ONLY) && !ci->light_masked))
	{
		if (!m_bDryRun)
			_canvas_item_render_commands(ci, NULL, sit.m_bReclip, get_material());
		else
			fill_canvas_item_render_commands(ci, NULL, sit.m_bReclip, get_material());
	}

	// this could be dealt with .. only if conditionals change
	// NOTE .. uncomment this if there are regressions
	//sig.m_bRebindShader = true; // hacked in for now.
}

void Playback::Playback_Item_ReenableScissor()
{

	State_ItemGroup &sig = m_State_ItemGroup;
	//	State_Item &sit = m_State_Item;

	//  note is this even needed here? scissor gets set at the start of an item
	// this could result in redundant state changes...

	// re-enable scissor if a scissor item is set.
	// this will be a no-op with adjacent items with the same scissor
	if (sig.m_pScissorItem)
		State_SetScissor(true);

	/*
	if (!m_bDryRun)
	{
		if (sit.m_bReclip)
		{
			glEnable(GL_SCISSOR_TEST);
			int y = storage->frame.current_rt->height - (sig.m_pScissorItem->final_clip_rect.position.y + sig.m_pScissorItem->final_clip_rect.size.y);
			if (storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_VFLIP])
				y = sig.m_pScissorItem->final_clip_rect.position.y;

			glScissor(sig.m_pScissorItem->final_clip_rect.position.x, y, sig.m_pScissorItem->final_clip_rect.size.width, sig.m_pScissorItem->final_clip_rect.size.height);
		}
	} // dry run
	*/
}

void Playback::Playback_Item_ProcessLight(Item *ci, Light *light, bool &light_used, VS::CanvasLightMode &mode)
{
	State_ItemGroup &sig = m_State_ItemGroup;
	const BItemGroup &big = *sig.m_pItemGroup;
	int p_z = big.p_z;
	State_Item &sit = m_State_Item;

	if (ci->light_mask & light->item_mask &&
			p_z >= light->z_min && p_z <= light->z_max &&
			ci->global_rect_cache.intersects_transformed(light->xform_cache, light->rect_cache))
	{

		//intersects this light

		if (!light_used || mode != light->mode)
		{

			mode = light->mode;

			if (!m_bDryRun)
			{
				GL_SetState_LightBlend(mode);
			} // dry run
		}

		if (!light_used)
		{

			if (!m_bDryRun)
				state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_LIGHTING, true);
			light_used = true;
		}

		bool has_shadow = light->shadow_buffer.is_valid() && ci->light_mask & light->item_shadow_mask;

		if (!m_bDryRun)
		{
			CanvasShader_SetConditionals_Light(has_shadow, light);

			state.canvas_shader.bind();
			state.using_light = light;
			state.using_shadow = has_shadow;

			//always re-set uniforms, since light parameters changed
			_set_uniforms();
			state.canvas_shader.use_material((void *)get_material());

			glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 4);
			RasterizerStorageGLES2::Texture *t = storage->texture_owner.getornull(light->texture);
			if (!t)
			{
				glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);
			}
			else
			{
				t = t->get_ptr();

				glBindTexture(t->target, t->tex_id);
			}

			glActiveTexture(GL_TEXTURE0);
		} // dry run

		if (!m_bDryRun)
			_canvas_item_render_commands(ci, NULL, sit.m_bReclip, get_material()); //redraw using light
		else
			fill_canvas_item_render_commands(ci, NULL, sit.m_bReclip, get_material()); //redraw using light

		state.using_light = NULL;
	}
}

void Playback::Playback_Item_ProcessLights(Item *ci)
{
	State_ItemGroup &sig = m_State_ItemGroup;
	const BItemGroup &big = *sig.m_pItemGroup;
	State_Item &sit = m_State_Item;

	if ((get_blendmode() == RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_MIX || get_blendmode() == RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_PMALPHA) && big.p_light && !sit.m_bUnshaded)
	{
		// lights ALWAYS break the batching between items. Can only batch commands within items
		AddItemChangeFlag(CF_LIGHT);

		Light *light = big.p_light;
		bool light_used = false;
		VS::CanvasLightMode mode = VS::CANVAS_LIGHT_MODE_ADD;

		if (!m_bDryRun)
		{
			state.uniforms.final_modulate = ci->final_modulate; // remove the canvas modulate
		}

		while (light)
		{
			Playback_Item_ProcessLight(ci, light, light_used, mode);
			light = light->next_ptr;
		}

		if (light_used)
		{

			if (!m_bDryRun)
			{
				CanvasShader_SetConditionals_Light(false, 0);
				state.canvas_shader.bind();
			} // dry run

			sig.m_iLastBlendMode = -1;
		}
	}
}

void Playback::Playback_Change_Item(const Batch &batch)
{
	m_State_Item.Reset_Item();
	Item *ci = batch.change_item.m_pItem;

	// alias
	//	State_ItemGroup &sig = m_State_ItemGroup;
	//	State_Item &sit = m_State_Item;
	//	const BItemGroup &big = *sig.m_pItemGroup;
	//	const Color &p_modulate = big.p_modulate;
	//	int p_z = big.p_z;

	Playback_Item_ChangeScissorItem(ci->final_clip_owner);
	Playback_Item_CopyBackBuffer(ci);
	Playback_Item_SkeletonHandling(ci);
	Playback_Item_ChangeMaterial(ci);
	Playback_Item_SetBlendModeAndUniforms(ci);
	Playback_Item_RenderCommandsNormal(ci);
	Playback_Item_ProcessLights(ci);
	Playback_Item_ReenableScissor();

	if (m_bDryRun)
	{
#ifdef KESSEL_PRINT_ITEM_STATE_CHANGES
		String sz = m_State_Item.ChangeFlagsToString();
		print_line(sz);
#endif
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

	if (!m_bDryRun)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);
	}
}

void Playback::Playback_Change_ItemGroup_End(const Batch &batch)
{
	if (!m_bDryRun)
	{
		if (m_State_ItemGroup.m_pScissorItem)
		{
			glDisable(GL_SCISSOR_TEST);
		}

		state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SKELETON, false);
	} // if not a dry run
}

} // namespace Batch
