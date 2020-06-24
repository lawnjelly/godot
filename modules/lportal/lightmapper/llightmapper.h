#pragma once

#include "scene/3d/spatial.h"
#include "scene/3d/mesh_instance.h"
#include "llightscene.h"
#include "llightimage.h"
#include "lraybank.h"


class LightMapper : public Spatial
{
	friend class LLightMapper;

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

public:
	enum eMode
	{
		MODE_FORWARD,
		MODE_BACKWARD,
	};

	// these enable feedback in the Godot UI as we bake
	typedef void (*BakeBeginFunc)(int);
	typedef bool (*BakeStepFunc)(int, const String &);
	typedef void (*BakeEndFunc)();
	static BakeBeginFunc bake_begin_function;
	static BakeStepFunc bake_step_function;
	static BakeEndFunc bake_end_function;

	// main function called from the godot class
	bool lightmap_mesh(MeshInstance * pMI, Spatial * pLR, Image * pIm);

private:
	bool LightmapMesh(const MeshInstance &mi, const Spatial &light_root, Image &output_image);
	void Reset();

private:
	void FindLights_Recursive(const Node * pNode);
	void FindLight(const Node * pNode);

	void ProcessLight(int light_id);
	void PrepareImageMaps();
	void ProcessRay(LM::Ray r, int depth, float power, int dest_tri_id = 0, const Vector2i * pUV = 0);


	void ProcessTexels();
	void ProcessTexel(int tx, int ty);
	void ProcessSubTexel(float fx, float fy);
	float ProcessTexel_Light(int light_id, const Vector3 &ptDest, const Vector3 &ptNormal, uint32_t tri_ignore);

	void ProcessTexels_Bounce();
	float ProcessTexel_Bounce(int x, int y);
	void Normalize();

	void WriteOutputImage(Image &output_image);
	void RandomUnitDir(Vector3 &dir) const;

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
	LM::RayBank m_RayBank;
	LVector<LLight> m_Lights;

	// for stats
	int m_iNumTests;

	// if user cancels bake in editor
	bool m_bCancel;


	// params
	int m_Settings_Forward_NumRays;
	int m_Settings_Forward_NumBounces;
	float m_Settings_Forward_RayPower;
	float m_Settings_Forward_BouncePower;
	float m_Settings_Forward_BounceDirectionality;

	int m_Settings_Backward_NumRays;
	int m_Settings_Backward_NumBounceRays;
	int m_Settings_Backward_NumBounces;
	float m_Settings_Backward_RayPower;
	float m_Settings_Backward_BouncePower;


	eMode m_Settings_Mode;
	LM::Vec3i m_Settings_VoxelDims;

	int m_Settings_TexWidth;
	int m_Settings_TexHeight;

	bool m_Settings_Normalize;
	float m_Settings_NormalizeBias;

	NodePath m_Settings_Path_Mesh;
	NodePath m_Settings_Path_Lights;
	String m_Settings_ImageFilename;

protected:
	LightMapper();
	static void _bind_methods();

private:
	float InverseSquareDropoff(float dist) const
	{
		dist *= 0.2f;
		dist += 0.282f;
		// 4 PI = 12.5664
		float area = 4.0f * Math_PI * (dist * dist);
		return 1.0f / area;
	}
};


inline void LightMapper::RandomUnitDir(Vector3 &dir) const
{
	while (true)
	{
		dir.x = Math::random(-1.0f, 1.0f);
		dir.y = Math::random(-1.0f, 1.0f);
		dir.z = Math::random(-1.0f, 1.0f);

		float l = dir.length();
		if (l > 0.001f)
		{
			dir /= l;
			return;
		}
	}
}


VARIANT_ENUM_CAST(LightMapper::eMode);
