#include "rasterizer_storage_bgfx.h"
#include "core/project_settings.h"
#include "rasterizer_scene_bgfx.h"

bool RasterizerStorageBGFX::has_os_feature(const String &p_feature) const {
	if (p_feature == "pvrtc") {
		//return config.pvrtc_supported;
		return true;
	}

	if (p_feature == "s3tc") {
		return true;
		//		return config.s3tc_supported;
	}

	if (p_feature == "etc") {
		//return config.etc1_supported;
		return true;
	}

	if (p_feature == "skinning_fallback") {
		return true;
	}

	return false;
}

void RasterizerStorageBGFX::update_dirty_resources() {
	update_dirty_shaders();
	update_dirty_materials();
	//	update_dirty_blend_shapes();
	//	update_dirty_skeletons();
	//	update_dirty_multimeshes();
	//	update_dirty_captures();
}

void RasterizerStorageBGFX::_shader_make_dirty(Shader *p_shader) {
	if (p_shader->dirty_list.in_list()) {
		return;
	}

	_shader_dirty_list.add(&p_shader->dirty_list);
}

RID RasterizerStorageBGFX::shader_create() {
	Shader *shader = memnew(Shader);
	shader->mode = VS::SHADER_SPATIAL;
	shader->shader = &scene->state.scene_shader;
	RID rid = shader_owner.make_rid(shader);
	_shader_make_dirty(shader);
	shader->self = rid;

	return rid;
}

void RasterizerStorageBGFX::shader_set_default_texture_param(RID p_shader, const StringName &p_name, RID p_texture) {
	Shader *shader = shader_owner.get(p_shader);
	ERR_FAIL_COND(!shader);
	ERR_FAIL_COND(p_texture.is_valid() && !texture_owner.owns(p_texture));

	if (p_texture.is_valid()) {
		shader->default_textures[p_name] = p_texture;
	} else {
		shader->default_textures.erase(p_name);
	}

	_shader_make_dirty(shader);
}

RID RasterizerStorageBGFX::shader_get_default_texture_param(RID p_shader, const StringName &p_name) const {
	const Shader *shader = shader_owner.get(p_shader);
	ERR_FAIL_COND_V(!shader, RID());

	const Map<StringName, RID>::Element *E = shader->default_textures.find(p_name);

	if (!E) {
		return RID();
	}

	return E->get();
}

