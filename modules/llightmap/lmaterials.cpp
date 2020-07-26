#include "lmaterials.h"

namespace LM
{

void LMaterial::Destroy()
{
	if (pAlbedo)
	{
		memdelete(pAlbedo);
		pAlbedo = 0;
	}
}

/////////////////////////////////

LMaterials::LMaterials()
{
}

LMaterials::~LMaterials()
{
	Reset();
}

void LMaterials::Reset()
{
	for (int n=0; n<m_Materials.size(); n++)
	{
		m_Materials[n].Destroy();
	}

	m_Materials.clear(true);
}


int LMaterials::FindOrCreateMaterial(const MeshInstance &mi, Ref<Mesh> rmesh, int surf_id)
{
	Ref<Material> src_material;

	// mesh instance has the material?
	src_material = mi.get_surface_material(surf_id);
	if (src_material.ptr())
	{
		//mi.set_surface_material(0, mat);
	}
	else
	{
		// mesh has the material?
		src_material = rmesh->surface_get_material(surf_id);
		//mi.set_surface_material(0, smat);
	}



//	Ref<Material> src_material = rmesh->surface_get_material(surf_id);
	const Material * pSrcMaterial = src_material.ptr();

	if (!pSrcMaterial)
		return 0;

	// already exists?
	for (int n=0; n<m_Materials.size(); n++)
	{
		if (m_Materials[n].pGodotMaterial == pSrcMaterial)
			return n+1;
	}

	// doesn't exist create a new material
	LMaterial * pNew = m_Materials.request();
	pNew->Create();
	pNew->pGodotMaterial = pSrcMaterial;

	//	int mesh_id = m_Tri_MeshIDs[tri_id];
	//	int surf_id = m_Tri_SurfIDs[tri_id];

	//	const MeshInstance &mi = *m_Meshes[mesh_id];
	//	Ref<Mesh> rmesh = mi.get_mesh();

	Ref<SpatialMaterial> mat = src_material;

	if (mat.is_valid())
	{
		Ref<Texture> albedo_tex = mat->get_texture(SpatialMaterial::TEXTURE_ALBEDO);
		Ref<Image> img_albedo;
		if (albedo_tex.is_valid())
		{
			img_albedo = albedo_tex->get_data();
			pNew->pAlbedo = _get_bake_texture(img_albedo, mat->get_albedo(), Color(0, 0, 0)); // albedo texture, color is multiplicative
			//albedo_texture = _get_bake_texture(img_albedo, size, mat->get_albedo(), Color(0, 0, 0)); // albedo texture, color is multiplicative
		} else
		{
			//albedo_texture = _get_bake_texture(img_albedo, size, Color(1, 1, 1), mat->get_albedo()); // no albedo texture, color is additive
		}
	}


	// returns the new material ID plus 1
	return m_Materials.size();
}

LTexture * LMaterials::_get_bake_texture(Ref<Image> p_image, const Color &p_color_mul, const Color &p_color_add)
{
	LTexture * lt = memnew(LTexture);

	if (p_image.is_null() || p_image->empty())
	{
		// dummy texture
		lt->colors.resize(1);
		lt->colors.set(0, p_color_add);
		lt->width = 1;
		lt->height = 1;
		return lt;
	}

	int w = p_image->get_width();
	int h = p_image->get_height();
	int size = w * h;

	lt->width = w;
	lt->height = h;

	lt->colors.resize(size);

	PoolVector<uint8_t>::Read r = p_image->get_data().read();

	for (int i = 0; i<size; i++)
	{
		Color c;
		c.r = (r[i * 4 + 0] / 255.0) * p_color_mul.r + p_color_add.r;
		c.g = (r[i * 4 + 1] / 255.0) * p_color_mul.g + p_color_add.g;
		c.b = (r[i * 4 + 2] / 255.0) * p_color_mul.b + p_color_add.b;

		// srgb to linear?


		c.a = r[i * 4 + 3] / 255.0;

		lt->colors.set(i, c);
	}

	return lt;
}


bool LMaterials::FindColors(int mat_id, const Vector2 &uv, Color &aldedo)
{

	return true;
}


} // namespace
