#include "llightscene.h"
#include "core/os/thread_work_pool.h"
#include "llightmapper_base.h"
#include "llighttests_simd.h"
#include "llighttypes.h"
#include "scene/3d/mesh_instance.h"

//#define LLIGHTSCENE_VERBOSE
//#define LLIGHTSCENE_TRACE_VERBOSE

using namespace LM;

bool LightScene::test_voxel_hits(const Ray &ray, const PackedRay &pray, const Voxel &voxel, float max_dist, bool bCullBackFaces) {
	int quads = voxel.packed_triangles.size();

	if (!bCullBackFaces) {
		for (int q = 0; q < quads; q++) {
			// get pointers to 4 triangles
			const PackedTriangles &ptris = voxel.packed_triangles[q];
			if (pray.IntersectTest(ptris, max_dist))
				return true;
		}
	} else {
		/*
		// test backface culling
		Tri t;
		t.pos[0] = Vector3(0, 0, 0);
		t.pos[2] = Vector3(1, 1, 0);
		t.pos[1] = Vector3(1, 0, 0);
		t.ConvertToEdgeForm();
		PackedTriangles pttest;
		pttest.Create();
		pttest.Set(0, t);
		pttest.Set(1, t);
		pttest.Set(2, t);
		pttest.Set(3, t);
		pttest.Finalize(4);

		Ray rtest;
		rtest.o = Vector3(0.5, 0.4, -1);
		rtest.d = Vector3(0, 0, 1);
		PackedRay prtest;
		prtest.Create(rtest);

		prtest.IntersectTest_CullBackFaces(pttest, 10);
		*/

		for (int q = 0; q < quads; q++) {
			// get pointers to 4 triangles
			const PackedTriangles &ptris = voxel.packed_triangles[q];
			if (pray.IntersectTest_CullBackFaces(ptris, max_dist))
				return true;
		}
	}

	return false;
}

