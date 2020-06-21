#include "llighttests_simd.h"

namespace LM {



void multi_cross(__m128 result[3], const __m128 a[3], const __m128 b[3])
{
	__m128 tmp;
	__m128 tmp2;

	tmp = _mm_mul_ps(a[1], b[2]);
	tmp2 = _mm_mul_ps(b[1], a[2]);
	result[0] = _mm_sub_ps(tmp, tmp2);

	tmp = _mm_mul_ps(a[2], b[0]);
	tmp2 = _mm_mul_ps(b[2], a[0]);
	result[1] = _mm_sub_ps(tmp, tmp2);

	tmp = _mm_mul_ps(a[0], b[1]);
	tmp2 = _mm_mul_ps(b[0], a[1]);
	result[2] = _mm_sub_ps(tmp, tmp2);
}

//real_t Vector3::dot(const Vector3 &p_b) const {

//	return x * p_b.x + y * p_b.y + z * p_b.z;
//}


__m128 multi_dot(const __m128 a[3], const __m128 b[3])
{
	__m128 tmp = _mm_mul_ps(a[0], b[0]);
	__m128 tmp2 = _mm_mul_ps(a[1], b[1]);
	__m128 tmp3 = _mm_mul_ps(a[2], b[2]);

	return _mm_add_ps(tmp, _mm_add_ps(tmp2, tmp3));
	//    return fmadd(a[2], b[2], fmadd(a[1], b[1], mul(a[0], b[0])));
}

void multi_sub(__m128 result[3], const __m128 a[3], const __m128 b[3])
{
	result[0] = _mm_sub_ps(a[0], b[0]);
	result[1] = _mm_sub_ps(a[1], b[1]);
	result[2] = _mm_sub_ps(a[2], b[2]);
}

const __m128 oneM128 = _mm_set1_ps(1.0f);
const __m128 minusOneM128 = _mm_set1_ps(-1.0f);
const __m128 positiveEpsilonM128 = _mm_set1_ps(0.000001f);
const __m128 negativeEpsilonM128 = _mm_set1_ps(-0.000001f);
const __m128 zeroM128 = _mm_set1_ps(0.0f);


void PackedRay::Create(const Ray &ray)
{
	m_origin[0].mm128 = _mm_set1_ps(ray.o.x);
	m_origin[1].mm128 = _mm_set1_ps(ray.o.y);
	m_origin[2].mm128 = _mm_set1_ps(ray.o.z);

	m_direction[0].mm128 = _mm_set1_ps(ray.d.x);
	m_direction[1].mm128 = _mm_set1_ps(ray.d.y);
	m_direction[2].mm128 = _mm_set1_ps(ray.d.z);
}


bool PackedRay::Intersect(const PackedTriangles& packedTris, float &nearest_dist, int &r_winner) const
{
	//Begin calculating determinant - also used to calculate u parameter
	// P
	__m128 q[3];
	multi_cross(q, &m_direction[0].mm128, &packedTris.e2[0].mm128);

	//if determinant is near zero, ray lies in plane of triangle
	// det
	__m128 a = multi_dot(&packedTris.e1[0].mm128, q);

	// reject based on det being close to zero
	// NYI

	// inv_det
	__m128 f = _mm_div_ps(oneM128, a);

	// distance from v1 to ray origin
	// T
	__m128 s[3];
	multi_sub(s, &m_origin[0].mm128, &packedTris.v0[0].mm128);

	// Calculate u parameter and test bound
	__m128 u = _mm_mul_ps(f, multi_dot(s, q));

	// the intersection lies outside triangle
	// NYI

	// Prepare to test v parameter
	// Q
	__m128 r[3];
	multi_cross(r, s, &packedTris.e1[0].mm128);

	// calculate V parameter and test bound
	// v
	__m128 v = _mm_mul_ps(f, multi_dot(&m_direction[0].mm128, r));

	// intersection outside of triangles?
	// NYI

	// t
	__m128 t = _mm_mul_ps(f, multi_dot(&packedTris.e2[0].mm128, r));

	// if t > epsilon, hit

	// Failure conditions
	// determinant close to zero?
	__m128 failed = _mm_and_ps(_mm_cmpge_ps(a, negativeEpsilonM128), _mm_cmple_ps(a, positiveEpsilonM128));

	//    __m128 failed = _mm_and_ps(
	//        _mm_cmp(a, negativeEpsilonM256, _CMP_GT_OQ),
	//        cmp(a, positiveEpsilonM256, _CMP_LT_OQ)
	//    );

	failed = _mm_or_ps(failed, _mm_cmple_ps(u, zeroM128));
	failed = _mm_or_ps(failed, _mm_cmple_ps(v, zeroM128));
	failed = _mm_or_ps(failed, _mm_cmpge_ps(_mm_add_ps(u, v), oneM128));
	failed = _mm_or_ps(failed, _mm_cmple_ps(t, zeroM128));
	//    failed = _mm_or_ps(failed, _mm_cmpge_ps(t, m_length));
	//failed = _mm_or_ps(failed, packedTris.inactiveMask);

	// use simplified way of getting results
	uint32_t * pResults = (uint32_t*) &failed;
	float * pT = (float*) &t;

	bool bHit  = false;
	for (int n=0; n<4; n++)
	{
		// hit!
		if (!pResults[n] && n<packedTris.num_tris)
		{
			if (pT[n] < nearest_dist)
			{
				nearest_dist = pT[n];
				r_winner = n;
				bHit = true;
			}
		}
	}

	return bHit;

	/*
	__m128 tResults = _mm256_blendv_ps(t, minusOneM256, failed);

	int mask = _mm256_movemask_ps(tResults);
	if (mask != 0xFF)
	{
		// There is at least one intersection
		result.idx = -1;

		float* ptr = (float*)&tResults;
		for (int i = 0; i < 8; ++i)
		{
			if (ptr[i] >= 0.0f && ptr[i] < result.t)
			{
				result.t = ptr[i];
				result.idx = i;
			}
		}

		return result.idx != -1;
	}
	*/

	//    return false;
}

bool LightTests_SIMD::TestIntersect4_Packed(const PackedTriangles &ptris, const Ray &ray, float &r_nearest_t, int &r_winner) const
{
	// simd ray
	PackedRay pray;
	pray.m_origin[0].mm128 = _mm_set1_ps(ray.o.x);
	pray.m_origin[1].mm128 = _mm_set1_ps(ray.o.y);
	pray.m_origin[2].mm128 = _mm_set1_ps(ray.o.z);

	pray.m_direction[0].mm128 = _mm_set1_ps(ray.d.x);
	pray.m_direction[1].mm128 = _mm_set1_ps(ray.d.y);
	pray.m_direction[2].mm128 = _mm_set1_ps(ray.d.z);

	return pray.Intersect(ptris, r_nearest_t, r_winner);
}


bool LightTests_SIMD::TestIntersect4(const Tri *tris[4], const Ray &ray, float &r_nearest_t, int &r_winner) const
{
	/*
	PackedTriangles ptris;

	// load up .. this could cause all kinds of cache misses..
	for (int m=0; m<3; m++)
	{
		// NOTE : set ps takes input IN REVERSE!!!!
		ptris.e1[m].mm128 = _mm_set_ps(tris[3]->pos[0].coord[m],
		tris[2]->pos[0].coord[m],
		tris[1]->pos[0].coord[m],
		tris[0]->pos[0].coord[m]);

		ptris.e2[m].mm128 = _mm_set_ps(tris[3]->pos[1].coord[m],
		tris[2]->pos[1].coord[m],
		tris[1]->pos[1].coord[m],
		tris[0]->pos[1].coord[m]);

		ptris.v0[m].mm128 = _mm_set_ps(tris[3]->pos[2].coord[m],
		tris[2]->pos[2].coord[m],
		tris[1]->pos[2].coord[m],
		tris[0]->pos[2].coord[m]);
	}
//	ptris.inactiveMask = _mm_set1_ps(0.0f);

	// simd ray
	PackedRay pray;
	pray.m_origin[0] = _mm_set1_ps(ray.o.x);
	pray.m_origin[1] = _mm_set1_ps(ray.o.y);
	pray.m_origin[2] = _mm_set1_ps(ray.o.z);

	pray.m_direction[0] = _mm_set1_ps(ray.d.x);
	pray.m_direction[1] = _mm_set1_ps(ray.d.y);
	pray.m_direction[2] = _mm_set1_ps(ray.d.z);

	return pray.intersect(ptris, r_nearest_t, r_winner);
	*/
	return true;
}

} // namespace
