#pragma once

#include "scene/3d/spatial.h"
#include "scene/3d/mesh_instance.h"
#include "llightmapper.h"

class LLightmap : public Spatial
{
	GDCLASS(LLightmap, Spatial);
public:
	enum eMode
	{
		MODE_FORWARD = LM::LightMapper::LMMODE_FORWARD,
		MODE_BACKWARD = LM::LightMapper::LMMODE_BACKWARD,
	};

	enum eBakeMode
	{
		BAKEMODE_LIGHTMAP  = LM::LightMapper::LMBAKEMODE_LIGHTMAP,
		BAKEMODE_AO = LM::LightMapper::LMBAKEMODE_AO,
		BAKEMODE_MERGE = LM::LightMapper::LMBAKEMODE_MERGE,
		BAKEMODE_COMBINED = LM::LightMapper::LMBAKEMODE_COMBINED,
	};

	bool lightmap_mesh(Node * pMeshInstance, Node * pLightRoot, Object * pOutputImage_Lightmap, Object * pOutputImage_AO, Object * pOutputImage_Combined);
	bool lightmap_bake();
	bool lightmap_bake_to_image(Object * pOutputLightmapImage, Object * pOutputAOImage, Object * pOutputCombinedImage);

	void set_mode(LLightmap::eMode p_mode);
	LLightmap::eMode get_mode() const;

	void set_bake_mode(LLightmap::eBakeMode p_mode);
	LLightmap::eBakeMode get_bake_mode() const;

	void set_mesh_path(const NodePath &p_path);
	NodePath get_mesh_path() const;
	void set_lights_path(const NodePath &p_path);
	NodePath get_lights_path() const;

	////////////////////////////
	void set_forward_num_rays(int num_rays);
	int get_forward_num_rays() const;

	void set_forward_num_bounces(int num_bounces);
	int get_forward_num_bounces() const;

	void set_forward_ray_power(float ray_power);
	float get_forward_ray_power() const;

	void set_forward_bounce_power(float bounce_power);
	float get_forward_bounce_power() const;

	void set_forward_bounce_directionality(float bounce_dir);
	float get_forward_bounce_directionality() const;

	////////////////////////////
	void set_backward_num_rays(int num_rays);
	int get_backward_num_rays() const;

	void set_backward_num_bounce_rays(int num_rays);
	int get_backward_num_bounce_rays() const;

	void set_backward_num_bounces(int num_bounces);
	int get_backward_num_bounces() const;

	void set_backward_ray_power(float ray_power);
	float get_backward_ray_power() const;

	void set_backward_bounce_power(float bounce_power);
	float get_backward_bounce_power() const;
	////////////////////////////

	void set_ao_range(float ao_range);
	float get_ao_range() const;

	void set_ao_cut_range(float ao_cut_range);
	float get_ao_cut_range() const;

	void set_ao_num_samples(int ao_num_samples);
	int get_ao_num_samples() const;
	////////////////////////////

	void set_tex_width(int width);
	int get_tex_width() const;

	void set_tex_height(int height);
	int get_tex_height() const;

	void set_voxel_dims(const Vector3 &dims);
	Vector3 get_voxel_dims() const;

	void set_surface_bias(float bias);
	float get_surface_bias() const;

	void set_normalize(bool norm);
	bool get_normalize() const;

	void set_normalize_bias(float bias);
	float get_normalize_bias() const;

	void set_light_ao_ratio(float ratio);
	float get_light_ao_ratio() const;

	void set_lightmap_filename(const String &p_filename);
	String get_lightmap_filename() const;

	void set_ao_filename(const String &p_filename);
	String get_ao_filename() const;

	void set_combined_filename(const String &p_filename);
	String get_combined_filename() const;

private:
	LM::LightMapper m_LM;

protected:
	static void _bind_methods();
};


VARIANT_ENUM_CAST(LLightmap::eMode);
VARIANT_ENUM_CAST(LLightmap::eBakeMode);
