#pragma once

#include "scene/3d/spatial.h"
#include "scene/3d/mesh_instance.h"
#include "llightscene.h"
#include "llightimage.h"


class LLightMapper : public Spatial
{
	GDCLASS(LLightMapper, Spatial);
public:
	bool lightmap_mesh(Node * pMeshInstance, Object * pOutputImage);

private:
	bool LightmapMesh(const MeshInstance &mi, Image &output_image);

private:
	void ProcessTexels();
	void ProcessLight();
	void ProcessTexel(int x, int y);
	void PrepareImageMaps();
	void ProcessRay(LM::Ray &r);

	void WriteOutputImage(Image &output_image);

	// luminosity
	LM::LightImage<float> m_Image_L;
	LM::LightImage<uint32_t> m_Image_ID_p1;
	LM::LightImage<Vector3> m_Image_Barycentric;

	int m_iWidth;
	int m_iHeight;

	LM::LightScene m_Scene;

protected:
	static void _bind_methods();
};