void RasterizerStorageBGFX::_update_shader(Shader *p_shader) const {
	_shader_dirty_list.remove(&p_shader->dirty_list);

	p_shader->valid = false;

	p_shader->uniforms.clear();

	if (p_shader->code == String()) {
		return; //just invalid, but no error
	}

#if 0

	ShaderCompilerGLES2::GeneratedCode gen_code;
	ShaderCompilerGLES2::IdentifierActions *actions = nullptr;

	switch (p_shader->mode) {
		case VS::SHADER_CANVAS_ITEM: {
			p_shader->canvas_item.light_mode = Shader::CanvasItem::LIGHT_MODE_NORMAL;
			p_shader->canvas_item.blend_mode = Shader::CanvasItem::BLEND_MODE_MIX;

			p_shader->canvas_item.uses_screen_texture = false;
			p_shader->canvas_item.uses_screen_uv = false;
			p_shader->canvas_item.uses_time = false;
			p_shader->canvas_item.uses_modulate = false;
			p_shader->canvas_item.uses_color = false;
			p_shader->canvas_item.uses_vertex = false;
			p_shader->canvas_item.batch_flags = 0;

			p_shader->canvas_item.uses_world_matrix = false;
			p_shader->canvas_item.uses_extra_matrix = false;
			p_shader->canvas_item.uses_projection_matrix = false;
			p_shader->canvas_item.uses_instance_custom = false;

			shaders.actions_canvas.render_mode_values["blend_add"] = Pair<int *, int>(&p_shader->canvas_item.blend_mode, Shader::CanvasItem::BLEND_MODE_ADD);
			shaders.actions_canvas.render_mode_values["blend_mix"] = Pair<int *, int>(&p_shader->canvas_item.blend_mode, Shader::CanvasItem::BLEND_MODE_MIX);
			shaders.actions_canvas.render_mode_values["blend_sub"] = Pair<int *, int>(&p_shader->canvas_item.blend_mode, Shader::CanvasItem::BLEND_MODE_SUB);
			shaders.actions_canvas.render_mode_values["blend_mul"] = Pair<int *, int>(&p_shader->canvas_item.blend_mode, Shader::CanvasItem::BLEND_MODE_MUL);
			shaders.actions_canvas.render_mode_values["blend_premul_alpha"] = Pair<int *, int>(&p_shader->canvas_item.blend_mode, Shader::CanvasItem::BLEND_MODE_PMALPHA);

			shaders.actions_canvas.render_mode_values["unshaded"] = Pair<int *, int>(&p_shader->canvas_item.light_mode, Shader::CanvasItem::LIGHT_MODE_UNSHADED);
			shaders.actions_canvas.render_mode_values["light_only"] = Pair<int *, int>(&p_shader->canvas_item.light_mode, Shader::CanvasItem::LIGHT_MODE_LIGHT_ONLY);

			shaders.actions_canvas.usage_flag_pointers["SCREEN_UV"] = &p_shader->canvas_item.uses_screen_uv;
			shaders.actions_canvas.usage_flag_pointers["SCREEN_PIXEL_SIZE"] = &p_shader->canvas_item.uses_screen_uv;
			shaders.actions_canvas.usage_flag_pointers["SCREEN_TEXTURE"] = &p_shader->canvas_item.uses_screen_texture;
			shaders.actions_canvas.usage_flag_pointers["TIME"] = &p_shader->canvas_item.uses_time;
			shaders.actions_canvas.usage_flag_pointers["MODULATE"] = &p_shader->canvas_item.uses_modulate;
			shaders.actions_canvas.usage_flag_pointers["COLOR"] = &p_shader->canvas_item.uses_color;

			shaders.actions_canvas.usage_flag_pointers["VERTEX"] = &p_shader->canvas_item.uses_vertex;

			shaders.actions_canvas.usage_flag_pointers["WORLD_MATRIX"] = &p_shader->canvas_item.uses_world_matrix;
			shaders.actions_canvas.usage_flag_pointers["EXTRA_MATRIX"] = &p_shader->canvas_item.uses_extra_matrix;
			shaders.actions_canvas.usage_flag_pointers["PROJECTION_MATRIX"] = &p_shader->canvas_item.uses_projection_matrix;
			shaders.actions_canvas.usage_flag_pointers["INSTANCE_CUSTOM"] = &p_shader->canvas_item.uses_instance_custom;

			actions = &shaders.actions_canvas;
			actions->uniforms = &p_shader->uniforms;
		} break;

		case VS::SHADER_SPATIAL: {
			p_shader->spatial.blend_mode = Shader::Spatial::BLEND_MODE_MIX;
			p_shader->spatial.depth_draw_mode = Shader::Spatial::DEPTH_DRAW_OPAQUE;
			p_shader->spatial.cull_mode = Shader::Spatial::CULL_MODE_BACK;
			p_shader->spatial.uses_alpha = false;
			p_shader->spatial.uses_alpha_scissor = false;
			p_shader->spatial.uses_discard = false;
			p_shader->spatial.unshaded = false;
			p_shader->spatial.no_depth_test = false;
			p_shader->spatial.uses_sss = false;
			p_shader->spatial.uses_time = false;
			p_shader->spatial.uses_vertex_lighting = false;
			p_shader->spatial.uses_screen_texture = false;
			p_shader->spatial.uses_depth_texture = false;
			p_shader->spatial.uses_vertex = false;
			p_shader->spatial.uses_tangent = false;
			p_shader->spatial.uses_ensure_correct_normals = false;
			p_shader->spatial.writes_modelview_or_projection = false;
			p_shader->spatial.uses_world_coordinates = false;

			shaders.actions_scene.render_mode_values["blend_add"] = Pair<int *, int>(&p_shader->spatial.blend_mode, Shader::Spatial::BLEND_MODE_ADD);
			shaders.actions_scene.render_mode_values["blend_mix"] = Pair<int *, int>(&p_shader->spatial.blend_mode, Shader::Spatial::BLEND_MODE_MIX);
			shaders.actions_scene.render_mode_values["blend_sub"] = Pair<int *, int>(&p_shader->spatial.blend_mode, Shader::Spatial::BLEND_MODE_SUB);
			shaders.actions_scene.render_mode_values["blend_mul"] = Pair<int *, int>(&p_shader->spatial.blend_mode, Shader::Spatial::BLEND_MODE_MUL);

			shaders.actions_scene.render_mode_values["depth_draw_opaque"] = Pair<int *, int>(&p_shader->spatial.depth_draw_mode, Shader::Spatial::DEPTH_DRAW_OPAQUE);
			shaders.actions_scene.render_mode_values["depth_draw_always"] = Pair<int *, int>(&p_shader->spatial.depth_draw_mode, Shader::Spatial::DEPTH_DRAW_ALWAYS);
			shaders.actions_scene.render_mode_values["depth_draw_never"] = Pair<int *, int>(&p_shader->spatial.depth_draw_mode, Shader::Spatial::DEPTH_DRAW_NEVER);
			shaders.actions_scene.render_mode_values["depth_draw_alpha_prepass"] = Pair<int *, int>(&p_shader->spatial.depth_draw_mode, Shader::Spatial::DEPTH_DRAW_ALPHA_PREPASS);

			shaders.actions_scene.render_mode_values["cull_front"] = Pair<int *, int>(&p_shader->spatial.cull_mode, Shader::Spatial::CULL_MODE_FRONT);
			shaders.actions_scene.render_mode_values["cull_back"] = Pair<int *, int>(&p_shader->spatial.cull_mode, Shader::Spatial::CULL_MODE_BACK);
			shaders.actions_scene.render_mode_values["cull_disabled"] = Pair<int *, int>(&p_shader->spatial.cull_mode, Shader::Spatial::CULL_MODE_DISABLED);

			shaders.actions_scene.render_mode_flags["unshaded"] = &p_shader->spatial.unshaded;
			shaders.actions_scene.render_mode_flags["depth_test_disable"] = &p_shader->spatial.no_depth_test;

			shaders.actions_scene.render_mode_flags["vertex_lighting"] = &p_shader->spatial.uses_vertex_lighting;

			shaders.actions_scene.render_mode_flags["world_vertex_coords"] = &p_shader->spatial.uses_world_coordinates;

			shaders.actions_scene.render_mode_flags["ensure_correct_normals"] = &p_shader->spatial.uses_ensure_correct_normals;

			shaders.actions_scene.usage_flag_pointers["ALPHA"] = &p_shader->spatial.uses_alpha;
			shaders.actions_scene.usage_flag_pointers["ALPHA_SCISSOR"] = &p_shader->spatial.uses_alpha_scissor;

			shaders.actions_scene.usage_flag_pointers["SSS_STRENGTH"] = &p_shader->spatial.uses_sss;
			shaders.actions_scene.usage_flag_pointers["DISCARD"] = &p_shader->spatial.uses_discard;
			shaders.actions_scene.usage_flag_pointers["SCREEN_TEXTURE"] = &p_shader->spatial.uses_screen_texture;
			shaders.actions_scene.usage_flag_pointers["DEPTH_TEXTURE"] = &p_shader->spatial.uses_depth_texture;
			shaders.actions_scene.usage_flag_pointers["TIME"] = &p_shader->spatial.uses_time;

			// Use of any of these BUILTINS indicate the need for transformed tangents.
			// This is needed to know when to transform tangents in software skinning.
			shaders.actions_scene.usage_flag_pointers["TANGENT"] = &p_shader->spatial.uses_tangent;
			shaders.actions_scene.usage_flag_pointers["NORMALMAP"] = &p_shader->spatial.uses_tangent;

			shaders.actions_scene.write_flag_pointers["MODELVIEW_MATRIX"] = &p_shader->spatial.writes_modelview_or_projection;
			shaders.actions_scene.write_flag_pointers["PROJECTION_MATRIX"] = &p_shader->spatial.writes_modelview_or_projection;
			shaders.actions_scene.write_flag_pointers["VERTEX"] = &p_shader->spatial.uses_vertex;

			actions = &shaders.actions_scene;
			actions->uniforms = &p_shader->uniforms;

			if (p_shader->spatial.uses_screen_texture && p_shader->spatial.uses_depth_texture) {
				ERR_PRINT_ONCE("Using both SCREEN_TEXTURE and DEPTH_TEXTURE is not supported in GLES2");
			}

			if (p_shader->spatial.uses_depth_texture && !config.support_depth_texture) {
				ERR_PRINT_ONCE("Using DEPTH_TEXTURE is not permitted on this hardware, operation will fail.");
			}
		} break;

		default: {
			return;
		} break;
	}

	Error err = shaders.compiler.compile(p_shader->mode, p_shader->code, actions, p_shader->path, gen_code);
	if (err != OK) {
		return;
	}

	p_shader->shader->set_custom_shader_code(p_shader->custom_code_id, gen_code.vertex, gen_code.vertex_global, gen_code.fragment, gen_code.light, gen_code.fragment_global, gen_code.uniforms, gen_code.texture_uniforms, gen_code.custom_defines);

	p_shader->texture_count = gen_code.texture_uniforms.size();
	p_shader->texture_hints = gen_code.texture_hints;

	p_shader->uses_vertex_time = gen_code.uses_vertex_time;
	p_shader->uses_fragment_time = gen_code.uses_fragment_time;

	// some logic for batching
	if (p_shader->mode == VS::SHADER_CANVAS_ITEM) {
		if (p_shader->canvas_item.uses_modulate | p_shader->canvas_item.uses_color) {
			p_shader->canvas_item.batch_flags |= RasterizerStorageCommon::PREVENT_COLOR_BAKING;
		}
		if (p_shader->canvas_item.uses_vertex) {
			p_shader->canvas_item.batch_flags |= RasterizerStorageCommon::PREVENT_VERTEX_BAKING;
		}
		if (p_shader->canvas_item.uses_world_matrix | p_shader->canvas_item.uses_extra_matrix | p_shader->canvas_item.uses_projection_matrix | p_shader->canvas_item.uses_instance_custom) {
			p_shader->canvas_item.batch_flags |= RasterizerStorageCommon::PREVENT_ITEM_JOINING;
		}
	}

	p_shader->shader->set_custom_shader(p_shader->custom_code_id);
	p_shader->shader->bind();
#endif

	// cache uniform locations

	for (SelfList<Material> *E = p_shader->materials.first(); E; E = E->next()) {
		_material_make_dirty(E->self());
	}

	p_shader->valid = true;
	p_shader->version++;
}

void RasterizerStorageBGFX::update_dirty_shaders() {
	while (_shader_dirty_list.first()) {
		_update_shader(_shader_dirty_list.first()->self());
	}
}

void RasterizerStorageBGFX::update_dirty_materials() {
	while (_material_dirty_list.first()) {
		Material *material = _material_dirty_list.first()->self();
		_update_material(material);
	}
}

void RasterizerStorageBGFX::_material_make_dirty(Material *p_material) const {
	if (p_material->dirty_list.in_list()) {
		return;
	}

	_material_dirty_list.add(&p_material->dirty_list);
}

RID RasterizerStorageBGFX::material_create() {
	Material *material = memnew(Material);
	ERR_FAIL_COND_V(!material, RID());
	return material_owner.make_rid(material);
}

