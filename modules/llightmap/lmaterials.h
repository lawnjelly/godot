#pragma once

#include "lvector.h"
#include "scene/3d/mesh_instance.h"

namespace LM {

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
	bool find_colors(int mat_id, const Vector2 &uv, Color &albedo, Color &r_emission, bool &bTransparent);

	const LMaterial &get_material(int i) const { return _materials[i]; }

	// in order to account for emission density we need to reduce
	// power to keep brightness the same
	void adjust_materials(float emission_density);

private:
	Variant find_custom_albedo_tex(Ref<Material> src_material);
	void find_custom_shader_params(Ref<Material> src_material, float &emission, Color &emission_color);
	
	LTexture * _load_bake_texture(Ref<Texture> p_texture, Color p_color_mul = Color(1, 1, 1, 1), Color p_color_add = Color(0, 0, 0, 0)) const;
	LTexture *_get_bake_texture(Ref<Image> p_image, const Color &p_color_mul, const Color &p_color_add) const;
	LTexture *_make_dummy_texture(LTexture *pLTexture, Color col) const;

	LVector<LMaterial> _materials;
	unsigned int _max_material_size;
};

} //namespace LM
