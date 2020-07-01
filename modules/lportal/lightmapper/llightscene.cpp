#include "llightscene.h"
#include "scene/3d/mesh_instance.h"
#include "llighttests_simd.h"


//#define LLIGHTSCENE_VERBOSE

using namespace LM;

void LightScene::ProcessVoxelHits(const Ray &ray, const PackedRay &pray, const Voxel &voxel, float &r_nearest_t, int &r_nearest_tri)//, int ignore_triangle_id_p1)
{
	//#define LLIGHTMAPPED_DEBUG_COMPARE_SIMD

//	float record_nearest_t = r_nearest_t;
//	int record_nearest_tri = r_nearest_tri;

#ifdef LLIGHTMAPPER_USE_SIMD
	if (m_bUseSIMD)
	{
		//LightTests_SIMD simd;

		// groups of 4
		int quads = voxel.m_PackedTriangles.size();

		// special case of ignore triangles being set
//		int ignore_quad = -1;
//		int ignore_quad_index;
//		if (ignore_triangle_id_p1)
//		{
//			int test_tri_id = ignore_triangle_id_p1-1;

//			for (int n=0; n<voxel.m_iNumTriangles; n++)
//			{
//				if (voxel.m_TriIDs[n] == test_tri_id)
//				{
//					// found
//					ignore_quad = n / 4;
//					ignore_quad_index = n % 4;
//				}
//			}
//		}

		for (int q=0; q<quads; q++)
		{
			// get pointers to 4 triangles
			const PackedTriangles & ptris = voxel.m_PackedTriangles[q];

			//  we need to deal with the special case of the ignore_triangle being set. This will normally only occur on the first voxel.
			int winner;
			// if there is no ignore triangle, or the quad is not the one with the ignore triangle, do the fast path
//			if ((!ignore_triangle_id_p1) || (q != ignore_quad))
//			{
				winner = pray.Intersect(ptris, r_nearest_t);
//			}
//			else
//			{
//				// slow path
//				PackedTriangles pcopy = ptris;
//				pcopy.MakeInactive(ignore_quad_index);
//				winner = pray.Intersect(pcopy, r_nearest_t);
//			}

			if (winner != 0)
			//if (pray.Intersect(ptris, r_nearest_t, winner))
			{
				winner--;
				int winner_tri_index = (q * 4) + winner;
				//int winner_tri = m_Tracer.m_TriHits[winner_tri_index];
				int winner_tri = voxel.m_TriIDs[winner_tri_index];

				/*
			// test assert condition
			if (winner_tri != ref_winner_tri_id)
			{
				// do again to debug
				float test_nearest_t = FLT_MAX;
				simd.TestIntersect4(pTris, ray, test_nearest_t, winner);

				// repeat reference test
				int ref_start = nStart-4;
				for (int n=0; n<4; n++)
				{
					unsigned int tri_id = m_Tracer.m_TriHits[ref_start++];

					float t = 0.0f;
					ray.TestIntersect_EdgeForm(m_Tris_EdgeForm[tri_id], t);

				}
			}
			*/


				r_nearest_tri = winner_tri;
			}

			//		assert (r_nearest_t <= (nearest_ref_dist+0.001f));
		}

		// print result
//		if (r_nearest_tri != -1)
//			print_line("SIMD\tr_nearest_tri " + itos (r_nearest_tri) + " dist " + String(Variant(r_nearest_t)));

		return;
	} // if use SIMD
#endif

	// trace after every voxel
	int nHits = m_Tracer.m_TriHits.size();
	int nStart = 0;


	// just for debugging do whole test again
//	int simd_nearest_tri = r_nearest_tri;
//	r_nearest_t = record_nearest_t;
//	r_nearest_tri = record_nearest_tri;

	// leftovers
	for (int n=nStart; n<nHits; n++)
	{
		unsigned int tri_id = m_Tracer.m_TriHits[n];

		float t = 0.0f;
		//			if (ray.TestIntersect(m_Tris[tri_id], t))
		if (ray.TestIntersect_EdgeForm(m_Tris_EdgeForm[tri_id], t))
		{
			if (t < r_nearest_t)
			{
				r_nearest_t = t;
				r_nearest_tri = tri_id;
			}
		}
	}

	// print result
//	if (r_nearest_tri != -1)
//		print_line("REF\tr_nearest_tri " + itos (r_nearest_tri) + " dist " + String(Variant(r_nearest_t)));

//	assert (r_nearest_tri == simd_nearest_tri);
}