void RasterizerStorageBGFX::_update_material(Material *p_material) {
	if (p_material->dirty_list.in_list()) {
		_material_dirty_list.remove(&p_material->dirty_list);
	}

	if (p_material->shader && p_material->shader->dirty_list.in_list()) {
		_update_shader(p_material->shader);
	}

	if (p_material->shader && !p_material->shader->valid) {
		return;
	}

	{
		bool can_cast_shadow = false;
		bool is_animated = false;

		if (p_material->shader && p_material->shader->mode == VS::SHADER_SPATIAL) {
			if (p_material->shader->spatial.blend_mode == Shader::Spatial::BLEND_MODE_MIX &&
					(!(p_material->shader->spatial.uses_alpha && !p_material->shader->spatial.uses_alpha_scissor) || p_material->shader->spatial.depth_draw_mode == Shader::Spatial::DEPTH_DRAW_ALPHA_PREPASS)) {
				can_cast_shadow = true;
			}

			if (p_material->shader->spatial.uses_discard && p_material->shader->uses_fragment_time) {
				is_animated = true;
			}

			if (p_material->shader->spatial.uses_vertex && p_material->shader->uses_vertex_time) {
				is_animated = true;
			}

			if (can_cast_shadow != p_material->can_cast_shadow_cache || is_animated != p_material->is_animated_cache) {
				p_material->can_cast_shadow_cache = can_cast_shadow;
				p_material->is_animated_cache = is_animated;

				for (Map<Geometry *, int>::Element *E = p_material->geometry_owners.front(); E; E = E->next()) {
					E->key()->material_changed_notify();
				}

				for (Map<RasterizerScene::InstanceBase *, int>::Element *E = p_material->instance_owners.front(); E; E = E->next()) {
					E->key()->base_changed(false, true);
				}
			}
		}
	}

	// uniforms and other things will be set in the use_material method in ShaderGLES2

	if (p_material->shader && p_material->shader->texture_count > 0) {
		p_material->textures.resize(p_material->shader->texture_count);

		for (Map<StringName, ShaderLanguage::ShaderNode::Uniform>::Element *E = p_material->shader->uniforms.front(); E; E = E->next()) {
			if (E->get().texture_order < 0) {
				continue; // not a texture, does not go here
			}

			RID texture;

			Map<StringName, Variant>::Element *V = p_material->params.find(E->key());

			if (V) {
				texture = V->get();
			}

			if (!texture.is_valid()) {
				Map<StringName, RID>::Element *W = p_material->shader->default_textures.find(E->key());

				if (W) {
					texture = W->get();
				}
			}

			p_material->textures.write[E->get().texture_order] = Pair<StringName, RID>(E->key(), texture);
		}
	} else {
		p_material->textures.clear();
	}
}
void RasterizerStorageBGFX::material_set_shader(RID p_shader_material, RID p_shader) {
	Material *material = material_owner.get(p_shader_material);
	ERR_FAIL_COND(!material);

	Shader *shader = shader_owner.getornull(p_shader);

	if (material->shader) {
		// if a shader is present, remove the old shader
		material->shader->materials.remove(&material->list);
	}

	material->shader = shader;

	if (shader) {
		shader->materials.add(&material->list);
	}

	_material_make_dirty(material);
}

RID RasterizerStorageBGFX::material_get_shader(RID p_shader_material) const {
	const Material *material = material_owner.get(p_shader_material);
	ERR_FAIL_COND_V(!material, RID());

	if (material->shader) {
		return material->shader->self;
	}

	return RID();
}

void RasterizerStorageBGFX::material_set_param(RID p_material, const StringName &p_param, const Variant &p_value) {
	Material *material = material_owner.get(p_material);
	ERR_FAIL_COND(!material);

	if (p_value.get_type() == Variant::NIL) {
		material->params.erase(p_param);
	} else {
		material->params[p_param] = p_value;
	}

	_material_make_dirty(material);
}

Variant RasterizerStorageBGFX::material_get_param(RID p_material, const StringName &p_param) const {
	const Material *material = material_owner.get(p_material);
	ERR_FAIL_COND_V(!material, RID());

	if (material->params.has(p_param)) {
		return material->params[p_param];
	}

	return material_get_param_default(p_material, p_param);
	//	return Variant();
}

Variant RasterizerStorageBGFX::material_get_param_default(RID p_material, const StringName &p_param) const {
	const Material *material = material_owner.get(p_material);
	ERR_FAIL_COND_V(!material, Variant());

	if (material->shader) {
		if (material->shader->uniforms.has(p_param)) {
			ShaderLanguage::ShaderNode::Uniform uniform = material->shader->uniforms[p_param];
			Vector<ShaderLanguage::ConstantNode::Value> default_value = uniform.default_value;
			return ShaderLanguage::constant_value_to_variant(default_value, uniform.type, uniform.hint);
		}
	}
	return Variant();
	//	return Variant();
}

void RasterizerStorageBGFX::material_add_instance_owner(RID p_material, RasterizerScene::InstanceBase *p_instance) {
	Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND(!material);

	Map<RasterizerScene::InstanceBase *, int>::Element *E = material->instance_owners.find(p_instance);
	if (E) {
		E->get()++;
	} else {
		material->instance_owners[p_instance] = 1;
	}
}

void RasterizerStorageBGFX::material_remove_instance_owner(RID p_material, RasterizerScene::InstanceBase *p_instance) {
	Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND(!material);

	Map<RasterizerScene::InstanceBase *, int>::Element *E = material->instance_owners.find(p_instance);
	ERR_FAIL_COND(!E);

	E->get()--;

	if (E->get() == 0) {
		material->instance_owners.erase(E);
	}
}

RID RasterizerStorageBGFX::texture_create() {
	//	return RID();
	Texture *texture = memnew(Texture);
	ERR_FAIL_COND_V(!texture, RID());
	return texture_owner.make_rid(texture);
}

void RasterizerStorageBGFX::texture_allocate(RID p_texture, int p_width, int p_height, int p_depth_3d, Image::Format p_format, VisualServer::TextureType p_type, uint32_t p_flags) {
	Texture *t = texture_owner.getornull(p_texture);
	ERR_FAIL_COND(!t);
	t->width = p_width;
	t->height = p_height;
	t->flags = p_flags;
	t->format = p_format;
	t->image = Ref<Image>(memnew(Image));
	t->image->create(p_width, p_height, false, p_format);

	// defer creation of bgfx texture until data is available
}

void RasterizerStorageBGFX::texture_set_data(RID p_texture, const Ref<Image> &p_image, int p_level) {
	Texture *t = texture_owner.getornull(p_texture);
	ERR_FAIL_COND(!t);

	if (p_image.is_valid()) {
		print_line(String(t->path) + " : width " + itos(p_image->get_width()) + ", height " + itos(p_image->get_height()) + ", format : " + Image::get_format_name(p_image->get_format()));
	}

	t->width = p_image->get_width();
	t->height = p_image->get_height();
	t->format = p_image->get_format();
	t->image->create(t->width, t->height, p_image->has_mipmaps(), p_image->get_format(), p_image->get_data());

	if (true) {
		//	if (!bgfx::isValid(t->bg_handle)) {
		bgfx::TextureFormat::Enum format = bgfx::TextureFormat::RGBA8;

		bool pow2 = false;

		switch (t->format) {
			case Image::FORMAT_ETC: {
				format = bgfx::TextureFormat::ETC1;
			} break;
			case Image::FORMAT_ETC2_RGB8: {
				format = bgfx::TextureFormat::ETC2;
			} break;
				//		case Image::FORMAT_ETC2_RGBA8: {
				//			format = bgfx::TextureFormat::ETC2A;
				//		} break;
			case Image::FORMAT_RGBA8: {
				format = bgfx::TextureFormat::RGBA8;
			} break;
			case Image::FORMAT_RGB8: {
				format = bgfx::TextureFormat::RGB8;
			} break;
			case Image::FORMAT_L8:
			case Image::FORMAT_R8: {
				format = bgfx::TextureFormat::R8;
			} break;
			case Image::FORMAT_LA8: {
				//format = bgfx::TextureFormat::RG8;
				t->image->convert(Image::FORMAT_RGBA8);
				t->format = p_image->get_format();
				format = bgfx::TextureFormat::RGBA8;
			} break;
			case Image::FORMAT_DXT1: {
				format = bgfx::TextureFormat::BC1;
				pow2 = true;
			} break;
			case Image::FORMAT_DXT3: {
				format = bgfx::TextureFormat::BC2;
			} break;
			case Image::FORMAT_DXT5: {
				format = bgfx::TextureFormat::BC3;
			} break;
			default: {
				ERR_FAIL_MSG("Texture format not supported.");
			} break;
		}
		//		return;

		PoolVector<uint8_t> data = t->image->get_data();
		const PoolVector<uint8_t>::Read r = data.read();

		if (pow2) {
			unsigned int w = t->width;
			unsigned int h = t->height;
			if ((w != next_power_of_2(w)) || (h != next_power_of_2(h))) {
				// bodge, convert to RGBA
				Image temp;
				temp.create(w, h, t->image->has_mipmaps(), t->image->get_format(), data);
				//temp.convert(Image::FORMAT_RGBA8);
				temp.decompress();
				PoolVector<uint8_t> data_rgba = temp.get_data();
				const PoolVector<uint8_t>::Read r2 = data_rgba.read();

				const bgfx::Memory *mem = bgfx::alloc(data_rgba.size());
				memcpy(mem->data, r2.ptr(), data_rgba.size());
				t->bg_handle = bgfx::createTexture2D(t->width, t->height, temp.has_mipmaps(), 1, bgfx::TextureFormat::RGBA8, 0, mem);

				return;
			}
		}
		//		int bytes_required = p_image->get_image_data_size(p_image->get_width(), p_image->get_height(), p_image->get_format(), p_image->has_mipmaps());
		//		DEV_ASSERT(bytes_required == data.size());

		const bgfx::Memory *mem = bgfx::alloc(data.size());
		memcpy(mem->data, r.ptr(), data.size());

		// delete any previous texture

		if (!t->bg_is_mutable) {
			BGFX_DESTROY(t->bg_handle);

			if (t->bg_update_count < 4) {
				// static
				t->bg_handle = bgfx::createTexture2D(t->width, t->height, t->image->has_mipmaps(), 1, format, 0, mem);
			} else {
				// convert to dynamic (mutable)
				t->bg_handle = bgfx::createTexture2D(t->width, t->height, t->image->has_mipmaps(), 1, format, 0, nullptr);
				t->bg_is_mutable = true;
				bgfx::updateTexture2D(t->bg_handle, 0, 0, 0, 0, t->width, t->height, mem);
			}
		} else {
			// mutable update
			bgfx::updateTexture2D(t->bg_handle, 0, 0, 0, 0, t->width, t->height, mem);
		}

		t->bg_update_count++;
	} else {
		ERR_FAIL_MSG("Immutable texture can only be set once.");
	}
}

