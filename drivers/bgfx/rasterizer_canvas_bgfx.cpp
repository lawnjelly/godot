#include "rasterizer_canvas_bgfx.h"
#include "servers/visual/visual_server_raster.h"

void RasterizerCanvasBGFX::canvas_render_items(Item *p_item_list, int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_transform) {
	batch_canvas_render_items(p_item_list, p_z, p_modulate, p_light, p_transform);
}

void RasterizerCanvasBGFX::canvas_render_items_implementation(Item *p_item_list, int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform) {
	// parameters are easier to pass around in a structure
	RenderItemState ris;
	ris.item_group_z = p_z;
	ris.item_group_modulate = p_modulate;
	ris.item_group_light = p_light;
	ris.item_group_base_transform = p_base_transform;

	//state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SKELETON, false);

	state.current_tex = RID();
	state.current_tex_ptr = nullptr;
	state.current_normal = RID();
	state.canvas_texscreen_used = false;

	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);

	// hard code no batching for now
	if (false) {
		//	if (bdata.settings_use_batching) {
		for (int j = 0; j < bdata.items_joined.size(); j++) {
			render_joined_item(bdata.items_joined[j], ris);
		}
	} else {
		while (p_item_list) {
			Item *ci = p_item_list;
			_legacy_canvas_render_item(ci, ris);
			p_item_list = p_item_list->next;
		}
	}

	//	if (ris.current_clip) {
	//		glDisable(GL_SCISSOR_TEST);
	//	}

	//	state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SKELETON, false);
}

void RasterizerCanvasBGFX::_legacy_canvas_render_item(Item *p_ci, RenderItemState &r_ris) {
	//	storage->info.render._2d_item_count++;

	if (r_ris.current_clip != p_ci->final_clip_owner) {
		r_ris.current_clip = p_ci->final_clip_owner;

		if (r_ris.current_clip) {
			//glEnable(GL_SCISSOR_TEST);
			int y = storage->frame.current_rt->height - (r_ris.current_clip->final_clip_rect.position.y + r_ris.current_clip->final_clip_rect.size.y);

			//			if (true) {
			if (storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_VFLIP]) {
				y = r_ris.current_clip->final_clip_rect.position.y;
			}
			state.canvas_shader.set_scissor(r_ris.current_clip->final_clip_rect.position.x, y, r_ris.current_clip->final_clip_rect.size.width, r_ris.current_clip->final_clip_rect.size.height);
			//glScissor(r_ris.current_clip->final_clip_rect.position.x, y, r_ris.current_clip->final_clip_rect.size.width, r_ris.current_clip->final_clip_rect.size.height);
		} else {
			state.canvas_shader.set_scissor_disable();
			//glDisable(GL_SCISSOR_TEST);
		}
	}

	// TODO: copy back buffer

	if (p_ci->copy_back_buffer) {
		if (p_ci->copy_back_buffer->full) {
			_copy_texscreen(Rect2());
		} else {
			_copy_texscreen(p_ci->copy_back_buffer->rect);
		}
	}

#if 0
	RasterizerStorageBGFX::Skeleton *skeleton = nullptr;

	{
		//skeleton handling
		if (p_ci->skeleton.is_valid() && storage->skeleton_owner.owns(p_ci->skeleton)) {
			skeleton = storage->skeleton_owner.get(p_ci->skeleton);
			if (!skeleton->use_2d) {
				skeleton = nullptr;
			} else {
				state.skeleton_transform = r_ris.item_group_base_transform * skeleton->base_transform_2d;
				state.skeleton_transform_inverse = state.skeleton_transform.affine_inverse();
				state.skeleton_texture_size = Vector2(skeleton->size * 2, 0);
			}
		}

		bool use_skeleton = skeleton != nullptr;
		if (r_ris.prev_use_skeleton != use_skeleton) {
			r_ris.rebind_shader = true;
			state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SKELETON, use_skeleton);
			r_ris.prev_use_skeleton = use_skeleton;
		}

		if (skeleton) {
			glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 3);
			glBindTexture(GL_TEXTURE_2D, skeleton->tex_id);
			state.using_skeleton = true;
		} else {
			state.using_skeleton = false;
		}
	}
