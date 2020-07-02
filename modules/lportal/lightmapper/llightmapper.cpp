#include "llightmapper.h"

//#include <omp.h>

namespace LM {

bool LightMapper::lightmap_mesh(MeshInstance * pMI, Spatial * pLR, Image * pIm)
{
	// get the output dimensions before starting, because we need this
	// to determine number of rays, and the progress range
	m_iWidth = pIm->get_width();
	m_iHeight = pIm->get_height();
	m_iNumRays = m_Settings_Forward_NumRays;

	int nTexels = m_iWidth * m_iHeight;
	int progress_range = m_iHeight;

	// set num rays depending on method
	if (m_Settings_Mode == LMMODE_FORWARD)
	{
		// the num rays / texel. This is per light!
		m_iNumRays *= nTexels;
		progress_range = m_iNumRays / m_iRaysPerSection;
	}

	if (bake_begin_function) {
		bake_begin_function(progress_range);
	}

	// do twice to test SIMD
	uint32_t beforeA = OS::get_singleton()->get_ticks_msec();
	m_Scene.m_bUseSIMD = true;
	m_Scene.m_Tracer.m_bSIMD = true;
	bool res = LightmapMesh(*pMI, *pLR, *pIm);
	uint32_t afterA = OS::get_singleton()->get_ticks_msec();

	uint32_t beforeB = OS::get_singleton()->get_ticks_msec();
	m_Scene.m_bUseSIMD = false;
	m_Scene.m_Tracer.m_bSIMD = false;
	//res = LightmapMesh(*pMI, *pLR, *pIm);
	uint32_t afterB = OS::get_singleton()->get_ticks_msec();

	print_line("SIMD version took : " + itos(afterA - beforeA));
	print_line("reference version took : " + itos(afterB- beforeB));

	if (bake_end_function) {
		bake_end_function();
	}
	return res;
}

void LightMapper::Reset()
{
	m_Lights.clear(true);
	m_Scene.Reset();
	RayBank_Reset();
}

bool LightMapper::LightmapMesh(const MeshInstance &mi, const Spatial &light_root, Image &output_image)
{
	// print out settings
	print_line("Lightmap mesh");
	print_line("\tnum_bounces " + itos(m_Settings_Forward_NumBounces));
	print_line("\tbounce_power " + String(Variant(m_Settings_Forward_BouncePower)));

	Reset();
	m_bCancel = false;

	m_QMC.Create(m_Settings_AO_Samples);

	uint32_t before, after;
	FindLights_Recursive(&light_root);
	print_line("Found " + itos (m_Lights.size()) + " lights.");

	if (m_iWidth <= 0)
		return false;
	if (m_iHeight <= 0)
		return false;

	m_Image_L.Create(m_iWidth, m_iHeight);
	m_Image_L_mirror.Create(m_iWidth, m_iHeight);
	m_Image_AO.Create(m_iWidth, m_iHeight);

	m_Image_ID_p1.Create(m_iWidth, m_iHeight);
	m_Image_ID2_p1.Create(m_iWidth, m_iHeight);
	m_Image_Barycentric.Create(m_iWidth, m_iHeight);

	print_line("Scene Create");
	before = OS::get_singleton()->get_ticks_msec();
	if (!m_Scene.Create(mi, m_iWidth, m_iHeight, m_Settings_VoxelDims))
		return false;

	RayBank_Create();

	after = OS::get_singleton()->get_ticks_msec();
	print_line("SceneCreate took " + itos(after -before) + " ms");

	if (m_bCancel)
		return false;

	print_line("PrepareImageMaps");
	before = OS::get_singleton()->get_ticks_msec();
	PrepareImageMaps();
	after = OS::get_singleton()->get_ticks_msec();
	print_line("PrepareImageMaps took " + itos(after -before) + " ms");

	if (m_bCancel)
		return false;

	print_line("ProcessAO");
	before = OS::get_singleton()->get_ticks_msec();
	ProcessAO();
	after = OS::get_singleton()->get_ticks_msec();
	print_line("ProcessAO took " + itos(after -before) + " ms");


	print_line("ProcessTexels");
	before = OS::get_singleton()->get_ticks_msec();
	if (m_Settings_Mode == LMMODE_BACKWARD)
		ProcessTexels();
	else
	{
		ProcessLights();
	}
	after = OS::get_singleton()->get_ticks_msec();
	print_line("ProcessTexels took " + itos(after -before) + " ms");

	if (m_bCancel)
		return false;

	print_line("WriteOutputImage");
	before = OS::get_singleton()->get_ticks_msec();
	WriteOutputImage(output_image);
	after = OS::get_singleton()->get_ticks_msec();
	print_line("WriteOutputImage took " + itos(after -before) + " ms");

	// clear everything out of ram as no longer needed
	Reset();

	return true;
}

void LightMapper::ProcessTexels_Bounce()
{
	m_Image_L_mirror.Blank();


	for (int y=0; y<m_iHeight; y++)
	{
		if ((y % 10) == 0)
		{
//			print_line("\tTexels bounce line " + itos(y));
//			OS::get_singleton()->delay_usec(1);

			if (bake_step_function) {
				m_bCancel = bake_step_function(y, String("Process TexelsBounce: ") + " (" + itos(y) + ")");
				if (m_bCancel)
					return;
			}
		}

		for (int x=0; x<m_iWidth; x++)
		{
			float power = ProcessTexel_Bounce(x, y);

			// save the incoming light power in the mirror image (as the source is still being used)
			m_Image_L_mirror.GetItem(x, y) = power;
		}
	}

	// merge the 2 luminosity maps
	for (int y=0; y<m_iHeight; y++)
	{
		for (int x=0; x<m_iWidth; x++)
		{
			float f = m_Image_L.GetItem(x, y);
			f += (m_Image_L_mirror.GetItem(x, y) * m_Settings_Backward_BouncePower);
			m_Image_L.GetItem(x, y) = f;
		}
	}

}


void LightMapper::ProcessAO()
{
	for (int y=0; y<m_iHeight; y++)
	{
//		if ((y % 10) == 0)
//		{
//			if (bake_step_function) {
//				m_bCancel = bake_step_function(y, String("Process Texels: ") + " (" + itos(y) + ")");
//				if (m_bCancel)
//					return;
//			}
//		}

		for (int x=0; x<m_iWidth; x++)
		{
			ProcessAO_Texel(x, y);
		}
	}
}

void LightMapper::ProcessTexels()
{
	m_iNumTests = 0;

	for (int y=0; y<m_iHeight; y++)
	{
		if ((y % 10) == 0)
		{
			//print_line("\tTexels line " + itos(y));
			//OS::get_singleton()->delay_usec(1);

			if (bake_step_function) {
				m_bCancel = bake_step_function(y, String("Process Texels: ") + " (" + itos(y) + ")");
				if (m_bCancel)
					return;
			}
		}

		for (int x=0; x<m_iWidth; x++)
		{
			ProcessTexel(x, y);
		}
	}

//	m_iNumTests /= (m_iHeight * m_iWidth);
	print_line("num tests : " + itos(m_iNumTests));

	for (int b=0; b<m_Settings_Backward_NumBounces; b++)
	{
		ProcessTexels_Bounce();
	}
}


float LightMapper::CalculateAO(int tx, int ty, uint32_t tri0, uint32_t tri1_p1)
{
//	Vector3 push = ptNormal * 0.005f;

	Ray r;
//	r.o = ptStart + push;

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
		Vector2 st;
		st.x = Math::randf() + tx;
		st.y = Math::randf() + ty;

		// has to be ranged 0 to 1
		st.x /= m_iWidth;
		st.y /= m_iHeight;

		// barycentric coords.
		float u,v,w;

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

//		if (tri1_p1)
//		{
//			if (!pUVTri->ContainsPoint(st))
//			{
//				// assume it is in the other tri
//				tri = tri1_p1 - 1;
//				pUVTri = &m_Scene.m_UVTris[tri];
//			}
//		}



		// calculate world position ray origin from barycentric
		m_Scene.m_Tris[tri].InterpolateBarycentric(r.o, u, v, w);

		Vector3 ptNormal;
		m_Scene.m_TriNormals[tri].InterpolateBarycentric(ptNormal, u, v, w);

		Vector3 push = ptNormal * 0.005f;

		// push ray origin
		r.o += push;

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
//		float u, v, w, t;
		float t;

		m_Scene.m_Tracer.m_bUseSDF = true;
		int tri_hit = m_Scene.IntersectRay(r, u, v, w, t, &voxel_range, m_iNumTests);

		// nothing hit
//		if ((tri == -1) || (tri == (int) tri_ignore))
		if (tri_hit == -1)
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

/*
float LightMapper::CalculateAO(const Vector3 &ptStart, const Vector3 &ptNormal, uint32_t tri0, uint32_t tri1_p1)
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

float LightMapper::ProcessTexel_Light(int light_id, const Vector3 &ptDest, const Vector3 &ptNormal, uint32_t tri_ignore)
{
	const LLight &light = m_Lights[light_id];

	Ray r;
//	r.o = Vector3(0, 5, 0);

//	float range = light.scale.x;
//	const float range = 2.0f;

	// the power should depend on the volume, with 1x1x1 being normal power
//	float power = light.scale.x * light.scale.y * light.scale.z;
	float power = light.energy;
	power *= m_Settings_Backward_RayPower;

	int nSamples = m_Settings_Backward_NumRays;

	// total light hitting texel
	float fTotal = 0.0f;

	// each ray
	for (int n=0; n<nSamples; n++)
	{
		r.o = light.pos;

		switch (light.type)
		{
		case LLight::LT_SPOT:
			{
//				r.d = light.dir;
//				float spot_ball = 0.2f;
//				float x = Math::random(-spot_ball, spot_ball);
//				float y = Math::random(-spot_ball, spot_ball);
//				float z = Math::random(-spot_ball, spot_ball);
//				r.d += Vector3(x, y, z);
//				r.d.normalize();
			}
			break;
		default:
			{
				float x = Math::random(-light.scale.x, light.scale.x);
				float y = Math::random(-light.scale.y, light.scale.y);
				float z = Math::random(-light.scale.z, light.scale.z);
				r.o += Vector3(x, y, z);
			}
			break;
		}

		// offset from origin to destination texel
		r.d = ptDest - r.o;

		// collision detect
		r.d.normalize();
		float u, v, w, t;

		m_Scene.m_Tracer.m_bUseSDF = true;
		int tri = m_Scene.IntersectRay(r, u, v, w, t, nullptr, m_iNumTests);
//		m_Scene.m_Tracer.m_bUseSDF = false;
//		int tri2 = m_Scene.IntersectRay(r, u, v, w, t, m_iNumTests);
//		if (tri != tri2)
//		{
//			// repeat SDF version
//			m_Scene.m_Tracer.m_bUseSDF = true;
//			int tri = m_Scene.IntersectRay(r, u, v, w, t, m_iNumTests);
//		}

		// nothing hit
		if ((tri == -1) || (tri == tri_ignore))
		{
			// for backward tracing, first pass, this is a special case, because we DO
			// take account of distance to the light, and normal, in order to simulate the effects
			// of the likelihood of 'catching' a ray. In forward tracing this happens by magic.
			float dist = (r.o - ptDest).length();
			float local_power = power * InverseSquareDropoff(dist);

			// take into account normal
			float dot = r.d.dot(ptNormal);
			dot = fabs(dot);

			local_power *= dot;

			fTotal += local_power;
		}
	}

	// save in the texel
	return fTotal;
}


float LightMapper::ProcessTexel_Bounce(int x, int y)
{
	// find triangle
	uint32_t tri_source = *m_Image_ID_p1.Get(x, y);
	if (!tri_source)
		return 0.0f;
	tri_source--; // plus one based

	// barycentric
	const Vector3 &bary = *m_Image_Barycentric.Get(x, y);

	Vector3 pos;
	m_Scene.m_Tris[tri_source].InterpolateBarycentric(pos, bary.x, bary.y, bary.z);

	Vector3 norm;
	const Tri &triangle_normal = m_Scene.m_TriNormals[tri_source];
	triangle_normal.InterpolateBarycentric(norm, bary.x, bary.y, bary.z);
	norm.normalize();

	float fTotal = 0.0f;

	int nSamples = m_Settings_Backward_NumBounceRays;
	for (int n=0; n<nSamples; n++)
	{
		// bounce

		// first dot
			Ray r;

			// SLIDING
//			Vector3 temp = r.d.cross(norm);
//			new_ray.d = norm.cross(temp);

			// BOUNCING - mirror
			//new_ray.d = r.d - (2.0f * (dot * norm));

			// random hemisphere
			RandomUnitDir(r.d);

			// compare direction to normal, if opposite, flip it
			if (r.d.dot(norm) < 0.0f)
				r.d = -r.d;

			// add a little epsilon to prevent self intersection
			r.o = pos + (norm * 0.01f);
			//ProcessRay(new_ray, depth+1, power * 0.4f);

			// collision detect
			//r.d.normalize();
			float u, v, w, t;
			int tri_hit = m_Scene.IntersectRay(r, u, v, w, t, nullptr, m_iNumTests);

			// nothing hit
			if ((tri_hit != -1) && (tri_hit != tri_source))
			{
				// look up the UV of the tri hit
				Vector2 uvs;
				m_Scene.FindUVsBarycentric(tri_hit, uvs, u, v, w);

				// find texel
				int dx = (uvs.x * m_iWidth); // round?
				int dy = (uvs.y * m_iHeight);

				if (m_Image_L.IsWithin(dx, dy))
				{
					float power = m_Image_L.GetItem(dx, dy);
					fTotal += power;
				}

			}
	}

	return fTotal / nSamples;
}


void LightMapper::ProcessAO_Texel(int tx, int ty)
{
	// find triangle
	uint32_t tri = *m_Image_ID_p1.Get(tx, ty);
	if (!tri)
		return;
	tri--; // plus one based

	// may be more than 1 triangle on this texel
	uint32_t tri2 = *m_Image_ID2_p1.Get(tx, ty);

	// barycentric
	const Vector3 &bary = *m_Image_Barycentric.Get(tx, ty);

	Vector3 pos;
	m_Scene.m_Tris[tri].InterpolateBarycentric(pos, bary.x, bary.y, bary.z);

	Vector3 normal;
	m_Scene.m_TriNormals[tri].InterpolateBarycentric(normal, bary.x, bary.y, bary.z);

	// could be off the image
	float * pfTexel = m_Image_AO.Get(tx, ty);
#ifdef DEBUG_ENABLED
	assert (pfTexel);
#endif

//	float power = CalculateAO(pos, normal, tri, tri2);
	float power = CalculateAO(tx, ty, tri, tri2);
	*pfTexel = power;
}

void LightMapper::ProcessTexel(int tx, int ty)
{
	// find triangle
	uint32_t tri = *m_Image_ID_p1.Get(tx, ty);
	if (!tri)
		return;
	tri--; // plus one based

	// barycentric
	const Vector3 &bary = *m_Image_Barycentric.Get(tx, ty);

	Vector3 pos;
	m_Scene.m_Tris[tri].InterpolateBarycentric(pos, bary.x, bary.y, bary.z);

	Vector3 normal;
	m_Scene.m_TriNormals[tri].InterpolateBarycentric(normal, bary.x, bary.y, bary.z);


	//Vector2i tex_uv = Vector2i(x, y);

	// could be off the image
	float * pfTexel = m_Image_L.Get(tx, ty);
	if (!pfTexel)
		return;

	for (int l=0; l<m_Lights.size(); l++)
	{
		float power = ProcessTexel_Light(l, pos, normal, tri);
		*pfTexel += power;
	}
}

void LightMapper::ProcessRay(LM::Ray r, int depth, float power, int dest_tri_id, const Vector2i * pUV)
{
	// unlikely
	if (r.d.x == 0.0f && r.d.y == 0.0f && r.d.z == 0.0f)
		return;

	// test
//	r.d = Vector3(0, -1, 0);
//	r.d = Vector3(-2.87, -5.0 + 0.226, 4.076);

	r.d.normalize();
	float u, v, w, t;
	int tri = m_Scene.IntersectRay(r, u, v, w, t, nullptr, m_iNumTests);

	// nothing hit
	if (tri == -1)
		return;

	// convert barycentric to uv coords in the lightmap
	Vector2 uv;
	m_Scene.FindUVsBarycentric(tri, uv, u, v, w);
//	m_UVTris[tri].FindUVBarycentric(uvs, u, v, w);

	// texel address
	int tx = uv.x * m_iWidth;
	int ty = uv.y * m_iHeight;

	// override?
	if (pUV && tri == dest_tri_id)
	{
		tx = pUV->x;
		ty = pUV->y;
	}

	// could be off the image
	float * pf = m_Image_L.Get(tx, ty);
	if (!pf)
		return;

	// scale according to distance
	t /= 10.0f;
	t = 1.0f - t;
	if (t < 0.0f)
		t = 0.0f;
	t *= 2.0f;

	t = power;
//	if (t > *pf)

//	if (depth > 0)
		*pf += t;

	// bounce and lower power

	if (depth < m_Settings_Forward_NumBounces)
	{
		Vector3 pos;
		const Tri &triangle = m_Scene.m_Tris[tri];
		triangle.InterpolateBarycentric(pos, u, v, w);

		Vector3 norm;
		const Tri &triangle_normal = m_Scene.m_TriNormals[tri];
		triangle_normal.InterpolateBarycentric(norm, u, v, w);
		norm.normalize();

		// first dot
		float dot = norm.dot(r.d);
		if (dot <= 0.0f)
		{

			Ray new_ray;

			// SLIDING
//			Vector3 temp = r.d.cross(norm);
//			new_ray.d = norm.cross(temp);

			// BOUNCING - mirror
			Vector3 mirror_dir = r.d - (2.0f * (dot * norm));

			// random hemisphere
			const float range = 1.0f;
			Vector3 hemi_dir;
			while (true)
			{
				hemi_dir.x = Math::random(-range, range);
				hemi_dir.y = Math::random(-range, range);
				hemi_dir.z = Math::random(-range, range);

				float sl = hemi_dir.length_squared();
				if (sl > 0.0001f)
				{
					break;
				}
			}
			// compare direction to normal, if opposite, flip it
			if (hemi_dir.dot(norm) < 0.0f)
				hemi_dir = -hemi_dir;

			new_ray.d = hemi_dir.linear_interpolate(mirror_dir, m_Settings_Forward_BounceDirectionality);

			new_ray.o = pos + (norm * 0.01f);
			ProcessRay(new_ray, depth+1, power * m_Settings_Forward_BouncePower);
		} // in opposite directions
	}

}

void LightMapper::ProcessLights()
{
//	const int rays_per_section = 1024 * 16;

	int num_sections = m_iNumRays / m_iRaysPerSection;

	for (int n=0; n<m_Lights.size(); n++)
	{
		for (int s=0; s<num_sections; s++)
		{
			// double check the voxels are clear
#ifdef DEBUG_ENABLED
			//RayBank_CheckVoxelsClear();
#endif

			if ((s % 1) == 0)
			{
				if (bake_step_function)
				{
					m_bCancel = bake_step_function(s, String("Process Light Section: ") + " (" + itos(s) + ")");
					if (m_bCancel)
						return;
				}
			}


			ProcessLight(n, m_iRaysPerSection);

			for (int b=0; b<m_Settings_Forward_NumBounces+1; b++)
			{
				RayBank_Process();
				RayBank_Flush();
			} // for bounce
		} // for section

		// left over
		{
			int num_leftover = m_iNumRays - (num_sections * m_iRaysPerSection);
			ProcessLight(n, num_leftover);

			for (int b=0; b<m_Settings_Forward_NumBounces+1; b++)
			{
				RayBank_Process();
				RayBank_Flush();
			} // for bounce
		}

	} // for light

}

void LightMapper::ProcessLight(int light_id, int num_rays)
{
	const LLight &light = m_Lights[light_id];

	Ray r;
//	r.o = Vector3(0, 5, 0);

//	float range = light.scale.x;
//	const float range = 2.0f;

	// the power should depend on the volume, with 1x1x1 being normal power
//	float power = light.scale.x * light.scale.y * light.scale.z;
	float power = light.energy;
	power *= m_Settings_Forward_RayPower;



	// each ray
	for (int n=0; n<num_rays; n++)
	{
//		if ((n % 10000) == 0)
//		{
//			if (bake_step_function)
//			{
//				m_bCancel = bake_step_function(n, String("Process Rays: ") + " (" + itos(n) + ")");
//				if (m_bCancel)
//					return;
//			}
//		}

		r.o = light.pos;

		switch (light.type)
		{
		case LLight::LT_SPOT:
			{
				r.d = light.dir;
				float spot_ball = 0.2f;
				float x = Math::random(-spot_ball, spot_ball);
				float y = Math::random(-spot_ball, spot_ball);
				float z = Math::random(-spot_ball, spot_ball);
				r.d += Vector3(x, y, z);
				r.d.normalize();
			}
			break;
		default:
			{
				Vector3 offset;
				RandomUnitDir(offset);
				offset *= light.scale;
				r.o += offset;

//				float x = Math::random(-light.scale.x, light.scale.x);
//				float y = Math::random(-light.scale.y, light.scale.y);
//				float z = Math::random(-light.scale.z, light.scale.z);
//				r.o += Vector3(x, y, z);

				RandomUnitDir(r.d);
			}
			break;
		}
		//r.d.normalize();

		RayBank_RequestNewRay(r, m_Settings_Forward_NumBounces + 1, power, 0);

//		ProcessRay(r, 0, power);
	}
}

} // namespace
