#pragma once

#include "scene/3d/mesh_instance.h"
#include "llightscene.h"
#include "llightimage.h"

namespace LM
{

class LightMapper
{
public:
	bool LightmapMesh(const MeshInstance &mi, Image &output_image);

private:
	void ProcessLight();
	void WriteOutputImage(Image &output_image);

	// luminosity
	LightImage<float> m_Image_L;
	int m_iWidth;
	int m_iHeight;

	LightScene m_Scene;
};

}
