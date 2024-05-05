#include "lmaterials.h"

#define LM_STRING_TRANSPARENT "_T_"

namespace LM {

void LTexture::max_rgbs() {
	for (int n = 0; n < colors.size(); n++) {
		Color &c = colors[n];
		float f = c.r;
		f = MAX(f, c.g);
		f = MAX(f, c.b);
		c = Color(f, f, f, 1);
	}
}

void LMaterial::destroy() {
	if (tex_albedo) {
		memdelete(tex_albedo);
		tex_albedo = nullptr;
	}
	if (tex_emission) {
		memdelete(tex_emission);
		tex_emission = nullptr;
	}
	if (tex_orm) {
		memdelete(tex_orm);
		tex_orm = nullptr;
	}
}

/////////////////////////////////

LMaterials::LMaterials() {
	_max_material_size = 256;
}

LMaterials::~LMaterials() {
	reset();
}

void LMaterials::reset() {
	for (int n = 0; n < _materials.size(); n++) {
		_materials[n].destroy();
	}

	_materials.clear(true);
}

void LMaterials::adjust_materials(float emission_density) {
	if (emission_density == 0.0f)
		return;

	float emission_multiplier = 1.0f / emission_density;

	for (int n = 0; n < _materials.size(); n++) {
		LMaterial &mat = _materials[n];

		if (mat.is_emitter) {
			mat.power_emission *= emission_multiplier;
			mat.color_emission *= emission_multiplier;
		}
	}
}

int LMaterials::find_or_create_material(const MeshInstance &mi, Ref<Mesh> rmesh, int surf_id) {
	Ref<Material> src_material;

	// mesh instance has the material?
	src_material = mi.get_active_material(surf_id);
	if (src_material.ptr()) {
		//mi.set_surface_material(0, mat);
	} else {
		// mesh has the material?
		src_material = rmesh->surface_get_material(surf_id);
		//mi.set_surface_material(0, smat);
	}

	//	Ref<Material> src_material = rmesh->surface_get_material(surf_id);
	const Material *pSrcMaterial = src_material.ptr();

	if (!pSrcMaterial)
		return 0;

	// already exists?
	for (int n = 0; n < _materials.size(); n++) {
		if (_materials[n].godot_material == pSrcMaterial)
			return n + 1;
	}

	// doesn't exist create a new material
	LMaterial *pMat = _materials.request();
	pMat->create();
	pMat->godot_material = pSrcMaterial;

	// spatial material?
	Ref<SpatialMaterial> spatial_mat = src_material;

	Ref<Texture> albedo_tex;
	Ref<Texture> emission_tex;
	Ref<Texture> occlusion_tex;
	Ref<Texture> roughness_tex;
	Ref<Texture> metallic_tex;

	Color albedo = Color(1, 1, 1, 1);
	float emission = 0;
	Color emission_color(1, 1, 1, 1);
	float roughness = 1;

	if (spatial_mat.is_valid()) {
		print_line("Adding Spatial Material: " + spatial_mat->get_name());
		albedo_tex = spatial_mat->get_texture(SpatialMaterial::TEXTURE_ALBEDO);
		albedo = spatial_mat->get_albedo();

		if (spatial_mat->get_feature(Material3D::FEATURE_EMISSION)) {
			emission = spatial_mat->get_emission_energy();
			emission_color = spatial_mat->get_emission();
			emission_tex = spatial_mat->get_texture(SpatialMaterial::TEXTURE_EMISSION);
			if (emission_tex.is_valid()) {
				pMat->is_emitter = true;
				print_line("\temission_texture " + emission_tex->get_name());
				// debug save
				//				Ref<Image> img = emission_tex->get_data();
				//				if (img.is_valid()) {
				//					img->save_png("res://Lightmap/emission_tex.png");
				//				}
			}
		}

		{
			roughness = spatial_mat->get_roughness();
			roughness_tex = spatial_mat->get_texture(SpatialMaterial::TEXTURE_ROUGHNESS);
			if (roughness_tex.is_valid()) {
				print_line("\troughness_texture " + roughness_tex->get_name());
#if 0
				Ref<Image> img = roughness_tex->get_data();
				if (img.is_valid()) {
					img->save_png("res://Lightmap/roughness_" + roughness_tex->get_name() + ".png");
				}
#endif
			}
		}

	} else {
		// shader material?
		print_line("Adding Shader Material: " + pSrcMaterial->get_name());
		Variant shader_tex = find_custom_albedo_tex(src_material);
		albedo_tex = shader_tex;

		// check the name of the material to allow emission
		String szMat = src_material->get_path();
		if (szMat.find(LM_STRING_TRANSPARENT) != -1) {
			pMat->is_transparent = true;
		}

		find_custom_shader_params(src_material, emission, emission_color);

	} // not spatial mat

	// emission
	if (emission > 0.0f) {
		pMat->is_emitter = true;
		pMat->power_emission = emission / 1000.0f; // some constant to be comparable to lights
		pMat->color_emission = emission_color * pMat->power_emission;

		print_line("\tpower_emission " + String(Variant(pMat->power_emission)));
		print_line("\tcolor_emission " + String(Variant(pMat->color_emission)));

		// test
		//pMat->is_emitter = false;

		// apply a modifier for the emission density. As the number of samples go up, the power per sample
		// must reduce in order to prevent brightness changing.
	}

	pMat->tex_albedo = _load_bake_texture(albedo_tex, albedo);
	pMat->tex_emission = _load_emission_texture(emission_tex);
	pMat->tex_orm = _load_separate_orm_texture(occlusion_tex, roughness_tex, metallic_tex);

	pMat->power_roughness = roughness;
	if (roughness != 1) {
		print_line("\tpower_roughness " + String(Variant(roughness)));
	}

	// returns the new material ID plus 1
	return _materials.size();
}

LTexture *LMaterials::_load_separate_orm_texture(Ref<Texture> p_tex_occlusion, Ref<Texture> p_tex_roughness, Ref<Texture> p_tex_metallic) const {
	LTexture *roughness = _load_bake_texture(p_tex_roughness);

	if (roughness) {
		roughness->max_rgbs();
	}

	return roughness;
}

LTexture *LMaterials::_load_emission_texture(Ref<Texture> p_texture) const {
	return _load_bake_texture(p_texture);

	if (p_texture.is_valid()) {
		Ref<Image> img;
		img = p_texture->get_data();

		if (!img.is_valid())
			return nullptr;

		uint32_t mw = 32;
		uint32_t mh = 32;

		uint32_t sw = img->get_width();
		uint32_t sh = img->get_height();

		mw = MIN(mw, sw);
		mh = MIN(mh, sh);

		float stepx = sw / mw;
		float stepy = sh / mh;

		uint32_t stepx_i = Math::ceil(stepx);
		uint32_t stepy_i = Math::ceil(stepy);

		Ref<Image> mip = memnew(Image(mw, mh, false, Image::Format::FORMAT_RGB8));

		mip->lock();
		img->lock();
		for (uint32_t y = 0; y < mh; y++) {
			for (uint32_t x = 0; x < mw; x++) {
				Color col;

				// Find from source.
				uint32_t start_x = x * stepx;
				uint32_t start_y = y * stepy;

				// Make sample.
				for (uint32_t sy = 0; sy < stepy_i; sy++) {
					uint32_t source_y = sy + start_y;
					if (source_y >= sh)
						continue;

					for (uint32_t sx = 0; sx < stepx_i; sx++) {
						uint32_t source_x = sx + start_x;

						if (source_x >= sw)
							continue;

						Color source_pixel = img->get_pixel(source_x, source_y);
						_max_color(source_pixel, col);
					}
				}

				mip->set_pixel(x, y, col);
			}
		}
		img->unlock();
		mip->unlock();

		//mip->save_png("res://emission_test.png");

		return _get_bake_texture(mip, Color(1, 1, 1, 1), Color(0, 0, 0, 0), 0); // albedo texture, color is multiplicative
	}

	return nullptr;
}

LTexture *LMaterials::_load_bake_texture(Ref<Texture> p_texture, Color p_color_mul, Color p_color_add, int p_shrink) const {
	if (p_texture.is_valid()) {
		Ref<Image> img;
		img = p_texture->get_data();
		return _get_bake_texture(img, p_color_mul, p_color_add, p_shrink); // albedo texture, color is multiplicative
	}

	return nullptr;
}

void LMaterials::find_custom_shader_params(Ref<Material> src_material, float &emission, Color &emission_color) {
	// defaults
	emission = 0.0f;
	emission_color = Color(1, 1, 1, 1);

	Ref<ShaderMaterial> shader_mat = src_material;

	if (!shader_mat.is_valid())
		return;

	Variant p_emission = shader_mat->get_shader_param("emission");
	if (p_emission)
		emission = p_emission;

	Variant p_emission_color = shader_mat->get_shader_param("emission_color");
	if (p_emission_color)
		emission_color = p_emission_color;
}

Variant LMaterials::find_custom_albedo_tex(Ref<Material> src_material) {
	Ref<ShaderMaterial> shader_mat = src_material;

	if (!shader_mat.is_valid())
		return Variant::NIL;

	// get the shader
	Ref<Shader> shader = shader_mat->get_shader();
	if (!shader.is_valid())
		return Variant::NIL;

	// first - is there a named albedo texture?
	Variant named_param = shader_mat->get_shader_param("texture_albedo");
	//	if (named_param)
	//		return named_param;
	return named_param;

	/*
	// find the most likely albedo texture
	List<PropertyInfo> plist;
	shader->get_param_list(&plist);

	String sz_first_obj_param;

	for (List<PropertyInfo>::Element *E = plist.front(); E; E = E->next()) {
		String szName = E->get().name;
		Variant::Type t = E->get().type;
//				print_line("shader param : " + szName);
//				print_line("shader type : " + String(Variant(t)));
		if (t == Variant::OBJECT)
		{
			sz_first_obj_param = szName;
			break;
		}

		//r_options->push_back(quote_style + E->get().name.replace_first("shader_param/", "") + quote_style);
	}

	if (sz_first_obj_param == "")
		return Variant::NIL;

	StringName pr = shader->remap_param(sz_first_obj_param);
	if (!pr) {
		String n = sz_first_obj_param;
		if (n.find("param/") == 0) { //backwards compatibility
			pr = n.substr(6, n.length());
		}
		if (n.find("shader_param/") == 0) { //backwards compatibility
			pr = n.replace_first("shader_param/", "");
		}
	}

	if (!pr)
		return Variant::NIL;

	Variant param = shader_mat->get_shader_param(pr);

	print_line("\tparam is " + String(param));
	return param;
	*/
}

LTexture *LMaterials::_make_dummy_texture(LTexture *pLTexture, Color col) const {
	pLTexture->colors.resize(1);
	pLTexture->colors[0] = col;
	pLTexture->width = 1;
	pLTexture->height = 1;
	return pLTexture;
}

LTexture *LMaterials::_get_bake_texture(Ref<Image> p_image, const Color &p_color_mul, const Color &p_color_add, int p_shrink) const {
	LTexture *lt = memnew(LTexture);

	// no image exists, use dummy texture
	if (p_image.is_null() || p_image->empty()) {
		return _make_dummy_texture(lt, p_color_mul);
	}

	p_image = p_image->duplicate();

	if (p_image->is_compressed()) {
		Error err = p_image->decompress();
		if (err != OK) {
			// could not decompress
			WARN_PRINT("LMaterials::_get_bake_texture : could not decompress texture");
			return _make_dummy_texture(lt, p_color_mul);
		}
	}

	p_image->convert(Image::FORMAT_RGBA8);

	// downsize if necessary
	int w = p_image->get_width();
	int h = p_image->get_height();

	uint32_t max_material_size = _max_material_size;
	if (p_shrink) {
		max_material_size >>= p_shrink;
		print_line("max_material_size " + itos(max_material_size));
	}

	bool bResize = false;
	while (true) {
		if ((w > max_material_size) || (h > max_material_size)) {
			w /= 2;
			h /= 2;
			bResize = true;
		} else {
			w = MAX(w, 1);
			h = MAX(h, 1);
			break;
		}
	}
	if (bResize)
		p_image->resize(w, h, Image::INTERPOLATE_CUBIC);

	w = p_image->get_width();
	h = p_image->get_height();
	int size = w * h;

	lt->width = w;
	lt->height = h;

	lt->colors.resize(size);

	PoolVector<uint8_t>::Read r = p_image->get_data().read();

	for (int i = 0; i < size; i++) {
		Color c;
		c.r = (r[i * 4 + 0] / 255.0) * p_color_mul.r + p_color_add.r;
		c.g = (r[i * 4 + 1] / 255.0) * p_color_mul.g + p_color_add.g;
		c.b = (r[i * 4 + 2] / 255.0) * p_color_mul.b + p_color_add.b;

		// srgb to linear?

		c.a = r[i * 4 + 3] / 255.0;

		lt->colors[i] = c;
	}

	return lt;
}

/*
bool LMaterials::find_colors(int mat_id, const Vector2 &uv, Color &albedo, Color &r_emission, bool &bTransparent) {
	// mat_id is plus one
	if (!mat_id) {
		albedo = Color(1, 1, 1, 1);
		bTransparent = false;
		return false;
	}

	mat_id--;
	const LMaterial &mat = _materials[mat_id];

	// return whether transparent
	bTransparent = mat.is_transparent;

	bool found_any = false;

	if (mat.tex_albedo) {
		const LTexture &tex = *mat.tex_albedo;
		tex.sample(uv, albedo);
		found_any = true;
	} else {
		albedo = Color(1, 1, 1, 1);
	}

	if (mat.tex_emission) {
		const LTexture &tex = *mat.tex_emission;
		tex.sample(uv, r_emission);
		found_any = true;
	}

	return found_any;
}
*/

} //namespace LM
