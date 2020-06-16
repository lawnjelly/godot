#pragma once

#include "scene/3d/spatial.h"
#include "scene/3d/mesh_instance.h"
#include "llightscene.h"
#include "llightimage.h"


class LLightMapper : public Spatial
{
	class LLight
	{
	public:
		enum LType
		{
			LT_OMNI,
			LT_SPOT,
			LT_DIRECTIONAL,
		};

		LType type;
		Vector3 pos;
		Vector3 dir;
		Vector3 scale;
		float energy;
		float indirect_energy;
		float range;
		float spot_angle;
		const Light * m_pLight;
	};

	GDCLASS(LLightMapper, Spatial);
public:
	enum eMode
	{
		MODE_FORWARD,
		MODE_BACKWARD,
	};

	typedef void (*BakeBeginFunc)(int);
	typedef bool (*BakeStepFunc)(int, const String &);
	typedef void (*BakeEndFunc)();
	static BakeBeginFunc bake_begin_function;
	static BakeStepFunc bake_step_function;
	static BakeEndFunc bake_end_function;

//	void lightmap_set_params(int num_rays, float power, float bounce_power);
	bool lightmap_mesh(Node * pMeshInstance, Node * pLightRoot, Object * pOutputImage);
	bool lightmap_bake();
	bool lightmap_bake_to_image(Object * pOutputImage);

	void set_mode(LLightMapper::eMode p_mode);
	LLightMapper::eMode get_mode() const;

	void set_mesh_path(const NodePath &p_path);
	NodePath get_mesh_path() const;
	void set_lights_path(const NodePath &p_path);
	NodePath get_lights_path() const;

	void set_num_rays(int num_rays);
	int get_num_rays() const;

	void set_num_bounces(int num_bounces);
	int get_num_bounces() const;

	void set_ray_power(float ray_power);
	float get_ray_power() const;

	void set_bounce_power(float bounce_power);
	float get_bounce_power() const;

	void set_tex_width(int width);
	int get_tex_width() const;

	void set_tex_height(int height);
	int get_tex_height() const;

	void set_normalize(bool norm);
	bool get_normalize() const;

	void set_normalize_bias(float bias);
	float get_normalize_bias() const;

	void set_image_filename(const String &p_filename);
	String get_image_filename() const;


private:
	bool LightmapMesh(const MeshInstance &mi, const Spatial &light_root, Image &output_image);

private:
	void FindLights_Recursive(const Node * pNode);
	void FindLight(const Node * pNode);

	void ProcessLight(int light_id);
	void PrepareImageMaps();
	void ProcessRay(LM::Ray r, int depth, float power, int dest_tri_id = 0, const Vector2i * pUV = 0);


	void ProcessTexels();
	void ProcessTexel(int tx, int ty);
	void ProcessSubTexel(float fx, float fy);
	float ProcessTexel_Light(int light_id, const Vector3 &ptDest, uint32_t tri_ignore);

	void ProcessTexels_Bounce();
	float ProcessTexel_Bounce(int x, int y);
	void Normalize();

	void WriteOutputImage(Image &output_image);

	// luminosity
	LM::LightImage<float> m_Image_L;

	// for bounces
	LM::LightImage<float> m_Image_L_mirror;

	LM::LightImage<uint32_t> m_Image_ID_p1;
	LM::LightImage<Vector3> m_Image_Barycentric;

	int m_iWidth;
	int m_iHeight;
	int m_iNumRays; // this will be modified from the settings_numrays

	LM::LightScene m_Scene;
	LVector<LLight> m_Lights;

	// for stats
	int m_iNumTests;

	// if user cancels bake in editor
	bool m_bCancel;


	// params
	int m_Settings_NumRays;
	int m_Settings_NumBounces;
	float m_Settings_RayPower;
	float m_Settings_BouncePower;
	eMode m_Settings_Mode;

	int m_Settings_TexWidth;
	int m_Settings_TexHeight;

	bool m_Settings_Normalize;
	float m_Settings_NormalizeBias;

	NodePath m_Settings_Path_Mesh;
	NodePath m_Settings_Path_Lights;
	String m_Settings_ImageFilename;

protected:
	LLightMapper();
	static void _bind_methods();
};

VARIANT_ENUM_CAST(LLightMapper::eMode);
