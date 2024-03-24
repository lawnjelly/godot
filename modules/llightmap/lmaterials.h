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
		albedo = 0;
		godot_material = 0;
		is_emitter = false;
		power_emission = 0.0f;
		is_transparent = false;
	}
	void destroy();

	const Material *godot_material;
	LTexture *albedo;

	bool is_transparent;
	bool is_emitter;

	float power_emission;
	Color color_emission; // color multiplied by emission power
};

class LMaterials {
public:
	LMaterials();
	~LMaterials();
	void reset();
	void prepare(unsigned int max_material_size) { _max_material_size = max_material_size; }

	int find_or_create_material(const MeshInstance &mi, Ref<Mesh> rmesh, int surf_id);
	bool find_colors(int mat_id, const Vector2 &uv, Color &albedo, bool &bTransparent);

	const LMaterial &get_material(int i) const { return _materials[i]; }

	// in order to account for emission density we need to reduce
	// power to keep brightness the same
	void adjust_materials(float emission_density);

private:
	Variant find_custom_albedo_tex(Ref<Material> src_material);
	void find_custom_shader_params(Ref<Material> src_material, float &emission, Color &emission_color);

	LTexture *_get_bake_texture(Ref<Image> p_image, const Color &p_color_mul, const Color &p_color_add);
	LTexture *_make_dummy_texture(LTexture *pLTexture, Color col);

	LVector<LMaterial> _materials;
	unsigned int _max_material_size;
};

} //namespace LM
