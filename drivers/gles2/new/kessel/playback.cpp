#include "playback.h"
#include "servers/visual/visual_server_raster.h"


namespace Batch {

void Playback::Playback_Item_ChangeScissorItem(Item * pNewScissorItem)
{
	State_ItemGroup &sig = m_State_ItemGroup;

	if (sig.current_clip != pNewScissorItem) {

		sig.current_clip = pNewScissorItem;

		if (!m_bDryRun)
		{
			if (sig.current_clip) {
				glEnable(GL_SCISSOR_TEST);
				int y = storage->frame.current_rt->height - (sig.current_clip->final_clip_rect.position.y + sig.current_clip->final_clip_rect.size.y);
				if (storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_VFLIP])
					y = sig.current_clip->final_clip_rect.position.y;
				glScissor(sig.current_clip->final_clip_rect.position.x, y, sig.current_clip->final_clip_rect.size.width, sig.current_clip->final_clip_rect.size.height);
			} else {
				glDisable(GL_SCISSOR_TEST);
			}
		} // dry run
	}
}


void Playback::Playback_Item_CopyBackBuffer(Item *ci)
{
	// TODO: copy back buffer

	if (!m_bDryRun)
	{
		if (ci->copy_back_buffer) {
			if (ci->copy_back_buffer->full) {
				_copy_texscreen(Rect2());
			} else {
				_copy_texscreen(ci->copy_back_buffer->rect);
			}
		}
	} // dry run

}

void Playback::Playback_Item_SkeletonHandling(Item *ci)
{
	State_Item &sit = m_State_Item;
	State_ItemGroup &sig = m_State_ItemGroup;

	sit.skeleton = NULL;

	{
		//skeleton handling
		if (ci->skeleton.is_valid() && storage->skeleton_owner.owns(ci->skeleton)) {
			sit.skeleton = storage->skeleton_owner.get(ci->skeleton);
			if (!sit.skeleton->use_2d) {
				sit.skeleton = NULL;
			} else {
				if (!m_bDryRun)
				{
					state.skeleton_transform = sig.m_pItemGroup->p_base_transform * sit.skeleton->base_transform_2d;
					state.skeleton_transform_inverse = state.skeleton_transform.affine_inverse();
					state.skeleton_texture_size = Vector2(sit.skeleton->size * 2, 0);
				} // dry run
			}
		}

		sit.use_skeleton = sit.skeleton != NULL;
		if (sig.prev_use_skeleton != sit.use_skeleton) {
			sig.rebind_shader = true;
			if (!m_bDryRun)
			{
				state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SKELETON, sit.use_skeleton);
			}
			sig.prev_use_skeleton = sit.use_skeleton;
		}

		if (!m_bDryRun)
		{
			if (sit.skeleton) {
				glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 3);
				glBindTexture(GL_TEXTURE_2D, sit.skeleton->tex_id);
				state.using_skeleton = true;
			} else {
				state.using_skeleton = false;
			}
		} // dry run
	}
}

