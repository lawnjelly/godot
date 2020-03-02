#include "rasterizer_canvas_gles2.h"

#include "core/os/os.h"
#include "core/project_settings.h"
#include "rasterizer_scene_gles2.h"
#include "servers/visual/visual_server_raster.h"

#ifndef GLES_OVER_GL
#define glClearDepth glClearDepthf
#endif


bool g_bUseKessel = true;
//static bool g_bUseKessel = true;
static bool g_bKesselFlash = true;
//#define FLASH_KESSEL




static const GLenum gl_primitive[] = {
	GL_POINTS,
	GL_LINES,
	GL_LINE_STRIP,
	GL_LINE_LOOP,
	GL_TRIANGLES,
	GL_TRIANGLE_STRIP,
	GL_TRIANGLE_FAN
};

void RasterizerCanvasGLES2::canvas_render_items(Item *p_item_list, int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform) {

	if (m_bUseBatching && g_bUseKessel && g_bKesselFlash)
	{
		batch_canvas_render_items(p_item_list, p_z, p_modulate, p_light, p_base_transform);
		return;
	}
	else
	{
		RasterizerCanvasSimpleGLES2::canvas_render_items(p_item_list, p_z, p_modulate, p_light, p_base_transform);
	}

}

void RasterizerCanvasGLES2::canvas_begin() {

#ifdef FLASH_KESSEL
	if (Engine::get_singleton()->get_frames_drawn() % 2)
//	if (g_bKesselFlash)
		g_bKesselFlash = false;
	else
		g_bKesselFlash = true;

//	if (g_bUseKessel)
//		g_bUseKessel = g_bFlash;
//	else
//		g_bUseKessel = true;
#endif


	batch_pass_begin();
	RasterizerCanvasSimpleGLES2::canvas_begin();
}

void RasterizerCanvasGLES2::canvas_end() {
	batch_pass_end();
	RasterizerCanvasSimpleGLES2::canvas_end();
}

Batch::AliasMaterial * RasterizerCanvasGLES2::batch_get_material_and_shader_from_RID(const RID &rid, Batch::AliasShader ** ppShader) const
{
	RasterizerStorageGLES2::Material * pMaterial = storage->material_owner.getornull(rid);

	RasterizerStorageGLES2::Shader *pShader = NULL;

	if (pMaterial) {
		pShader = pMaterial->shader;

		if (pShader && pShader->mode != VS::SHADER_CANVAS_ITEM) {
			pShader = NULL; // not a canvas item shader, don't use.
		}
	}

	*ppShader = (Batch::AliasShader *) pShader;

	return (Batch::AliasMaterial *) pMaterial;
}

RasterizerStorageGLES2::Texture *RasterizerCanvasGLES2::batch_get_canvas_texture(const RID &p_texture) const {
	if (p_texture.is_valid()) {

		RasterizerStorageGLES2::Texture *texture = storage->texture_owner.getornull(p_texture);

		if (texture) {
			return texture->get_ptr();
		}
	}

	return 0;
}

void RasterizerCanvasGLES2::_batch_get_item_clip_rect(const Item &item, int &x, int &y, int &width, int &height) const
{
	y = storage->frame.current_rt->height - (item.final_clip_rect.position.y + item.final_clip_rect.size.y);

	if (storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_VFLIP])
		y = item.final_clip_rect.position.y;

	x = item.final_clip_rect.position.x;
	width = item.final_clip_rect.size.width;
	height = item.final_clip_rect.size.height;
}

// return whether texture found
bool RasterizerCanvasGLES2::batch_get_texture_pixel_size_and_tile(const RID &rid, float &x, float &y, bool p_tile, bool &npot) const
{
	// get the texture
	RasterizerStorageGLES2::Texture *texture = batch_get_canvas_texture(rid);

	if (texture) {
		x = 1.0 / texture->width;
		y = 1.0 / texture->height;
	} else {
		// maybe doesn't need doing...
		x = 1.0;
		y = 1.0;

		return false;
	}

	if (p_tile) {
		if (texture) {
			// default
			npot = false;

			// no hardware support for non power of 2 tiling
			if (!storage->config.support_npot_repeat_mipmap) {
				if (next_power_of_2(texture->alloc_width) != (unsigned int)texture->alloc_width && next_power_of_2(texture->alloc_height) != (unsigned int)texture->alloc_height) {
					npot = true;
				}
			}
		} else {
			// this should not happen?
			//b.tile_mode = BatchTex::TILE_OFF;
		}
	} else {
		//b.tile_mode = BatchTex::TILE_OFF;
	}

	return true;
}


int RasterizerCanvasGLES2::batch_get_blendmode_from_shader(Batch::AliasShader * pShader) const
{
	if (pShader)
	{
		RasterizerStorageGLES2::Shader * pS = (RasterizerStorageGLES2::Shader *) pShader;
		return pS->canvas_item.blend_mode;
	}
	return RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_MIX;
}


void RasterizerCanvasGLES2::_batch_canvas_shader_bind_simple()
{
	if (state.canvas_shader.bind()) {
		//_set_uniforms();
//		state.canvas_shader.set_uniform(CanvasShaderGLES2::FINAL_MODULATE, state.uniforms.final_modulate);
	}
}

void RasterizerCanvasGLES2::_batch_canvas_shader_bind(RasterizerStorageGLES2::Material * p_material, bool use_texture_rect)
{
	state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_TEXTURE_RECT, use_texture_rect);

	if (state.canvas_shader.bind()) {
		_set_uniforms();
		state.canvas_shader.use_material((void *)p_material);
	}
	else
	{
		// shouldn't need to set all uniforms?
		//_set_uniforms();

		// reset the model matrix
		state.canvas_shader.set_uniform(CanvasShaderGLES2::MODELVIEW_MATRIX, state.uniforms.modelview_matrix);
//		if (m_GLState.m_bExtraMatrixSet)
			state.canvas_shader.set_uniform(CanvasShaderGLES2::EXTRA_MATRIX, state.uniforms.extra_matrix);

		// this cured a state bug
		state.canvas_shader.set_uniform(CanvasShaderGLES2::FINAL_MODULATE, state.uniforms.final_modulate);
	}
}

