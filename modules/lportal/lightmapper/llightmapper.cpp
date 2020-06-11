#include "llightmapper.h"
#include "ldilate.h"
#include "core/os/os.h"

using namespace LM;

void LLightMapper::_bind_methods()
{
	// main functions
	ClassDB::bind_method(D_METHOD("lightmap_mesh", "mesh_instance", "output_image", "num_rays", "ray_power"), &LLightMapper::lightmap_mesh);

}

bool LLightMapper::lightmap_mesh(Node * pMeshInstance, Object * pOutputImage, int num_rays, float power)
{
	m_Settings_NumRays = num_rays;
	m_Settings_RayPower = power;

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

bool LLightMapper::LightmapMesh(const MeshInstance &mi, Image &output_image)
{
	uint32_t before, after;

	m_iWidth = output_image.get_width();
	m_iHeight = output_image.get_height();

	if (m_iWidth <= 0)
		return false;
	if (m_iHeight <= 0)
		return false;

	m_Image_L.Create(m_iWidth, m_iHeight);
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
	//ProcessTexels();
	ProcessLight();
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
//	dilate.DilateImage(m_Image_L, m_Image_ID_p1, 256);

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

//			f = 0.0f;
//				f = 1.0f;


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
}


void LLightMapper::ProcessTexel(int x, int y)
{
	// find triangle
	uint32_t tri = *m_Image_ID_p1.Get(x, y);
	if (!tri)
		return;
	tri--; // plus one based

	// barycentric
	const Vector3 &bary = *m_Image_Barycentric.Get(x, y);

	Vector3 pos;
	m_Scene.m_Tris[tri].InterpolateBarycentric(pos, bary.x, bary.y, bary.z);

	Vector2i tex_uv = Vector2i(x, y);

	Ray r;

	int range = 0;
	float scale = 0.2f;
	for (int y=-range; y<=range; y++)
	{
		for (int x=-range; x<=range; x++)
		{
			r.o = Vector3(x * scale, 5, y * scale);
			Vector3 offset = pos - r.o;
			r.d = offset;
			r.d.normalize();
			ProcessRay(r, 0, m_Settings_RayPower, tri, &tex_uv);
		}
	}
}

void LLightMapper::ProcessRay(const LM::Ray &r, int depth, float power, int dest_tri_id, const Vector2i * pUV)
{
	// unlikely
	if (r.d.x == 0.0f && r.d.y == 0.0f && r.d.z == 0.0f)
		return;

	// test
//	r.d = Vector3(0, -1, 0);
//	r.d = Vector3(-2.87, -5.0 + 0.226, 4.076);

//	r.d.normalize();
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

	if (depth <= 0)
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

			// BOUNCING
			new_ray.d = r.d - (2.0f * (dot * norm));

			new_ray.o = pos + (norm * 0.01f);
			ProcessRay(new_ray, depth+1, power * 0.5f);
		} // in opposite directions
	}

}


void LLightMapper::ProcessLight()
{
	Ray r;
	r.o = Vector3(0, 5, 0);

	const float range = 2.0f;

	// each ray
	for (int n=0; n<m_Settings_NumRays; n++)
	{
		float x = Math::random(-range, range);
		float y = Math::random(-range, range);
		r.o = Vector3(x, 5, y);


		r.d.x = Math::random(-1.0f, 1.0f);
		r.d.y = Math::random(-1.0f, -0.7f);
		r.d.z = Math::random(-1.0f, 1.0f);
		r.d.normalize();

		ProcessRay(r, 0, m_Settings_RayPower);
	}
}