uint32_t RasterizerStorageBGFX::texture_get_width(RID p_texture) const {
	Texture *t = texture_owner.getornull(p_texture);
	ERR_FAIL_COND_V(!t, 0);
	return t->width;
}

uint32_t RasterizerStorageBGFX::texture_get_height(RID p_texture) const {
	Texture *t = texture_owner.getornull(p_texture);
	ERR_FAIL_COND_V(!t, 0);
	return t->height;
}

void RasterizerStorageBGFX::texture_set_size_override(RID p_texture, int p_width, int p_height, int p_depth_3d) {
	Texture *t = texture_owner.getornull(p_texture);
	ERR_FAIL_NULL(t);
}

void RasterizerStorageBGFX::texture_set_force_redraw_if_visible(RID p_texture, bool p_enable) {
	Texture *texture = texture_owner.getornull(p_texture);
	ERR_FAIL_COND(!texture);

	texture->redraw_if_visible = p_enable;
}

void RasterizerStorageBGFX::texture_set_proxy(RID p_texture, RID p_proxy) {
	Texture *texture = texture_owner.getornull(p_texture);
	ERR_FAIL_COND(!texture);

	if (texture->proxy) {
		texture->proxy->proxy_owners.erase(texture);
		texture->proxy = nullptr;
	}

	if (p_proxy.is_valid()) {
		Texture *proxy = texture_owner.get(p_proxy);
		ERR_FAIL_COND(!proxy);
		ERR_FAIL_COND(proxy == texture);
		proxy->proxy_owners.insert(texture);
		texture->proxy = proxy;
	}
}

Size2 RasterizerStorageBGFX::texture_size_with_proxy(RID p_texture) const {
	const Texture *texture = texture_owner.getornull(p_texture);
	ERR_FAIL_COND_V(!texture, Size2());
	if (texture->proxy) {
		return Size2(texture->proxy->width, texture->proxy->height);
	} else {
		return Size2(texture->width, texture->height);
	}
}

void RasterizerStorageBGFX::_render_target_clear(RenderTarget *rt) {
	//return;

	if (rt->id_view != UINT16_MAX) {
		//		if (rt->id_view != 0)
		//			bgfx::setViewFrameBuffer(rt->id_view, BGFX_INVALID_HANDLE);
	}

	if (bgfx::isValid(rt->hFrameBuffer)) {
		BGFX_DESTROY(rt->hFrameBuffer);
	}
}

void RasterizerStorageBGFX::_render_target_allocate(RenderTarget *rt) {
	//	return;
	//DEV_ASSERT(rt->id_view == UINT16_MAX);

	// do not allocate a render target with no size
	if (rt->width <= 0 || rt->height <= 0) {
		return;
	}

	if (!rt->flags[RENDER_TARGET_DIRECT_TO_SCREEN] && (rt->id_view != 0)) {
		DEV_ASSERT(!bgfx::isValid(rt->hFrameBuffer));
		rt->hFrameBuffer = bgfx::createFrameBuffer(rt->width, rt->height, bgfx::TextureFormat::RGBA8);

		// associate framebuffer with view
		bgfx::setViewFrameBuffer(rt->id_view, rt->hFrameBuffer);

		Texture *t = texture_owner.getornull(rt->texture);
		if (t) {
			BGFX_DESTROY(t->bg_handle);
			t->bg_handle = bgfx::getTexture(rt->hFrameBuffer);
			t->width = rt->width;
			t->height = rt->height;
		}
	}

	_render_target_set_viewport(rt, 0, 0, rt->width, rt->height);
	bgfx::setViewMode(rt->id_view, bgfx::ViewMode::Sequential);
	//	bgfx::setViewMode(rt->id_view, bgfx::ViewMode::DepthAscending);
}

void RasterizerStorageBGFX::_render_target_set_viewport(RenderTarget *rt, uint16_t p_x, uint16_t p_y, uint16_t p_width, uint16_t p_height) {
	//	return;
	ERR_FAIL_COND(rt->id_view == UINT16_MAX);
	bgfx::setViewRect(rt->id_view, p_x, p_y, p_width, p_height);
}

void RasterizerStorageBGFX::_render_target_set_scissor(RenderTarget *rt, uint16_t p_x, uint16_t p_y, uint16_t p_width, uint16_t p_height) {
	//	return;
	ERR_FAIL_COND(rt->id_view == UINT16_MAX);
	//bgfx::setViewScissor(rt->id_view, p_x, p_y, p_width, p_height);
}

void RasterizerStorageBGFX::_render_target_set_view_clear(RenderTarget *rt, const Color &p_color) {
	//	bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH | BGFX_CLEAR_STENCIL, p_color.to_rgba32());
	//	return;
	ERR_FAIL_COND(rt->id_view == UINT16_MAX);
	bgfx::setViewClear(rt->id_view, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH | BGFX_CLEAR_STENCIL, p_color.to_rgba32());
}

RID RasterizerStorageBGFX::render_target_create() {
	uint32_t view_id = _request_bgfx_view();
	ERR_FAIL_COND_V(view_id == UINT16_MAX, RID());

	RenderTarget *rt = memnew(RenderTarget);

	Texture *t = memnew(Texture);

	//t->type = VS::TEXTURE_TYPE_2D;
	//t->alloc_height = 0;
	//t->alloc_width = 0;
	t->format = Image::FORMAT_RGBA8;
	//t->target = GL_TEXTURE_2D;
	//t->gl_format_cache = 0;
	//t->gl_internal_format_cache = 0;
	//t->gl_type_cache = 0;
	//t->data_size = 0;
	//	t->total_data_size = 0;
	//	t->ignore_mipmaps = false;
	//	t->compressed = false;
	//	t->mipmaps = 1;
	//	t->active = true;
	//	t->tex_id = 0;
	t->render_target = rt;

	rt->texture = texture_owner.make_rid(t);

	rt->id_view = view_id;

	return render_target_owner.make_rid(rt);
}

void RasterizerStorageBGFX::render_target_set_position(RID p_render_target, int p_x, int p_y) {
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	rt->x = p_x;
	rt->y = p_y;
}

void RasterizerStorageBGFX::render_target_set_size(RID p_render_target, int p_width, int p_height) {
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	if (p_width == rt->width && p_height == rt->height) {
		return;
	}

	_render_target_clear(rt);

	rt->width = p_width;
	rt->height = p_height;

	_render_target_allocate(rt);
}