void RasterizerCanvasGLES2::_batch_render_lines(const Batch::Batch &batch, RasterizerStorageGLES2::Material *p_material) {

	const Batch::BatchData &bdata = m_Batcher.m_Data;
//	ERR_FAIL_COND(batch.primitive.num_commands <= 0);
	ERR_FAIL_COND(batch.primitive.num_verts <= 0);

	const bool &colored_verts = bdata.use_colored_vertices;
	int sizeof_vert;
	if (!colored_verts) {
		sizeof_vert = sizeof(Batch::BVert);
	} else {
		sizeof_vert = sizeof(Batch::BVert_colored);
	}

	bool use_identity = true;
	//	if (p_material && p_material->shader && p_material->shader->uses_transform_uniform)
	//		use_identity = false;

	Transform2D extra;
	if (use_identity)
	{
		state.uniforms.modelview_matrix = Transform2D();
		if (m_GLState.m_bExtraMatrixSet)
		{
			extra = state.uniforms.extra_matrix;
			state.uniforms.extra_matrix = Transform2D();
		}
	}
	// should this be after setting extra matrix?
	_batch_canvas_shader_bind(p_material);
	if (use_identity)
	{
		//state.uniforms.modelview_matrix = Transform2D();
		if (m_GLState.m_bExtraMatrixSet)
			state.uniforms.extra_matrix = extra;
	}


	RasterizerStorageGLES2::Texture *texture = _bind_canvas_texture(RID(), RID());

	// bind the index and vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, m_BatchData.gl_vertex_buffer);
//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_BatchData.gl_index_buffer);

//	uint64_t pointer = sizeof_vert * batch.primitive.first_quad * 4;
	uint64_t pointer = 0;
	glVertexAttribPointer(VS::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof_vert, (const void *)pointer);

	if (texture) {
		glVertexAttribPointer(VS::ARRAY_TEX_UV, 2, GL_FLOAT, GL_FALSE, sizeof_vert, CAST_INT_TO_UCHAR_PTR(pointer + (2 * 4)));
		glEnableVertexAttribArray(VS::ARRAY_TEX_UV);
	} else {
		glDisableVertexAttribArray(VS::ARRAY_TEX_UV);
	}

	// color
	if (!colored_verts) {
		glDisableVertexAttribArray(VS::ARRAY_COLOR);
		glVertexAttrib4fv(VS::ARRAY_COLOR, m_GLState.m_Color.components);
	} else {
		glVertexAttribPointer(VS::ARRAY_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof_vert, CAST_INT_TO_UCHAR_PTR(pointer + (4 * 4)));
		glEnableVertexAttribArray(VS::ARRAY_COLOR);
	}


	// we need to convert explicitly from pod Vec2 to Vector2 ...
	// could use a cast but this might be unsafe in future
//	Vector2 tps;
//	tex.tex_pixel_size.to(tps);
//	state.canvas_shader.set_uniform(CanvasShaderGLES2::COLOR_TEXPIXEL_SIZE, tps);

//	int64_t offset = batch.primitive.first_quad * 6 * 2; // 6 inds per quad at 2 bytes each

//	int num_elements = batch.primitive.num_commands * 6;
//	glDrawElements(GL_TRIANGLES, num_elements, GL_UNSIGNED_SHORT, (void *)offset);

//	glDrawArrays(GL_LINES, batch.primitive.first_quad * 4, batch.primitive.num_commands * 4);
	glDrawArrays(GL_LINES, batch.primitive.first_vert, batch.primitive.num_verts);

//	switch (tex.tile_mode) {
//		case Batch::BTex::TILE_FORCE_REPEAT: {
//			state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_FORCE_REPEAT, false);
//		} break;
//		case Batch::BTex::TILE_NORMAL: {
//			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//		} break;
//		default: {
//		} break;
//	}

	glDisableVertexAttribArray(VS::ARRAY_TEX_UV);
	glDisableVertexAttribArray(VS::ARRAY_COLOR);

	// may not be necessary .. state change optimization still TODO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	//	_resend_modelview_matrix();
}


void RasterizerCanvasGLES2::_batch_render_rects(const Batch::Batch &batch, RasterizerStorageGLES2::Material *p_material) {
//		return;
	const Batch::BatchData &bdata = m_Batcher.m_Data;

	ERR_FAIL_COND(batch.primitive.num_quads <= 0);

	const bool &colored_verts = bdata.use_colored_vertices;
	int sizeof_vert;
	if (!colored_verts) {
		sizeof_vert = sizeof(Batch::BVert);
	} else {
		sizeof_vert = sizeof(Batch::BVert_colored);
	}

	// reset matrices to identity (we are using CPU transform)
//	state.uniforms.modelview_matrix = m_GLState.m_matModelView;
//	Transform2D temp = state.uniforms.modelview_matrix;

	bool use_identity = true;
	//	if (p_material && p_material->shader && p_material->shader->uses_transform_uniform)
	//		use_identity = false;

	Transform2D extra;
	if (use_identity)
	{
		state.uniforms.modelview_matrix = Transform2D();
		if (m_GLState.m_bExtraMatrixSet)
		{
			extra = state.uniforms.extra_matrix;
			state.uniforms.extra_matrix = Transform2D();
		}
	}
	// should this be after setting extra matrix?
	_batch_canvas_shader_bind(p_material);
	if (use_identity)
	{
		//state.uniforms.modelview_matrix = Transform2D();
		if (m_GLState.m_bExtraMatrixSet)
			state.uniforms.extra_matrix = extra;
	}


	// no need to reset!!
	//	state.uniforms.modelview_matrix = temp;

	/*
		state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_TEXTURE_RECT, false);
		if (state.canvas_shader.bind()) {
			_set_uniforms();
			state.uniforms.modelview_matrix = temp;
			state.canvas_shader.use_material((void *)p_material);
		}
		*/

	// batch tex
	const Batch::BTex &tex = bdata.batch_textures[batch.primitive.batch_texture_id];

	switch (tex.tile_mode) {
		case Batch::BTex::TILE_FORCE_REPEAT: {
			state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_FORCE_REPEAT, true);
		} break;
		case Batch::BTex::TILE_NORMAL: {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		} break;
		default: {
		} break;
	}


	RasterizerStorageGLES2::Texture *texture = _bind_canvas_texture(tex.RID_texture, tex.RID_normal);

	// bind the index and vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, m_BatchData.gl_vertex_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_BatchData.gl_index_buffer);

	uint64_t pointer = 0;
	glVertexAttribPointer(VS::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof_vert, (const void *)pointer);

	if (texture) {
		glVertexAttribPointer(VS::ARRAY_TEX_UV, 2, GL_FLOAT, GL_FALSE, sizeof_vert, CAST_INT_TO_UCHAR_PTR(pointer + (2 * 4)));
		glEnableVertexAttribArray(VS::ARRAY_TEX_UV);
	} else {
		glDisableVertexAttribArray(VS::ARRAY_TEX_UV);
	}

	// color
	if (!colored_verts) {
		glDisableVertexAttribArray(VS::ARRAY_COLOR);
		glVertexAttrib4fv(VS::ARRAY_COLOR, m_GLState.m_Color.components);
	} else {
		glVertexAttribPointer(VS::ARRAY_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof_vert, CAST_INT_TO_UCHAR_PTR(pointer + (4 * 4)));
		glEnableVertexAttribArray(VS::ARRAY_COLOR);
	}


	// we need to convert explicitly from pod Vec2 to Vector2 ...
	// could use a cast but this might be unsafe in future
	Vector2 tps;
	tex.tex_pixel_size.to(tps);
	state.canvas_shader.set_uniform(CanvasShaderGLES2::COLOR_TEXPIXEL_SIZE, tps);

	int64_t offset = batch.primitive.first_quad * 6 * 2; // 6 inds per quad at 2 bytes each