RasterizerStorageGLES2::Material * Playback::Playback_Item_ChangeMaterial(Item *ci)
{
	State_ItemGroup &sig = m_State_ItemGroup;

	Item *material_owner = ci->material_owner ? ci->material_owner : ci;

	RID material = material_owner->material;
	RasterizerStorageGLES2::Material *material_ptr = storage->material_owner.getornull(material);

	if (material != sig.canvas_last_material || sig.rebind_shader) {

		RasterizerStorageGLES2::Shader *shader_ptr = NULL;

		if (material_ptr) {
			shader_ptr = material_ptr->shader;

			if (shader_ptr && shader_ptr->mode != VS::SHADER_CANVAS_ITEM) {
				shader_ptr = NULL; // not a canvas item shader, don't use.
			}
		}

		if (!m_bDryRun)
		{
			if (shader_ptr) {
				if (shader_ptr->canvas_item.uses_screen_texture) {
					if (!state.canvas_texscreen_used) {
						//copy if not copied before
						_copy_texscreen(Rect2());

						// blend mode will have been enabled so make sure we disable it again later on
						//last_blend_mode = last_blend_mode != RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_DISABLED ? last_blend_mode : -1;
					}

					if (storage->frame.current_rt->copy_screen_effect.color) {
						glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 4);
						glBindTexture(GL_TEXTURE_2D, storage->frame.current_rt->copy_screen_effect.color);
					}
				}

				if (shader_ptr != sig.shader_cache) {

					if (shader_ptr->canvas_item.uses_time) {
						VisualServerRaster::redraw_request();
					}

					state.canvas_shader.set_custom_shader(shader_ptr->custom_code_id);
					state.canvas_shader.bind();
				}

				int tc = material_ptr->textures.size();
				Pair<StringName, RID> *textures = material_ptr->textures.ptrw();

				ShaderLanguage::ShaderNode::Uniform::Hint *texture_hints = shader_ptr->texture_hints.ptrw();

				for (int i = 0; i < tc; i++) {

					glActiveTexture(GL_TEXTURE0 + i);

					RasterizerStorageGLES2::Texture *t = storage->texture_owner.getornull(textures[i].second);

					if (!t) {

						switch (texture_hints[i]) {
						case ShaderLanguage::ShaderNode::Uniform::HINT_BLACK_ALBEDO:
						case ShaderLanguage::ShaderNode::Uniform::HINT_BLACK: {
								glBindTexture(GL_TEXTURE_2D, storage->resources.black_tex);
							} break;
						case ShaderLanguage::ShaderNode::Uniform::HINT_ANISO: {
								glBindTexture(GL_TEXTURE_2D, storage->resources.aniso_tex);
							} break;
						case ShaderLanguage::ShaderNode::Uniform::HINT_NORMAL: {
								glBindTexture(GL_TEXTURE_2D, storage->resources.normal_tex);
							} break;
						default: {
								glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);
							} break;
						}

						continue;
					}

					if (t->redraw_if_visible) {
						VisualServerRaster::redraw_request();
					}

					t = t->get_ptr();

#ifdef TOOLS_ENABLED
					if (t->detect_normal && texture_hints[i] == ShaderLanguage::ShaderNode::Uniform::HINT_NORMAL) {
						t->detect_normal(t->detect_normal_ud);
					}
#endif
					if (t->render_target)
						t->render_target->used_in_frame = true;

					glBindTexture(t->target, t->tex_id);
				}

			} else {
				state.canvas_shader.set_custom_shader(0);
				state.canvas_shader.bind();
			}
			state.canvas_shader.use_material((void *)material_ptr);

		} // dry run

		sig.shader_cache = shader_ptr;

		sig.canvas_last_material = material;

		sig.rebind_shader = false;
	}

	return material_ptr;
}


void Playback::Playback_Item_SetBlendModeAndUniforms(Item * ci)
{
	State_ItemGroup &sig = m_State_ItemGroup;
	const BItemGroup &big = *sig.m_pItemGroup;
	const Color &p_modulate = big.p_modulate;
	State_Item &sit = m_State_Item;

	sit.blend_mode = 0;

	sit.blend_mode = sig.shader_cache ? sig.shader_cache->canvas_item.blend_mode : RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_MIX;
	sit.unshaded = sig.shader_cache && (sig.shader_cache->canvas_item.light_mode == RasterizerStorageGLES2::Shader::CanvasItem::LIGHT_MODE_UNSHADED || (sit.blend_mode != RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_MIX && sit.blend_mode != RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_PMALPHA));

	if (!m_bDryRun)
	{
		if (sig.last_blend_mode != sit.blend_mode) {

			switch (sit.blend_mode) {

			case RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_MIX: {
					glBlendEquation(GL_FUNC_ADD);
					if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
						glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
					} else {
						glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
					}

				} break;
			case RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_ADD: {

					glBlendEquation(GL_FUNC_ADD);
					if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
						glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ONE);
					} else {
						glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
					}

				} break;
			case RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_SUB: {

					glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
					if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
						glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ONE);
					} else {
						glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
					}
				} break;
			case RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_MUL: {
					glBlendEquation(GL_FUNC_ADD);
					if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
						glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_DST_ALPHA, GL_ZERO);
					} else {
						glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_ZERO, GL_ONE);
					}
				} break;
			case RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_PMALPHA: {
					glBlendEquation(GL_FUNC_ADD);
					if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
						glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
					} else {
						glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
					}
				} break;
			}
		}


		state.uniforms.final_modulate = sit.unshaded ? ci->final_modulate : Color(ci->final_modulate.r * p_modulate.r, ci->final_modulate.g * p_modulate.g, ci->final_modulate.b * p_modulate.b, ci->final_modulate.a * p_modulate.a);

		state.uniforms.modelview_matrix = ci->final_transform;
		state.uniforms.extra_matrix = Transform2D();

		_set_uniforms();
	} // if dry run

}

