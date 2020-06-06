#include "llightmapper.h"

using namespace LM;

bool LightMapper::LightmapMesh(const MeshInstance &mi, Image &output_image)
{
	m_iWidth = output_image.get_width();
	m_iHeight = output_image.get_height();

	m_Scene.Create(mi);

	return true;
}