void LightScene::ProcessVoxelHits_Old(const Ray &ray, const Voxel &voxel, float &r_nearest_t, int &r_nearest_tri)
{
	// trace after every voxel
	int nHits = m_Tracer.m_TriHits.size();
	int nStart = 0;

	//#define LLIGHTMAPPED_DEBUG_COMPARE_SIMD

#ifdef LLIGHTMAPPER_USE_SIMD
	if (m_bUseSIMD)
	{
		LightTests_SIMD simd;

		// groups of 4
		int quads = nHits / 4;


		for (int q=0; q<quads; q++)
		{
			// get pointers to 4 triangles
			const Tri * pTris[4];
#if LLIGHTMAPPED_DEBUG_COMPARE_SIMD
			float nearest_ref_dist = FLT_MAX;
			int ref_winner_tri_id = -1;
			int ref_winner_n = -1;
#endif

			for (int n=0; n<4; n++)
			{
				unsigned int tri_id = m_Tracer.m_TriHits[nStart++];

#if LLIGHTMAPPED_DEBUG_COMPARE_SIMD
				float t = 0.0f;
				print_line ("ref input triangle" + itos(n));
				const Tri &ref_input_tri = m_Tris_EdgeForm[tri_id];
				String sz = "\t";
				for (int abc=0; abc<3; abc++)
				{
					sz += "(" + String(Variant(ref_input_tri.pos[abc])) + ") ";
				}
				print_line(sz);

				if (ray.TestIntersect_EdgeForm(ref_input_tri, t))
				{
					if (t < nearest_ref_dist)
					{
						nearest_ref_dist = t;
						ref_winner_tri_id = tri_id;
						ref_winner_n = n;
					}
				}
#endif

				pTris[n] = &m_Tris_EdgeForm[tri_id];
			}

			// compare with old

			// test 4
			//		int test[4];
			int winner;
			if (simd.TestIntersect4(pTris, ray, r_nearest_t, winner))
			{
				int winner_tri_index = nStart-4 + winner;
				int winner_tri = m_Tracer.m_TriHits[winner_tri_index];

				/*
			// test assert condition
			if (winner_tri != ref_winner_tri_id)
			{
				// do again to debug
				float test_nearest_t = FLT_MAX;
				simd.TestIntersect4(pTris, ray, test_nearest_t, winner);

				// repeat reference test
				int ref_start = nStart-4;
				for (int n=0; n<4; n++)
				{
					unsigned int tri_id = m_Tracer.m_TriHits[ref_start++];

					float t = 0.0f;
					ray.TestIntersect_EdgeForm(m_Tris_EdgeForm[tri_id], t);

				}
			}
			*/


				r_nearest_tri = winner_tri;
			}

			//		assert (r_nearest_t <= (nearest_ref_dist+0.001f));
		}

	} // if use SIMD
#endif

	// leftovers
	for (int n=nStart; n<nHits; n++)
	{
		unsigned int tri_id = m_Tracer.m_TriHits[n];

		float t = 0.0f;
		//			if (ray.TestIntersect(m_Tris[tri_id], t))
		if (ray.TestIntersect_EdgeForm(m_Tris_EdgeForm[tri_id], t))
		{
			if (t < r_nearest_t)
			{
				r_nearest_t = t;
				r_nearest_tri = tri_id;
			}
		}
	}

}