void Playback::Playback_Item_RenderCommandsNormal(Item * ci, RasterizerStorageGLES2::Material * material_ptr)
{
	State_ItemGroup &sig = m_State_ItemGroup;
	State_Item &sit = m_State_Item;

	if (sit.unshaded || (state.uniforms.final_modulate.a > 0.001 && (!sig.shader_cache || sig.shader_cache->canvas_item.light_mode != RasterizerStorageGLES2::Shader::CanvasItem::LIGHT_MODE_LIGHT_ONLY) && !ci->light_masked))
	{
		if (!m_bDryRun)
			_canvas_item_render_commands(ci, NULL, sit.reclip, material_ptr);
		else
			fill_canvas_item_render_commands(ci, NULL, sit.reclip, material_ptr);
	}

	sig.rebind_shader = true; // hacked in for now.

}

void Playback::Playback_Item_ReenableScissor()
{
	State_ItemGroup &sig = m_State_ItemGroup;
	State_Item &sit = m_State_Item;

	if (!m_bDryRun)
	{
		if (sit.reclip) {
			glEnable(GL_SCISSOR_TEST);
			int y = storage->frame.current_rt->height - (sig.current_clip->final_clip_rect.position.y + sig.current_clip->final_clip_rect.size.y);
			if (storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_VFLIP])
				y = sig.current_clip->final_clip_rect.position.y;
			glScissor(sig.current_clip->final_clip_rect.position.x, y, sig.current_clip->final_clip_rect.size.width, sig.current_clip->final_clip_rect.size.height);
		}
	} // dry run
}


