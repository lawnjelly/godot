#include "llighttests_simd.h"
#include <emmintrin.h>


//#define LLIGHTMAPPER_SIMD_REFERENCE

namespace LM {

union my128
{
	struct
	{
		float v[4];
	};
	__m128 q;
};

struct PackedTriangles
{
	__m128 e1[3];
	__m128 e2[3];
	__m128 v0[3];
	__m128 inactiveMask; // Required. We cant always have 8 triangles per packet.
};

struct PackedIntersectionResult
{
	float t;
	int idx;
};

struct PackedRay
{
	__m128 m_origin[3];
	__m128 m_direction[3];
	__m128 m_length;

	bool intersect(const PackedTriangles& packedTris, float &nearest_dist, int &r_winner) const;
};


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
const __m128 positiveEpsilonM128 = _mm_set1_ps(1e-6f);
const __m128 negativeEpsilonM128 = _mm_set1_ps(-1e-6f);
const __m128 zeroM128 = _mm_set1_ps(0.0f);

bool PackedRay::intersect(const PackedTriangles& packedTris, float &nearest_dist, int &r_winner) const
{
#ifdef LLIGHTMAPPER_SIMD_REFERENCE
	Tri ref_tri;
	ref_tri.pos[0].x = packedTris.e1[0][0];
	ref_tri.pos[0].y = packedTris.e1[1][0];
	ref_tri.pos[0].z = packedTris.e1[2][0];
	ref_tri.pos[1].x = packedTris.e2[0][0];
	ref_tri.pos[1].y = packedTris.e2[1][0];
	ref_tri.pos[1].z = packedTris.e2[2][0];
	ref_tri.pos[2].x = packedTris.v0[0][0];
	ref_tri.pos[2].y = packedTris.v0[1][0];
	ref_tri.pos[2].z = packedTris.v0[2][0];

	Ray ref_ray;
	ref_ray.o.x = m_origin[0][0];
	ref_ray.o.y = m_origin[1][0];
	ref_ray.o.z = m_origin[2][0];
	ref_ray.d.x = m_direction[0][0];
	ref_ray.d.y = m_direction[1][0];
	ref_ray.d.z = m_direction[2][0];

	//Edge1, Edge2
	const Vector3 &ref_e1 = ref_tri.pos[0];
	const Vector3 &ref_e2 = ref_tri.pos[1];
	const Vector3 &ref_a = ref_tri.pos[2];

	const float ref_cfEpsilon = 0.000001f;
	Vector3 ref_P, ref_Q, ref_T;
	float ref_det, ref_inv_det, ref_u, ref_v;
	float ref_t;

	//Begin calculating determinant - also used to calculate u parameter
	ref_P = ref_ray.d.cross(ref_e2);

	print_line("P is " + String(ref_P));

	//if determinant is near zero, ray lies in plane of triangle
	ref_det = ref_e1.dot(ref_P);

	print_line("det is " + String(Variant(ref_det)));

	//NOT CULLING
	if(ref_det > -ref_cfEpsilon && ref_det < ref_cfEpsilon)
	{
		print_line("FALSE, det near zero");
		//return false;//IR_NONE;
	}
	else
	{
		ref_inv_det = 1.f / ref_det;
		print_line("inv_det is " + String(Variant(ref_inv_det)));

		//calculate distance from V1 to ray origin
		ref_T = ref_ray.o - ref_a;
		print_line("T is " + String(ref_T));

		//Calculate u parameter and test bound
		ref_u = ref_T.dot(ref_P) * ref_inv_det;
		print_line("u is " + String(Variant(ref_u)));

		//The intersection lies outside of the triangle
		if(ref_u < 0.f || ref_u > 1.f)
		{
			print_line("FALSE u out of range");
			//return false; // IR_NONE;
		}
		else
		{
			//Prepare to test v parameter
			ref_Q = ref_T.cross(ref_e1);
			print_line("Q is " + String(ref_Q));

			//Calculate V parameter and test bound
			ref_v = ref_ray.d.dot(ref_Q) * ref_inv_det;
			print_line("v is " + String(Variant(ref_v)));

			//The intersection lies outside of the triangle
			if(ref_v < 0.f || ref_u + ref_v  > 1.f)
			{
				print_line("FALSE v or uv out of range");
				//return false; //IR_NONE;
			}
			else
			{
				ref_t = ref_e2.dot(ref_Q) * ref_inv_det;
				print_line("t is " + String(Variant(ref_t)));


				if(ref_t > ref_cfEpsilon)
				{ //ray intersection
					//	*out = t;
					print_line("HIT");
					//return true; //IR_HIT;
				}
				else
				{
					print_line("FALSE t is behind");
				}

				// No hit, no win
//				return false; //IR_BEHIND;
			} // uv in range
		} // u not outof range
	} // det not zero
#endif


	//Begin calculating determinant - also used to calculate u parameter
	// P
	__m128 q[3];
	multi_cross(q, m_direction, packedTris.e2);

	//if determinant is near zero, ray lies in plane of triangle
	// det
	__m128 a = multi_dot(packedTris.e1, q);

	// reject based on det being close to zero
	// NYI

	// inv_det
	__m128 f = _mm_div_ps(oneM128, a);

	// distance from v1 to ray origin
	// T
	__m128 s[3];
	multi_sub(s, m_origin, packedTris.v0);

	// Calculate u parameter and test bound
	__m128 u = _mm_mul_ps(f, multi_dot(s, q));

	// the intersection lies outside triangle
	// NYI

	// Prepare to test v parameter
	// Q
	__m128 r[3];
	multi_cross(r, s, packedTris.e1);

	// calculate V parameter and test bound
	// v
	__m128 v = _mm_mul_ps(f, multi_dot(m_direction, r));

	// intersection outside of triangles?
	// NYI

	// t
	__m128 t = _mm_mul_ps(f, multi_dot(packedTris.e2, r));

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
		if (!pResults[n])
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


bool LightTests_SIMD::TestIntersect4(const Tri *tris[4], const Ray &ray, float &r_nearest_t, int &r_winner) const
{
	PackedTriangles ptris;

	// load up .. this could cause all kinds of cache misses..
	for (int m=0; m<3; m++)
	{
		// NOTE : set ps takes input IN REVERSE!!!!
		ptris.e1[m] = _mm_set_ps(tris[3]->pos[0].coord[m],
		tris[2]->pos[0].coord[m],
		tris[1]->pos[0].coord[m],
		tris[0]->pos[0].coord[m]);

		ptris.e2[m] = _mm_set_ps(tris[3]->pos[1].coord[m],
		tris[2]->pos[1].coord[m],
		tris[1]->pos[1].coord[m],
		tris[0]->pos[1].coord[m]);

		ptris.v0[m] = _mm_set_ps(tris[3]->pos[2].coord[m],
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
}


/*
//Edge1, Edge2
const Vector3 &e1 = tri_edge.pos[0];
const Vector3 &e2 = tri_edge.pos[1];
const Vector3 &a = tri_edge.pos[2];

const float cfEpsilon = 0.000001f;
Vector3 P, Q, T;
float det, inv_det, u, v;

//Begin calculating determinant - also used to calculate u parameter
P = d.cross(e2);

//if determinant is near zero, ray lies in plane of triangle
det = e1.dot(P);

//NOT CULLING
if(det > -cfEpsilon && det < cfEpsilon) return false;//IR_NONE;
inv_det = 1.f / det;

//calculate distance from V1 to ray origin
T = o - a;

//Calculate u parameter and test bound
u = T.dot(P) * inv_det;

//The intersection lies outside of the triangle
if(u < 0.f || u > 1.f) return false; // IR_NONE;

//Prepare to test v parameter
Q = T.cross(e1);

//Calculate V parameter and test bound
v = d.dot(Q) * inv_det;

//The intersection lies outside of the triangle
if(v < 0.f || u + v  > 1.f) return false; //IR_NONE;

t = e2.dot(Q) * inv_det;

if(t > cfEpsilon)
{ //ray intersection
	//	*out = t;
	return true; //IR_HIT;
}

// No hit, no win
return false; //IR_BEHIND;
*/



} // namespace
