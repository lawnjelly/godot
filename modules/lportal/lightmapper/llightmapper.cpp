#include "llightmapper.h"
#include "ldilate.h"
#include "core/os/os.h"
#include "scene/3d/light.h"

using namespace LM;

void LLightMapper::_bind_methods()
{
	// main functions
	ClassDB::bind_method(D_METHOD("lightmap_mesh", "mesh_instance", "output_image", "num_rays", "ray_power", "bounce_power"), &LLightMapper::lightmap_mesh);

}

bool LLightMapper::lightmap_mesh(Node * pMeshInstance, Object * pOutputImage, int num_rays, float power, float bounce_power)
{
	m_Settings_NumRays = num_rays;
	m_Settings_RayPower = power;
	m_Settings_BouncePower = bounce_power / num_rays;

	MeshInstance * pMI = Object::cast_to<MeshInstance>(pMeshInstance);
	if (!pMI)
	{
		WARN_PRINT("lightmap_mesh : not a mesh instance");
		return false;
	}

	Image * pIm = Object::cast_to<Image>(pOutputImage);
	if (!pIm)
	{
		WARN_PRINT("lightmap_mesh : not an image");
		return false;
	}

	return LightmapMesh(*pMI, *pIm);
}

void LLightMapper::FindLights(const MeshInstance &mi)
{
	// find lights that are children of the mesh instance
	int nChildren = mi.get_child_count();
	for (int n=0; n<nChildren; n++)
	{
		Node * pChild = mi.get_child(n);

		Light * pLight = Object::cast_to<Light>(pChild);
		if (!pLight)
			continue;

		// is it visible?
		if (!pLight->is_visible_in_tree())
			continue;

		LLight * l = m_Lights.request();
		l->m_pLight = pLight;
		// get global transform only works if glight is in the tree
		Transform trans = pLight->get_global_transform();
		l->pos = trans.origin;
		l->dir = -trans.basis.get_axis(2); // or possibly get_axis .. z is what we want
		l->dir.normalize();

		trans = pLight->get_transform();
		l->scale = trans.basis.get_scale();

		l->energy = pLight->get_param(Light::PARAM_ENERGY);
		l->indirect_energy = pLight->get_param(Light::PARAM_INDIRECT_ENERGY);
		l->range = pLight->get_param(Light::PARAM_RANGE);
		l->spot_angle = pLight->get_param(Light::PARAM_SPOT_ANGLE);

		DirectionalLight * pDLight = Object::cast_to<DirectionalLight>(pChild);
		if (pDLight)
			l->type = LLight::LT_DIRECTIONAL;

		SpotLight * pSLight = Object::cast_to<SpotLight>(pChild);
		if (pSLight)
			l->type = LLight::LT_SPOT;

		OmniLight * pOLight = Object::cast_to<OmniLight>(pChild);
		if (pOLight)
			l->type = LLight::LT_OMNI;

	}

}


bool LLightMapper::LightmapMesh(const MeshInstance &mi, Image &output_image)
{
	uint32_t before, after;
	FindLights(mi);

	m_iWidth = output_image.get_width();
	m_iHeight = output_image.get_height();

	if (m_iWidth <= 0)
		return false;
	if (m_iHeight <= 0)
		return false;

	m_Image_L.Create(m_iWidth, m_iHeight);
	m_Image_L_mirror.Create(m_iWidth, m_iHeight);

	m_Image_ID_p1.Create(m_iWidth, m_iHeight);
	m_Image_Barycentric.Create(m_iWidth, m_iHeight);

	print_line("Scene Create");
	before = OS::get_singleton()->get_ticks_msec();
	m_Scene.Create(mi, m_iWidth, m_iHeight);
	after = OS::get_singleton()->get_ticks_msec();
	print_line("SceneCreate took " + itos(after -before) + " ms");

	print_line("PrepareImageMaps");
	before = OS::get_singleton()->get_ticks_msec();
	PrepareImageMaps();
	after = OS::get_singleton()->get_ticks_msec();
	print_line("PrepareImageMaps took " + itos(after -before) + " ms");


	print_line("ProcessTexels");
	before = OS::get_singleton()->get_ticks_msec();
	ProcessTexels();
//	for (int n=0; n<m_Lights.size(); n++)
//	{
//		ProcessLight(n);
//	}
	after = OS::get_singleton()->get_ticks_msec();
	print_line("ProcessTexels took " + itos(after -before) + " ms");


	print_line("WriteOutputImage");
	before = OS::get_singleton()->get_ticks_msec();
	WriteOutputImage(output_image);
	after = OS::get_singleton()->get_ticks_msec();
	print_line("WriteOutputImage took " + itos(after -before) + " ms");
	return true;
}