int LightScene::IntersectRay(const Ray &ray, float &u, float &v, float &w, float &nearest_t, const Vec3i * pVoxelRange, int &num_tests)//, int ignore_tri_p1)
{
	nearest_t = FLT_MAX;
	int nearest_tri = -1;

	Ray voxel_ray;
	Vec3i ptVoxel;

	// prepare voxel trace
	if (!m_Tracer.RayTrace_Start(ray, voxel_ray, ptVoxel))
		return nearest_tri;

	bool bFirstHit = false;
	Vec3i ptVoxelFirstHit;

	// if we have specified a (optional) maximum range for the trace in voxels
	Vec3i ptVoxelStart;
	if (pVoxelRange)
	{
		ptVoxelStart = ptVoxel;
	}

	// create the packed ray as a once off and reuse it for each voxel
	PackedRay pray;
	pray.Create(ray);

	while (true)
	{
		Vec3i ptVoxelBefore = ptVoxel;

		const Voxel * pVoxel = m_Tracer.RayTrace(voxel_ray, voxel_ray, ptVoxel);
//		if (!m_Tracer.RayTrace(voxel_ray, voxel_ray, ptVoxel))
		if (!pVoxel)
			break;

		ProcessVoxelHits(ray, pray, *pVoxel, nearest_t, nearest_tri);

		// count number of tests for stats
				int nHits = m_Tracer.m_TriHits.size();
				num_tests += nHits;

		// first hit?
		if (!bFirstHit)
		{
			if (nearest_tri != -1)
			{
				bFirstHit = true;
				ptVoxelFirstHit = ptVoxelBefore;
			}
			else
			{
				// check for voxel range
				if (pVoxelRange)
				{
					if (abs(ptVoxel.x - ptVoxelStart.x) > pVoxelRange->x)
						break;
					if (abs(ptVoxel.y - ptVoxelStart.y) > pVoxelRange->y)
						break;
					if (abs(ptVoxel.z - ptVoxelStart.z) > pVoxelRange->z)
						break;
				} // if voxel range
			}
		}
		else
		{
			// out of range of first voxel?
			if (abs(ptVoxel.x - ptVoxelFirstHit.x) > 1)
				break;
			if (abs(ptVoxel.y - ptVoxelFirstHit.y) > 1)
				break;
			if (abs(ptVoxel.z - ptVoxelFirstHit.z) > 1)
				break;
		}

#ifdef LIGHTTRACER_IGNORE_VOXELS
		break;
#endif

	} // while

	if (nearest_tri != -1)
	{
		ray.FindIntersect(m_Tris[nearest_tri], nearest_t, u, v, w);
	}

	return nearest_tri;
}

int LightScene::IntersectRay_old(const Ray &r, float &u, float &v, float &w, float &nearest_t) const
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

void LightScene::Reset()
{
	m_ptPositions.resize(0);
	m_ptNormals.resize(0);
	m_UVs.resize(0);
	m_Inds.resize(0);

	m_UVTris.clear(true);
	m_TriUVaabbs.clear(true);
	m_TriPos_aabbs.clear(true);
	m_Tracer.Reset();

	m_Tris.clear(true);
	m_TriNormals.clear(true);
	m_Tris_EdgeForm.clear(true);

}


