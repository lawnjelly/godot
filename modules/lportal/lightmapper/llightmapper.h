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
	//void FindLights_Recursive(const Node * pNode);
	//void FindLight(const Node * pNode);

	void ProcessLight(int light_id);
//	void PrepareImageMaps();
	void ProcessRay(LM::Ray r, int depth, float power, int dest_tri_id = 0, const Vector2i * pUV = 0);


	void ProcessTexels();
	void ProcessTexel(int tx, int ty);
	void ProcessSubTexel(float fx, float fy);
	float ProcessTexel_Light(int light_id, const Vector3 &ptDest, const Vector3 &ptNormal, uint32_t tri_ignore);

	void ProcessTexels_Bounce();
	float ProcessTexel_Bounce(int x, int y);
//	void Normalize();

//	void WriteOutputImage(Image &output_image);
//	void RandomUnitDir(Vector3 &dir) const;

};


} // namespace
