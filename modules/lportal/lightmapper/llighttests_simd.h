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
};


class LightTests_SIMD
{
public:
	bool TestIntersect4(const Tri *tris[4], const Ray &ray, float &r_nearest_t, int &r_winner) const;


};


} // namespace