bool LightScene::Create(const MeshInstance &mi, int width, int height, const Vec3i &voxel_dims)
{
	m_bUseSIMD = true;

	Ref<Mesh> rmesh = mi.get_mesh();
	Array arrays = rmesh->surface_get_arrays(0);
	if (!arrays.size())
		return false;

	PoolVector<Vector3> verts = arrays[VS::ARRAY_VERTEX];
	if (!verts.size())
		return false;
	PoolVector<Vector3> norms = arrays[VS::ARRAY_NORMAL];
	if (!norms.size())
		return false;

	m_UVs = arrays[VS::ARRAY_TEX_UV];
	if (!m_UVs.size())
		m_UVs = arrays[VS::ARRAY_TEX_UV2];
	if (!m_UVs.size())
		return false;

	m_Inds = arrays[VS::ARRAY_INDEX];
	if (!m_Inds.size())
		return false;

	// we need to get the vert positions and normals from local space to world space to match up with the
	// world space coords in the merged mesh
	Transform trans = mi.get_global_transform();

	Transform_Verts(verts, m_ptPositions, trans);
	Transform_Norms(norms, m_ptNormals, trans);

	// convert to longhand non indexed versions
	int nTris = m_Inds.size() / 3;

	m_Tris.resize(nTris);
	m_TriNormals.resize(nTris);
	m_Tris_EdgeForm.resize(nTris);

	m_UVTris.resize(nTris);
	m_TriUVaabbs.resize(nTris);
	m_TriPos_aabbs.resize(nTris);

	int i = 0;
	for (int n=0; n<nTris; n++)
	{
		Tri &t = m_Tris[n];
		Tri &tri_norm = m_TriNormals[n];
		Tri &tri_edge = m_Tris_EdgeForm[n];
		UVTri &uvt = m_UVTris[n];
		Rect2 &rect = m_TriUVaabbs[n];
		AABB &aabb = m_TriPos_aabbs[n];

		int ind = m_Inds[i];
		rect = Rect2(m_UVs[ind], Vector2(0, 0));
		aabb.position = m_ptPositions[ind];
		aabb.size = Vector3(0, 0, 0);

		for (int c=0; c<3; c++)
		{
			ind = m_Inds[i++];

			t.pos[c] = m_ptPositions[ind];
			tri_norm.pos[c] = m_ptNormals[ind];
			uvt.uv[c] = m_UVs[ind];
			//rect = Rect2(uvt.uv[0], Vector2(0, 0));
			rect.expand_to(uvt.uv[c]);
			//aabb.position = t.pos[0];
			aabb.expand_to(t.pos[c]);
		}

		// make sure winding is standard in UV space
		if (uvt.IsWindingCW())
		{
			uvt.FlipWinding();
			t.FlipWinding();
			tri_norm.FlipWinding();
		}

		// calculate edge form
		{
			// b - a
			tri_edge.pos[0] = t.pos[1] - t.pos[0];
			// c - a
			tri_edge.pos[1] = t.pos[2] - t.pos[0];
			// a
			tri_edge.pos[2] = t.pos[0];
		}

#ifdef LLIGHTSCENE_VERBOSE
		String sz;
		sz = "found triangle : ";
		for (int s=0; s<3; s++)
		{
			sz += "(" + String(t.pos[s]) + ") ... ";
		}
		print_line(sz);
		sz = "\tnormal : ";
		for (int s=0; s<3; s++)
		{
			sz += "(" + String(tri_norm.pos[s]) + ") ... ";
		}
		print_line(sz);

		sz = "\t\tUV : ";
		for (int s=0; s<3; s++)
		{
			sz += "(" + String(uvt.uv[s]) + ") ... ";
		}
		print_line(sz);

#endif


		// convert aabb from 0-1 to texels
		//		aabb.position.x *= width;
		//		aabb.position.y *= height;
		//		aabb.size.x *= width;
		//		aabb.size.y *= height;

		// expand aabb just a tad
		rect.expand(Vector2(0.01, 0.01));
	}

	m_Tracer.Create(*this, voxel_dims);

	return true;
}

void LightScene::RasterizeTriangleIDs(LightImage<uint32_t> &im_p1, LightImage<uint32_t> &im2_p1, LightImage<Vector3> &im_bary)
{
	int width = im_p1.GetWidth();
	int height = im_p1.GetHeight();

	for (int n=0; n<m_UVTris.size(); n++)
	{
		const Rect2 &aabb = m_TriUVaabbs[n];
		const UVTri &tri = m_UVTris[n];

		int min_x = aabb.position.x * width;
		int min_y = aabb.position.y * height;
		int max_x = (aabb.position.x + aabb.size.x) * width;
		int max_y = (aabb.position.y + aabb.size.y) * height;

		// add a bit for luck
		min_x--; min_y--; max_x++; max_y++;

		// clamp
		min_x = CLAMP(min_x, 0, width);
		min_y = CLAMP(min_y, 0, height);
		max_x = CLAMP(max_x, 0, width);
		max_y = CLAMP(max_y, 0, height);

		int debug_overlap_count = 0;

		for (int y=min_y; y<max_y; y++)
		{
			for (int x=min_x; x<max_x; x++)
			{
				float s = (x + 0.5f) / (float) width;
				float t = (y + 0.5f) / (float) height;

				if (tri.ContainsPoint(Vector2(s, t)))
				{
					uint32_t &id_p1 = im_p1.GetItem(x, y);

					// hopefully this was 0 before
					if (id_p1)
					{
						debug_overlap_count++;
						if (debug_overlap_count == 64)
						{
							print_line("overlap detected");
						}

						// store the overlapped ID in a second map
						im2_p1.GetItem(x, y) = id_p1;
					}

					// save new id
					id_p1 = n+1;

					// find barycentric coords
					float u,v,w;
					tri.FindBarycentricCoords(Vector2(s, t), u, v, w);
					Vector3 &bary = im_bary.GetItem(x, y);
					bary =Vector3(u,v,w);
				}

			} // for x
		} // for y
	} // for tri
}

/*
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
*/

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
