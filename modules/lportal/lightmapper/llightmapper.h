#pragma once

#include "scene/3d/mesh_instance.h"
#include "llightscene.h"

namespace LM
{

class LightMapper
{
public:
	bool LightmapMesh(const MeshInstance &mi, Image &output_image);


private:


	LightScene m_Scene;
	int m_iWidth;
	int m_iHeight;
};

}