//	int64_t offset = batch.primitive.first_vert * 6 * 2; // 6 inds per quad at 2 bytes each

	int num_elements = batch.primitive.num_quads * 6;
	glDrawElements(GL_TRIANGLES, num_elements, GL_UNSIGNED_SHORT, (void *)offset);

	switch (tex.tile_mode) {
		case Batch::BTex::TILE_FORCE_REPEAT: {
			state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_FORCE_REPEAT, false);
		} break;
		case Batch::BTex::TILE_NORMAL: {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		} break;
		default: {
		} break;
	}

	glDisableVertexAttribArray(VS::ARRAY_TEX_UV);
	glDisableVertexAttribArray(VS::ARRAY_COLOR);

	// may not be necessary .. state change optimization still TODO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	//	_resend_modelview_matrix();
}

void RasterizerCanvasGLES2::_batch_light_begin(Batch::AliasLight * pLight, Item *p_item)
{
	RasterizerCanvas::Item * ci = m_GLState.m_pItem;

	state.uniforms.final_modulate = ci->final_modulate; // remove the canvas modulate

	//intersects this light
	Light *light = (Light *) pLight;
//	if (!light_used || mode != light->mode) {

		VS::CanvasLightMode mode = light->mode;

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
//	}

//	if (!light_used) {

		state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_LIGHTING, true);
	//	light_used = true;
	//}




	bool has_shadow = light->shadow_buffer.is_valid() && ci->light_mask & light->item_shadow_mask;

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

	_batch_canvas_shader_bind_simple();
	//state.canvas_shader.bind();
	state.using_light = light;
	state.using_shadow = has_shadow;

	//always re-set uniforms, since light parameters changed
	_set_uniforms();

//	state.canvas_shader.use_material((void *)material_ptr);
	state.canvas_shader.use_material((void *)m_GLState.m_pMaterial);

	glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 4);
	RasterizerStorageGLES2::Texture *t = storage->texture_owner.getornull(light->texture);
	if (!t) {
		glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);
	} else {
		t = t->get_ptr();

		glBindTexture(t->target, t->tex_id);
	}

	glActiveTexture(GL_TEXTURE0);
}

void RasterizerCanvasGLES2::_batch_light_end()
{
	state.using_light = NULL;

	state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_LIGHTING, false);
	state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SHADOWS, false);
	state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_NEAREST, false);
	state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF3, false);
	state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF5, false);
	state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF7, false);
	state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF9, false);
	state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF13, false);

//	state.canvas_shader.bind();
	_batch_canvas_shader_bind_simple();
}


void RasterizerCanvasGLES2::_batch_unset_extra_matrix()
{
	state.uniforms.extra_matrix = Transform2D();
}


//void RasterizerCanvasGLES2::_batch_change_color_modulate(const Color &col, bool bRedundant)
void RasterizerCanvasGLES2::_batch_change_color_modulate(const Color &col)
{
//	if (bRedundant)
//	{
//		if (col != state.uniforms.final_modulate)
//		{
//			WARN_PRINT_ONCE("Redundant color change changed uniform");
//		}
//		//state.canvas_shader.set_uniform(CanvasShaderGLES2::FINAL_MODULATE, state.uniforms.final_modulate);
//		return;
//	}

	state.uniforms.final_modulate = col;
	state.canvas_shader.set_uniform(CanvasShaderGLES2::FINAL_MODULATE, state.uniforms.final_modulate);
}

