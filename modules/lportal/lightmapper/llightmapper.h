#pragma once

#include "lraybank.h"

namespace LM {

class LightMapper : public RayBank
{
public:

	// main function called from the godot class
	bool lightmap_mesh(MeshInstance * pMI, Spatial * pLR, Image * pIm);

private:
	bool LightmapMesh(const MeshInstance &mi, const Spatial &light_root, Image &output_image);
	void Reset();

private:
	void ProcessLights();
	void ProcessLight(int light_id, int num_rays);
	void ProcessRay(LM::Ray r, int depth, float power, int dest_tri_id = 0, const Vector2i * pUV = 0);


	void ProcessTexels();
	void ProcessTexel(int tx, int ty);
	float ProcessTexel_Light(int light_id, const Vector3 &ptDest, const Vector3 &ptNormal, uint32_t tri_ignore);

	void ProcessTexels_Bounce();
	float ProcessTexel_Bounce(int x, int y);

	void ProcessAO();
	void ProcessAO_Texel(int tx, int ty);
	float CalculateAO(int tx, int ty);
	bool ProcessAO_BehindCuts(const MiniList &ml, const Vector2 &st, int main_tri_id_p1);

	void ProcessAO_Triangle(int tri_id);
	void ProcessAO_Sample(const Vector3 &bary, int tri_id, const UVTri &uvtri);

//	float CalculateAO(const Vector3 &ptStart, const Vector3 &ptNormal, uint32_t tri0, uint32_t tri1_p1);

	const int m_iRaysPerSection = 1024 * 1024 * 4; // 64
	// 1024*1024 is 46 megs
};


} // namespace