void LightScene::process_voxel_hits(const Ray &ray, const PackedRay &pray, const Voxel &voxel, float &r_nearest_t, int &r_nearest_tri) //, int ignore_triangle_id_p1)
{
	//#define LLIGHTMAPPED_DEBUG_COMPARE_SIMD

	//	float record_nearest_t = r_nearest_t;
	//	int record_nearest_tri = r_nearest_tri;

	//#ifdef LLIGHTMAPPER_USE_SIMD
	if (_use_SIMD) {
		//LightTests_SIMD simd;

		// groups of 4
		int quads = voxel.packed_triangles.size();

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

		for (int q = 0; q < quads; q++) {
			// get pointers to 4 triangles
			const PackedTriangles &ptris = voxel.packed_triangles[q];

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

#ifdef LLIGHTSCENE_TRACE_VERBOSE
			String sz = "\ttesting tris ";
			for (int t = 0; t < 4; t++) {
				int test_tri = (q * 4) + t;
				if (test_tri < voxel.m_TriIDs.size()) {
					sz += itos(voxel.m_TriIDs[(q * 4) + t]);
					sz += ", ";
				}
			}
			print_line(sz);
#endif

			if (winner != 0)
			//if (pray.Intersect(ptris, r_nearest_t, winner))
			{
				winner--;
				int winner_tri_index = (q * 4) + winner;
				//int winner_tri = m_Tracer.m_TriHits[winner_tri_index];
				int winner_tri = voxel.tri_ids[winner_tri_index];

#ifdef LLIGHTSCENE_TRACE_VERBOSE
				const AABB &tri_bb = m_TriPos_aabbs[winner_tri];
				print_line("\t\tWINNER tri : " + itos(winner_tri) + " at dist " + String(Variant(r_nearest_t)) + " aabb " + String(tri_bb));
#endif

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
	//#endif

	// trace after every voxel
	int nHits = _tracer._tri_hits.size();
	int nStart = 0;

	// just for debugging do whole test again
	//	int simd_nearest_tri = r_nearest_tri;
	//	r_nearest_t = record_nearest_t;
	//	r_nearest_tri = record_nearest_tri;

	// leftovers
	for (int n = nStart; n < nHits; n++) {
		unsigned int tri_id = _tracer._tri_hits[n];

		float t = 0.0f;
		//			if (ray.TestIntersect(m_Tris[tri_id], t))
		if (ray.test_intersect_edge_form(_tris_edge_form[tri_id], t)) {
			if (t < r_nearest_t) {
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

void LightScene::process_voxel_hits_old(const Ray &ray, const Voxel &voxel, float &r_nearest_t, int &r_nearest_tri) {
	// trace after every voxel
	int nHits = _tracer._tri_hits.size();
	int nStart = 0;

	//#define LLIGHTMAPPED_DEBUG_COMPARE_SIMD

#ifdef LLIGHTMAPPER_USE_SIMD
	if (_use_SIMD) {
		LightTests_SIMD simd;

		// groups of 4
		int quads = nHits / 4;

		for (int q = 0; q < quads; q++) {
			// get pointers to 4 triangles
			const Tri *pTris[4];
#if LLIGHTMAPPED_DEBUG_COMPARE_SIMD
			float nearest_ref_dist = FLT_MAX;
			int ref_winner_tri_id = -1;
			int ref_winner_n = -1;
#endif

			for (int n = 0; n < 4; n++) {
				unsigned int tri_id = _tracer._tri_hits[nStart++];

#if LLIGHTMAPPED_DEBUG_COMPARE_SIMD
				float t = 0.0f;
				print_line("ref input triangle" + itos(n));
				const Tri &ref_input_tri = m_Tris_EdgeForm[tri_id];
				String sz = "\t";
				for (int abc = 0; abc < 3; abc++) {
					sz += "(" + String(Variant(ref_input_tri.pos[abc])) + ") ";
				}
				print_line(sz);

				if (ray.TestIntersect_EdgeForm(ref_input_tri, t)) {
					if (t < nearest_ref_dist) {
						nearest_ref_dist = t;
						ref_winner_tri_id = tri_id;
						ref_winner_n = n;
					}
				}
#endif

				pTris[n] = &_tris_edge_form[tri_id];
			}

			// compare with old

			// test 4
			//		int test[4];
			int winner;
			if (simd.TestIntersect4(pTris, ray, r_nearest_t, winner)) {
				int winner_tri_index = nStart - 4 + winner;
				int winner_tri = _tracer._tri_hits[winner_tri_index];

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
	for (int n = nStart; n < nHits; n++) {
		unsigned int tri_id = _tracer._tri_hits[n];

		float t = 0.0f;
		//			if (ray.TestIntersect(m_Tris[tri_id], t))
		if (ray.test_intersect_edge_form(_tris_edge_form[tri_id], t)) {
			if (t < r_nearest_t) {
				r_nearest_t = t;
				r_nearest_tri = tri_id;
			}
		}
	}
}

bool LightScene::test_intersect_line(const Vector3 &a, const Vector3 &b, bool bCullBackFaces) {
	Ray r;
	r.o = a;
	r.d = b - a;
	float dist = r.d.length();
	if (dist > 0.0f) {
		// normalize
		r.d /= dist;
		bool res = test_intersect_ray(r, dist);
		/*
		// double check
		Vector3 bary;
		float t;
		int tri = FindIntersect_Ray(r, bary, t);

		if (res)
		{
			assert (tri != -1);
			assert (t <= dist);
		}
		else
		{
			if (tri != -1)
			{
				assert (t > dist);
			}
		}
*/
		return res;
	}
	// no distance, no intersection
	return false;
}

bool LightScene::test_intersect_ray(const Ray &ray, float max_dist, bool bCullBackFaces) {
	Vec3i voxel_range;
	_tracer.get_distance_in_voxels(max_dist, voxel_range);
	return test_intersect_ray(ray, max_dist, voxel_range, bCullBackFaces);
}

bool LightScene::test_intersect_ray(const Ray &ray, float max_dist, const Vec3i &voxel_range, bool bCullBackFaces) {
	Ray voxel_ray;
	Vec3i ptVoxel;

	// prepare voxel trace
	if (!_tracer.ray_trace_start(ray, voxel_ray, ptVoxel))
		return false;

	//bool bFirstHit = false;
	//	Vec3i ptVoxelFirstHit;

	// if we have specified a (optional) maximum range for the trace in voxels
	Vec3i ptVoxelStart = ptVoxel;

	// create the packed ray as a once off and reuse it for each voxel
	PackedRay pray;
	pray.Create(ray);

	while (true) {
		//Vec3i ptVoxelBefore = ptVoxel;

		const Voxel *pVoxel = _tracer.ray_trace(voxel_ray, voxel_ray, ptVoxel);
		if (!pVoxel)
			break;

		if (test_voxel_hits(ray, pray, *pVoxel, max_dist, bCullBackFaces))
			return true;

		// check for voxel range
		if (abs(ptVoxel.x - ptVoxelStart.x) > voxel_range.x)
			break;
		if (abs(ptVoxel.y - ptVoxelStart.y) > voxel_range.y)
			break;
		if (abs(ptVoxel.z - ptVoxelStart.z) > voxel_range.z)
			break;

	} // while

	return false;
}

int LightScene::find_intersect_ray(const Ray &ray, float &u, float &v, float &w, float &nearest_t, const Vec3i *pVoxelRange) //, int ignore_tri_p1)
{
	nearest_t = FLT_MAX;
	int nearest_tri = -1;

	Ray voxel_ray;
	Vec3i ptVoxel;

	// prepare voxel trace
	if (!_tracer.ray_trace_start(ray, voxel_ray, ptVoxel))
		return nearest_tri;

	bool bFirstHit = false;
	Vec3i ptVoxelFirstHit;

	// if we have specified a (optional) maximum range for the trace in voxels
	Vec3i ptVoxelStart = ptVoxel;

	// create the packed ray as a once off and reuse it for each voxel
	PackedRay pray;
	pray.Create(ray);

	// keep track of when we need to expand the bounds of the trace
	int nearest_tri_so_far = -1;
	int square_length_from_start_to_terminate = INT_MAX;

	while (true) {
		//		Vec3i ptVoxelBefore = ptVoxel;

		const Voxel *pVoxel = _tracer.ray_trace(voxel_ray, voxel_ray, ptVoxel);
		//		if (!m_Tracer.RayTrace(voxel_ray, voxel_ray, ptVoxel))
		if (!pVoxel)
			break;

		process_voxel_hits(ray, pray, *pVoxel, nearest_t, nearest_tri);

		// count number of tests for stats
		//int nHits = m_Tracer.m_TriHits.size();
		//num_tests += nHits;

		// if there is a nearest hit, calculate the voxel in which the hit occurs.
		// if we have travelled more than 1 voxel more than this, no need to traverse further.
		if (nearest_tri != nearest_tri_so_far) {
			nearest_tri_so_far = nearest_tri;
			Vector3 ptNearestHit = ray.o + (ray.d * nearest_t);
			_tracer.find_nearest_voxel(ptNearestHit, ptVoxelFirstHit);
			bFirstHit = true;

			// length in voxels to nearest hit
			Vec3i voxel_diff = ptVoxelFirstHit;
			voxel_diff -= ptVoxelStart;
			float voxel_length_to_nearest_hit = voxel_diff.length();
			// add a bit
			voxel_length_to_nearest_hit += 2.0f;

			// square length
			voxel_length_to_nearest_hit *= voxel_length_to_nearest_hit;

			// plus 1 for rounding up.
			square_length_from_start_to_terminate = voxel_length_to_nearest_hit + 1;
		}

		// first hit?
		if (!bFirstHit) {
			// check for voxel range
			if (pVoxelRange) {
				if (abs(ptVoxel.x - ptVoxelStart.x) > pVoxelRange->x)
					break;
				if (abs(ptVoxel.y - ptVoxelStart.y) > pVoxelRange->y)
					break;
				if (abs(ptVoxel.z - ptVoxelStart.z) > pVoxelRange->z)
					break;
			} // if voxel range
		} else {
			// check the range to this voxel. Have we gone further than the terminate voxel distance?
			Vec3i voxel_diff = ptVoxel;
			voxel_diff -= ptVoxelStart;
			int sl = voxel_diff.square_length();
			if (sl >= square_length_from_start_to_terminate)
				break;
		}

		/*
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
		*/

#ifdef LIGHTTRACER_IGNORE_VOXELS
		break;
#endif

	} // while

	if (nearest_tri != -1) {
		ray.find_intersect(_tris[nearest_tri], nearest_t, u, v, w);
	}

	return nearest_tri;
}

void LightScene::reset() {
	_materials.reset();
	//	m_ptPositions.resize(0);
	//	m_ptNormals.resize(0);
	//m_UVs.resize(0);
	//m_Inds.resize(0);

	_uv_tris.clear(true);
	_tri_uv_aabbs.clear(true);
	_tri_pos_aabbs.clear(true);
	_tracer.reset();
	_tri_texel_size_world_space.clear(true);

	_tris.clear(true);
	_tri_normals.clear(true);
	_tris_edge_form.clear(true);
	_tri_planes.clear(true);

	_emission_tris.clear(true);
	_emission_pixels.clear(true);
	_emission_tri_bitfield.Create(0);

	_meshes.clear(true);
	//m_Tri_MeshIDs.clear(true);
	//m_Tri_SurfIDs.clear(true);
	_tri_lmaterial_ids.clear(true);
	_uv_tris_primary.clear(true);
}

void LightScene::find_meshes(Spatial *pNode) {
	// mesh instance?
	MeshInstance *pMI = Object::cast_to<MeshInstance>(pNode);
	if (pMI) {
		if (is_mesh_instance_suitable(*pMI)) {
			_meshes.push_back(pMI);
		}
	}

	for (int n = 0; n < pNode->get_child_count(); n++) {
		Spatial *pChild = Object::cast_to<Spatial>(pNode->get_child(n));
		if (pChild) {
			find_meshes(pChild);
		}
	}
}

bool LightScene::create_from_mesh_surface(int mesh_id, int surf_id, Ref<Mesh> rmesh, int width, int height) {
	const MeshInstance &mi = *_meshes[mesh_id];

	if (rmesh->surface_get_primitive_type(surf_id) != Mesh::PRIMITIVE_TRIANGLES)
		return false; //only triangles

	Array arrays = rmesh->surface_get_arrays(surf_id);
	if (!arrays.size())
		return false;

	PoolVector<Vector3> verts = arrays[VS::ARRAY_VERTEX];
	if (!verts.size())
		return false;
	PoolVector<Vector3> norms = arrays[VS::ARRAY_NORMAL];
	if (!norms.size())
		return false;

	// uvs for lightmapping
	PoolVector<Vector2> uvs = arrays[VS::ARRAY_TEX_UV2];

	// optional uvs for albedo etc
	PoolVector<Vector2> uvs_primary;
	if (!uvs.size()) {
		uvs = arrays[VS::ARRAY_TEX_UV];
	} else {
		uvs_primary = arrays[VS::ARRAY_TEX_UV];
	}

	if (!uvs.size())
		return false;

	PoolVector<int> inds = arrays[VS::ARRAY_INDEX];
	if (!inds.size())
		return false;

	// we need to get the vert positions and normals from local space to world space to match up with the
	// world space coords in the merged mesh
	Transform trans = mi.get_global_transform();

	PoolVector<Vector3> positions_world;
	PoolVector<Vector3> normals_world;
	transform_verts(verts, positions_world, trans);
	transform_norms(norms, normals_world, trans);

	// convert to longhand non indexed versions
	int nTris = inds.size() / 3;
	int num_old_tris = _tris.size();
	int num_new_tris = num_old_tris + nTris;

	_tris.resize(num_new_tris);
	_tri_normals.resize(num_new_tris);
	_tris_edge_form.resize(num_new_tris);
	_tri_planes.resize(num_new_tris);

	_uv_tris.resize(num_new_tris);
	_tri_uv_aabbs.resize(num_new_tris);
	_tri_pos_aabbs.resize(num_new_tris);
	_tri_texel_size_world_space.resize(num_new_tris);

	//m_Tri_MeshIDs.resize(nNewTris);
	//m_Tri_SurfIDs.resize(nNewTris);
	_tri_lmaterial_ids.resize(num_new_tris);
	_uv_tris_primary.resize(num_new_tris);

	// lmaterial
	int lmat_id = _materials.find_or_create_material(mi, rmesh, surf_id);

	// emission tri?
	bool bEmit = false;
	if (lmat_id) {
		bEmit = _materials.get_material(lmat_id - 1).is_emitter;
	}

	int num_bad_normals = 0;

	int i = 0;
	for (int n = 0; n < nTris; n++) {
		// adjusted n
		int an = n + num_old_tris;

		Tri &t = _tris[an];
		Tri &tri_norm = _tri_normals[an];
		Tri &tri_edge = _tris_edge_form[an];
		Plane &tri_plane = _tri_planes[an];
		UVTri &uvt = _uv_tris[an];
		Rect2 &uv_rect = _tri_uv_aabbs[an];
		AABB &aabb = _tri_pos_aabbs[an];

		//		m_Tri_MeshIDs[an] = mesh_id;
		//		m_Tri_SurfIDs[an] = surf_id;
		_tri_lmaterial_ids[an] = lmat_id;
		UVTri &uvt_primary = _uv_tris_primary[an];

		int ind = inds[i];
		uv_rect = Rect2(uvs[ind], Vector2(0, 0));
		aabb.position = positions_world[ind];
		aabb.size = Vector3(0, 0, 0);

		for (int c = 0; c < 3; c++) {
			ind = inds[i++];

			t.pos[c] = positions_world[ind];
			tri_norm.pos[c] = normals_world[ind];
			uvt.uv[c] = uvs[ind];
			//rect = Rect2(uvt.uv[0], Vector2(0, 0));
			uv_rect.expand_to(uvt.uv[c]);
			//aabb.position = t.pos[0];
			aabb.expand_to(t.pos[c]);

			// store primary uvs if present
			if (uvs_primary.size()) {
				uvt_primary.uv[c] = uvs_primary[ind];
			} else {
				uvt_primary.uv[c] = Vector2(0, 0);
			}
		}

		// plane - calculate normal BEFORE changing winding into UV space
		// because the normal is determined by the winding in world space
		tri_plane = Plane(t.pos[0], t.pos[1], t.pos[2], CLOCKWISE);

		// sanity check for bad normals
		Vector3 average_normal = (tri_norm.pos[0] + tri_norm.pos[1] + tri_norm.pos[2]) * (1.0f / 3.0f);
		if (average_normal.dot(tri_plane.normal) < 0.0f) {
			num_bad_normals++;

			// flip the face normal
			tri_plane = -tri_plane;
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

		// ALWAYS DO THE UV WINDING LAST!!
		// make sure winding is standard in UV space
		if (uvt.is_winding_CW()) {
			uvt.flip_winding();
			t.flip_winding();
			tri_norm.flip_winding();
		}

		if (bEmit) {
			EmissionTri et;
			et.tri_id = an;
			et.area = t.calculate_area();
			_emission_tris.push_back(et);
		}

#ifdef LLIGHTSCENE_VERBOSE
		String sz;
		sz = "found triangle : ";
		for (int s = 0; s < 3; s++) {
			sz += "(" + String(t.pos[s]) + ") ... ";
		}
		print_line(sz);
		sz = "\tnormal : ";
		for (int s = 0; s < 3; s++) {
			sz += "(" + String(tri_norm.pos[s]) + ") ... ";
		}
		print_line(sz);

		sz = "\t\tUV : ";
		for (int s = 0; s < 3; s++) {
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
		// FIXME: This line was causing a bug where nothing was showing,
		// when it was active.
		//uv_rect = uv_rect.expand(Vector2(0.01, 0.01));

		calculate_tri_texel_size(an, width, height);
	}

	if (num_bad_normals) {
		print_line("mesh " + itos(mesh_id) + " contains " + itos(num_bad_normals) + " bad normals (face normal and vertex normals are opposite)");
	}

	return true;
}

bool LightScene::create_from_mesh(int mesh_id, int width, int height) {
	const MeshInstance &mi = *_meshes[mesh_id];

	Ref<Mesh> rmesh = mi.get_mesh();

	int num_surfaces = rmesh->get_surface_count();

	for (int surf = 0; surf < num_surfaces; surf++) {
		if (!create_from_mesh_surface(mesh_id, surf, rmesh, width, height)) {
			String sz;
			sz = "Mesh " + itos(mesh_id) + " surf " + itos(surf) + " cannot be converted.";
			WARN_PRINT(sz);
		}
	}

	return true;
}

bool LightScene::create(Spatial *pMeshesRoot, int width, int height, int voxel_density, int max_material_size, float emission_density) {
	_materials.prepare(max_material_size);

	_use_SIMD = true;

	find_meshes(pMeshesRoot);
	if (!_meshes.size())
		return false;

	for (int n = 0; n < _meshes.size(); n++) {
		if (!create_from_mesh(n, width, height))
			return false;
	}

	_tracer.create(*this, voxel_density);

	// By this point all the tris should be created,
	// so create bitfield as one off.
	_emission_tri_bitfield.Create(_tris.size(), true);
	for (int n = 0; n < _emission_tris.size(); n++) {
		uint32_t emission_tri_id = _emission_tris[n].tri_id;
		_emission_tri_bitfield.SetBit(emission_tri_id, 1);
	}

	// adjust material emission power to take account of sample density,
	// to keep brightness the same
	_materials.adjust_materials(emission_density);

	return true;
}

// note this is assuming 1:1 aspect ratio lightmaps. This could do x and y size separately,
// but more complex.
void LightScene::calculate_tri_texel_size(int tri_id, int width, int height) {
	const Tri &tri = _tris[tri_id];
	const UVTri &uvtri = _uv_tris[tri_id];

	// length of edges in world space
	float l0 = (tri.pos[1] - tri.pos[0]).length();
	float l1 = (tri.pos[2] - tri.pos[0]).length();

	// texel edge lengths
	Vector2 te0 = uvtri.uv[1] - uvtri.uv[0];
	Vector2 te1 = uvtri.uv[2] - uvtri.uv[0];

	// convert texel edges from uvs to texels
	te0.x *= width;
	te0.y *= height;
	te1.x *= width;
	te1.y *= height;

	// texel edge lengths
	float tl0 = te0.length();
	float tl1 = te1.length();

	// default
	float texel_size = 1.0f;

	if (tl0 >= tl1) {
		// check for divide by zero
		if (tl0 > 0.00001f)
			texel_size = l0 / tl0;
	} else {
		if (tl1 > 0.00001f)
			texel_size = l1 / tl1;
	}

	_tri_texel_size_world_space[tri_id] = texel_size;
}

bool LightScene::find_emission_color(int tri_id, const Vector3 &bary, Color &texture_col, Color &col) {
	Vector2 uvs;
	_uv_tris_primary[tri_id].find_uv_barycentric(uvs, bary.x, bary.y, bary.z);

	//	int mat_id_p1 = _tri_lmaterial_ids[tri_id];

	ColorSample cols;
	if (!take_triangle_color_sample(tri_id, bary, cols)) {
		return false;
	}

	if (!cols.is_emitter) {
		return false;
	}

	texture_col = cols.albedo;
	col = cols.emission;
	texture_col *= col;
	return true;

	/*
		// should never happen?
		if (!mat_id_p1) {
			texture_col = Color(0, 0, 0, 0);
			col = Color(0, 0, 0, 0);
			return false;
		}

		const LMaterial &mat = _materials.get_material(mat_id_p1 - 1);
		if (!mat.is_emitter)
			return false;

		// albedo
		// return whether texture found
		bool bTransparent;
		Color texture_emission;
		bool res = _materials.find_colors(mat_id_p1, uvs, texture_col, texture_emission, bTransparent);

		texture_col *= mat.color_emission;
		//		power = mat.m_Power_Emission;

		col = mat.color_emission;
		return res;
		*/
}

void LightScene::thread_rasterize_triangle_ids(uint32_t p_tile, uint32_t *p_dummy) {
	RasterizeTriangleIDParams &rtip = _rasterize_triangle_id_params;

	//	if (rtip.base->bake_step_function) {
	//		bool cancel = rtip.base->bake_step_function(p_tile / (float)rtip.num_tiles_high, String("Process UVTris: ") + " (" + itos(p_tile) + ")");
	//	   			if (cancel) {
	//	   				if (base.bake_end_function) {
	//	   					base.bake_end_function();
	//	   				}
	//	   				return false;
	//	   			}
	//	}

	int width = rtip.im_p1->get_width();
	int height = rtip.im_p1->get_height();

	int height_min = p_tile * rtip.tile_height;
	int height_max = height_min + rtip.tile_height;

	if (height_min >= height)
		return;

	height_min = CLAMP(height_min, 0, height);
	height_max = CLAMP(height_max, 0, height);

	for (int n = 0; n < _uv_tris.size(); n++) {
		//		if (base.bake_step_function && ((n % 4096) == 0)) {
		//			bool cancel = base.bake_step_function(n / (float)m_UVTris.size(), String("Process UVTris: ") + " (" + itos(n) + ")");
		//			if (cancel) {
		//				if (base.bake_end_function) {
		//					base.bake_end_function();
		//				}
		//				return false;
		//			}
		//		}

		//const Rect2 &aabb = m_TriUVaabbs[n];
		const UVTri &tri = _uv_tris[n];
		if (tri.is_degenerate())
			continue;

		bool is_emission_tri = _emission_tri_bitfield.GetBit(n);

		const Rect2i &bound = _tri_uv_bounds[n];

		// clamp
		int min_x = bound.min_x;
		int min_y = CLAMP(bound.min_y, height_min, height_max);
		int max_x = bound.max_x;
		int max_y = CLAMP(bound.max_y, height_min, height_max);

//#define LLIGHTMAP_DEBUG_RASTERIZE_OVERLAP
#ifdef LLIGHTMAP_DEBUG_RASTERIZE_OVERLAP
		int debug_overlap_count = 0;
#endif
		int pixel_count = (max_y - min_y) * (max_x - min_x);
		if (is_emission_tri && pixel_count) {
			_emission_pixels_mutex.lock();
		}

		for (int y = min_y; y < max_y; y++) {
			for (int x = min_x; x < max_x; x++) {
				float s = (x + 0.5f) / (float)width;
				float t = (y + 0.5f) / (float)height;

				//				if ((x == 26) && (y == 25))
				//				{
				//					print_line("testing");
				//				}

				if (tri.contains_texel(x, y, width, height)) {
					if (rtip.base->logic.rasterize_mini_lists)
						rtip.temp_image_tris.get_item(x, y).push_back(n);
					if (tri.contains_point(Vector2(s, t))) {
						uint32_t &id_p1 = rtip.im_p1->get_item(x, y);

#ifdef LLIGHTMAP_DEBUG_RASTERIZE_OVERLAP
						// hopefully this was 0 before
						if (id_p1) {
							debug_overlap_count++;
							//						if (debug_overlap_count == 64)
							//						{
							//							print_line("overlap detected");
							//						}

							// store the overlapped ID in a second map
							//im2_p1.GetItem(x, y) = id_p1;
						}
#endif

						// save new id
						id_p1 = n + 1;

						// find barycentric coords
						float u, v, w;

						// note this returns NAN for degenerate triangles!
						tri.find_barycentric_coords(Vector2(s, t), u, v, w);

						//					assert (!isnan(u));
						//					assert (!isnan(v));
						//					assert (!isnan(w));

						Vector3 &bary = rtip.im_bary->get_item(x, y);
						bary = Vector3(u, v, w);

						if (is_emission_tri) {
							Color &em_col = _image_emission_done.get_item(x, y);

							if (_image_emission_done.get_item(x, y).a < 2) {
								_emission_pixels.push_back(Vec2_i16(x, y));
								em_col.a = 4;
							}

							// store the highest emission color
							ColorSample cols;
							take_triangle_color_sample(n, bary, cols);

							em_col.r = MAX(em_col.r, cols.emission.r);
							em_col.g = MAX(em_col.g, cols.emission.g);
							em_col.b = MAX(em_col.b, cols.emission.b);
						}
					} // contains point
				} // contains texel

			} // for x
		} // for y

		if (is_emission_tri && pixel_count) {
			_emission_pixels_mutex.unlock();
		}

	} // for tri

	/*
	if (base.logic.Process_AO) {
		// translate temporary image vectors into mini lists
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				MiniList &ml = base.m_Image_TriIDs.GetItem(x, y);
				ml.first = base.m_TriIDs.size();

				const LocalVector<uint32_t> &vec = temp_image_tris.GetItem(x, y);

				for (int n = 0; n < vec.size(); n++) {
					base.m_TriIDs.push_back(vec[n]);
					ml.num += 1;
				}

					   //				if (!ml.num)
					   //				{
					   //					ml.first = base.m_TriIDs.size();
					   //				}
					   //				BUG IS THESE ARE NOT CONTIGUOUS
					   //				ml.num += 1;
					   //				base.m_TriIDs.push_back(n);
			} // for x
		} // for y

	} // only if processing AO
	*/

	//	if (base.bake_end_function) {
	//		base.bake_end_function();
	//	}
}

bool LightScene::rasterize_triangles_ids(LightMapper_Base &base, LightImage<uint32_t> &im_p1, LightImage<Vector3> &im_bary) {
	int width = im_p1.get_width();
	int height = im_p1.get_height();

	// create a temporary image of vectors to store the triangles per texel
	if (base.logic.rasterize_mini_lists)
		_rasterize_triangle_id_params.temp_image_tris.create(width, height, false);

	// First find tri min maxes in texture space.
	_tri_uv_bounds.resize(_uv_tris.size());

	for (int n = 0; n < _uv_tris.size(); n++) {
		const Rect2 &aabb = _tri_uv_aabbs[n];

		int min_x = aabb.position.x * width;
		int min_y = aabb.position.y * height;
		int max_x = (aabb.position.x + aabb.size.x) * width;
		int max_y = (aabb.position.y + aabb.size.y) * height;

		// add a bit for luck
		min_x--;
		min_y--;
		max_x++;
		max_y++;

		// clamp
		min_x = CLAMP(min_x, 0, width);
		min_y = CLAMP(min_y, 0, height);
		max_x = CLAMP(max_x, 0, width);
		max_y = CLAMP(max_y, 0, height);

		Rect2i &bound = _tri_uv_bounds[n];
		bound.min_x = min_x;
		bound.min_y = min_y;
		bound.max_x = max_x;
		bound.max_y = max_y;
	}

	_rasterize_triangle_id_params.base = &base;
	_rasterize_triangle_id_params.im_bary = &im_bary;
	_rasterize_triangle_id_params.im_p1 = &im_p1;

//#if 1
#if (defined NO_THREADS) || (!defined LLIGHTMAP_MULTITHREADED)
	_rasterize_triangle_id_params.tile_height = height;
	_rasterize_triangle_id_params.num_tiles_high = 1;

	LightScene::thread_rasterize_triangle_ids(0, nullptr);
#else

	ThreadWorkPool thread_pool;
	int thread_cores = OS::get_singleton()->get_default_thread_pool_size();

	_rasterize_triangle_id_params.tile_height = (height / thread_cores) + 1;
	_rasterize_triangle_id_params.num_tiles_high = (height / _rasterize_triangle_id_params.tile_height) + 1;

	thread_pool.init(thread_cores);
	thread_pool.do_work(
			_rasterize_triangle_id_params.num_tiles_high,
			this,
			&LightScene::thread_rasterize_triangle_ids,
			nullptr);
	thread_pool.finish();

#endif

	if (base.logic.rasterize_mini_lists) {
		uint32_t debug_valid_pixels = 0;
		uint32_t debug_overlapped_pixels = 0;

		// translate temporary image vectors into mini lists
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				MiniList &ml = base._image_tri_minilists.get_item(x, y);
				ml.first = base._minilist_tri_ids.size();
				ml.num = 0;

				const LocalVector<uint32_t> &vec = _rasterize_triangle_id_params.temp_image_tris.get_item(x, y);

				if (vec.size()) {
					debug_valid_pixels++;
					debug_overlapped_pixels += vec.size();

					// NEW 2024: Deal with no triangle ID being registered on this texel.
					// For instance, if an edge of a triangle glances the texel, but is not covering the centre.
#define LLIGHTMAP_SPREAD_AT_EDGES
#ifdef LLIGHTMAP_SPREAD_AT_EDGES
					if (im_p1.get_item(x, y) == 0) {
						im_p1.get_item(x, y) = vec[0] + 1;
					}
#endif
				}

				for (int n = 0; n < vec.size(); n++) {
					base._minilist_tri_ids.push_back(vec[n]);
					ml.num += 1;
				}

				//				if (!ml.num)
				//				{
				//					ml.first = base.m_TriIDs.size();
				//				}
				//				BUG IS THESE ARE NOT CONTIGUOUS
				//				ml.num += 1;
				//				base.m_TriIDs.push_back(n);
			} // for x
		} // for y

		print_line("Found " + itos(debug_valid_pixels) + " valid pixels, " + itos(debug_overlapped_pixels) + " overlapped pixels.");

	} // only if processing AO

	if (base.bake_end_function) {
		base.bake_end_function();
	}

	_rasterize_triangle_id_params.temp_image_tris.reset();

	print_line("\tnum emission tris : " + itos(_emission_tris.size()));
	print_line("\tnum emission pixels : " + itos(_emission_pixels.size()));

	//base.debug_save(im_p1, "imp1.png");

	return true;
}

void LightScene::transform_verts(const PoolVector<Vector3> &ptsLocal, PoolVector<Vector3> &ptsWorld, const Transform &tr) const {
	for (int n = 0; n < ptsLocal.size(); n++) {
		Vector3 ptWorld = tr.xform(ptsLocal[n]);
		ptsWorld.push_back(ptWorld);
	}
}

void LightScene::transform_norms(const PoolVector<Vector3> &normsLocal, PoolVector<Vector3> &normsWorld, const Transform &tr) const {
	int invalid_normals = 0;

	for (int n = 0; n < normsLocal.size(); n++) {
		// hacky way for normals, we should use transpose of inverse matrix, dunno if godot supports this
		Vector3 ptNormA = Vector3(0, 0, 0);
		Vector3 ptNormWorldA = tr.xform(ptNormA);
		Vector3 ptNormWorldB = tr.xform(normsLocal[n]);

		Vector3 ptNorm = ptNormWorldB - ptNormWorldA;

		ptNorm = ptNorm.normalized();

		if (ptNorm.length_squared() < 0.9f)
			invalid_normals++;

		normsWorld.push_back(ptNorm);
	}

	if (invalid_normals) {
		WARN_PRINT("Invalid normals detected : " + itos(invalid_normals));
	}
}
