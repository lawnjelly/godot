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
	void ProcessLight();
	void WriteOutputImage(Image &output_image);

	// luminosity
	LM::LightImage<float> m_Image_L;
	int m_iWidth;
	int m_iHeight;

	LM::LightScene m_Scene;

protected:
	static void _bind_methods();
};

