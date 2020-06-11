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
		Light * m_pLight;
	};

	GDCLASS(LLightMapper, Spatial);
public:
	bool lightmap_mesh(Node * pMeshInstance, Object * pOutputImage, int num_rays, float power);

private:
	bool LightmapMesh(const MeshInstance &mi, Image &output_image);

private:
	void FindLights(const MeshInstance &mi);
	void ProcessTexels();
	void ProcessLight(int light_id);
	void ProcessTexel(int x, int y);
	void PrepareImageMaps();
	void ProcessRay(const LM::Ray &r, int depth, float power, int dest_tri_id = 0, const Vector2i * pUV = 0);

	void WriteOutputImage(Image &output_image);

	// luminosity
	LM::LightImage<float> m_Image_L;
	LM::LightImage<uint32_t> m_Image_ID_p1;
	LM::LightImage<Vector3> m_Image_Barycentric;

	int m_iWidth;
	int m_iHeight;

	LM::LightScene m_Scene;
	LVector<LLight> m_Lights;

	// for stats
	int m_iNumTests;


	// params
	int m_Settings_NumRays;
	float m_Settings_RayPower;

protected:
	static void _bind_methods();
};

