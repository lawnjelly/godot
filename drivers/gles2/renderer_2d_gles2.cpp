#include "renderer_2d_gles2.h"

static bool g_bKessel2DOnly = true;

void Renderer2dGles2::batch_initialize() {
	const Batch::BatchData &bd = m_Batcher.m_Data;

	m_BatchData.index_buffer_size_bytes = bd.index_buffer_size_units * 2; // 16 bit inds

	// just reserve some space (may not be needed as we are orphaning, but hey ho)
	glGenBuffers(1, &m_BatchData.gl_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_BatchData.gl_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, bd.vertex_buffer_size_bytes, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// pre fill index buffer, the indices never need to change so can be static
	glGenBuffers(1, &m_BatchData.gl_index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_BatchData.gl_index_buffer);

	Vector<uint16_t> indices;
	indices.resize(bd.index_buffer_size_units);

	for (int q = 0; q < (int)bd.max_quads; q++) {
		int i_pos = q * 6; //  6 inds per quad
		int q_pos = q * 4; // 4 verts per quad

		indices.set(i_pos, q_pos);
		indices.set(i_pos + 1, q_pos + 1);
		indices.set(i_pos + 2, q_pos + 2);
		indices.set(i_pos + 3, q_pos);
		indices.set(i_pos + 4, q_pos + 2);
		indices.set(i_pos + 5, q_pos + 3);

		// test
//		indices.set(i_pos, 0);
//		indices.set(i_pos + 1, q_pos + 1);
//		indices.set(i_pos + 2, q_pos + 2);
//		indices.set(i_pos + 3, 0);
//		indices.set(i_pos + 4, q_pos + 1);
//		indices.set(i_pos + 5, q_pos + 2);


		// we can only use 16 bit indices in GLES2!
#ifdef DEBUG_ENABLED
		CRASH_COND((q_pos + 3) > 65535);
#endif
	}

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_BatchData.index_buffer_size_bytes, &indices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Renderer2dGles2::batch_terminate() {
}

void Renderer2dGles2::batch_upload_buffers() {

	const Batch::BatchData &bd = m_Batcher.m_Data;

	// noop?
	if (!bd.vertices.size())
		return;

	glBindBuffer(GL_ARRAY_BUFFER, m_BatchData.gl_vertex_buffer);

	// orphan the old (for now)
	glBufferData(GL_ARRAY_BUFFER, 0, 0, GL_DYNAMIC_DRAW);

	if (!bd.use_colored_vertices) {
		glBufferData(GL_ARRAY_BUFFER, sizeof(Batch::BVert) * bd.vertices.size(), bd.vertices.get_data(), GL_DYNAMIC_DRAW);
	} else {
		glBufferData(GL_ARRAY_BUFFER, sizeof(Batch::BVert_colored) * bd.vertices_colored.size(), bd.vertices_colored.get_data(), GL_DYNAMIC_DRAW);
	}

	// might not be necessary
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer2dGles2::batch_pass_begin() {
}

void Renderer2dGles2::batch_pass_end() {
	if (g_bKessel2DOnly)
		batch_flush();

	m_Batcher.pass_end();
}


void Renderer2dGles2::batch_flush_render() {
	const Batch::BatchData &bdata = m_Batcher.m_Data;

	RasterizerStorageGLES2::Material *p_material = 0;

	int num_batches = bdata.batches.size();

	// test
//	const int ciLimit = 32;
//	if (num_batches > ciLimit)
//		num_batches = ciLimit;


	Item::Command *const *commands = 0;

	for (int batch_num = 0; batch_num < num_batches; batch_num++) {
		const Batch::Batch &batch = bdata.batches[batch_num];

		switch (batch.type) {
			case Batch::Batch::BT_RECT: {
				_batch_render_rects(batch, p_material);
			} break;
		case Batch::Batch::BT_LINE: {
			_batch_render_lines(batch, p_material);
		} break;
			case Batch::Batch::BT_LIGHT_BEGIN: {
				_batch_light_begin(batch.light_begin.m_pLight, m_GLState.m_pItem);
			} break;
		case Batch::Batch::BT_LIGHT_END: {
				_batch_light_end();
		} break;
			case Batch::Batch::BT_CHANGE_ITEM: {
				//CRASH_COND(!batch.item);
				m_GLState.m_pItem = (RasterizerCanvas::Item *)batch.item_change.m_pItem;
				commands = m_GLState.m_pItem->commands.ptr();
				// if comands is null, we are probably quitting the app
				if (!commands)
					return;
			} break;
		case Batch::Batch::BT_CHANGE_BLEND_MODE: {
				_batch_change_blend_mode(batch.blend_mode_change.blend_mode);
			} break;
		case Batch::Batch::BT_CHANGE_MATERIAL: {
				int bmat_id = batch.material_change.batch_material_id;
				const Batch::BMaterial & bmat = m_Batcher.m_Data.batch_materials[bmat_id];

				// don't crash, in some cases (going from a non-rect batch previously) we will lose track of the current material
				// so could get a redundant material change - FIXME
				if (m_GLState.m_pMaterial == (RasterizerStorageGLES2::Material *) bmat.m_pMaterial)
				{
					WARN_PRINT_ONCE("kessel - redundant material change detected");
				}
				//CRASH_COND(m_GLState.m_pMaterial == (RasterizerStorageGLES2::Material *) bmat.m_pMaterial);
				m_GLState.m_pMaterial = (RasterizerStorageGLES2::Material *) bmat.m_pMaterial;
				p_material = m_GLState.m_pMaterial;

				// work out the shader
				RasterizerStorageGLES2::Shader * pShader = 0;

				if (p_material) {
					pShader = p_material->shader;

					if (pShader && pShader->mode != VS::SHADER_CANVAS_ITEM) {
						pShader = 0; // not a canvas item shader, don't use.
					}
				}
				if (pShader != m_GLState.m_pShader)
				{
					m_GLState.m_pShader = pShader;
					m_GLState.m_bChangedShader = true;
				}
				_batch_change_material();
				//_batch_change_material(bmat.RID_material, p_material);
			} break;
		case Batch::Batch::BT_COPY_BACK_BUFFER: {
				Rect2 r;
				batch.copy_back_buffer_rect.to(r);
				_batch_copy_back_buffer(r);
			} break;
		case Batch::Batch::BT_CHANGE_TRANSFORM: {
				int iTransformID = batch.transform_change.transform_id;
				m_GLState.m_matModelView = bdata.transforms[iTransformID].tr;
				// update uniforms
			} break;
		case Batch::Batch::BT_SET_EXTRA_MATRIX: {
				m_GLState.m_bExtraMatrixSet = true;
			} break;
		case Batch::Batch::BT_UNSET_EXTRA_MATRIX: {
				_batch_unset_extra_matrix();
				m_GLState.m_bExtraMatrixSet = false;
			} break;
		case Batch::Batch::BT_CHANGE_COLOR: {
				batch.color_change.color.to(m_GLState.m_Color);
			} break;
		case Batch::Batch::BT_CHANGE_COLOR_MODULATE: {
				Color col;
				batch.color_modulate_change.color.to(col);
				//_batch_change_color_modulate(col, batch.color_modulate_change.bRedundant);
				_batch_change_color_modulate(col);
			} break;
		case Batch::Batch::BT_SCISSOR_ON: {
				// this could probably be isolated to the derived renderer but for now...
				glEnable(GL_SCISSOR_TEST);
				glScissor(batch.scissor_rect.x,
				batch.scissor_rect.y,
				batch.scissor_rect.width,
				batch.scissor_rect.height);
			} break;
		case Batch::Batch::BT_SCISSOR_OFF: {
				glDisable(GL_SCISSOR_TEST);
			} break;
			case Batch::Batch::BT_DEFAULT: {
				bool reclip = false;
				_batch_render_default_batch(batch_num, m_GLState.m_pItem, m_GLState.m_pClipItem, reclip, p_material);
			} break;
			default: {
			} break;
		} // switch

	} // for
}

void Renderer2dGles2::batch_flush() {
	Batch::BatchData &bd = m_Batcher.m_Data;

	// render the batches
	// noop?
	if (!bd.batches.size()) {
		m_Batcher.flush();
		return;
	}

	// some heuristic to decide whether to use colored verts.
	// feel free to tweak this.
	// this could use hysteresis, to prevent jumping between methods
	// .. however probably not necessary
	if ((bd.total_color_changes * 4) > (bd.total_quads * 1)) {
		// small perf cost versus going straight to colored verts (maybe around 10%)
		// however more straightforward
		m_Batcher.batch_translate_to_colored();
	}

	batch_upload_buffers();

	batch_flush_render();

	m_Batcher.flush();
}

void Renderer2dGles2::batch_canvas_render_items(RasterizerCanvas::Item *p_item_list, int p_z, const Color &p_modulate, RasterizerCanvas::Light *p_light, const Transform2D &p_base_transform) {

//	m_Batcher.process_prepare(this, p_z, p_modulate, (Batch::AliasLight *)p_light, p_base_transform);
	m_Batcher.process_prepare(this, p_z, p_modulate, p_base_transform);

	m_GLState.m_pClipItem = NULL;

	while (p_item_list) {

		RasterizerCanvas::Item *ci = p_item_list;

		// returns whether to render without lighting
		if (m_Batcher.process_item_start(ci))
		{
			// no lighting?
			while (1) {
				// keep on processing the same item until it is finished
				if (m_Batcher.process_item(ci) == true)
					break;

				// needs a render
				batch_flush();
			}

		} // if render without lighting

		// lighting
		RasterizerCanvas::Light * pCurrLight = p_light;
		//bool light_used = false;
		//VS::CanvasLightMode mode = VS::CANVAS_LIGHT_MODE_ADD;

		// each light
		bool used_light = false;
		while (pCurrLight)
		{
			// let the batcher know which light we are using
			//m_Batcher.process_next_light((Batch::AliasLight *)pCurrLight);

			// does it intersect light?
			if (ci->light_mask & pCurrLight->item_mask && p_z >= pCurrLight->z_min && p_z <= pCurrLight->z_max && ci->global_rect_cache.intersects_transformed(pCurrLight->xform_cache, pCurrLight->rect_cache))
			{
				if (m_Batcher.process_lit_item_start(ci, (Batch::AliasLight *)pCurrLight))
				{
					used_light = true;

					while (1) {
						// keep on processing the same item until it is finished
						if (m_Batcher.process_lit_item(ci) == true)
							break;

						// needs a render
						batch_flush();
					}

					// flush after every light
					//batch_flush();
				} // if lit item started
			} // intersects light


			// try the next lights
			pCurrLight = pCurrLight->next_ptr;
		}  // while through lights

		if (used_light)
			m_Batcher.process_lit_item_end();

		// next item
		p_item_list = p_item_list->next;
	}

	// do an extra flush for any left overs
	if (!g_bKessel2DOnly)
		batch_flush();
}


/*
RasterizerStorageGLES2::Texture *Renderer2dGles2::_batch_bind_canvas_texture(const RID &p_texture, const RID &p_normal_map) {

	RasterizerStorageGLES2::Texture *tex_return = NULL;

	if (p_texture.is_valid()) {

		RasterizerStorageGLES2::Texture *texture = storage->texture_owner.getornull(p_texture);

		if (!texture) {
			state.current_tex = RID();
			state.current_tex_ptr = NULL;

			glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 1);
			glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);

		} else {

			if (texture->redraw_if_visible) {
				VisualServerRaster::redraw_request();
			}

			texture = texture->get_ptr();

			if (texture->render_target) {
				texture->render_target->used_in_frame = true;
			}

			glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 1);
			glBindTexture(GL_TEXTURE_2D, texture->tex_id);

			state.current_tex = p_texture;
			state.current_tex_ptr = texture;

			tex_return = texture;
		}
	} else {
		state.current_tex = RID();
		state.current_tex_ptr = NULL;

		glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 1);
		glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);
	}

	if (p_normal_map == state.current_normal) {
		//do none
		state.canvas_shader.set_uniform(CanvasShaderGLES2::USE_DEFAULT_NORMAL, state.current_normal.is_valid());

	} else if (p_normal_map.is_valid()) {

		RasterizerStorageGLES2::Texture *normal_map = storage->texture_owner.getornull(p_normal_map);

		if (!normal_map) {
			state.current_normal = RID();
			glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 2);
			glBindTexture(GL_TEXTURE_2D, storage->resources.normal_tex);
			state.canvas_shader.set_uniform(CanvasShaderGLES2::USE_DEFAULT_NORMAL, false);

		} else {

			if (normal_map->redraw_if_visible) { //check before proxy, because this is usually used with proxies
				VisualServerRaster::redraw_request();
			}

			normal_map = normal_map->get_ptr();

			glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 2);
			glBindTexture(GL_TEXTURE_2D, normal_map->tex_id);
			state.current_normal = p_normal_map;
			state.canvas_shader.set_uniform(CanvasShaderGLES2::USE_DEFAULT_NORMAL, true);
		}

	} else {

		state.current_normal = RID();
		glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 2);
		glBindTexture(GL_TEXTURE_2D, storage->resources.normal_tex);
		state.canvas_shader.set_uniform(CanvasShaderGLES2::USE_DEFAULT_NORMAL, false);
	}

	return tex_return;
}
*/
