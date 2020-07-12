#include "lmerger.h"

namespace LM {

MeshInstance * Merger::lightmap_internal(String szProxyFilename, String szLevelFilename, Spatial * pRoot)
{
	FindMeshes(pRoot);

	return 0;
}



void Merger::FindMeshes(Spatial * pNode)
{
	// mesh instance?
	MeshInstance * pMI = Object::cast_to<MeshInstance>(pNode);
	if (pMI)
	{
		m_Meshes.push_back(pMI);
	}

	for (int n=0; n<pNode->get_child_count(); n++)
	{
		Spatial * pChild = Object::cast_to<Spatial>(pNode->get_child(n));
		if (pChild)
		{
			FindMeshes(pChild);
		}
	}
}



} // namespace