#endif

	Item *material_owner = p_ci->material_owner ? p_ci->material_owner : p_ci;

	RID material = material_owner->material;
	RasterizerStorageBGFX::Material *material_ptr = storage->material_owner.getornull(material);

	if (material != r_ris.canvas_last_material || r_ris.rebind_shader) {
		RasterizerStorageBGFX::Shader *shader_ptr = nullptr;

		if (material_ptr) {
			shader_ptr = material_ptr->shader;

			if (shader_ptr && shader_ptr->mode != VS::SHADER_CANVAS_ITEM) {
				shader_ptr = nullptr; // not a canvas item shader, don't use.
			}
		}

		if (shader_ptr) {
			if (shader_ptr->canvas_item.uses_screen_texture) {
				if (!state.canvas_texscreen_used) {
					//copy if not copied before
					_copy_texscreen(Rect2());

					// blend mode will have been enabled so make sure we disable it again later on
					//last_blend_mode = last_blend_mode != RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_DISABLED ? last_blend_mode : -1;
				}

				//				if (storage->frame.current_rt->copy_screen_effect.color) {
				//					glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 4);
				//					glBindTexture(GL_TEXTURE_2D, storage->frame.current_rt->copy_screen_effect.color);
				//				}
			}

			if (shader_ptr != r_ris.shader_cache) {
				if (shader_ptr->canvas_item.uses_time) {
					VisualServerRaster::redraw_request(false);
				}

				//			state.canvas_shader.set_custom_shader(shader_ptr->custom_code_id);
				//				state.canvas_shader.bind();
			}

			int tc = material_ptr->textures.size();
			Pair<StringName, RID> *textures = material_ptr->textures.ptrw();

			ShaderLanguage::ShaderNode::Uniform::Hint *texture_hints = shader_ptr->texture_hints.ptrw();

			for (int i = 0; i < tc; i++) {
				//glActiveTexture(GL_TEXTURE0 + i);

				RasterizerStorageBGFX::Texture *t = storage->texture_owner.getornull(textures[i].second);

				if (!t) {
					switch (texture_hints[i]) {
						case ShaderLanguage::ShaderNode::Uniform::HINT_BLACK_ALBEDO:
						case ShaderLanguage::ShaderNode::Uniform::HINT_BLACK: {
							//glBindTexture(GL_TEXTURE_2D, storage->resources.black_tex);
						} break;
						case ShaderLanguage::ShaderNode::Uniform::HINT_TRANSPARENT: {
							//glBindTexture(GL_TEXTURE_2D, storage->resources.transparent_tex);
						} break;
						case ShaderLanguage::ShaderNode::Uniform::HINT_ANISO: {
							//glBindTexture(GL_TEXTURE_2D, storage->resources.aniso_tex);
						} break;
						case ShaderLanguage::ShaderNode::Uniform::HINT_NORMAL: {
							//glBindTexture(GL_TEXTURE_2D, storage->resources.normal_tex);
						} break;
						default: {
							//glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);
						} break;
					}

					continue;
				}

				if (t->redraw_if_visible) {
					VisualServerRaster::redraw_request(false);
				}

				t = t->get_ptr();

#ifdef TOOLS_ENABLED
				if (t->detect_normal && texture_hints[i] == ShaderLanguage::ShaderNode::Uniform::HINT_NORMAL) {
					t->detect_normal(t->detect_normal_ud);
				}
#endif
				if (t->render_target) {
					t->render_target->used_in_frame = true;
				}

				//glBindTexture(t->target, t->tex_id);
			}

		} else {
			state.canvas_shader.set_custom_shader(0);
			state.canvas_shader.bind();
		}
		state.canvas_shader.use_material((void *)material_ptr);

		r_ris.shader_cache = shader_ptr;

		r_ris.canvas_last_material = material;

		r_ris.rebind_shader = false;
	}

	int blend_mode = r_ris.shader_cache ? r_ris.shader_cache->canvas_item.blend_mode : RasterizerStorageBGFX::Shader::CanvasItem::BLEND_MODE_MIX;
	bool unshaded = r_ris.shader_cache && (r_ris.shader_cache->canvas_item.light_mode == RasterizerStorageBGFX::Shader::CanvasItem::LIGHT_MODE_UNSHADED || (blend_mode != RasterizerStorageBGFX::Shader::CanvasItem::BLEND_MODE_MIX && blend_mode != RasterizerStorageBGFX::Shader::CanvasItem::BLEND_MODE_PMALPHA));
	bool reclip = false;

	if (r_ris.last_blend_mode != blend_mode) {
		switch (blend_mode) {
			case RasterizerStorageBGFX::Shader::CanvasItem::BLEND_MODE_MIX: {
				//glBlendEquation(GL_FUNC_ADD);
				state.canvas_shader.set_blend_state(BGFX_STATE_BLEND_ALPHA);
				if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
					//glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				} else {
					//glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
				}

			} break;
			case RasterizerStorageBGFX::Shader::CanvasItem::BLEND_MODE_ADD: {
				//glBlendEquation(GL_FUNC_ADD);
				if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
					//glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ONE);
				} else {
					//glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
				}

			} break;
			case RasterizerStorageBGFX::Shader::CanvasItem::BLEND_MODE_SUB: {
				//glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
				if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
					//glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ONE);
				} else {
					//glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
				}
			} break;
			case RasterizerStorageBGFX::Shader::CanvasItem::BLEND_MODE_MUL: {
				//glBlendEquation(GL_FUNC_ADD);
				if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
					//glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_DST_ALPHA, GL_ZERO);
				} else {
					//glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_ZERO, GL_ONE);
				}
			} break;
			case RasterizerStorageBGFX::Shader::CanvasItem::BLEND_MODE_PMALPHA: {
				//glBlendEquation(GL_FUNC_ADD);
				if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
					//glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				} else {
					//glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
				}
			} break;
		}
	}

	state.uniforms.final_modulate = unshaded ? p_ci->final_modulate : Color(p_ci->final_modulate.r * r_ris.item_group_modulate.r, p_ci->final_modulate.g * r_ris.item_group_modulate.g, p_ci->final_modulate.b * r_ris.item_group_modulate.b, p_ci->final_modulate.a * r_ris.item_group_modulate.a);

	state.uniforms.modelview_matrix = p_ci->final_transform;
	state.uniforms.extra_matrix = Transform2D();

	_set_uniforms();

	if (unshaded || (state.uniforms.final_modulate.a > 0.001 && (!r_ris.shader_cache || r_ris.shader_cache->canvas_item.light_mode != RasterizerStorageBGFX::Shader::CanvasItem::LIGHT_MODE_LIGHT_ONLY) && !p_ci->light_masked)) {
		_legacy_canvas_item_render_commands(p_ci, nullptr, reclip, material_ptr);
	}

	r_ris.rebind_shader = true; // hacked in for now.

	if ((blend_mode == RasterizerStorageBGFX::Shader::CanvasItem::BLEND_MODE_MIX || blend_mode == RasterizerStorageBGFX::Shader::CanvasItem::BLEND_MODE_PMALPHA) && r_ris.item_group_light && !unshaded) {
		Light *light = r_ris.item_group_light;
		bool light_used = false;
		VS::CanvasLightMode mode = VS::CANVAS_LIGHT_MODE_ADD;
		state.uniforms.final_modulate = p_ci->final_modulate; // remove the canvas modulate

		while (light) {
			if (p_ci->light_mask & light->item_mask && r_ris.item_group_z >= light->z_min && r_ris.item_group_z <= light->z_max && p_ci->global_rect_cache.intersects_transformed(light->xform_cache, light->rect_cache)) {
				//intersects this light

				if (!light_used || mode != light->mode) {
					mode = light->mode;

					switch (mode) {
						case VS::CANVAS_LIGHT_MODE_ADD: {
							//							glBlendEquation(GL_FUNC_ADD);
							//							glBlendFunc(GL_SRC_ALPHA, GL_ONE);

						} break;
						case VS::CANVAS_LIGHT_MODE_SUB: {
							//							glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
							//							glBlendFunc(GL_SRC_ALPHA, GL_ONE);
						} break;
						case VS::CANVAS_LIGHT_MODE_MIX:
						case VS::CANVAS_LIGHT_MODE_MASK: {
							//							glBlendEquation(GL_FUNC_ADD);
							//							glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

						} break;
					}
				}

				if (!light_used) {
					//state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_LIGHTING, true);
					light_used = true;
				}

				bool has_shadow = light->shadow_buffer.is_valid() && p_ci->light_mask & light->item_shadow_mask;

				//state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SHADOWS, has_shadow);
				if (has_shadow) {
					//state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_USE_GRADIENT, light->shadow_gradient_length > 0);
					//state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_NEAREST, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_NONE);
					//state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF3, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF3);
					//state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF5, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF5);
					//state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF7, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF7);
					//state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF9, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF9);
					//state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF13, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF13);
				}

				//state.canvas_shader.bind();
				state.using_light = light;
				state.using_shadow = has_shadow;

				//always re-set uniforms, since light parameters changed
				_set_uniforms();
				//state.canvas_shader.use_material((void *)material_ptr);

				//glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 6);
				RasterizerStorageBGFX::Texture *t = storage->texture_owner.getornull(light->texture);
				if (!t) {
					//glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);
				} else {
					t = t->get_ptr();

					//glBindTexture(t->target, t->tex_id);
				}

				//glActiveTexture(GL_TEXTURE0);
				_legacy_canvas_item_render_commands(p_ci, nullptr, reclip, material_ptr); //redraw using light

				state.using_light = nullptr;
			}

			light = light->next_ptr;
		}

		if (light_used) {
#if 0
			state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_LIGHTING, false);
			state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SHADOWS, false);
			state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_NEAREST, false);
			state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF3, false);
			state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF5, false);
			state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF7, false);
			state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF9, false);
			state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF13, false);

			state.canvas_shader.bind();
