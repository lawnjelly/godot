#pragma once

#include "scene/3d/spatial.h"
#include "scene/3d/mesh_instance.h"
#include "llightmapper.h"

class LLightMapper : public Spatial
{
	GDCLASS(LLightMapper, Spatial);
public:
	enum eMode
	{
		MODE_FORWARD,
		MODE_BACKWARD,
	};

	bool lightmap_mesh(Node * pMeshInstance, Node * pLightRoot, Object * pOutputImage);
	bool lightmap_bake();
	bool lightmap_bake_to_image(Object * pOutputImage);

	void set_mode(LightMapper::eMode p_mode);
	LightMapper::eMode get_mode() const;

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


	void set_tex_width(int width);
	int get_tex_width() const;

	void set_tex_height(int height);
	int get_tex_height() const;

	void set_voxel_dims(const Vector3 &dims);
	Vector3 get_voxel_dims() const;

	void set_normalize(bool norm);
	bool get_normalize() const;

	void set_normalize_bias(float bias);
	float get_normalize_bias() const;

	void set_image_filename(const String &p_filename);
	String get_image_filename() const;

private:
	LightMapper m_LM;

protected:
	static void _bind_methods();
};


VARIANT_ENUM_CAST(LLightMapper::eMode);
