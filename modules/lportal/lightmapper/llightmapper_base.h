#pragma once

#include "scene/3d/spatial.h"
#include "scene/3d/light.h"
#include "scene/3d/mesh_instance.h"
#include "llightscene.h"
#include "llightimage.h"
#include "lqmc.h"

namespace LM {

class LightMapper_Base
{
	friend class LightScene;

protected:
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
	enum eLMMode
	{
		LMMODE_FORWARD,
		LMMODE_BACKWARD,
	};

	// these enable feedback in the Godot UI as we bake
	typedef void (*BakeBeginFunc)(int);
	typedef bool (*BakeStepFunc)(int, const String &);
	typedef void (*BakeEndFunc)();
	static BakeBeginFunc bake_begin_function;
	static BakeStepFunc bake_step_function;
	static BakeEndFunc bake_end_function;

protected:
	void FindLights_Recursive(const Node * pNode);
	void FindLight(const Node * pNode);

	void PrepareImageMaps();
	void Normalize();
	void Normalize_AO();

	void WriteOutputImage(Image &output_image);
	void RandomUnitDir(Vector3 &dir) const;
	void RandomBarycentric(Vector3 &bary) const;

protected:
	// luminosity
	LightImage<float> m_Image_L;

	// for bounces
	LightImage<float> m_Image_L_mirror;

	// ambient occlusion
	LightImage<float> m_Image_AO;

	// just in case of overlapping triangles, for anti aliasing we will maintain 2 lists of triangles per pixel
	LightImage<uint32_t> m_Image_ID_p1;
	LightImage<uint32_t> m_Image_ID2_p1;

	// store multiple triangles per texel
	LightImage<MiniList> m_Image_TriIDs;
	LVector<uint32_t> m_TriIDs;

	LightImage<Vector3> m_Image_Barycentric;

	// triangles that cut texels (prevent shadow leaks)
	LightImage<MiniList_Cuts> m_Image_Cuts;
	LVector<uint32_t> m_CuttingTris;

	int m_iWidth;
	int m_iHeight;
	int m_iNumRays; // this will be modified from the settings_numrays

	LightScene m_Scene;
	LVector<LLight> m_Lights;

	QMC m_QMC;

	// for stats
	int m_iNumTests;

	// if user cancels bake in editor
	bool m_bCancel;


	// these need to be public because can't get friend to work
	// with LLightMapper_Base. It is not recognised in global namespace.
	// Perhaps some Godot template fu is involved.
public:
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

	int m_Settings_AO_Samples;
	float m_Settings_AO_Range;
	float m_Settings_AO_CutRange;

	eLMMode m_Settings_Mode;
	Vec3i m_Settings_VoxelDims;

	int m_Settings_TexWidth;
	int m_Settings_TexHeight;

	bool m_Settings_Normalize;
	float m_Settings_NormalizeBias;

	NodePath m_Settings_Path_Mesh;
	NodePath m_Settings_Path_Lights;
	String m_Settings_ImageFilename;

	LightMapper_Base();
protected:
//	static void _bind_methods();

	float InverseSquareDropoff(float dist) const
	{
		dist *= 0.2f;
		dist += 0.282f;
		// 4 PI = 12.5664
		float area = 4.0f * ((float) Math_PI) * (dist * dist);
		return 1.0f / area;
	}
};

inline void LightMapper_Base::RandomBarycentric(Vector3 &bary) const
{
	float r1 = Math::randf();
	float r2 = Math::randf();
	float sqrt_r1 = sqrtf(r1);

	bary.x = 1.0f - sqrt_r1;
	bary.y = r2 * sqrt_r1;
	bary.z = 1.0f - bary.x - bary.y;
}


inline void LightMapper_Base::RandomUnitDir(Vector3 &dir) const
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


} // namespace