#endif

			r_ris.last_blend_mode = -1;
		}
	}

	if (reclip) {
		//glEnable(GL_SCISSOR_TEST);
		int y = storage->frame.current_rt->height - (r_ris.current_clip->final_clip_rect.position.y + r_ris.current_clip->final_clip_rect.size.y);
		//		if (true) {
		if (storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_VFLIP]) {
			y = r_ris.current_clip->final_clip_rect.position.y;
		}
		state.canvas_shader.set_scissor(r_ris.current_clip->final_clip_rect.position.x, y, r_ris.current_clip->final_clip_rect.size.width, r_ris.current_clip->final_clip_rect.size.height);
		//glScissor(r_ris.current_clip->final_clip_rect.position.x, y, r_ris.current_clip->final_clip_rect.size.width, r_ris.current_clip->final_clip_rect.size.height);
	}
}

void RasterizerCanvasBGFX::render_joined_item(const BItemJoined &p_bij, RenderItemState &r_ris) {
}

bool RasterizerCanvasBGFX::try_join_item(Item *p_ci, RenderItemState &r_ris, bool &r_batch_break) {
	return false;
}

void RasterizerCanvasBGFX::render_batches(Item *p_current_clip, bool &r_reclip, RasterizerStorageBGFX::Material *p_material) {
	int num_batches = bdata.batches.size();

	for (int batch_num = 0; batch_num < num_batches; batch_num++) {
		const Batch &batch = bdata.batches[batch_num];

		switch (batch.type) {
			case RasterizerStorageCommon::BT_RECT: {
				_batch_render_generic(batch, p_material);
			} break;
			case RasterizerStorageCommon::BT_POLY: {
				_batch_render_generic(batch, p_material);
			} break;
			case RasterizerStorageCommon::BT_LINE: {
				_batch_render_lines(batch, p_material, false);
			} break;
			case RasterizerStorageCommon::BT_LINE_AA: {
				_batch_render_lines(batch, p_material, true);
			} break;
			default: {
				int end_command = batch.first_command + batch.num_commands;

				DEV_ASSERT(batch.item);
				RasterizerCanvas::Item::Command *const *commands = batch.item->commands.ptr();

				for (int i = batch.first_command; i < end_command; i++) {
					Item::Command *command = commands[i];

					switch (command->type) {
						case Item::Command::TYPE_LINE: {
						} break;
						case Item::Command::TYPE_PRIMITIVE: {
						} break;

						case Item::Command::TYPE_POLYGON: {
							Item::CommandPolygon *polygon = static_cast<Item::CommandPolygon *>(command);

							_set_texture_rect_mode(false);

							if (state.canvas_shader.bind()) {
								_set_uniforms();
								state.canvas_shader.use_material((void *)p_material);
							}

							_bind_canvas_texture(polygon->texture, polygon->normal_map);
							//							RasterizerStorageBGFX::Texture *texture = _bind_canvas_texture(polygon->texture, polygon->normal_map);

							//							if (texture) {
							//								Size2 texpixel_size(1.0 / texture->width, 1.0 / texture->height);
							//								state.canvas_shader.set_uniform(CanvasShaderBGFX::COLOR_TEXPIXEL_SIZE, texpixel_size);
							//							}

							_draw_polygon(polygon->indices.ptr(), polygon->count, polygon->points.size(), polygon->points.ptr(), polygon->uvs.ptr(), polygon->colors.ptr(), polygon->colors.size() == 1, polygon->weights.ptr(), polygon->bones.ptr());
						} break;

						case Item::Command::TYPE_POLYLINE: {
							Item::CommandPolyLine *pline = static_cast<Item::CommandPolyLine *>(command);

							_set_texture_rect_mode(false);

							if (state.canvas_shader.bind()) {
								_set_uniforms();
								state.canvas_shader.use_material((void *)p_material);
							}

							_bind_canvas_texture(RID(), RID());

							if (pline->triangles.size()) {
								if (pline->multiline) {
								} else {
								}
							}
						} break;

						case Item::Command::TYPE_RECT: {
							Item::CommandRect *r = static_cast<Item::CommandRect *>(command);

							//							glDisableVertexAttribArray(VS::ARRAY_COLOR);
							//							glVertexAttrib4fv(VS::ARRAY_COLOR, r->modulate.components);
							Color *cols = (Color *)alloca(4 * sizeof(Color));
							for (int n = 0; n < 4; n++) {
								cols[n] = r->modulate;
							}

							bool can_tile = true;
							//							if (r->texture.is_valid() && r->flags & CANVAS_RECT_TILE && !storage->config.support_npot_repeat_mipmap) {
							//								// workaround for when setting tiling does not work due to hardware limitation

							//								RasterizerStorageGLES2::Texture *texture = storage->texture_owner.getornull(r->texture);

							//								if (texture) {
							//									texture = texture->get_ptr();

							//									if (next_power_of_2(texture->alloc_width) != (unsigned int)texture->alloc_width && next_power_of_2(texture->alloc_height) != (unsigned int)texture->alloc_height) {
							//										state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_FORCE_REPEAT, true);
							//										can_tile = false;
							//									}
							//								}
							//							}

							// On some widespread Nvidia cards, the normal draw method can produce some
							// flickering in draw_rect and especially TileMap rendering (tiles randomly flicker).
							// See GH-9913.
							// To work it around, we use a simpler draw method which does not flicker, but gives
							// a non negligible performance hit, so it's opt-in (GH-24466).
							//							if (use_nvidia_rect_workaround) {
							if (true) {
								// are we using normal maps, if so we want to use light angle
								bool send_light_angles = false;

								// only need to use light angles when normal mapping
								// otherwise we can use the default shader
								if (state.current_normal != RID()) {
									send_light_angles = true;
								}

								_set_texture_rect_mode(false, send_light_angles);

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

								RasterizerStorageBGFX::Texture *texture = _bind_canvas_texture(r->texture, r->normal_map);

								if (texture) {
									DEV_ASSERT(texture->width);
									DEV_ASSERT(texture->height);
									Size2 texpixel_size(1.0 / texture->width, 1.0 / texture->height);

									Rect2 src_rect = (r->flags & CANVAS_RECT_REGION) ? Rect2(r->source.position * texpixel_size, r->source.size * texpixel_size) : Rect2(0, 0, 1, 1);

									Vector2 uvs[4] = {
										src_rect.position,
										src_rect.position + Vector2(src_rect.size.x, 0.0),
										src_rect.position + src_rect.size,
										src_rect.position + Vector2(0.0, src_rect.size.y),
									};

									// for encoding in light angle
									bool flip_h = false;
									bool flip_v = false;

									if (r->flags & CANVAS_RECT_TRANSPOSE) {
										SWAP(uvs[1], uvs[3]);
									}

									if (r->flags & CANVAS_RECT_FLIP_H) {
										SWAP(uvs[0], uvs[1]);
										SWAP(uvs[2], uvs[3]);
										flip_h = true;
										flip_v = !flip_v;
									}
									if (r->flags & CANVAS_RECT_FLIP_V) {
										SWAP(uvs[0], uvs[3]);
										SWAP(uvs[1], uvs[2]);
										flip_v = !flip_v;
									}

									//state.canvas_shader.set_uniform(CanvasShaderGLES2::COLOR_TEXPIXEL_SIZE, texpixel_size);

									bool untile = false;

									if (can_tile && r->flags & CANVAS_RECT_TILE && !(texture->flags & VS::TEXTURE_FLAG_REPEAT)) {
										//										glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
										//										glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
										untile = true;
									}

									if (send_light_angles) {
										// for single rects, there is no need to fully utilize the light angle,
										// we only need it to encode flips (horz and vert). But the shader can be reused with
										// batching in which case the angle encodes the transform as well as
										// the flips.
										// Note transpose is NYI. I don't think it worked either with the non-nvidia method.

										// if horizontal flip, angle is 180
										float angle = 0.0f;
										if (flip_h) {
											angle = Math_PI;
										}

										// add 1 (to take care of zero floating point error with sign)
										angle += 1.0f;

										// flip if necessary
										if (flip_v) {
											angle *= -1.0f;
										}

										// light angle must be sent for each vert, instead as a single uniform in the uniform draw method
										// this has the benefit of enabling batching with light angles.
										float light_angles[4] = { angle, angle, angle, angle };

										_draw_gui_primitive(4, points, cols, uvs, light_angles);
									} else {
										_draw_gui_primitive(4, points, cols, uvs);
									}

									if (untile) {
										//										glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
										//										glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
									}
								} else {
									static const Vector2 uvs[4] = {
										Vector2(0.0, 0.0),
										Vector2(0.0, 1.0),
										Vector2(1.0, 1.0),
										Vector2(1.0, 0.0),
									};

									//state.canvas_shader.set_uniform(CanvasShaderGLES2::COLOR_TEXPIXEL_SIZE, Vector2());
									_draw_gui_primitive(4, points, cols, uvs);
								}

							} else {
							}

							//							state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_FORCE_REPEAT, false);

						} break;

						case Item::Command::TYPE_TRANSFORM: {
							Item::CommandTransform *transform = static_cast<Item::CommandTransform *>(command);
							state.uniforms.extra_matrix = transform->xform;
							state.canvas_shader.set_uniform(CanvasShaderBGFX::EXTRA_MATRIX, state.uniforms.extra_matrix);
						} break;

						case Item::Command::TYPE_CLIP_IGNORE: {
							Item::CommandClipIgnore *ci = static_cast<Item::CommandClipIgnore *>(command);
							if (p_current_clip) {
								//bgfx::ViewId view_id = storage->frame.current_rt->id_view;

								if (ci->ignore != r_reclip) {
									if (ci->ignore) {
										//glDisable(GL_SCISSOR_TEST);
										state.canvas_shader.set_scissor_disable();
										r_reclip = true;
									} else {
										//glEnable(GL_SCISSOR_TEST);

										int x = p_current_clip->final_clip_rect.position.x;
										int y = storage->frame.current_rt->height - (p_current_clip->final_clip_rect.position.y + p_current_clip->final_clip_rect.size.y);
										int w = p_current_clip->final_clip_rect.size.x;
										int h = p_current_clip->final_clip_rect.size.y;

										if (storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_VFLIP]) {
											y = p_current_clip->final_clip_rect.position.y;
										}

										//glScissor(x, y, w, h);
										state.canvas_shader.set_scissor(x, y, w, h);

										r_reclip = false;
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

			} // default
			break;
		}
	}
}

void RasterizerCanvasBGFX::_batch_upload_buffers() {
}

void RasterizerCanvasBGFX::_batch_render_generic(const Batch &p_batch, RasterizerStorageBGFX::Material *p_material) {
}

void RasterizerCanvasBGFX::_batch_render_lines(const Batch &p_batch, RasterizerStorageBGFX::Material *p_material, bool p_anti_alias) {
}

void RasterizerCanvasBGFX::gl_enable_scissor(int p_x, int p_y, int p_width, int p_height) const {
}

void RasterizerCanvasBGFX::gl_disable_scissor() const {
}
