#pragma once

namespace LM
{

class Vec3i
{
public:
	Vec3i() {}
	Vec3i(int xx, int yy, int zz) {x = xx; y = yy; z = zz;}
	int32_t x, y, z;
	void Set(int xx, int yy, int zz) {x = xx; y = yy; z = zz;}
};

class Vec2_i16
{
public:
	Vec2_i16() {}
	Vec2_i16(int xx, int yy) {x = xx; y = yy;}
	int16_t x;
	int16_t y;
	void Set(int xx, int yy) {x = xx; y = yy;}
	bool IsZero() const {return (x == 0) && (y == 0);}
	bool IsNonZero() const {return !IsZero();}
};

class Tri
{
public:
	Vector3 pos[3];

	void FindBarycentric(const Vector3 &pt, float &u, float &v, float &w) const;
	void InterpolateBarycentric(Vector3 &pt, float u, float v, float w) const;
};


class Ray
{
public:
	Vector3 o; // origin
	Vector3 d; // direction

	bool TestIntersect(const Tri &tri, float &t) const;
	void FindIntersect(const Tri &tri, float t, float &u, float &v, float &w) const;

};

class Line2
{
public:
	Vector2 a;
	Vector2 b;
	float Distance(const Vector2 &pt) const
	{
		Vector2 dir = b - a;
		Vector2 norm = Vector2(dir.y, -dir.x);
		norm.normalize();

		Vector2 test_vec = pt - a;
		return test_vec.dot(norm);
	}
};

class UVTri
{
public:
	Vector2 uv[3];

	void FindUVBarycentric(Vector2 &res, float u, float v, float w) const;
	bool ContainsPoint(const Vector2 &pt, float epsilon = 0.0f) const;
	void FindBarycentricCoords(const Vector2 &pt, float &u, float &v, float &w) const;
};


inline bool UVTri::ContainsPoint(const Vector2 &pt, float epsilon) const
{
	const Vector2 &a = uv[0];
	const Vector2 &b = uv[1];
	const Vector2 &c = uv[2];

	// if the point is in front of any of the edges, it is outside the tri,
	// if not, it must be inside

	// N.B. the triangle must be ordered clockwise for this to work
	Line2 edge;
	edge.a = b;
	edge.b = a;

	if (edge.Distance(pt) < epsilon)
		return false;

	edge.a = c;
	edge.b = b;

	if (edge.Distance(pt) < epsilon)
		return false;

	edge.a = a;
	edge.b = c;

	if (edge.Distance(pt) < epsilon)
		return false;

	return true;
}

inline void UVTri::FindBarycentricCoords(const Vector2 &pt, float &u, float &v, float &w) const
{
	const Vector2 &a = uv[0];
	const Vector2 &b = uv[1];
	const Vector2 &c = uv[2];
	Vector2 v0 = b - a, v1 = c - a, v2 = pt - a;
    float d00 = v0.dot(v0);
    float d01 = v0.dot(v1);
    float d11 = v1.dot(v1);
    float d20 = v2.dot(v0);
    float d21 = v2.dot(v1);
    float invDenom = 1.0f / (d00 * d11 - d01 * d01);
    v = (d11 * d20 - d01 * d21) * invDenom;
    w = (d00 * d21 - d01 * d20) * invDenom;
    u = 1.0f - v - w;
}


inline void UVTri::FindUVBarycentric(Vector2 &res, float u, float v, float w) const
{
	res = (uv[0] * u) + (uv[1] * v) + (uv[2] * w);
}
//////////////////////////////////////////////////////////

inline void Tri::FindBarycentric(const Vector3 &pt, float &u, float &v, float &w) const
{
	const Vector3 &a = pos[0];
	const Vector3 &b = pos[1];
	const Vector3 &c = pos[2];

	// we can alternatively precache
	Vector3 v0 = b - a, v1 = c - a, v2 = pt - a;
    float d00 = v0.dot(v0);
    float d01 = v0.dot(v1);
    float d11 = v1.dot(v1);
    float d20 = v2.dot(v0);
    float d21 = v2.dot(v1);
    float invDenom = 1.0f / (d00 * d11 - d01 * d01);
    v = (d11 * d20 - d01 * d21) * invDenom;
    w = (d00 * d21 - d01 * d20) * invDenom;
    u = 1.0f - v - w;
}

inline void Tri::InterpolateBarycentric(Vector3 &pt, float u, float v, float w) const
{
	const Vector3 &a = pos[0];
	const Vector3 &b = pos[1];
	const Vector3 &c = pos[2];
	pt.x = (a.x * u) + (b.x * v) + (c.x * w);
	pt.y = (a.y * u) + (b.y * v) + (c.y * w);
	pt.z = (a.z * u) + (b.z * v) + (c.z * w);
}


// returns barycentric, assumes we have already tested and got a collision
inline void Ray::FindIntersect(const Tri &tri, float t, float &u, float &v, float &w) const
{
	Vector3 pt = o + (d * t);
	tri.FindBarycentric(pt, u, v, w);
}


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