RID RasterizerStorageBGFX::render_target_get_texture(RID p_render_target) const {
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND_V(!rt, RID());
	return rt->texture;
}

uint32_t RasterizerStorageBGFX::render_target_get_depth_texture_id(RID p_render_target) const {
	return 0;
}

void RasterizerStorageBGFX::render_target_set_flag(RID p_render_target, RenderTargetFlags p_flag, bool p_value) {
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	// When setting DIRECT_TO_SCREEN, you need to clear before the value is set, but allocate after as
	// those functions change how they operate depending on the value of DIRECT_TO_SCREEN
	if (p_flag == RENDER_TARGET_DIRECT_TO_SCREEN && p_value != rt->flags[RENDER_TARGET_DIRECT_TO_SCREEN]) {
		_render_target_clear(rt);
		rt->flags[p_flag] = p_value;
		_render_target_allocate(rt);
	}

	rt->flags[p_flag] = p_value;

	switch (p_flag) {
		case RENDER_TARGET_TRANSPARENT:
		case RENDER_TARGET_HDR:
		case RENDER_TARGET_NO_3D:
		case RENDER_TARGET_NO_SAMPLING:
		case RENDER_TARGET_NO_3D_EFFECTS: {
			//must reset for these formats
			_render_target_clear(rt);
			_render_target_allocate(rt);

		} break;
		default: {
		}
	}
}

bool RasterizerStorageBGFX::render_target_was_used(RID p_render_target) {
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND_V(!rt, false);

	return rt->used_in_frame;
}

void RasterizerStorageBGFX::render_target_clear_used(RID p_render_target) {
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	rt->used_in_frame = false;
}

bool RasterizerStorageBGFX::free(RID p_rid) {
	if (render_target_owner.owns(p_rid)) {
		RenderTarget *rt = render_target_owner.getornull(p_rid);
		_render_target_clear(rt);

		if (rt->id_view != UINT16_MAX) {
			_free_bgfx_view(rt->id_view);
			rt->id_view = UINT16_MAX;
		}

		free(rt->texture);
		//		Texture *t = texture_owner.get(rt->texture);
		//		texture_owner.free(rt->texture);
		//		memdelete(t);
		rt->texture = RID();
		render_target_owner.free(p_rid);
		memdelete(rt);

		return true;
	} else if (texture_owner.owns(p_rid)) {
		// delete the texture
		Texture *texture = texture_owner.get(p_rid);

		if (bgfx::isValid(texture->bg_handle)) {
			bgfx::destroy(texture->bg_handle);
			texture->bg_handle = BGFX_INVALID_HANDLE;
		}

		texture_owner.free(p_rid);
		memdelete(texture);
	} else if (mesh_owner.owns(p_rid)) {
		// delete the mesh
		BGFXMesh *mesh = mesh_owner.getornull(p_rid);
		mesh_owner.free(p_rid);
		memdelete(mesh);
	} else if (lightmap_capture_data_owner.owns(p_rid)) {
		// delete the lightmap
		LightmapCapture *lightmap_capture = lightmap_capture_data_owner.getornull(p_rid);
		lightmap_capture_data_owner.free(p_rid);
		memdelete(lightmap_capture);

	} else if (material_owner.owns(p_rid)) {
		Material *m = material_owner.get(p_rid);

		if (m->shader) {
			m->shader->materials.remove(&m->list);
		}

		for (Map<Geometry *, int>::Element *E = m->geometry_owners.front(); E; E = E->next()) {
			Geometry *g = E->key();
			g->material = RID();
		}

		for (Map<RasterizerScene::InstanceBase *, int>::Element *E = m->instance_owners.front(); E; E = E->next()) {
			RasterizerScene::InstanceBase *ins = E->key();

			if (ins->material_override == p_rid) {
				ins->material_override = RID();
			}

			if (ins->material_overlay == p_rid) {
				ins->material_overlay = RID();
			}

			for (int i = 0; i < ins->materials.size(); i++) {
				if (ins->materials[i] == p_rid) {
					ins->materials.write[i] = RID();
				}
			}
		}

		material_owner.free(p_rid);
		memdelete(m);

		return true;
		//	} else if (skeleton_owner.owns(p_rid)) {
	} else if (shader_owner.owns(p_rid)) {
		Shader *shader = shader_owner.get(p_rid);

		if (shader->shader && shader->custom_code_id) {
			shader->shader->free_custom_shader(shader->custom_code_id);
		}

		if (shader->dirty_list.in_list()) {
			_shader_dirty_list.remove(&shader->dirty_list);
		}

		while (shader->materials.first()) {
			Material *m = shader->materials.first()->self();

			m->shader = nullptr;
			_material_make_dirty(m);

			shader->materials.remove(shader->materials.first());
		}

		shader_owner.free(p_rid);
		memdelete(shader);

		return true;
	} else {
		return false;
	}

	return true;
}