void Playback::Playback_Item_ProcessLight(Item * ci, Light * light, bool &light_used, VS::CanvasLightMode &mode, RasterizerStorageGLES2::Material * material_ptr)
{
	State_ItemGroup &sig = m_State_ItemGroup;
	const BItemGroup &big = *sig.m_pItemGroup;
	int p_z = big.p_z;
	State_Item &sit = m_State_Item;


	if (ci->light_mask & light->item_mask && p_z >= light->z_min && p_z <= light->z_max && ci->global_rect_cache.intersects_transformed(light->xform_cache, light->rect_cache)) {

		//intersects this light

		if (!light_used || mode != light->mode) {

			mode = light->mode;

			if (!m_bDryRun)
			{
				switch (mode) {

				case VS::CANVAS_LIGHT_MODE_ADD: {
						glBlendEquation(GL_FUNC_ADD);
						glBlendFunc(GL_SRC_ALPHA, GL_ONE);

					} break;
				case VS::CANVAS_LIGHT_MODE_SUB: {
						glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
						glBlendFunc(GL_SRC_ALPHA, GL_ONE);
					} break;
				case VS::CANVAS_LIGHT_MODE_MIX:
				case VS::CANVAS_LIGHT_MODE_MASK: {
						glBlendEquation(GL_FUNC_ADD);
						glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

					} break;
				}
			} // dry run
		}

		if (!light_used) {

			if (!m_bDryRun)
				state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_LIGHTING, true);
			light_used = true;
		}

		bool has_shadow = light->shadow_buffer.is_valid() && ci->light_mask & light->item_shadow_mask;

		if (!m_bDryRun)
		{
			state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SHADOWS, has_shadow);
			if (has_shadow) {
				state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_USE_GRADIENT, light->shadow_gradient_length > 0);
				state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_NEAREST, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_NONE);
				state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF3, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF3);
				state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF5, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF5);
				state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF7, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF7);
				state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF9, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF9);
				state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF13, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF13);
			}

			state.canvas_shader.bind();
			state.using_light = light;
			state.using_shadow = has_shadow;

			//always re-set uniforms, since light parameters changed
			_set_uniforms();
			state.canvas_shader.use_material((void *)material_ptr);

			glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 4);
			RasterizerStorageGLES2::Texture *t = storage->texture_owner.getornull(light->texture);
			if (!t) {
				glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);
			} else {
				t = t->get_ptr();

				glBindTexture(t->target, t->tex_id);
			}

			glActiveTexture(GL_TEXTURE0);
		} // dry run

		if (!m_bDryRun)
			_canvas_item_render_commands(ci, NULL, sit.reclip, material_ptr); //redraw using light
		else
			fill_canvas_item_render_commands(ci, NULL, sit.reclip, material_ptr); //redraw using light

		state.using_light = NULL;
	}


}

void Playback::Playback_Item_ProcessLights(Item * ci, RasterizerStorageGLES2::Material * material_ptr)
{
	State_ItemGroup &sig = m_State_ItemGroup;
	const BItemGroup &big = *sig.m_pItemGroup;
	State_Item &sit = m_State_Item;

	if ((sit.blend_mode == RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_MIX || sit.blend_mode == RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_PMALPHA) && big.p_light && !sit.unshaded) {

		Light *light = big.p_light;
		bool light_used = false;
		VS::CanvasLightMode mode = VS::CANVAS_LIGHT_MODE_ADD;


		if (!m_bDryRun)
		{
			state.uniforms.final_modulate = ci->final_modulate; // remove the canvas modulate
		}

		while (light)
		{
			Playback_Item_ProcessLight(ci, light, light_used, mode, material_ptr);
			light = light->next_ptr;
		}

		if (light_used) {

			if (!m_bDryRun)
			{
				state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_LIGHTING, false);
				state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SHADOWS, false);
				state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_NEAREST, false);
				state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF3, false);
				state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF5, false);
				state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF7, false);
				state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF9, false);
				state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF13, false);

				state.canvas_shader.bind();
			} // dry run

			sig.last_blend_mode = -1;
		}
	}

}


void Playback::Playback_Change_Item(const Batch &batch)
{
	Item *ci = batch.change_item.m_pItem;

	// alias
//	State_ItemGroup &sig = m_State_ItemGroup;
	State_Item &sit = m_State_Item;
//	const BItemGroup &big = *sig.m_pItemGroup;
//	const Color &p_modulate = big.p_modulate;
//	int p_z = big.p_z;


	Playback_Item_ChangeScissorItem(ci->final_clip_owner);
	Playback_Item_CopyBackBuffer(ci);
	Playback_Item_SkeletonHandling(ci);
	RasterizerStorageGLES2::Material *material_ptr = Playback_Item_ChangeMaterial(ci);
	Playback_Item_SetBlendModeAndUniforms(ci);

	sit.reclip = false;

	Playback_Item_RenderCommandsNormal(ci, material_ptr);
	Playback_Item_ProcessLights(ci, material_ptr);

	Playback_Item_ReenableScissor();

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
		if (m_State_ItemGroup.current_clip)
		{
			glDisable(GL_SCISSOR_TEST);
		}

		state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SKELETON, false);
	} // if not a dry run
}




} // namespace
