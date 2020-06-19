#pragma once

#define LLIGHTMAPPER_USE_SIMD

#include "llighttypes.h"

namespace LM
{

class LightTests_SIMD
{
public:
	void TestIntersect4(const Tri *tris[4], const Ray &ray, float &r_nearest_t, int &r_nearest_tri) const;


};


} // namespace