void RasterizerCanvasGLES2::_batch_change_blend_mode(int p_blend_mode)
{
//	if (last_blend_mode != blend_mode) {

		switch (p_blend_mode) {

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
	//}


}

void RasterizerCanvasGLES2::_batch_change_material()
{
//	if (material != canvas_last_material || rebind_shader) {

	// an alias
	RasterizerStorageGLES2::Material * material_ptr = m_GLState.m_pMaterial;
	RasterizerStorageGLES2::Shader *shader_ptr = m_GLState.m_pShader;

//		if (material_ptr) {
//			shader_ptr = material_ptr->shader;

//			if (shader_ptr && shader_ptr->mode != VS::SHADER_CANVAS_ITEM) {
//				shader_ptr = NULL; // not a canvas item shader, don't use.
//			}
//		}

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

			if (m_GLState.m_bChangedShader) {
//			if (shader_ptr != shader_cache) {

				if (shader_ptr->canvas_item.uses_time) {
					VisualServerRaster::redraw_request();
				}

				state.canvas_shader.set_custom_shader(shader_ptr->custom_code_id);
				//state.canvas_shader.bind();
				_batch_canvas_shader_bind_simple();
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
			//state.canvas_shader.bind();
			_batch_canvas_shader_bind_simple();
		}
		state.canvas_shader.use_material((void *)material_ptr);

		m_GLState.m_bChangedShader = false;
//		shader_cache = shader_ptr;

	//	canvas_last_material = material;

		//rebind_shader = false;
//	}
}

void RasterizerCanvasGLES2::_batch_copy_back_buffer(const Rect2 &rect)
{
	_copy_texscreen(rect);
}

void RasterizerCanvasGLES2::_batch_render_default_batch(int batch_id, Item *p_item, Item *current_clip, bool &reclip, RasterizerStorageGLES2::Material *p_material) {

//	return;

	const Batch::Batch &batch = m_Batcher.m_Data.batches[batch_id];

	// set the model view
	state.uniforms.modelview_matrix = m_GLState.m_matModelView;

	// test test test
	state.canvas_shader.set_uniform(CanvasShaderGLES2::MODELVIEW_MATRIX, state.uniforms.modelview_matrix);



//	int command_count = p_item->commands.size();
//	int command_start = 0;

//	Item::Command **commands = p_item->commands.ptrw();
	Item::Command * const *commands = p_item->commands.ptr();

//		for (int batch_num = 0; batch_num < num_batches; batch_num++)
//		{
//			const Batch &batch = bdata.batches[batch_num];

			switch (batch.type) {
				default: {
					CRASH_COND(batch.type != Batch::Batch::BT_DEFAULT);
				} break;
				case Batch::Batch::BT_DEFAULT: {
					int end_command = batch.def.first_command + batch.def.num_commands;

					for (int i = batch.def.first_command; i < end_command; i++) {

						Item::Command *command = commands[i];

						switch (command->type) {

							case Item::Command::TYPE_LINE: {

								Item::CommandLine *line = static_cast<Item::CommandLine *>(command);

								state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_TEXTURE_RECT, false);
								if (state.canvas_shader.bind()) {
									_set_uniforms();
									state.canvas_shader.use_material((void *)p_material);
								}

								_bind_canvas_texture(RID(), RID());

								glDisableVertexAttribArray(VS::ARRAY_COLOR);
								glVertexAttrib4fv(VS::ARRAY_COLOR, line->color.components);

								state.canvas_shader.set_uniform(CanvasShaderGLES2::MODELVIEW_MATRIX, state.uniforms.modelview_matrix);

								if (line->width <= 1) {
									Vector2 verts[2] = {
										Vector2(line->from.x, line->from.y),
										Vector2(line->to.x, line->to.y)
									};

#ifdef GLES_OVER_GL
									if (line->antialiased)
										glEnable(GL_LINE_SMOOTH);
#endif
									_draw_gui_primitive(2, verts, NULL, NULL);

#ifdef GLES_OVER_GL
									if (line->antialiased)
										glDisable(GL_LINE_SMOOTH);
#endif
								} else {
									Vector2 t = (line->from - line->to).normalized().tangent() * line->width * 0.5;

									Vector2 verts[4] = {
										line->from - t,
										line->from + t,
										line->to + t,
										line->to - t
									};

									_draw_gui_primitive(4, verts, NULL, NULL);
#ifdef GLES_OVER_GL
									if (line->antialiased) {
										glEnable(GL_LINE_SMOOTH);
										for (int j = 0; j < 4; j++) {
											Vector2 vertsl[2] = {
												verts[j],
												verts[(j + 1) % 4],
											};
											_draw_gui_primitive(2, vertsl, NULL, NULL);
										}
										glDisable(GL_LINE_SMOOTH);
									}
#endif
								}
							} break;

							case Item::Command::TYPE_RECT: {

								Item::CommandRect *r = static_cast<Item::CommandRect *>(command);

								glDisableVertexAttribArray(VS::ARRAY_COLOR);
								glVertexAttrib4fv(VS::ARRAY_COLOR, r->modulate.components);

								bool can_tile = true;
								if (r->texture.is_valid() && r->flags & CANVAS_RECT_TILE && !storage->config.support_npot_repeat_mipmap) {
									// workaround for when setting tiling does not work due to hardware limitation

									RasterizerStorageGLES2::Texture *texture = storage->texture_owner.getornull(r->texture);

									if (texture) {

										texture = texture->get_ptr();

										if (next_power_of_2(texture->alloc_width) != (unsigned int)texture->alloc_width && next_power_of_2(texture->alloc_height) != (unsigned int)texture->alloc_height) {
											state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_FORCE_REPEAT, true);
											can_tile = false;
										}
									}
								}

								// On some widespread Nvidia cards, the normal draw method can produce some
								// flickering in draw_rect and especially TileMap rendering (tiles randomly flicker).
								// See GH-9913.
								// To work it around, we use a simpler draw method which does not flicker, but gives
								// a non negligible performance hit, so it's opt-in (GH-24466).
								if (use_nvidia_rect_workaround) {
									state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_TEXTURE_RECT, false);

									if (state.canvas_shader.bind()) {
										_set_uniforms();
										state.canvas_shader.use_material((void *)p_material);
									}

									Vector2 points[4] = {
										r->rect.position,
										r->rect.position + Vector2(r->rect.size.x, 0.0),
										r->rect.position + r->rect.size,
										r->rect.position + Vector2(0.0, r->rect.size.y),
									};

									if (r->rect.size.x < 0) {
										SWAP(points[0], points[1]);
										SWAP(points[2], points[3]);
									}
									if (r->rect.size.y < 0) {
										SWAP(points[0], points[3]);
										SWAP(points[1], points[2]);
									}

									RasterizerStorageGLES2::Texture *texture = _bind_canvas_texture(r->texture, r->normal_map);

									if (texture) {
										Size2 texpixel_size(1.0 / texture->width, 1.0 / texture->height);

										Rect2 src_rect = (r->flags & CANVAS_RECT_REGION) ? Rect2(r->source.position * texpixel_size, r->source.size * texpixel_size) : Rect2(0, 0, 1, 1);

										Vector2 uvs[4] = {
											src_rect.position,
											src_rect.position + Vector2(src_rect.size.x, 0.0),
											src_rect.position + src_rect.size,
											src_rect.position + Vector2(0.0, src_rect.size.y),
										};

										if (r->flags & CANVAS_RECT_TRANSPOSE) {
											SWAP(uvs[1], uvs[3]);
										}

										if (r->flags & CANVAS_RECT_FLIP_H) {
											SWAP(uvs[0], uvs[1]);
											SWAP(uvs[2], uvs[3]);
										}
										if (r->flags & CANVAS_RECT_FLIP_V) {
											SWAP(uvs[0], uvs[3]);
											SWAP(uvs[1], uvs[2]);
										}

										state.canvas_shader.set_uniform(CanvasShaderGLES2::COLOR_TEXPIXEL_SIZE, texpixel_size);

										bool untile = false;

										if (can_tile && r->flags & CANVAS_RECT_TILE && !(texture->flags & VS::TEXTURE_FLAG_REPEAT)) {
											glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
											glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
											untile = true;
										}

										_draw_gui_primitive(4, points, NULL, uvs);

										if (untile) {
											glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
											glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
										}
									} else {
										static const Vector2 uvs[4] = {
											Vector2(0.0, 0.0),
											Vector2(0.0, 1.0),
											Vector2(1.0, 1.0),
											Vector2(1.0, 0.0),
										};

										state.canvas_shader.set_uniform(CanvasShaderGLES2::COLOR_TEXPIXEL_SIZE, Vector2());
										_draw_gui_primitive(4, points, NULL, uvs);
									}

								} else {
									// This branch is better for performance, but can produce flicker on Nvidia, see above comment.
									_bind_quad_buffer();

									state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_TEXTURE_RECT, true);

									if (state.canvas_shader.bind()) {
										_set_uniforms();
										state.canvas_shader.use_material((void *)p_material);
									}

									RasterizerStorageGLES2::Texture *tex = _bind_canvas_texture(r->texture, r->normal_map);

									if (!tex) {
										Rect2 dst_rect = Rect2(r->rect.position, r->rect.size);

										if (dst_rect.size.width < 0) {
											dst_rect.position.x += dst_rect.size.width;
											dst_rect.size.width *= -1;
										}
										if (dst_rect.size.height < 0) {
											dst_rect.position.y += dst_rect.size.height;
											dst_rect.size.height *= -1;
										}

										state.canvas_shader.set_uniform(CanvasShaderGLES2::DST_RECT, Color(dst_rect.position.x, dst_rect.position.y, dst_rect.size.x, dst_rect.size.y));
										state.canvas_shader.set_uniform(CanvasShaderGLES2::SRC_RECT, Color(0, 0, 1, 1));

										glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
									} else {

										bool untile = false;

										if (can_tile && r->flags & CANVAS_RECT_TILE && !(tex->flags & VS::TEXTURE_FLAG_REPEAT)) {
											glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
											glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
											untile = true;
										}

										Size2 texpixel_size(1.0 / tex->width, 1.0 / tex->height);
										Rect2 src_rect = (r->flags & CANVAS_RECT_REGION) ? Rect2(r->source.position * texpixel_size, r->source.size * texpixel_size) : Rect2(0, 0, 1, 1);

										Rect2 dst_rect = Rect2(r->rect.position, r->rect.size);

										if (dst_rect.size.width < 0) {
											dst_rect.position.x += dst_rect.size.width;
											dst_rect.size.width *= -1;
										}
										if (dst_rect.size.height < 0) {
											dst_rect.position.y += dst_rect.size.height;
											dst_rect.size.height *= -1;
										}

										if (r->flags & CANVAS_RECT_FLIP_H) {
											src_rect.size.x *= -1;
										}

										if (r->flags & CANVAS_RECT_FLIP_V) {
											src_rect.size.y *= -1;
										}

										if (r->flags & CANVAS_RECT_TRANSPOSE) {
											dst_rect.size.x *= -1; // Encoding in the dst_rect.z uniform
										}

										state.canvas_shader.set_uniform(CanvasShaderGLES2::COLOR_TEXPIXEL_SIZE, texpixel_size);

										state.canvas_shader.set_uniform(CanvasShaderGLES2::DST_RECT, Color(dst_rect.position.x, dst_rect.position.y, dst_rect.size.x, dst_rect.size.y));
										state.canvas_shader.set_uniform(CanvasShaderGLES2::SRC_RECT, Color(src_rect.position.x, src_rect.position.y, src_rect.size.x, src_rect.size.y));

										glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

										if (untile) {
											glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
											glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
										}
									}

									glBindBuffer(GL_ARRAY_BUFFER, 0);
									glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
								}

								state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_FORCE_REPEAT, false);

							} break;

							case Item::Command::TYPE_NINEPATCH: {

								Item::CommandNinePatch *np = static_cast<Item::CommandNinePatch *>(command);

								state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_TEXTURE_RECT, false);
								if (state.canvas_shader.bind()) {
									_set_uniforms();
									state.canvas_shader.use_material((void *)p_material);
								}

								glDisableVertexAttribArray(VS::ARRAY_COLOR);
								glVertexAttrib4fv(VS::ARRAY_COLOR, np->color.components);

								RasterizerStorageGLES2::Texture *tex = _bind_canvas_texture(np->texture, np->normal_map);

								if (!tex) {
									// FIXME: Handle textureless ninepatch gracefully
									WARN_PRINT("NinePatch without texture not supported yet in GLES2 backend, skipping.");
									continue;
								}
								if (tex->width == 0 || tex->height == 0) {
									WARN_PRINT("Cannot set empty texture to NinePatch.");
									continue;
								}

								Size2 texpixel_size(1.0 / tex->width, 1.0 / tex->height);

								// state.canvas_shader.set_uniform(CanvasShaderGLES2::MODELVIEW_MATRIX, state.uniforms.modelview_matrix);
								state.canvas_shader.set_uniform(CanvasShaderGLES2::COLOR_TEXPIXEL_SIZE, texpixel_size);

								Rect2 source = np->source;
								if (source.size.x == 0 && source.size.y == 0) {
									source.size.x = tex->width;
									source.size.y = tex->height;
								}

								float screen_scale = 1.0;

								if (source.size.x != 0 && source.size.y != 0) {

									screen_scale = MIN(np->rect.size.x / source.size.x, np->rect.size.y / source.size.y);
									screen_scale = MIN(1.0, screen_scale);
								}

								// prepare vertex buffer

								// this buffer contains [ POS POS UV UV ] *

								float buffer[16 * 2 + 16 * 2];

								{

									// first row

									buffer[(0 * 4 * 4) + 0] = np->rect.position.x;
									buffer[(0 * 4 * 4) + 1] = np->rect.position.y;

									buffer[(0 * 4 * 4) + 2] = source.position.x * texpixel_size.x;
									buffer[(0 * 4 * 4) + 3] = source.position.y * texpixel_size.y;

									buffer[(0 * 4 * 4) + 4] = np->rect.position.x + np->margin[MARGIN_LEFT] * screen_scale;
									buffer[(0 * 4 * 4) + 5] = np->rect.position.y;

									buffer[(0 * 4 * 4) + 6] = (source.position.x + np->margin[MARGIN_LEFT]) * texpixel_size.x;
									buffer[(0 * 4 * 4) + 7] = source.position.y * texpixel_size.y;

									buffer[(0 * 4 * 4) + 8] = np->rect.position.x + np->rect.size.x - np->margin[MARGIN_RIGHT] * screen_scale;
									buffer[(0 * 4 * 4) + 9] = np->rect.position.y;

									buffer[(0 * 4 * 4) + 10] = (source.position.x + source.size.x - np->margin[MARGIN_RIGHT]) * texpixel_size.x;
									buffer[(0 * 4 * 4) + 11] = source.position.y * texpixel_size.y;

									buffer[(0 * 4 * 4) + 12] = np->rect.position.x + np->rect.size.x;
									buffer[(0 * 4 * 4) + 13] = np->rect.position.y;

									buffer[(0 * 4 * 4) + 14] = (source.position.x + source.size.x) * texpixel_size.x;
									buffer[(0 * 4 * 4) + 15] = source.position.y * texpixel_size.y;

									// second row

									buffer[(1 * 4 * 4) + 0] = np->rect.position.x;
									buffer[(1 * 4 * 4) + 1] = np->rect.position.y + np->margin[MARGIN_TOP] * screen_scale;

									buffer[(1 * 4 * 4) + 2] = source.position.x * texpixel_size.x;
									buffer[(1 * 4 * 4) + 3] = (source.position.y + np->margin[MARGIN_TOP]) * texpixel_size.y;

									buffer[(1 * 4 * 4) + 4] = np->rect.position.x + np->margin[MARGIN_LEFT] * screen_scale;
									buffer[(1 * 4 * 4) + 5] = np->rect.position.y + np->margin[MARGIN_TOP] * screen_scale;

									buffer[(1 * 4 * 4) + 6] = (source.position.x + np->margin[MARGIN_LEFT]) * texpixel_size.x;
									buffer[(1 * 4 * 4) + 7] = (source.position.y + np->margin[MARGIN_TOP]) * texpixel_size.y;

									buffer[(1 * 4 * 4) + 8] = np->rect.position.x + np->rect.size.x - np->margin[MARGIN_RIGHT] * screen_scale;
									buffer[(1 * 4 * 4) + 9] = np->rect.position.y + np->margin[MARGIN_TOP] * screen_scale;

									buffer[(1 * 4 * 4) + 10] = (source.position.x + source.size.x - np->margin[MARGIN_RIGHT]) * texpixel_size.x;
									buffer[(1 * 4 * 4) + 11] = (source.position.y + np->margin[MARGIN_TOP]) * texpixel_size.y;

									buffer[(1 * 4 * 4) + 12] = np->rect.position.x + np->rect.size.x;
									buffer[(1 * 4 * 4) + 13] = np->rect.position.y + np->margin[MARGIN_TOP] * screen_scale;

									buffer[(1 * 4 * 4) + 14] = (source.position.x + source.size.x) * texpixel_size.x;
									buffer[(1 * 4 * 4) + 15] = (source.position.y + np->margin[MARGIN_TOP]) * texpixel_size.y;

									// third row

									buffer[(2 * 4 * 4) + 0] = np->rect.position.x;
									buffer[(2 * 4 * 4) + 1] = np->rect.position.y + np->rect.size.y - np->margin[MARGIN_BOTTOM] * screen_scale;

									buffer[(2 * 4 * 4) + 2] = source.position.x * texpixel_size.x;
									buffer[(2 * 4 * 4) + 3] = (source.position.y + source.size.y - np->margin[MARGIN_BOTTOM]) * texpixel_size.y;

									buffer[(2 * 4 * 4) + 4] = np->rect.position.x + np->margin[MARGIN_LEFT] * screen_scale;
									buffer[(2 * 4 * 4) + 5] = np->rect.position.y + np->rect.size.y - np->margin[MARGIN_BOTTOM] * screen_scale;

									buffer[(2 * 4 * 4) + 6] = (source.position.x + np->margin[MARGIN_LEFT]) * texpixel_size.x;
									buffer[(2 * 4 * 4) + 7] = (source.position.y + source.size.y - np->margin[MARGIN_BOTTOM]) * texpixel_size.y;

									buffer[(2 * 4 * 4) + 8] = np->rect.position.x + np->rect.size.x - np->margin[MARGIN_RIGHT] * screen_scale;
									buffer[(2 * 4 * 4) + 9] = np->rect.position.y + np->rect.size.y - np->margin[MARGIN_BOTTOM] * screen_scale;

									buffer[(2 * 4 * 4) + 10] = (source.position.x + source.size.x - np->margin[MARGIN_RIGHT]) * texpixel_size.x;
									buffer[(2 * 4 * 4) + 11] = (source.position.y + source.size.y - np->margin[MARGIN_BOTTOM]) * texpixel_size.y;

									buffer[(2 * 4 * 4) + 12] = np->rect.position.x + np->rect.size.x;
									buffer[(2 * 4 * 4) + 13] = np->rect.position.y + np->rect.size.y - np->margin[MARGIN_BOTTOM] * screen_scale;

									buffer[(2 * 4 * 4) + 14] = (source.position.x + source.size.x) * texpixel_size.x;
									buffer[(2 * 4 * 4) + 15] = (source.position.y + source.size.y - np->margin[MARGIN_BOTTOM]) * texpixel_size.y;

									// fourth row

									buffer[(3 * 4 * 4) + 0] = np->rect.position.x;
									buffer[(3 * 4 * 4) + 1] = np->rect.position.y + np->rect.size.y;

									buffer[(3 * 4 * 4) + 2] = source.position.x * texpixel_size.x;
									buffer[(3 * 4 * 4) + 3] = (source.position.y + source.size.y) * texpixel_size.y;

									buffer[(3 * 4 * 4) + 4] = np->rect.position.x + np->margin[MARGIN_LEFT] * screen_scale;
									buffer[(3 * 4 * 4) + 5] = np->rect.position.y + np->rect.size.y;

									buffer[(3 * 4 * 4) + 6] = (source.position.x + np->margin[MARGIN_LEFT]) * texpixel_size.x;
									buffer[(3 * 4 * 4) + 7] = (source.position.y + source.size.y) * texpixel_size.y;

									buffer[(3 * 4 * 4) + 8] = np->rect.position.x + np->rect.size.x - np->margin[MARGIN_RIGHT] * screen_scale;
									buffer[(3 * 4 * 4) + 9] = np->rect.position.y + np->rect.size.y;

									buffer[(3 * 4 * 4) + 10] = (source.position.x + source.size.x - np->margin[MARGIN_RIGHT]) * texpixel_size.x;
									buffer[(3 * 4 * 4) + 11] = (source.position.y + source.size.y) * texpixel_size.y;

									buffer[(3 * 4 * 4) + 12] = np->rect.position.x + np->rect.size.x;
									buffer[(3 * 4 * 4) + 13] = np->rect.position.y + np->rect.size.y;

									buffer[(3 * 4 * 4) + 14] = (source.position.x + source.size.x) * texpixel_size.x;
									buffer[(3 * 4 * 4) + 15] = (source.position.y + source.size.y) * texpixel_size.y;
								}

								glBindBuffer(GL_ARRAY_BUFFER, data.ninepatch_vertices);
								glBufferData(GL_ARRAY_BUFFER, sizeof(float) * (16 + 16) * 2, buffer, GL_DYNAMIC_DRAW);

								glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ninepatch_elements);

								glEnableVertexAttribArray(VS::ARRAY_VERTEX);
								glEnableVertexAttribArray(VS::ARRAY_TEX_UV);

								glVertexAttribPointer(VS::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), NULL);
								glVertexAttribPointer(VS::ARRAY_TEX_UV, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), CAST_INT_TO_UCHAR_PTR((sizeof(float) * 2)));

								glDrawElements(GL_TRIANGLES, 18 * 3 - (np->draw_center ? 0 : 6), GL_UNSIGNED_BYTE, NULL);

								glBindBuffer(GL_ARRAY_BUFFER, 0);
								glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

							} break;

							case Item::Command::TYPE_CIRCLE: {

								Item::CommandCircle *circle = static_cast<Item::CommandCircle *>(command);

								state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_TEXTURE_RECT, false);

								if (state.canvas_shader.bind()) {
									_set_uniforms();
									state.canvas_shader.use_material((void *)p_material);
								}

								static const int num_points = 32;

								Vector2 points[num_points + 1];
								points[num_points] = circle->pos;

								int indices[num_points * 3];

								for (int j = 0; j < num_points; j++) {
									points[j] = circle->pos + Vector2(Math::sin(j * Math_PI * 2.0 / num_points), Math::cos(j * Math_PI * 2.0 / num_points)) * circle->radius;
									indices[j * 3 + 0] = j;
									indices[j * 3 + 1] = (j + 1) % num_points;
									indices[j * 3 + 2] = num_points;
								}

								_bind_canvas_texture(RID(), RID());

								_draw_polygon(indices, num_points * 3, num_points + 1, points, NULL, &circle->color, true);
							} break;

							case Item::Command::TYPE_POLYGON: {

								Item::CommandPolygon *polygon = static_cast<Item::CommandPolygon *>(command);

								state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_TEXTURE_RECT, false);

								if (state.canvas_shader.bind()) {
									_set_uniforms();
									state.canvas_shader.use_material((void *)p_material);
								}

								RasterizerStorageGLES2::Texture *texture = _bind_canvas_texture(polygon->texture, polygon->normal_map);

								if (texture) {
									Size2 texpixel_size(1.0 / texture->width, 1.0 / texture->height);
									state.canvas_shader.set_uniform(CanvasShaderGLES2::COLOR_TEXPIXEL_SIZE, texpixel_size);
								}

								_draw_polygon(polygon->indices.ptr(), polygon->count, polygon->points.size(), polygon->points.ptr(), polygon->uvs.ptr(), polygon->colors.ptr(), polygon->colors.size() == 1, polygon->weights.ptr(), polygon->bones.ptr());
