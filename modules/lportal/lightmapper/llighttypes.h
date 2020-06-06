#pragma once

namespace LM
{
class Tri
{
public:
	Vector3 pos[3];
};


class Ray
{
public:
	Vector3 o; // origin
	Vector3 d; // direction

	bool TestIntersect(const Tri &tri, float &t) const;

};

class UVTri
{
public:
	Vector2 uv[3];
};




inline bool Ray::TestIntersect(const Tri &tri, float &t) const
{
	//	int triangle_intersection( const Vec3   V1,  // Triangle vertices
	//	const Vec3   V2,
	//	const Vec3   V3,
	//	const Vec3    O,  //Ray origin
	//	const Vec3    D,  //Ray direction
	//	float* out )
	//	{

	// V1  = a
	// V2 = b
	// V3 = c
	// O = ray.a
	// D = ray.b

	const Vector3 &a = tri.pos[0];
	const Vector3 &b = tri.pos[1];
	const Vector3 &c = tri.pos[2];

	const float cfEpsilon = 0.000001f;
	//#define EPSILON 0.000001
	Vector3 e1, e2;  //Edge1, Edge2
	Vector3 P, Q, T;
	float det, inv_det, u, v;
	//	float t;

	//Find vectors for two edges sharing V1
	e1 = b - a;
	e2 = c - a;

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

	if(t > cfEpsilon) { //ray intersection
	//	*out = t;
	return true; //IR_HIT;
	}

	// No hit, no win
	return false; //IR_BEHIND;
}


}
