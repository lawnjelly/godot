#pragma once

#include "scene/3d/mesh_instance.h"

namespace LM {

class Merger
{
public:
	MeshInstance * lightmap_internal(String szProxyFilename, String szLevelFilename, Spatial * pRoot);


private:
	void FindMeshes(Spatial * pNode);

	Vector<MeshInstance *> m_Meshes;

};


} // namespace
