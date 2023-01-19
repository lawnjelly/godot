#include "rasterizer_scene_bgfx.h"
#include "core/os/os.h"
#include "rasterizer_storage_bgfx.h"

void RasterizerSceneBGFX::render_scene(const Transform &p_cam_transform, const CameraMatrix &p_cam_projection, const int p_eye, bool p_cam_ortogonal, InstanceBase **p_cull_result, int p_cull_count, RID *p_light_cull_result, int p_light_cull_count, RID *p_reflection_probe_cull_result, int p_reflection_probe_cull_count, RID p_environment, RID p_shadow_atlas, RID p_reflection_atlas, RID p_reflection_probe, int p_reflection_probe_pass) {
	//	return;

	Transform cam_transform = p_cam_transform;
	bool reverse_cull = false;

	int viewport_width = 0;
	int viewport_height = 0;
	int viewport_x = 0;
	int viewport_y = 0;

	if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_VFLIP]) {
		cam_transform.basis.set_axis(1, -cam_transform.basis.get_axis(1));
		reverse_cull = true;
	}

	if (p_reflection_probe.is_valid()) {
		//		ReflectionProbeInstance *probe = reflection_probe_instance_owner.getornull(p_reflection_probe);
		//		ERR_FAIL_COND(!probe);
		//		state.render_no_shadows = !probe->probe_ptr->enable_shadows;

		//		if (!probe->probe_ptr->interior) { //use env only if not interior
		//			env = environment_owner.getornull(p_environment);
		//		}

		//		current_fb = probe->fbo[p_reflection_probe_pass];

		//		viewport_width = probe->probe_ptr->resolution;
		//		viewport_height = probe->probe_ptr->resolution;

		//		probe_interior = probe->probe_ptr->interior;

	} else {
		//		state.render_no_shadows = false;
		//		if (storage->frame.current_rt->multisample_active) {
		//			current_fb = storage->frame.current_rt->multisample_fbo;
		//		} else if (storage->frame.current_rt->external.fbo != 0) {
		//			current_fb = storage->frame.current_rt->external.fbo;
		//		} else {
		//			current_fb = storage->frame.current_rt->fbo;
		//		}
		//		env = environment_owner.getornull(p_environment);

		viewport_width = storage->frame.current_rt->width;
		viewport_height = storage->frame.current_rt->height;
		viewport_x = storage->frame.current_rt->x;

		if (storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_DIRECT_TO_SCREEN]) {
			viewport_y = OS::get_singleton()->get_window_size().height - viewport_height - storage->frame.current_rt->y;
		} else {
			viewport_y = storage->frame.current_rt->y;
		}
	}

	state.used_screen_texture = false;
	state.viewport_size.x = viewport_width;
	state.viewport_size.y = viewport_height;
	state.screen_pixel_size.x = 1.0 / viewport_width;
	state.screen_pixel_size.y = 1.0 / viewport_height;

	Transform cam_transform_inv = cam_transform.affine_inverse();

	BGFX::scene.prepare_scene(viewport_width, viewport_height);

	BGFX::scene.set_view_transform(p_cam_projection, cam_transform_inv);

	for (int i = 0; i < p_cull_count; i++) {
		InstanceBase *instance = p_cull_result[i];

		//print_line("instance " + itos(i) + " tr " + instance->transform);
		//Transform xform = cam_transform_inv * instance->transform; // this one looks correct

		switch (instance->base_type) {
			case VS::INSTANCE_MESH: {
				RasterizerStorageBGFX::BGFXMesh *mesh = storage->mesh_owner.getornull(instance->base);
				ERR_CONTINUE(!mesh);

				int num_surfaces = mesh->surfaces.size();

				for (int j = 0; j < num_surfaces; j++) {
					int material_index = instance->materials[j].is_valid() ? j : -1;

					RasterizerStorageBGFX::Material *mat = _choose_material(instance, material_index);
					_setup_material(mat);

					RasterizerStorageBGFX::BGFXSurface *surface = mesh->surfaces[j];
					//BGFX::scene.draw(p_cam_projection, cam_transform_inv, surface->bg_vertex_buffer, surface->bg_index_buffer, surface->primitive);
					BGFX::scene.draw(instance->transform, surface->bg_vertex_buffer, surface->bg_index_buffer, surface->primitive);
					//return;

					//					_add_geometry(surface, instance, nullptr, material_index, p_depth_pass, p_shadow_pass);
				}

			} break;

			default: {
			} break;
		}
	}

	//	bgfx::frame();
}

void RasterizerSceneBGFX::_setup_material(RasterizerStorageBGFX::Material *p_material) {
	if (!p_material)
		return;

	const StringName alb = "texture_albedo";

	if (p_material->params.has(alb)) {
		Variant v = p_material->params[alb];
		if (v.get_type() == Variant::Type::_RID) {
			RasterizerStorageBGFX::Texture *t = storage->texture_owner.getornull(v);

			if (t) {
				BGFX::scene.set_texture(t->bg_handle);
				return;
			}
		}
	}

	//	if (p_material->shader)
	//	{
	//		ShaderBGFX * s = p_material->shader->shader;
	//		if (s)
	//		{
	//			s->

	//		}
	//	}
}

//RasterizerStorageBGFX::Material * RasterizerSceneBGFX::_choose_material(RasterizerStorageBGFX::Geometry *p_geometry, InstanceBase *p_instance, RasterizerStorageBGFX::GeometryOwner *p_owner, int p_material, bool p_depth_pass, bool p_shadow_pass) {
RasterizerStorageBGFX::Material *RasterizerSceneBGFX::_choose_material(InstanceBase *p_instance, int p_material) {
	RasterizerStorageBGFX::Material *material = nullptr;
	RID material_src;

	if (p_instance->material_override.is_valid()) {
		material_src = p_instance->material_override;
	} else if (p_material >= 0) {
		material_src = p_instance->materials[p_material];
	} else {
		//material_src = p_geometry->material;
	}

	if (material_src.is_valid()) {
		material = storage->material_owner.getornull(material_src);

		//		if (!material->shader || !material->shader->valid) {
		//			material = nullptr;
		//		}
	}

	//	if (!material) {
	//		material = storage->material_owner.getptr(default_material);
	//	}

	//	ERR_FAIL_COND(!material);
	return material;
}

bool RasterizerSceneBGFX::free(RID p_rid) {
	return true;
}
