#pragma once

#include "lvector.h"
#include "scene/3d/mesh_instance.h"

namespace LM {

struct ColorSample;

struct LTexture {
	Vector<Color> colors;
	int width;
	int height;

	void sample(const Vector2 &uv, Color &col) const;
};

struct LMaterial {
	void create() {
		tex_albedo = nullptr;
		tex_emission = nullptr;
		godot_material = 0;
		is_emitter = false;
		power_emission = 0.0f;
		is_transparent = false;
	}
	void destroy();

	const Material *godot_material = nullptr;
	LTexture *tex_albedo = nullptr;
	LTexture *tex_emission = nullptr;

	bool is_transparent = false;
	bool is_emitter = false;

	float power_emission = 0;
	Color color_emission; // color multiplied by emission power
};

class LMaterials {
public:
	LMaterials();
	~LMaterials();
	void reset();
	void prepare(unsigned int max_material_size) { _max_material_size = max_material_size; }

	int find_or_create_material(const MeshInstance &mi, Ref<Mesh> rmesh, int surf_id);
	//bool find_colors(int mat_id, const Vector2 &uv, Color &albedo, Color &r_emission, bool &bTransparent);
	bool find_colors(int mat_id, const Vector2 &uv, ColorSample &r_sample) const;

	const LMaterial &get_material(int i) const { return _materials[i]; }

	// in order to account for emission density we need to reduce
	// power to keep brightness the same
	void adjust_materials(float emission_density);

private:
	Variant find_custom_albedo_tex(Ref<Material> src_material);
	void find_custom_shader_params(Ref<Material> src_material, float &emission, Color &emission_color);

	LTexture *_load_bake_texture(Ref<Texture> p_texture, Color p_color_mul = Color(1, 1, 1, 1), Color p_color_add = Color(0, 0, 0, 0), int p_shrink = 0) const;
	LTexture *_load_emission_texture(Ref<Texture> p_texture) const;
	LTexture *_get_bake_texture(Ref<Image> p_image, const Color &p_color_mul, const Color &p_color_add, int p_shrink) const;
	LTexture *_make_dummy_texture(LTexture *pLTexture, Color col) const;

	void _max_color(const Color &p_col, Color &r_col) const {
		r_col.r = MAX(r_col.r, p_col.r);
		r_col.g = MAX(r_col.g, p_col.g);
		r_col.b = MAX(r_col.b, p_col.b);
	}

	LVector<LMaterial> _materials;
	unsigned int _max_material_size;
};

inline bool LMaterials::find_colors(int mat_id, const Vector2 &uv, ColorSample &r_sample) const {
	// mat_id is plus one
	if (!mat_id) {
		r_sample.albedo = Color(1, 1, 1, 1);
		return false;
	}

	mat_id--;
	const LMaterial &mat = _materials[mat_id];

	r_sample.is_opaque = !mat.is_transparent;

	bool found_any = false;

	if (mat.tex_albedo) {
		const LTexture &tex = *mat.tex_albedo;
		tex.sample(uv, r_sample.albedo);
		found_any = true;
	} else {
		r_sample.albedo = Color(1, 1, 1, 1);
	}

	if (mat.tex_emission) {
		const LTexture &tex = *mat.tex_emission;
		tex.sample(uv, r_sample.emission);
		//r_sample.emission *= _emission_power;
		r_sample.is_emitter = true;
		found_any = true;
	}

	return found_any;
}

inline void LTexture::sample(const Vector2 &uv, Color &col) const {
	// mod to surface (tiling)
	float x = fmodf(uv.x, 1.0f);
	float y = fmodf(uv.y, 1.0f);

	// we need these because fmod can produce negative results
	if (x < 0.0f)
		x = 1.0f + x;
	if (y < 0.0f)
		y = 1.0f + y;

	x *= width;
	y *= height;

	// no filtering as yet
	int tx = x;
	int ty = y;

	tx = MIN(tx, width - 1);
	ty = MIN(ty, height - 1);

	int i = (ty * width) + tx;

	col = colors[i];
}

} //namespace LM