void LLightMapper::PrepareImageMaps()
{
	m_Image_ID_p1.Blank();

	// rasterize each triangle in turn
	m_Scene.RasterizeTriangleIDs(m_Image_ID_p1, m_Image_Barycentric);

	/*
	// go through each texel
	for (int y=0; y<m_iHeight; y++)
	{
		for (int x=0; x<m_iWidth; x++)
		{
			// use the texel centre!
			// find the triangle at this UV
			float u = (x + 0.5f) / (float) m_iWidth;
			float v = (y + 0.5f) / (float) m_iHeight;


			Vector3 &bary = *m_Image_Barycentric.Get(x, y);
			*m_Image_ID_p1.Get(x, y) = m_Scene.FindTriAtUV(u, v, bary.x, bary.y, bary.z);
		}
	}
	*/
}

void LLightMapper::WriteOutputImage(Image &output_image)
{
	Dilate<float> dilate;
	dilate.DilateImage(m_Image_L, m_Image_ID_p1, 256);

	// test
//	int test_size = 7;
//	LightImage<float> imf;
//	imf.Create(test_size, test_size);
//	LightImage<uint32_t> imi;
//	imi.Create(test_size, test_size);
//	imi.GetItem(3, 3) = 255;
//	dilate.DilateImage(imf, imi);

	output_image.lock();

	for (int y=0; y<m_iHeight; y++)
	{
		for (int x=0; x<m_iWidth; x++)
		{
			const float * pf = m_Image_L.Get(x, y);
			assert (pf);
			float f = *pf;

			// gamma correction
			float gamma = 1.0f / 2.2f;
			f = powf(f, gamma);

			output_image.set_pixel(x, y, Color(f, f, f, 255));
//			if (m_Image_ID_p1.GetItem(x, y))
//			{
//				output_image.set_pixel(x, y, Color(f, f, f, 255));
//			}
//			else
//			{
//				output_image.set_pixel(x, y, Color(0, 0, 0, 255));
//			}
		}
	}

	output_image.unlock();
}

void LLightMapper::ProcessTexels_Bounce()
{
	m_Image_L_mirror.Blank();


	for (int y=0; y<m_iHeight; y++)
	{
		if ((y % 10) == 0)
		{
			print_line("\tTexels bounce line " + itos(y));
			OS::get_singleton()->delay_usec(1);
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
			f += (m_Image_L_mirror.GetItem(x, y) * m_Settings_BouncePower);
			m_Image_L.GetItem(x, y) = f;
		}
	}

}


void LLightMapper::ProcessTexels()
{
	m_iNumTests = 0;

	for (int y=0; y<m_iHeight; y++)
	{
		if ((y % 10) == 0)
		{
			print_line("\tTexels line " + itos(y));
			OS::get_singleton()->delay_usec(1);
		}

		for (int x=0; x<m_iWidth; x++)
		{
			ProcessTexel(x, y);
		}
	}

	m_iNumTests /= (m_iHeight * m_iWidth);
	print_line("average num tests : " + itos(m_iNumTests));

	ProcessTexels_Bounce();
}

float LLightMapper::ProcessTexel_Light(int light_id, const Vector3 &ptDest, uint32_t tri_ignore)
{
	const LLight &light = m_Lights[light_id];

	Ray r;
//	r.o = Vector3(0, 5, 0);

//	float range = light.scale.x;
//	const float range = 2.0f;

	// the power should depend on the volume, with 1x1x1 being normal power
//	float power = light.scale.x * light.scale.y * light.scale.z;
	float power = light.energy;
	power *= m_Settings_RayPower;

	int nSamples = m_Settings_NumRays;

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
		int tri = m_Scene.IntersectRay(r, u, v, w, t, m_iNumTests);

		// nothing hit
		if ((tri == -1) || (tri == tri_ignore))
		{
			fTotal += power;
		}
	}

	// save in the texel
	return fTotal;
}


