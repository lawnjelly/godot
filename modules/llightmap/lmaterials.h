#pragma once

#include "scene/3d/mesh_instance.h"
#include "lvector.h"

namespace LM
{

struct LMaterial
{

};

class LMaterials
{
public:
	int FindOrCreateMaterial(Ref<Mesh> rmesh, int surf_id);

	bool FindColors(int mat_id, const Vector2 &uv, Color &aldedo);


private:
	LVector<LMaterial> m_Materials;
};



} // namespace