// Meshes
static PoolVector<uint8_t> _unpack_half_floats(const PoolVector<uint8_t> &array, uint32_t &format, int p_vertices) {
	uint32_t p_format = format;

	static int src_size[VS::ARRAY_MAX];
	static int dst_size[VS::ARRAY_MAX];
	static int to_convert[VS::ARRAY_MAX];

	int src_stride = 0;
	int dst_stride = 0;

//#define GODOT_LOG_UNPACK_VERTEX_FORMAT
#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
	String sz = "unpack ";
#endif

	for (int i = 0; i < VS::ARRAY_MAX; i++) {
		to_convert[i] = 0;
		if (!(p_format & (1 << i))) {
			src_size[i] = 0;
			dst_size[i] = 0;
			continue;
		}

		switch (i) {
			case VS::ARRAY_VERTEX: {
				if (p_format & VS::ARRAY_COMPRESS_VERTEX) {
					if (p_format & VS::ARRAY_FLAG_USE_2D_VERTICES) {
						src_size[i] = 4;
						dst_size[i] = 8;
						to_convert[i] = 2;
#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
						sz += "V 2D 4 to 8, ";
#endif
					} else {
						src_size[i] = 8;
						dst_size[i] = 12;
						to_convert[i] = 3;
#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
						sz += "V 8 to 12, ";
#endif
					}

					format &= ~VS::ARRAY_COMPRESS_VERTEX;
				} else {
					if (p_format & VS::ARRAY_FLAG_USE_2D_VERTICES) {
						src_size[i] = 8;
						dst_size[i] = 8;
#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
						sz += "V 2D 8 to 8, ";
#endif
					} else {
						src_size[i] = 12;
						dst_size[i] = 12;
#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
						sz += "V 12 to 12, ";
#endif
					}
				}

			} break;
			case VS::ARRAY_NORMAL: {
				if (p_format & VS::ARRAY_FLAG_USE_OCTAHEDRAL_COMPRESSION) {
					src_size[i] = 4;
					dst_size[i] = 4;
#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
					sz += "N 4 to 4 (oct), ";
#endif
				} else {
					if (p_format & VS::ARRAY_COMPRESS_NORMAL) {
						src_size[i] = 4;
						dst_size[i] = 4;
#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
						sz += "N 4 to 4, ";
#endif
					} else {
						src_size[i] = 12;
						dst_size[i] = 12;
#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
						sz += "N 12 to 12, ";
#endif
					}
				}

			} break;
			case VS::ARRAY_TANGENT: {
				if (p_format & VS::ARRAY_FLAG_USE_OCTAHEDRAL_COMPRESSION) {
					if (!(p_format & VS::ARRAY_COMPRESS_TANGENT && p_format & VS::ARRAY_COMPRESS_NORMAL)) {
						src_size[VS::ARRAY_NORMAL] = 8;
						dst_size[VS::ARRAY_NORMAL] = 8;

						// bug fix, we need to keep the stride in sync,
						// as we are updating an attribute that is not [i]
						src_stride += 4;
						dst_stride += 4;

#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
						sz += "NT 8 to 8, ";
#endif
					}
					src_size[i] = 0;
					dst_size[i] = 0;
				} else {
					if (p_format & VS::ARRAY_COMPRESS_TANGENT) {
						src_size[i] = 4;
						dst_size[i] = 4;
#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
						sz += "T 4 to 4, ";
#endif
					} else {
						src_size[i] = 16;
						dst_size[i] = 16;
#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
						sz += "T 16 to 16, ";
#endif
					}
				}

			} break;
			case VS::ARRAY_COLOR: {
				if (p_format & VS::ARRAY_COMPRESS_COLOR) {
					src_size[i] = 4;
					dst_size[i] = 4;
#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
					sz += "C 4 to 4, ";
#endif
				} else {
					src_size[i] = 16;
					dst_size[i] = 16;
#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
					sz += "C 16 to 16, ";
#endif
				}

			} break;
			case VS::ARRAY_TEX_UV: {
				if (p_format & VS::ARRAY_COMPRESS_TEX_UV) {
					src_size[i] = 4;
					to_convert[i] = 2;
					format &= ~VS::ARRAY_COMPRESS_TEX_UV;
#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
					sz += "UV 4 to 8, ";
#endif
				} else {
					src_size[i] = 8;
#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
					sz += "UV 8 to 8, ";
#endif
				}

				dst_size[i] = 8;

			} break;
			case VS::ARRAY_TEX_UV2: {
				if (p_format & VS::ARRAY_COMPRESS_TEX_UV2) {
					src_size[i] = 4;
					to_convert[i] = 2;
					format &= ~VS::ARRAY_COMPRESS_TEX_UV2;
#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
					sz += "UV2 4 to 8, ";
#endif
				} else {
					src_size[i] = 8;
#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
					sz += "UV2 8 to 8, ";
#endif
				}

				dst_size[i] = 8;

			} break;
			case VS::ARRAY_BONES: {
				if (p_format & VS::ARRAY_FLAG_USE_16_BIT_BONES) {
					src_size[i] = 8;
					dst_size[i] = 8;
#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
					sz += "Bones 8 to 8, ";
#endif
				} else {
					src_size[i] = 4;
					dst_size[i] = 4;
#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
					sz += "Bones 4 to 4, ";
#endif
				}

			} break;
			case VS::ARRAY_WEIGHTS: {
				if (p_format & VS::ARRAY_COMPRESS_WEIGHTS) {
					src_size[i] = 8;
					dst_size[i] = 8;
#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
					sz += "Weights 8 to 8, ";
#endif
				} else {
					src_size[i] = 16;
					dst_size[i] = 16;
#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
					sz += "Weights 16 to 16, ";
#endif
				}

			} break;
			case VS::ARRAY_INDEX: {
				src_size[i] = 0;
				dst_size[i] = 0;
#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
				sz += "Ind 0 to 0, ";
#endif

			} break;
		}

		src_stride += src_size[i];
		dst_stride += dst_size[i];
	}

	PoolVector<uint8_t> ret;
	ret.resize(p_vertices * dst_stride);

	PoolVector<uint8_t>::Read r = array.read();
	PoolVector<uint8_t>::Write w = ret.write();

	int src_offset = 0;
	int dst_offset = 0;

	for (int i = 0; i < VS::ARRAY_MAX; i++) {
		if (src_size[i] == 0) {
			continue; //no go
		}
		const uint8_t *rptr = r.ptr();
		uint8_t *wptr = w.ptr();
		if (to_convert[i]) { //converting

			for (int j = 0; j < p_vertices; j++) {
				const uint16_t *src = (const uint16_t *)&rptr[src_stride * j + src_offset];
				float *dst = (float *)&wptr[dst_stride * j + dst_offset];

				for (int k = 0; k < to_convert[i]; k++) {
					dst[k] = Math::half_to_float(src[k]);
				}
			}

		} else {
			//just copy
			for (int j = 0; j < p_vertices; j++) {
				for (int k = 0; k < src_size[i]; k++) {
					wptr[dst_stride * j + dst_offset + k] = rptr[src_stride * j + src_offset + k];
				}
			}
		}

		src_offset += src_size[i];
		dst_offset += dst_size[i];
	}

	r.release();
	w.release();

#ifdef GODOT_LOG_UNPACK_VERTEX_FORMAT
	print_line(sz);
#endif

	print_line("_unpack_half_floats source_stride " + itos(src_stride) + ", dest_stride " + itos(dst_stride));

	return ret;
}