float LLightMapper::ProcessTexel_Bounce(int x, int y)
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

	int nSamples = m_Settings_NumRays;
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
			const float range = 1.0f;
			while (true)
			{
				r.d.x = Math::random(-range, range);
				r.d.y = Math::random(-range, range);
				r.d.z = Math::random(-range, range);

				float sl = r.d.length_squared();
				if (sl > 0.0001f)
				{
					break;
				}
			}
			// compare direction to normal, if opposite, flip it
			if (r.d.dot(norm) < 0.0f)
				r.d = -r.d;


			r.o = pos + (norm * 0.01f);
			//ProcessRay(new_ray, depth+1, power * 0.4f);

			// collision detect
			r.d.normalize();
			float u, v, w, t;
			int tri_hit = m_Scene.IntersectRay(r, u, v, w, t, m_iNumTests);

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

	return fTotal;
}

void LLightMapper::ProcessSubTexel(float fx, float fy)
{

}

void LLightMapper::ProcessTexel(int tx, int ty)
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

	//Vector2i tex_uv = Vector2i(x, y);

	// could be off the image
	float * pfTexel = m_Image_L.Get(tx, ty);
	if (!pfTexel)
		return;

	for (int l=0; l<m_Lights.size(); l++)
	{
		float power = ProcessTexel_Light(l, pos, tri);
		*pfTexel += power;
	}
}

void LLightMapper::ProcessRay(LM::Ray r, int depth, float power, int dest_tri_id, const Vector2i * pUV)
{
	// unlikely
	if (r.d.x == 0.0f && r.d.y == 0.0f && r.d.z == 0.0f)
		return;

	// test
//	r.d = Vector3(0, -1, 0);
//	r.d = Vector3(-2.87, -5.0 + 0.226, 4.076);

	r.d.normalize();
	float u, v, w, t;
	int tri = m_Scene.IntersectRay(r, u, v, w, t, m_iNumTests);

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

	if (depth < 2)
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
			//new_ray.d = r.d - (2.0f * (dot * norm));

			// random hemisphere
			const float range = 1.0f;
			while (true)
			{
				new_ray.d.x = Math::random(-range, range);
				new_ray.d.y = Math::random(-range, range);
				new_ray.d.z = Math::random(-range, range);

				float sl = new_ray.d.length_squared();
				if (sl > 0.0001f)
				{
					break;
				}
			}
			// compare direction to normal, if opposite, flip it
			if (new_ray.d.dot(norm) < 0.0f)
				new_ray.d = -new_ray.d;


			new_ray.o = pos + (norm * 0.01f);
			ProcessRay(new_ray, depth+1, power * 0.4f);
		} // in opposite directions
	}

}


void LLightMapper::ProcessLight(int light_id)
{
	const LLight &light = m_Lights[light_id];

	Ray r;
//	r.o = Vector3(0, 5, 0);

//	float range = light.scale.x;
//	const float range = 2.0f;

	// the power should depend on the volume, with 1x1x1 being normal power
//	float power = light.scale.x * light.scale.y * light.scale.z;
	float power = light.energy;
	power *= m_Settings_RayPower;



	// each ray
	for (int n=0; n<m_Settings_NumRays; n++)
	{
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
				float x = Math::random(-light.scale.x, light.scale.x);
				float y = Math::random(-light.scale.y, light.scale.y);
				float z = Math::random(-light.scale.z, light.scale.z);
				r.o += Vector3(x, y, z);


				r.d.x = Math::random(-1.0f, 1.0f);
		//		r.d.y = Math::random(-1.0f, -0.7f);
				r.d.y = Math::random(-1.0f, 1.0f);
				r.d.z = Math::random(-1.0f, 1.0f);
			}
			break;
		}
		//r.d.normalize();

		ProcessRay(r, 0, power);
	}
}


