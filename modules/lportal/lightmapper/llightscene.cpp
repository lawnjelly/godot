#include "llightscene.h"
#include "scene/3d/mesh_instance.h"

using namespace LM;

int LightScene::IntersectRay(const Ray &r, float &u, float &v, float &w, float &nearest_t) const
{
	nearest_t = FLT_MAX;
	int nearest_tri = -1;

	for (int n=0; n<m_Tris.size(); n++)
	{
		float t = 0.0f;
		if (r.TestIntersect(m_Tris[n], t))
		{
			if (t < nearest_t)
			{
				nearest_t = t;
				nearest_tri = n;
			}
		}
	}


	if (nearest_tri != -1)
	{
		r.FindIntersect(m_Tris[nearest_tri], nearest_t, u, v, w);
	}

	return nearest_tri;
}


void LightScene::Create(const MeshInstance &mi, int width, int height)
{
	Ref<Mesh> rmesh = mi.get_mesh();
	Array arrays = rmesh->surface_get_arrays(0);
	PoolVector<Vector3> verts = arrays[VS::ARRAY_VERTEX];
	PoolVector<Vector3> norms = arrays[VS::ARRAY_NORMAL];
	m_UVs = arrays[VS::ARRAY_TEX_UV];
	m_Inds = arrays[VS::ARRAY_INDEX];

	// we need to get the vert positions and normals from local space to world space to match up with the
	// world space coords in the merged mesh
	Transform trans = mi.get_global_transform();

	Transform_Verts(verts, m_ptPositions, trans);
	Transform_Norms(norms, m_ptNormals, trans);

	// convert to longhand non indexed versions
	int nTris = m_Inds.size() / 3;
	m_Tris.resize(nTris);
	m_UVTris.resize(nTris);
	m_TriUVaabbs.resize(nTris);
	m_TriPos_aabbs.resize(nTris);

	int i = 0;
	for (int n=0; n<nTris; n++)
	{
		Tri &t = m_Tris[n];
		UVTri &uvt = m_UVTris[n];
		Rect2 &rect = m_TriUVaabbs[n];
		AABB &aabb = m_TriPos_aabbs[n];

		int ind = m_Inds[i++];
		t.pos[0] = m_ptPositions[ind];
		uvt.uv[0] = m_UVs[ind];
		rect = Rect2(uvt.uv[0], Vector2(0, 0));
		aabb.position = t.pos[0];

		ind = m_Inds[i++];
		t.pos[1] = m_ptPositions[ind];
		uvt.uv[1] = m_UVs[ind];
		rect.expand_to(uvt.uv[1]);
		aabb.expand_to(t.pos[1]);

		ind = m_Inds[i++];
		t.pos[2] = m_ptPositions[ind];
		uvt.uv[2] = m_UVs[ind];
		rect.expand_to(uvt.uv[2]);
		aabb.expand_to(t.pos[2]);

		// convert aabb from 0-1 to texels
//		aabb.position.x *= width;
//		aabb.position.y *= height;
//		aabb.size.x *= width;
//		aabb.size.y *= height;

		// expand aabb just a tad
		rect.expand(Vector2(0.01, 0.01));
	}

	m_Tracer.Create(*this);
}

int LightScene::FindTriAtUV(float x, float y, float &u, float &v, float &w) const
{
	for (int n=0; n<m_UVTris.size(); n++)
	{
		// check aabb
		const Rect2 &aabb = m_TriUVaabbs[n];
		if (!aabb.has_point(Point2(x, y)))
			continue;

		const UVTri &tri = m_UVTris[n];

		if (tri.ContainsPoint(Vector2(x, y)))
		{
			// find barycentric coords
			tri.FindBarycentricCoords(Vector2(x, y), u, v, w);

			return n+1; // plus 1 based
		}
	}

	return 0;
}


void LightScene::Transform_Verts(const PoolVector<Vector3> &ptsLocal, PoolVector<Vector3> &ptsWorld, const Transform &tr) const
{
	for (int n=0; n<ptsLocal.size(); n++)
	{
		Vector3 ptWorld = tr.xform(ptsLocal[n]);
		ptsWorld.push_back(ptWorld);
	}
}

void LightScene::Transform_Norms(const PoolVector<Vector3> &normsLocal, PoolVector<Vector3> &normsWorld, const Transform &tr) const
{
	for (int n=0; n<normsLocal.size(); n++)
	{
		// hacky way for normals, we should use transpose of inverse matrix, dunno if godot supports this
		Vector3 ptNormA = Vector3(0, 0, 0);
		Vector3 ptNormWorldA = tr.xform(ptNormA);
		Vector3 ptNormWorldB = tr.xform(normsLocal[n]);

		Vector3 ptNorm = ptNormWorldB - ptNormWorldA;

		ptNorm = ptNorm.normalized();

		normsWorld.push_back(ptNorm);
	}
}