void RasterizerStorageBGFX::mesh_add_surface(RID p_mesh, uint32_t p_format, VS::PrimitiveType p_primitive, const PoolVector<uint8_t> &p_array, int p_vertex_count, const PoolVector<uint8_t> &p_index_array, int p_index_count, const AABB &p_aabb, const Vector<PoolVector<uint8_t>> &p_blend_shapes, const Vector<AABB> &p_bone_aabbs) {
	BGFXMesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND(!mesh);
	ERR_FAIL_COND(!(p_format & VS::ARRAY_FORMAT_VERTEX));

	//must have index and bones, both.
	{
		uint32_t bones_weight = VS::ARRAY_FORMAT_BONES | VS::ARRAY_FORMAT_WEIGHTS;
		ERR_FAIL_COND_MSG((p_format & bones_weight) && (p_format & bones_weight) != bones_weight, "Array must have both bones and weights in format or none.");
	}

	bool use_split_stream = GLOBAL_GET("rendering/misc/mesh_storage/split_stream") && !(p_format & VS::ARRAY_FLAG_USE_DYNAMIC_UPDATE);
	BGFXSurface::Attrib attribs[VS::ARRAY_MAX];

	int attributes_base_offset = 0;
	int attributes_stride = 0;
	int positions_stride = 0;
	bool uses_half_float = false;

//#define GODOT_LOG_VERTEX_FORMAT
#ifdef GODOT_LOG_VERTEX_FORMAT
	String sz;
	if (use_split_stream) {
		sz += "SPLIT STREAM ";
	}
#endif

	for (int i = 0; i < VS::ARRAY_MAX; i++) {
		attribs[i].index = i;

		if (!(p_format & (1 << i))) {
			attribs[i].enabled = false;
			attribs[i].integer = false;
			continue;
		}

		attribs[i].enabled = true;
		attribs[i].offset = attributes_base_offset + attributes_stride;
		attribs[i].integer = false;

		switch (i) {
			case VS::ARRAY_VERTEX: {
				if (p_format & VS::ARRAY_FLAG_USE_2D_VERTICES) {
					attribs[i].size = 2;
#ifdef GODOT_LOG_VERTEX_FORMAT
					sz += "V 2D, ";
#endif
				} else {
					attribs[i].size = (p_format & VS::ARRAY_COMPRESS_VERTEX) ? 4 : 3;
				}

				if (p_format & VS::ARRAY_COMPRESS_VERTEX) {
					attribs[i].type = BGFXSurface::AT_HALF_FLOAT;
					positions_stride += attribs[i].size * 2;
					uses_half_float = true;
#ifdef GODOT_LOG_VERTEX_FORMAT
					sz += "V hf, ";
#endif
				} else {
					attribs[i].type = BGFXSurface::AT_FLOAT;
					positions_stride += attribs[i].size * 4;
#ifdef GODOT_LOG_VERTEX_FORMAT
					sz += "V f, ";
#endif
				}

				attribs[i].normalized = false;

				if (use_split_stream) {
					attributes_base_offset = positions_stride * p_vertex_count;
				} else {
					attributes_base_offset = positions_stride;
				}

			} break;
			case VS::ARRAY_NORMAL: {
				if (p_format & VS::ARRAY_FLAG_USE_OCTAHEDRAL_COMPRESSION) {
					// Always pack normal and tangent into vec4
					// normal will be xy tangent will be zw
					// normal will always be oct32 encoded
					// UNLESS tangent exists and is also compressed
					// then it will be oct16 encoded along with tangent
					attribs[i].normalized = true;
					attribs[i].size = 2;
					attribs[i].type = BGFXSurface::AT_SHORT;
					attributes_stride += 4;
#ifdef GODOT_LOG_VERTEX_FORMAT
					sz += "N oct, ";
#endif
				} else {
					attribs[i].size = 3;

					if (p_format & VS::ARRAY_COMPRESS_NORMAL) {
						attribs[i].type = BGFXSurface::AT_BYTE;
						attributes_stride += 4; //pad extra byte
						attribs[i].normalized = true;
#ifdef GODOT_LOG_VERTEX_FORMAT
						sz += "N comp, ";
#endif
					} else {
						attribs[i].type = BGFXSurface::AT_FLOAT;
						attributes_stride += 12;
						attribs[i].normalized = false;
#ifdef GODOT_LOG_VERTEX_FORMAT
						sz += "N f, ";
#endif
					}
				}

			} break;
			case VS::ARRAY_TANGENT: {
				if (p_format & VS::ARRAY_FLAG_USE_OCTAHEDRAL_COMPRESSION) {
					attribs[i].enabled = false;
					attribs[VS::ARRAY_NORMAL].size = 4;
					if (p_format & VS::ARRAY_COMPRESS_TANGENT && p_format & VS::ARRAY_COMPRESS_NORMAL) {
						// normal and tangent will each be oct16 (2 bytes each)
						// pack into single vec4<GL_BYTE> for memory bandwidth
						// savings while keeping 4 byte alignment
						attribs[VS::ARRAY_NORMAL].type = BGFXSurface::AT_BYTE;
#ifdef GODOT_LOG_VERTEX_FORMAT
						sz += "NT oct, ";
#endif
					} else {
						// normal and tangent will each be oct32 (4 bytes each)
						attributes_stride += 4;
#ifdef GODOT_LOG_VERTEX_FORMAT
						sz += "T oct, ";
#endif
					}
				} else {
					attribs[i].size = 4;

					if (p_format & VS::ARRAY_COMPRESS_TANGENT) {
						attribs[i].type = BGFXSurface::AT_BYTE;
						attributes_stride += 4;
						attribs[i].normalized = true;
#ifdef GODOT_LOG_VERTEX_FORMAT
						sz += "T comp, ";
#endif
					} else {
						attribs[i].type = BGFXSurface::AT_FLOAT;
						attributes_stride += 16;
						attribs[i].normalized = false;
#ifdef GODOT_LOG_VERTEX_FORMAT
						sz += "T f, ";
#endif
					}
				}

			} break;
			case VS::ARRAY_COLOR: {
				attribs[i].size = 4;

				if (p_format & VS::ARRAY_COMPRESS_COLOR) {
					attribs[i].type = BGFXSurface::AT_UBYTE;
					attributes_stride += 4;
					attribs[i].normalized = true;
#ifdef GODOT_LOG_VERTEX_FORMAT
					sz += "C b, ";
#endif
				} else {
					attribs[i].type = BGFXSurface::AT_FLOAT;
					attributes_stride += 16;
					attribs[i].normalized = false;
#ifdef GODOT_LOG_VERTEX_FORMAT
					sz += "C f, ";
#endif
				}

			} break;
			case VS::ARRAY_TEX_UV: {
				attribs[i].size = 2;

				if (p_format & VS::ARRAY_COMPRESS_TEX_UV) {
					attribs[i].type = BGFXSurface::AT_HALF_FLOAT;
					attributes_stride += 4;
					uses_half_float = true;
#ifdef GODOT_LOG_VERTEX_FORMAT
					sz += "UV hf, ";
#endif
				} else {
					attribs[i].type = BGFXSurface::AT_FLOAT;
					attributes_stride += 8;
#ifdef GODOT_LOG_VERTEX_FORMAT
					sz += "UV f, ";
#endif
				}

				attribs[i].normalized = false;

			} break;
			case VS::ARRAY_TEX_UV2: {
				attribs[i].size = 2;

				if (p_format & VS::ARRAY_COMPRESS_TEX_UV2) {
					attribs[i].type = BGFXSurface::AT_HALF_FLOAT;
					attributes_stride += 4;
					uses_half_float = true;
#ifdef GODOT_LOG_VERTEX_FORMAT
					sz += "UV2 hf, ";
#endif
				} else {
					attribs[i].type = BGFXSurface::AT_FLOAT;
					attributes_stride += 8;
#ifdef GODOT_LOG_VERTEX_FORMAT
					sz += "UV2 f, ";
#endif
				}
				attribs[i].normalized = false;

			} break;
			case VS::ARRAY_BONES: {
				attribs[i].size = 4;

				if (p_format & VS::ARRAY_FLAG_USE_16_BIT_BONES) {
					attribs[i].type = BGFXSurface::AT_USHORT;
					attributes_stride += 8;
#ifdef GODOT_LOG_VERTEX_FORMAT
					sz += "Bones us, ";
#endif
				} else {
					attribs[i].type = BGFXSurface::AT_UBYTE;
					attributes_stride += 4;
#ifdef GODOT_LOG_VERTEX_FORMAT
					sz += "Bones ub, ";
#endif
				}

				attribs[i].normalized = false;
				attribs[i].integer = true;

			} break;
			case VS::ARRAY_WEIGHTS: {
				attribs[i].size = 4;

				if (p_format & VS::ARRAY_COMPRESS_WEIGHTS) {
					attribs[i].type = BGFXSurface::AT_USHORT;
					attributes_stride += 8;
					attribs[i].normalized = true;
#ifdef GODOT_LOG_VERTEX_FORMAT
					sz += "Weights us, ";
#endif
				} else {
					attribs[i].type = BGFXSurface::AT_FLOAT;
					attributes_stride += 16;
					attribs[i].normalized = false;
#ifdef GODOT_LOG_VERTEX_FORMAT
					sz += "Weights f, ";
#endif
				}

			} break;
			case VS::ARRAY_INDEX: {
				attribs[i].size = 1;

				if (p_vertex_count >= (1 << 16)) {
					attribs[i].type = BGFXSurface::AT_UINT;
					attribs[i].stride = 4;
#ifdef GODOT_LOG_VERTEX_FORMAT
					sz += "Ind ui, ";
#endif
				} else {
					attribs[i].type = BGFXSurface::AT_USHORT;
					attribs[i].stride = 2;
#ifdef GODOT_LOG_VERTEX_FORMAT
					sz += "Ind us, ";
#endif
				}

				attribs[i].normalized = false;

			} break;
		}
	}

	if (use_split_stream) {
		attribs[VS::ARRAY_VERTEX].stride = positions_stride;
		for (int i = 1; i < VS::ARRAY_MAX - 1; i++) {
			attribs[i].stride = attributes_stride;
		}
	} else {
		for (int i = 0; i < VS::ARRAY_MAX - 1; i++) {
			attribs[i].stride = positions_stride + attributes_stride;
		}
	}

#ifdef GODOT_LOG_VERTEX_FORMAT
	print_line(sz);
#endif

	//validate sizes
	PoolVector<uint8_t> array = p_array;

	int stride = positions_stride + attributes_stride;
	int array_size = stride * p_vertex_count;
	int index_array_size = 0;
	if (array.size() != array_size && array.size() + p_vertex_count * 2 == array_size) {
		//old format, convert
		array = PoolVector<uint8_t>();

		array.resize(p_array.size() + p_vertex_count * 2);

		PoolVector<uint8_t>::Write w = array.write();
		PoolVector<uint8_t>::Read r = p_array.read();

		uint16_t *w16 = (uint16_t *)w.ptr();
		const uint16_t *r16 = (uint16_t *)r.ptr();

		uint16_t one = Math::make_half_float(1);

		for (int i = 0; i < p_vertex_count; i++) {
			*w16++ = *r16++;
			*w16++ = *r16++;
			*w16++ = *r16++;
			*w16++ = one;
			for (int j = 0; j < (stride / 2) - 4; j++) {
				*w16++ = *r16++;
			}
		}
	}

	if (array.size() != array_size) {
		ERR_FAIL_COND(array.size() != array_size);
	}

	// no support for half float yet
	if (uses_half_float) {
		//return;

		//	if (!config.support_half_float_vertices && uses_half_float) {
		uint32_t new_format = p_format;
		print_line("_unpack_half_floats stride_before " + itos(stride) + " ( " + itos(positions_stride) + " + " + itos(attributes_stride) + " )");
		PoolVector<uint8_t> unpacked_array = _unpack_half_floats(array, new_format, p_vertex_count);

		Vector<PoolVector<uint8_t>> unpacked_blend_shapes;
		for (int i = 0; i < p_blend_shapes.size(); i++) {
			uint32_t temp_format = p_format; // Just throw this away as it will be the same as new_format
			unpacked_blend_shapes.push_back(_unpack_half_floats(p_blend_shapes[i], temp_format, p_vertex_count));
		}

		mesh_add_surface(p_mesh, new_format, p_primitive, unpacked_array, p_vertex_count, p_index_array, p_index_count, p_aabb, unpacked_blend_shapes, p_bone_aabbs);
		return; //do not go any further, above function used unpacked stuff will be used instead.
	}

	if (p_format & VS::ARRAY_FORMAT_INDEX) {
		index_array_size = attribs[VS::ARRAY_INDEX].stride * p_index_count;
	}

	ERR_FAIL_COND(p_index_array.size() != index_array_size);

	ERR_FAIL_COND(p_blend_shapes.size() != mesh->blend_shape_count);

	for (int i = 0; i < p_blend_shapes.size(); i++) {
		ERR_FAIL_COND(p_blend_shapes[i].size() != array_size);
	}

	// all valid, create stuff
	BGFXSurface *s = memnew(BGFXSurface);
	mesh->surfaces.push_back(s);
	//		BGFXSurface *s = &m->surfaces.write[m->surfaces.size() - 1];
	s->format = p_format;
	s->primitive = p_primitive;
	s->array = array;
	s->vertex_count = p_vertex_count;
	s->index_array = p_index_array;
	s->index_count = p_index_count;
	s->aabb = p_aabb;
	s->blend_shapes = p_blend_shapes;
	s->bone_aabbs = p_bone_aabbs;

	for (int i = 0; i < VS::ARRAY_MAX; i++) {
		s->attribs[i] = attribs[i];
	}

	// now create the bg verts
	s->bg_verts.resize(p_vertex_count);
	s->bg_inds.resize(p_index_count);

	const uint8_t *data = array.read().ptr();
	for (int n = 0; n < p_vertex_count; n++) {
		const Vector3 *pos = (const Vector3 *)&data[(n * stride) + attribs[VS::ARRAY_VERTEX].offset];
		BGFX::PosColorVertex &dest = s->bg_verts[n];
		dest.x = pos->x;
		dest.y = pos->y;
		dest.z = pos->z;

		if (attribs[VS::ARRAY_TEX_UV].enabled) {
			if (attribs[VS::ARRAY_TEX_UV].type == BGFXSurface::AT_FLOAT) {
				const Vector2 *uv = (const Vector2 *)&data[(n * stride) + attribs[VS::ARRAY_TEX_UV].offset];
				dest.u = uv->x;
				dest.v = uv->y;
			}
			dest.abgr = UINT32_MAX;
		}
	}

	//	print_line("indices :");
	const uint8_t *ind_data = p_index_array.read().ptr();

	if (attribs[VS::ARRAY_INDEX].stride == 2) {
		const uint16_t *ind = (const uint16_t *)ind_data;
		for (int n = 0; n < p_index_count; n++) {
			s->bg_inds[n] = ind[n];
			//			print_line("\t" + itos(n) + " : " + itos(ind[n]));
		}
	} else {
		const uint32_t *ind = (const uint32_t *)ind_data;
		for (int n = 0; n < p_index_count; n++) {
			s->bg_inds[n] = ind[n];
			//			print_line("\t" + itos(n) + " : " + itos(ind[n]));
		}
	}

	s->bg_vertex_buffer = bgfx::createVertexBuffer(bgfx::makeRef(s->bg_verts.ptr(), sizeof(BGFX::PosColorVertex) * s->bg_verts.size()), BGFX::PosColorVertex::layout);
	s->bg_index_buffer = bgfx::createIndexBuffer(bgfx::makeRef(s->bg_inds.ptr(), sizeof(uint16_t) * s->bg_inds.size()));
}