#ifdef GLES_OVER_GL
								if (polygon->antialiased) {
									glEnable(GL_LINE_SMOOTH);
									if (polygon->antialiasing_use_indices) {
										_draw_generic_indices(GL_LINE_STRIP, polygon->indices.ptr(), polygon->count, polygon->points.size(), polygon->points.ptr(), polygon->uvs.ptr(), polygon->colors.ptr(), polygon->colors.size() == 1);
									} else {
										_draw_generic(GL_LINE_LOOP, polygon->points.size(), polygon->points.ptr(), polygon->uvs.ptr(), polygon->colors.ptr(), polygon->colors.size() == 1);
									}
									glDisable(GL_LINE_SMOOTH);
								}
#endif
							} break;
							case Item::Command::TYPE_MESH: {

								Item::CommandMesh *mesh = static_cast<Item::CommandMesh *>(command);
								state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_TEXTURE_RECT, false);

								if (state.canvas_shader.bind()) {
									_set_uniforms();
									state.canvas_shader.use_material((void *)p_material);
								}

								RasterizerStorageGLES2::Texture *texture = _bind_canvas_texture(mesh->texture, mesh->normal_map);

								if (texture) {
									Size2 texpixel_size(1.0 / texture->width, 1.0 / texture->height);
									state.canvas_shader.set_uniform(CanvasShaderGLES2::COLOR_TEXPIXEL_SIZE, texpixel_size);
								}

								RasterizerStorageGLES2::Mesh *mesh_data = storage->mesh_owner.getornull(mesh->mesh);
								if (mesh_data) {

									for (int j = 0; j < mesh_data->surfaces.size(); j++) {
										RasterizerStorageGLES2::Surface *s = mesh_data->surfaces[j];
										// materials are ignored in 2D meshes, could be added but many things (ie, lighting mode, reading from screen, etc) would break as they are not meant be set up at this point of drawing

										glBindBuffer(GL_ARRAY_BUFFER, s->vertex_id);

										if (s->index_array_len > 0) {
											glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s->index_id);
										}

										for (int k = 0; k < VS::ARRAY_MAX - 1; k++) {
											if (s->attribs[k].enabled) {
												glEnableVertexAttribArray(k);
												glVertexAttribPointer(s->attribs[k].index, s->attribs[k].size, s->attribs[k].type, s->attribs[k].normalized, s->attribs[k].stride, CAST_INT_TO_UCHAR_PTR(s->attribs[k].offset));
											} else {
												glDisableVertexAttribArray(k);
												switch (k) {
													case VS::ARRAY_NORMAL: {
														glVertexAttrib4f(VS::ARRAY_NORMAL, 0.0, 0.0, 1, 1);
													} break;
													case VS::ARRAY_COLOR: {
														glVertexAttrib4f(VS::ARRAY_COLOR, 1, 1, 1, 1);

													} break;
													default: {
													}
												}
											}
										}

										if (s->index_array_len > 0) {
											glDrawElements(gl_primitive[s->primitive], s->index_array_len, (s->array_len >= (1 << 16)) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT, 0);
										} else {
											glDrawArrays(gl_primitive[s->primitive], 0, s->array_len);
										}
									}

									for (int j = 1; j < VS::ARRAY_MAX - 1; j++) {
										glDisableVertexAttribArray(j);
									}
								}

							} break;
							case Item::Command::TYPE_MULTIMESH: {
								Item::CommandMultiMesh *mmesh = static_cast<Item::CommandMultiMesh *>(command);

								RasterizerStorageGLES2::MultiMesh *multi_mesh = storage->multimesh_owner.getornull(mmesh->multimesh);

								if (!multi_mesh)
									break;

								RasterizerStorageGLES2::Mesh *mesh_data = storage->mesh_owner.getornull(multi_mesh->mesh);

								if (!mesh_data)
									break;

								state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_INSTANCE_CUSTOM, multi_mesh->custom_data_format != VS::MULTIMESH_CUSTOM_DATA_NONE);
								state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_INSTANCING, true);
								state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_TEXTURE_RECT, false);

								if (state.canvas_shader.bind()) {
									_set_uniforms();
									state.canvas_shader.use_material((void *)p_material);
								}

								RasterizerStorageGLES2::Texture *texture = _bind_canvas_texture(mmesh->texture, mmesh->normal_map);

								if (texture) {
									Size2 texpixel_size(1.0 / texture->width, 1.0 / texture->height);
									state.canvas_shader.set_uniform(CanvasShaderGLES2::COLOR_TEXPIXEL_SIZE, texpixel_size);
								}

								//reset shader and force rebind

								int amount = MIN(multi_mesh->size, multi_mesh->visible_instances);

								if (amount == -1) {
									amount = multi_mesh->size;
								}

								int stride = multi_mesh->color_floats + multi_mesh->custom_data_floats + multi_mesh->xform_floats;

								int color_ofs = multi_mesh->xform_floats;
								int custom_data_ofs = color_ofs + multi_mesh->color_floats;

								// drawing

								const float *base_buffer = multi_mesh->data.ptr();

								for (int j = 0; j < mesh_data->surfaces.size(); j++) {
									RasterizerStorageGLES2::Surface *s = mesh_data->surfaces[j];
									// materials are ignored in 2D meshes, could be added but many things (ie, lighting mode, reading from screen, etc) would break as they are not meant be set up at this point of drawing

									//bind buffers for mesh surface
									glBindBuffer(GL_ARRAY_BUFFER, s->vertex_id);

									if (s->index_array_len > 0) {
										glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s->index_id);
									}

									for (int k = 0; k < VS::ARRAY_MAX - 1; k++) {
										if (s->attribs[k].enabled) {
											glEnableVertexAttribArray(k);
											glVertexAttribPointer(s->attribs[k].index, s->attribs[k].size, s->attribs[k].type, s->attribs[k].normalized, s->attribs[k].stride, CAST_INT_TO_UCHAR_PTR(s->attribs[k].offset));
										} else {
											glDisableVertexAttribArray(k);
											switch (k) {
												case VS::ARRAY_NORMAL: {
													glVertexAttrib4f(VS::ARRAY_NORMAL, 0.0, 0.0, 1, 1);
												} break;
												case VS::ARRAY_COLOR: {
													glVertexAttrib4f(VS::ARRAY_COLOR, 1, 1, 1, 1);

												} break;
												default: {
												}
											}
										}
									}

									for (int k = 0; k < amount; k++) {
										const float *buffer = base_buffer + k * stride;

										{

											glVertexAttrib4fv(INSTANCE_ATTRIB_BASE + 0, &buffer[0]);
											glVertexAttrib4fv(INSTANCE_ATTRIB_BASE + 1, &buffer[4]);
											if (multi_mesh->transform_format == VS::MULTIMESH_TRANSFORM_3D) {
												glVertexAttrib4fv(INSTANCE_ATTRIB_BASE + 2, &buffer[8]);
											} else {
												glVertexAttrib4f(INSTANCE_ATTRIB_BASE + 2, 0.0, 0.0, 1.0, 0.0);
											}
										}

										if (multi_mesh->color_floats) {
											if (multi_mesh->color_format == VS::MULTIMESH_COLOR_8BIT) {
												uint8_t *color_data = (uint8_t *)(buffer + color_ofs);
												glVertexAttrib4f(INSTANCE_ATTRIB_BASE + 3, color_data[0] / 255.0, color_data[1] / 255.0, color_data[2] / 255.0, color_data[3] / 255.0);
											} else {
												glVertexAttrib4fv(INSTANCE_ATTRIB_BASE + 3, buffer + color_ofs);
											}
										} else {
											glVertexAttrib4f(INSTANCE_ATTRIB_BASE + 3, 1.0, 1.0, 1.0, 1.0);
										}

										if (multi_mesh->custom_data_floats) {
											if (multi_mesh->custom_data_format == VS::MULTIMESH_CUSTOM_DATA_8BIT) {
												uint8_t *custom_data = (uint8_t *)(buffer + custom_data_ofs);
												glVertexAttrib4f(INSTANCE_ATTRIB_BASE + 4, custom_data[0] / 255.0, custom_data[1] / 255.0, custom_data[2] / 255.0, custom_data[3] / 255.0);
											} else {
												glVertexAttrib4fv(INSTANCE_ATTRIB_BASE + 4, buffer + custom_data_ofs);
											}
										}

										if (s->index_array_len > 0) {
											glDrawElements(gl_primitive[s->primitive], s->index_array_len, (s->array_len >= (1 << 16)) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT, 0);
										} else {
											glDrawArrays(gl_primitive[s->primitive], 0, s->array_len);
										}
									}
								}

								state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_INSTANCE_CUSTOM, false);
								state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_INSTANCING, false);

							} break;
							case Item::Command::TYPE_POLYLINE: {
								Item::CommandPolyLine *pline = static_cast<Item::CommandPolyLine *>(command);

								state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_TEXTURE_RECT, false);

								if (state.canvas_shader.bind()) {
									_set_uniforms();
									state.canvas_shader.use_material((void *)p_material);
								}

								_bind_canvas_texture(RID(), RID());

								if (pline->triangles.size()) {
									_draw_generic(GL_TRIANGLE_STRIP, pline->triangles.size(), pline->triangles.ptr(), NULL, pline->triangle_colors.ptr(), pline->triangle_colors.size() == 1);
#ifdef GLES_OVER_GL
									glEnable(GL_LINE_SMOOTH);
									if (pline->multiline) {
										//needs to be different
									} else {
										_draw_generic(GL_LINE_LOOP, pline->lines.size(), pline->lines.ptr(), NULL, pline->line_colors.ptr(), pline->line_colors.size() == 1);
									}
									glDisable(GL_LINE_SMOOTH);
#endif
								} else {

#ifdef GLES_OVER_GL
									if (pline->antialiased)
										glEnable(GL_LINE_SMOOTH);
#endif

									if (pline->multiline) {
										int todo = pline->lines.size() / 2;
										int max_per_call = data.polygon_buffer_size / (sizeof(real_t) * 4);
										int offset = 0;

										while (todo) {
											int to_draw = MIN(max_per_call, todo);
											_draw_generic(GL_LINES, to_draw * 2, &pline->lines.ptr()[offset], NULL, pline->line_colors.size() == 1 ? pline->line_colors.ptr() : &pline->line_colors.ptr()[offset], pline->line_colors.size() == 1);
											todo -= to_draw;
											offset += to_draw * 2;
										}
									} else {
										_draw_generic(GL_LINES, pline->lines.size(), pline->lines.ptr(), NULL, pline->line_colors.ptr(), pline->line_colors.size() == 1);
									}

#ifdef GLES_OVER_GL
									if (pline->antialiased)
										glDisable(GL_LINE_SMOOTH);
#endif
								}
							} break;

							case Item::Command::TYPE_PRIMITIVE: {

								Item::CommandPrimitive *primitive = static_cast<Item::CommandPrimitive *>(command);
								state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_TEXTURE_RECT, false);

								if (state.canvas_shader.bind()) {
									_set_uniforms();
									state.canvas_shader.use_material((void *)p_material);
								}

								ERR_CONTINUE(primitive->points.size() < 1);

								RasterizerStorageGLES2::Texture *texture = _bind_canvas_texture(primitive->texture, primitive->normal_map);

								if (texture) {
									Size2 texpixel_size(1.0 / texture->width, 1.0 / texture->height);
									state.canvas_shader.set_uniform(CanvasShaderGLES2::COLOR_TEXPIXEL_SIZE, texpixel_size);
								}

								if (primitive->colors.size() == 1 && primitive->points.size() > 1) {
									Color c = primitive->colors[0];
									glVertexAttrib4f(VS::ARRAY_COLOR, c.r, c.g, c.b, c.a);
								} else if (primitive->colors.empty()) {
									glVertexAttrib4f(VS::ARRAY_COLOR, 1, 1, 1, 1);
								}

								_draw_gui_primitive(primitive->points.size(), primitive->points.ptr(), primitive->colors.ptr(), primitive->uvs.ptr());
							} break;

							case Item::Command::TYPE_TRANSFORM: {
								Item::CommandTransform *transform = static_cast<Item::CommandTransform *>(command);
								state.uniforms.extra_matrix = transform->xform;
								state.canvas_shader.set_uniform(CanvasShaderGLES2::EXTRA_MATRIX, state.uniforms.extra_matrix);
							} break;

							case Item::Command::TYPE_PARTICLES: {

							} break;

							case Item::Command::TYPE_CLIP_IGNORE: {

								Item::CommandClipIgnore *ci = static_cast<Item::CommandClipIgnore *>(command);
								if (current_clip) {
									if (ci->ignore != reclip) {
										if (ci->ignore) {
											glDisable(GL_SCISSOR_TEST);
											reclip = true;
										} else {
											glEnable(GL_SCISSOR_TEST);

											int x = current_clip->final_clip_rect.position.x;
											int y = storage->frame.current_rt->height - (current_clip->final_clip_rect.position.y + current_clip->final_clip_rect.size.y);
											int w = current_clip->final_clip_rect.size.x;
											int h = current_clip->final_clip_rect.size.y;

											if (storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_VFLIP])
												y = current_clip->final_clip_rect.position.y;

											glScissor(x, y, w, h);

											reclip = false;
										}
									}
								}

							} break;

							default: {
								// FIXME: Proper error handling if relevant
								//print_line("other");
							} break;
						}
					}

				} // BT_DEFAULT
				break;
			}
//		}

//	} // while there are still more batches to fill
}

RasterizerCanvasGLES2::RasterizerCanvasGLES2()
{
	// turn off batching in the editor until it is considered stable
	// (if the editor can't start, you can't change the use_batching project setting!)
//	if (Engine::get_singleton()->is_editor_hint()) {
	if (0) {
		m_bUseBatching = false;
	} else {
		m_bUseBatching = GLOBAL_GET("rendering/quality/2d/use_batching");
	}
}
