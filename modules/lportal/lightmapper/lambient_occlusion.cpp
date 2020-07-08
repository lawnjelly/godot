#include "lambient_occlusion.h"

namespace LM {

void AmbientOcclusion::ProcessAO_Texel(int tx, int ty, int qmc_variation)
{
	if ((tx == 41) && (ty == 44))
		print_line("test");

	const MiniList &ml = m_Image_TriIDs.GetItem(tx, ty);
	if (!ml.num)
		return; // no triangles in this UV

	// could be off the image
	float * pfTexel = m_Image_AO.Get(tx, ty);

	float power;

	const MiniList_Cuts &ml_cuts = m_Image_Cuts.GetItem(tx, ty);
	if (!ml_cuts.num)
	{
		power = CalculateAO(tx, ty, qmc_variation, ml);
	}
	else
	{
		power = CalculateAO_Complex(tx, ty, qmc_variation, ml);
	}



	*pfTexel = power;
}

void AmbientOcclusion::ProcessAO_Triangle(int tri_id)
{
//	int width = m_iWidth;
//	int height = m_iHeight;

//	const Rect2 &aabb = m_Scene.m_TriUVaabbs[tri_id];
	const UVTri &tri = m_Scene.m_UVTris[tri_id];

//	int min_x = aabb.position.x * width;
//	int min_y = aabb.position.y * height;
//	int max_x = (aabb.position.x + aabb.size.x) * width;
//	int max_y = (aabb.position.y + aabb.size.y) * height;

//	// add a bit for luck
//	min_x--; min_y--; max_x++; max_y++;

//	// clamp
//	min_x = CLAMP(min_x, 0, width);
//	min_y = CLAMP(min_y, 0, height);
//	max_x = CLAMP(max_x, 0, width);
//	max_y = CLAMP(max_y, 0, height);

	// calculate the area of the triangle
	float area = fabsf(tri.CalculateTwiceArea());

	int nSamples = area * 1000000.0f * m_Settings_AO_Samples;
	Vector3 bary;

	for (int n=0; n<nSamples; n++)
	{
		RandomBarycentric(bary);
		ProcessAO_Sample(bary, tri_id, tri);
	}
}

void AmbientOcclusion::ProcessAO_Sample(const Vector3 &bary, int tri_id, const UVTri &uvtri)
{
	float range = m_Settings_AO_Range;

	// get the texel (this may need adjustment)
	Vector2 uv;
	uvtri.FindUVBarycentric(uv, bary);
	int tx = uv.x * m_iWidth;
	int ty = uv.y * m_iHeight;

	// outside? (should not happen, but just in case)
	if (!m_Image_L.IsWithin(tx, ty))
		return;

	// find position
	const Tri &tri_pos = m_Scene.m_Tris[tri_id];
	Vector3 pos;
	tri_pos.InterpolateBarycentric(pos, bary);

	// find normal
	const Tri &tri_norm = m_Scene.m_TriNormals[tri_id];
	Vector3 norm;
	tri_norm.InterpolateBarycentric(norm, bary);
	norm.normalize();

	Vector3 push = norm * 0.005f;

	Ray r;
	r.o = pos + push;
//	r.d = norm;

	//////

			RandomUnitDir(r.d);
	//		m_QMC.QMCRandomUnitDir(r.d, n);
			//m_QMC.QMCRandomUnitDir(r.d, nSamplesCounted-1);

			// clip?
			float dot = r.d.dot(norm);
			if (dot < 0.0f)
			{
				// make dot always positive for calculations as to the weight given to this hit
				//dot = -dot;
				r.d = -r.d;
			}

			// prevent parallel lines
			r.d += push;
		//	r.d = ptNormal;

			// collision detect
			r.d.normalize();

	/////



	float t, u, v, w;
	m_Scene.m_Tracer.m_bUseSDF = true;
	int tri_hit = m_Scene.FindIntersect_Ray(r, u, v, w, t, &m_Scene.m_VoxelRange, m_iNumTests);

	if (tri_hit != -1)
	{
		// scale the occlusion by distance t
		t = range - t;
		if (t > 0.0f)
		{
			//t *= dot;

//				t /= range;

			//fTotal += t;
//			if (t > fTotal)
//				fTotal = t;

//			nHits++;

			float * pfTexel = m_Image_AO.Get(tx, ty);
			*pfTexel += 1.0f / 64.0f;
		}
	}
}

void AmbientOcclusion::ProcessAO()
{
	if (bake_begin_function) {
		bake_begin_function(m_iHeight);
	}

//	int qmc_variation = 0;

	// find the max range in voxels. This can be used to speed up the ray trace
	const float range = m_Settings_AO_Range;
	m_Scene.m_Tracer.GetDistanceInVoxels(range, m_Scene.m_VoxelRange);

//	for (int t=0; t<m_Scene.m_Tris.size(); t++)
//	{
//		ProcessAO_Triangle(t);
//	}

//	Normalize_AO();

//#define LAMBIENT_OCCLUSION_USE_THREADS
#ifdef LAMBIENT_OCCLUSION_USE_THREADS
//	int nCores = OS::get_singleton()->get_processor_count();
//	int nSections = m_iHeight / (nCores * 2);
	int nSections = m_iHeight / 64;
	int y_section_size = m_iHeight / nSections;
	int leftover_start = y_section_size * nSections;

	for (int s=0; s<nSections; s++)
	{
		int y_section_start = s * y_section_size;

		if (bake_step_function) {
			m_bCancel = bake_step_function(y_section_start, String("Process Texels: ") + " (" + itos(y_section_start) + ")");
			if (m_bCancel)
			{
				if (bake_end_function) {
					bake_end_function();
				}
				return;
			}
		}

		thread_process_array(y_section_size, this, &AmbientOcclusion::ProcessAO_LineMT, y_section_start);
	}

	// leftovers
	int nLeftover = m_iHeight - leftover_start;

	if (nLeftover)
		thread_process_array(nLeftover, this, &AmbientOcclusion::ProcessAO_LineMT, leftover_start);
#else

	for (int y=0; y<m_iHeight; y++)
	{
		if ((y % 10) == 0)
		{
			if (bake_step_function) {
				m_bCancel = bake_step_function(y, String("Process Texels: ") + " (" + itos(y) + ")");
				if (m_bCancel)
				{
					if (bake_end_function) {
						bake_end_function();
					}
					return;
				}
			}
		}

		ProcessAO_LineMT(0, y);

//		for (int x=0; x<m_iWidth; x++)
//		{
//			qmc_variation = m_QMC.GetNextVariation(qmc_variation);
//			ProcessAO_Texel(x, y, qmc_variation);
//		}
	}
#endif

	if (bake_end_function) {
		bake_end_function();
	}
}

void AmbientOcclusion::ProcessAO_LineMT(uint32_t y_offset, int y_section_start)
{
	int ty = y_section_start + y_offset;

	// seed based on the line
	int qmc_variation = m_QMC.RandomVariation();

	for (int x=0; x<m_iWidth; x++)
	{
		qmc_variation = m_QMC.GetNextVariation(qmc_variation);
		ProcessAO_Texel(x, ty, qmc_variation);
	}
}

bool AmbientOcclusion::ProcessAO_InFrontCuts(const MiniList_Cuts &ml, const Vector2 &st, int main_tri_id_p1)
{
	if (!ml.num)
		return true;

	if (!main_tri_id_p1)
		return true;

	int tri_id = main_tri_id_p1-1;

	// calculate sample position in world space.
	Vector3 bary;
	const UVTri * pUVTri = &m_Scene.m_UVTris[tri_id];
	pUVTri->FindBarycentricCoords(st, bary);

	// this bary could be outside the triangle, but hey ho, this main triangle is
	// where the cuts are based on.
	const Tri &main_tri = m_Scene.m_Tris[tri_id];
	Vector3 ptSampleWorld;
	main_tri.InterpolateBarycentric(ptSampleWorld, bary);


	int infront = 0;

	for (int c=0; c<ml.num; c++)
	{
		int cut_tri_id = m_CuttingTris[ml.first + c];

		// must be in front of any cutting triangles
		// (assuming we don't have any super thin walls)
		const Plane &cut_plane = m_Scene.m_TriPlanes[cut_tri_id];

		float dist = cut_plane.distance_to(ptSampleWorld);
//		if (cut_plane.distance_to(ptSampleWorld) <= 0.00001f)
		if (dist >= 0.00001f)
			infront++;
	}

	/*
	if (ml.num == 2)
	{
		if (ml.convex)
		{
			// convex, must be ahead of 1 or more
			if (infront >= 1)
				return true;
		}
		else
		{
			// concave, must be behind both (assuming 2)
			// not handled otherwise.
			if (infront == 2)
				return true;
		}

		return false;
	}
	*/

	if (infront == ml.num)
		return true;

	return false;
}


float AmbientOcclusion::CalculateAO(int tx, int ty, int qmc_variation, const MiniList &ml)
{
	Ray r;
	int nSamples = m_Settings_AO_Samples;

	// find the max range in voxels. This can be used to speed up the ray trace
	Vec3i voxel_range = m_Scene.m_VoxelRange;

	int nHits = 0;
	int nSamplesInside = 0;

	for (int n=0; n<nSamples; n++)
	{
		// pick a float position within the texel
		Vector2 st;
		AO_RandomTexelSample(st, tx, ty, n);

		// find which triangle on the minilist we are inside (if any)
		uint32_t tri_inside_id;
		Vector3 bary;
		if (!AO_FindTexelTriangle(ml, st, tri_inside_id, bary))
			continue;

		nSamplesInside++;

		// calculate world position ray origin from barycentric
		m_Scene.m_Tris[tri_inside_id].InterpolateBarycentric(r.o, bary);

		// calculate surface normal (should be use plane?)
		Vector3 ptNormal;
		m_Scene.m_TriNormals[tri_inside_id].InterpolateBarycentric(ptNormal, bary);
		//const Vector3 &ptNormal = m_Scene.m_TriPlanes[tri_inside_id].normal;

		// construct a random ray to test
		AO_RandomQMCRay(r, ptNormal, n, qmc_variation);

		// test ray
		if (m_Scene.TestIntersect_Ray(r, m_Settings_AO_Range, voxel_range))
		{
			nHits++;
		}

	} // for samples

	float fTotal = (float) nHits / nSamplesInside;
	fTotal = 1.0f - (fTotal * 1.0f);

	if (fTotal < 0.0f)
		fTotal = 0.0f;

	return fTotal;
}


float AmbientOcclusion::CalculateAO_Complex(int tx, int ty, int qmc_variation, const MiniList &ml)
{
	// first we need to identify some sample points
	AOSample samples[MAX_COMPLEX_AO_TEXEL_SAMPLES];
	int nSampleLocs = AO_FindSamplePoints(tx, ty, ml, samples);

	// if no good samples locs found
	if (!nSampleLocs)
		return 1.0f; // could use an intermediate value for texture filtering to look better?

	int sample_counter = 0;
	int nHits = 0;
	Ray r;

	for (int n=0; n<m_Settings_AO_Samples; n++)
	{
		// get the sample to look from
		const AOSample &sample = samples[sample_counter++];

		// wraparound
		if (sample_counter == nSampleLocs)
			sample_counter = 0;

		r.o = sample.pos;

		// construct a random ray to test
		AO_RandomQMCDirection(r.d, sample.normal, n, qmc_variation);

		// test ray
		if (m_Scene.TestIntersect_Ray(r, m_Settings_AO_Range, m_Scene.m_VoxelRange))
		{
			nHits++;
		}
	} // for n

	float fTotal = (float) nHits / m_Settings_AO_Samples;
	fTotal = 1.0f - (fTotal * 1.0f);

	if (fTotal < 0.0f)
		fTotal = 0.0f;

	return fTotal;
}


int AmbientOcclusion::AO_FindSamplePoints(int tx, int ty, const MiniList &ml, AOSample samples[MAX_COMPLEX_AO_TEXEL_SAMPLES])
{
	int samples_found = 0;
	int attempts = m_Settings_AO_Samples + 64;

	Ray r;

	for (int n=0; n<attempts; n++)
	{
		Vector2 st;
		AO_RandomTexelSample(st, tx, ty, 10);

		// find which triangle on the minilist we are inside (if any)
		uint32_t tri_inside_id;
		Vector3 bary;

		if (!AO_FindTexelTriangle(ml, st, tri_inside_id, bary))
			continue; // not inside a triangle, failed this attempt

		// calculate world position ray origin from barycentric
		Vector3 pos;
		m_Scene.m_Tris[tri_inside_id].InterpolateBarycentric(pos, bary);
		r.o = pos;

		// calculate surface normal (should be use plane?)
		const Vector3 &ptNormal = m_Scene.m_TriPlanes[tri_inside_id].normal;

		// construct a random ray to test
		AO_RandomRay(r, ptNormal);

		// test ray
		if (m_Scene.TestIntersect_Ray(r, m_Settings_AO_Range, m_Scene.m_VoxelRange, true))
			continue; // hit, not interested

		// no hit .. we can use this sample!
		AOSample &sample = samples[samples_found++];

		// be super careful with this offset.
		// the find test offsets BACKWARDS to find tris on the floor
		// the actual ambient test offsets FORWARDS to avoid self intersection.
		// the ambient test could also use the backface culling test, but this is slower.
		sample.pos = pos + (ptNormal * 0.005f);
		sample.uv = st;
		sample.normal = ptNormal;
		sample.tri_id = tri_inside_id;

		// finished?
		if (samples_found == MAX_COMPLEX_AO_TEXEL_SAMPLES)
			return samples_found;
	}

	return samples_found;
}



float AmbientOcclusion::CalculateAO_Old(int tx, int ty, int qmc_variation)//, uint32_t tri0, uint32_t tri1_p1)
{
//	if ((tx == 22) && (ty == 2))
//	{
//		print_line("test");
//	}


	Ray r;
	int nSamples = m_Settings_AO_Samples;

	// total occlusion
	float fTotal = 1.0f;

//	const float range = m_Settings_AO_Range;

	// find the max range in voxels. This can be used to speed up the ray trace
	Vec3i voxel_range = m_Scene.m_VoxelRange;
//	m_Scene.m_Tracer.GetDistanceInVoxels(range, voxel_range);

	// preget the minilist of all triangles affecting this texel
	const MiniList &ml = m_Image_TriIDs.GetItem(tx, ty);
	if (!ml.num)
		return 0.0f; // nothing to do, no tris on this texel

	const MiniList_Cuts &ml_cuts = m_Image_Cuts.GetItem(tx, ty);

	// debug
//	if (!ml_cuts.num)
//		return 0.0f;

	// main triangle on this texel
	int main_tri_id_p1 = m_Image_ID_p1.GetItem(tx, ty);


	int nHits = 0;

	// each ray
	int nSamplesCounted = 0;
	int nDebugCutPassed = 0;
//	int nAttempts = nSamples * 10;
	for (int n=0; n<nSamples; n++)
	{

		// anti aliasing.
		// 1 pick a float position within the texel
		Vector2 st;
		if (n)
		{
			st.x = Math::randf() + tx;
			st.y = Math::randf() + ty;
		}
		else
		{
			// fix first to centre of texel
			st.x = 0.5f + tx;
			st.y = 0.5f + ty;
		}

		// has to be ranged 0 to 1
		st.x /= m_iWidth;
		st.y /= m_iHeight;

		// check against cutting tris.
		if (!ProcessAO_InFrontCuts(ml_cuts, st, main_tri_id_p1))
		{
			continue;
		}
		nDebugCutPassed++;


		// barycentric coords.
		float u,v,w;

		// new minilist
		uint32_t tri_id;
		const UVTri * pUVTri;
		bool bInside = false;

		for (uint32_t i=0; i<ml.num; i++)
		{
			tri_id = m_TriIDs[ml.first + i];
			pUVTri = &m_Scene.m_UVTris[tri_id];

			// only allow 1 tri
//			if (tri_id != 0)
//				continue;

			// within?
			pUVTri->FindBarycentricCoords(st, u, v, w);
			if (BarycentricInside(u, v, w))
			{
				bInside = true;
				break;
			}
		}

		// try another sample
		if (!bInside)
		{
			// if not inside any, just use the nearest. ?

			//n--;
			continue;
		}

		// check against cutting tris.
//		if (ProcessAO_BehindCuts(ml_cuts, st, main_tri_id_p1))
//		{
//			continue;
//		}


		nSamplesCounted++;




		// calculate world position ray origin from barycentric
		m_Scene.m_Tris[tri_id].InterpolateBarycentric(r.o, u, v, w);

		Vector3 ptNormal;
		m_Scene.m_TriNormals[tri_id].InterpolateBarycentric(ptNormal, u, v, w);

		Vector3 push = ptNormal * 0.005f;

		// push ray origin
		r.o += push;

//		RandomUnitDir(r.d);
		m_QMC.QMCRandomUnitDir(r.d, n, qmc_variation);

		// clip?
		float dot = r.d.dot(ptNormal);
		if (dot < 0.0f)
		{
			// make dot always positive for calculations as to the weight given to this hit
			//dot = -dot;
			r.d = -r.d;
		}

		// prevent parallel lines
		r.d += push;
	//	r.d = ptNormal;

		// collision detect
		r.d.normalize();
		//float t;

		m_Scene.m_Tracer.m_bUseSDF = true;

		if (m_Scene.TestIntersect_Ray(r, m_Settings_AO_Range, voxel_range))
		{
			nHits++;
		}

/*
		int tri_hit = m_Scene.FindIntersect_Ray(r, u, v, w, t, &voxel_range, m_iNumTests);

		// nothing hit
		if (tri_hit != -1)
		{
			// scale the occlusion by distance t
			// t was dist squared
			//t = sqrt(t);
			t = range - t;
			if (t > 0.0f)
			{
				//t *= dot;
//				t /= range;
				//fTotal += t;
				if (t > fTotal)
					fTotal = t;

				nHits++;
			}
		}
			*/
	}

	// debug output number of samples counted
	//return (float) nDebugCutPassed / nSamples;


//	fTotal /= range;

//	if (nSamplesCounted > (nSamples / 2))
	if (nSamplesCounted > 1)
		fTotal = (float) nHits / nSamplesCounted;
	else
	{
		// none counted, mark this texel as not hit, and allow dilation to cover it.
		// basically this texel is right on the edge, and random samples within the texel weren't enough to hit
		// the uv triangle. We don't want to sample outside the triangle, because the position will be outside,
		// and we could get artifacts. So either we clamp samples to the uvtriangle, or we dilate.

		// we are banditing the p1, but temporary, this should be changed so as not to interfere with the lightmapping
		m_Image_ID_p1.GetItem(tx, ty) = 0;
		//print_line("none_counted");
	}

//	fTotal = (float) nHits / nSamples;


	// save in the texel
	// should be scaled between 0 and 1
//	fTotal /= nSamples * range;

	fTotal = 1.0f - (fTotal * 1.0f);

	if (fTotal < 0.0f)
		fTotal = 0.0f;

	return fTotal;
}


/*
		// find which triangle
		uint32_t tri = tri0; // default
		const UVTri * pUVTri = &m_Scene.m_UVTris[tri0];
		pUVTri->FindBarycentricCoords(st, u, v, w);

		// not in first, try second
		if (!BarycentricInside(u, v, w))
		{
			bool bInside = false;

			// try other tri
			if (tri1_p1)
			{
				tri = tri1_p1 - 1;
				pUVTri = &m_Scene.m_UVTris[tri];
				pUVTri->FindBarycentricCoords(st, u, v, w);

				if (BarycentricInside(u, v, w))
				{
					bInside = true;
				}
			}

			if (!bInside)
			{
				// try another sample
				n--;
				continue;
			}
		}
		*/

//		if (tri1_p1)
//		{
//			if (!pUVTri->ContainsPoint(st))
//			{
//				// assume it is in the other tri
//				tri = tri1_p1 - 1;
//				pUVTri = &m_Scene.m_UVTris[tri];
//			}
//		}


/*
float AmbientOcclusion::CalculateAO(const Vector3 &ptStart, const Vector3 &ptNormal, uint32_t tri0, uint32_t tri1_p1)
{
	Vector3 push = ptNormal * 0.005f;

	Ray r;
	r.o = ptStart + push;

	int nSamples = m_Settings_AO_Samples;

	// total occlusion
	float fTotal = 0.0f;

	const float range = m_Settings_AO_Range;

	// find the max range in voxels. This can be used to speed up the ray trace
	Vec3i voxel_range;
	m_Scene.m_Tracer.GetDistanceInVoxels(range, voxel_range);

	int nHits = 0;
	// each ray
	for (int n=0; n<nSamples; n++)
	{
		// anti aliasing.
		// 1 pick a float position within the texel
//		float fx = Math::randf() + ;
//		float fy = Math::randf();


		//RandomUnitDir(r.d);
		m_QMC.QMCRandomUnitDir(r.d, n);

		// clip?
		float dot = r.d.dot(ptNormal);
		if (dot < 0.0f)
		{
			// make dot always positive for calculations as to the weight given to this hit
			//dot = -dot;
			r.d = -r.d;
		}

		// prevent parallel lines
		r.d += push;
//		r.d = ptNormal;

		// collision detect
		r.d.normalize();
		float u, v, w, t;

		m_Scene.m_Tracer.m_bUseSDF = true;
		int tri = m_Scene.IntersectRay(r, u, v, w, t, &voxel_range, m_iNumTests);

		// nothing hit
//		if ((tri == -1) || (tri == (int) tri_ignore))
		if (tri == -1)
		{
//			// for backward tracing, first pass, this is a special case, because we DO
//			// take account of distance to the light, and normal, in order to simulate the effects
//			// of the likelihood of 'catching' a ray. In forward tracing this happens by magic.
//			float dist = (r.o - ptDest).length();
//			float local_power = power * InverseSquareDropoff(dist);

//			// take into account normal
//			float dot = r.d.dot(ptNormal);
//			dot = fabs(dot);

//			local_power *= dot;

//			fTotal += local_power;
		}
		else
		{
			// scale the occlusion by distance t

			// t was dist squared
			//t = sqrt(t);

			t = range - t;
			if (t > 0.0f)
			{
				//t *= dot;

//				t /= range;

				//fTotal += t;
				if (t > fTotal)
					fTotal = t;

				nHits++;
			}
		}
	}

	fTotal /= range;

	fTotal = (float) nHits / nSamples;

	// save in the texel
	// should be scaled between 0 and 1
//	fTotal /= nSamples * range;

	fTotal = 1.0f - (fTotal * 1.0f);

	if (fTotal < 0.0f)
		fTotal = 0.0f;

	return fTotal;
}
*/




} // namespace