AABB RasterizerStorageBGFX::mesh_get_aabb(RID p_mesh, RID p_skeleton) const {
	BGFXMesh *mesh = mesh_owner.get(p_mesh);
	ERR_FAIL_COND_V(!mesh, AABB());

	//	if (mesh->custom_aabb != AABB()) {
	//		return mesh->custom_aabb;
	//	}

	//	Skeleton *sk = nullptr;
	//	if (p_skeleton.is_valid()) {
	//		sk = skeleton_owner.get(p_skeleton);
	//	}

	AABB aabb;

#if 0
	if (sk && sk->size != 0) {
		for (int i = 0; i < mesh->surfaces.size(); i++) {
			AABB laabb;
			if ((mesh->surfaces[i]->format & VS::ARRAY_FORMAT_BONES) && mesh->surfaces[i]->skeleton_bone_aabb.size()) {
				int bs = mesh->surfaces[i]->skeleton_bone_aabb.size();
				const AABB *skbones = mesh->surfaces[i]->skeleton_bone_aabb.ptr();
				const bool *skused = mesh->surfaces[i]->skeleton_bone_used.ptr();

				int sbs = sk->size;
				ERR_CONTINUE(bs > sbs);
				const float *texture = sk->bone_data.ptr();

				bool first = true;
				if (sk->use_2d) {
					for (int j = 0; j < bs; j++) {
						if (!skused[j]) {
							continue;
						}

						int base_ofs = j * 2 * 4;

						Transform mtx;

						mtx.basis[0].x = texture[base_ofs + 0];
						mtx.basis[0].y = texture[base_ofs + 1];
						mtx.origin.x = texture[base_ofs + 3];
						base_ofs += 4;
						mtx.basis[1].x = texture[base_ofs + 0];
						mtx.basis[1].y = texture[base_ofs + 1];
						mtx.origin.y = texture[base_ofs + 3];

						AABB baabb = mtx.xform(skbones[j]);

						if (first) {
							laabb = baabb;
							first = false;
						} else {
							laabb.merge_with(baabb);
						}
					}
				} else {
					for (int j = 0; j < bs; j++) {
						if (!skused[j]) {
							continue;
						}

						int base_ofs = j * 3 * 4;

						Transform mtx;

						mtx.basis[0].x = texture[base_ofs + 0];
						mtx.basis[0].y = texture[base_ofs + 1];
						mtx.basis[0].z = texture[base_ofs + 2];
						mtx.origin.x = texture[base_ofs + 3];
						base_ofs += 4;
						mtx.basis[1].x = texture[base_ofs + 0];
						mtx.basis[1].y = texture[base_ofs + 1];
						mtx.basis[1].z = texture[base_ofs + 2];
						mtx.origin.y = texture[base_ofs + 3];
						base_ofs += 4;
						mtx.basis[2].x = texture[base_ofs + 0];
						mtx.basis[2].y = texture[base_ofs + 1];
						mtx.basis[2].z = texture[base_ofs + 2];
						mtx.origin.z = texture[base_ofs + 3];

						AABB baabb = mtx.xform(skbones[j]);
						if (first) {
							laabb = baabb;
							first = false;
						} else {
							laabb.merge_with(baabb);
						}
					}
				}

			} else {
				laabb = mesh->surfaces[i]->aabb;
			}

			if (i == 0) {
				aabb = laabb;
			} else {
				aabb.merge_with(laabb);
			}
		}
	} else {
#endif
	for (int i = 0; i < mesh->surfaces.size(); i++) {
		if (i == 0) {
			aabb = mesh->surfaces[i]->aabb;
		} else {
			aabb.merge_with(mesh->surfaces[i]->aabb);
		}
	}
	//	}

	return aabb;
}

void RasterizerStorageBGFX::mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material) {
}

RID RasterizerStorageBGFX::mesh_surface_get_material(RID p_mesh, int p_surface) const {
	return RID();
}

uint32_t RasterizerStorageBGFX::_request_bgfx_view() {
	ERR_FAIL_COND_V(_bgfx_view_pool.used_size() >= 256, UINT16_MAX);
	uint32_t id;
	_bgfx_view_pool.request(id);
	//	print_line("requesting bgfx view " + itos(id));
	return id;
}

void RasterizerStorageBGFX::_free_bgfx_view(uint32_t p_id) {
	ERR_FAIL_COND(p_id > 255);
	_bgfx_view_pool.free(p_id);
	//	print_line("freeing bgfx view " + itos(p_id));
}

RasterizerStorageBGFX::RasterizerStorageBGFX() {
	BGFX::scene.create();
}

RasterizerStorageBGFX::~RasterizerStorageBGFX() {
	BGFX::scene.destroy();
}
