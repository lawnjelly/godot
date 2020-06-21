#pragma once

#define LLIGHTMAPPER_USE_SIMD

#ifdef LLIGHTMAPPER_USE_SIMD
#include <emmintrin.h>
#endif

#include "llighttypes.h"

namespace LM
{

union u_m128
{
#ifdef LLIGHTMAPPER_USE_SIMD
	__m128 mm128;
#endif
	struct
	{
		float v[4];
	};
};

// 4 triangles in edge form
// i.e. edge1, edge2 and a vertex
struct PackedTriangles
{
	u_m128 e1[3];
	u_m128 e2[3];
	u_m128 v0[3];
	// 0 to 4 filled
	uint32_t num_tris;
	void Create() {memset (this, 0, sizeof (PackedTriangles));} // num_tris = 0;
	void Set(int which_tri, const Tri &tri) {Set_e1(which_tri, tri); Set_e2(which_tri, tri); Set_v0(which_tri, tri);
		num_tris = which_tri+1;
	}
	void Set_e1(int which_tri, const Tri &tri) {for (int m=0; m<3; m++) {e1[m].v[which_tri] = tri.pos[0].coord[m];} }
	void Set_e2(int which_tri, const Tri &tri) {for (int m=0; m<3; m++) {e2[m].v[which_tri] = tri.pos[1].coord[m];} }
	void Set_v0(int which_tri, const Tri &tri) {for (int m=0; m<3; m++) {v0[m].v[which_tri] = tri.pos[2].coord[m];} }
};


struct PackedRay
{
	u_m128 m_origin[3];
	u_m128 m_direction[3];
	u_m128 m_length;

	void Create(const Ray &ray);
	bool Intersect(const PackedTriangles& packedTris, float &nearest_dist, int &r_winner) const;
};


class LightTests_SIMD
{
public:
	bool TestIntersect4(const Tri *tris[4], const Ray &ray, float &r_nearest_t, int &r_winner) const;
	bool TestIntersect4_Packed(const PackedTriangles &ptris, const Ray &ray, float &r_nearest_t, int &r_winner) const;


};


} // namespace
