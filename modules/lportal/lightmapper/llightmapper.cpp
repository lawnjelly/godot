#include "llightmapper.h"

using namespace LM;

bool LightMapper::LightmapMesh(const MeshInstance &mi, Image &output_image)
{
	m_iWidth = output_image.get_width();
	m_iHeight = output_image.get_height();

	m_Image_L.Create(m_iWidth, m_iHeight);
	m_Scene.Create(mi);

	ProcessLight();
	WriteOutputImage(output_image);
	return true;
}

void LightMapper::WriteOutputImage(Image &output_image)
{
	output_image.lock();

	for (int y=0; y<m_iHeight; y++)
	{
		for (int x=0; x<m_iWidth; x++)
		{
			const float * pf = m_Image_L.Get(x, y);
			assert (pf);
			float f = *pf;
			f *= 255.0f;
			f += 0.5f;
			int v = f;
			if (v < 0) v = 0;
			if (v > 255) v = 255;

			output_image.set_pixel(x, y, Color(v, v, v, 255));
		}
	}

	output_image.unlock();
}

void LightMapper::ProcessLight()
{
	Ray r;
	r.o = Vector3(0, 20, 0);

	// each ray
	for (int n=0; n<128; n++)
	{
		r.d.x = Math::random(-1.0f, 1.0f);
		r.d.y = Math::random(-1.0f, 1.0f);
		r.d.z = Math::random(-1.0f, 1.0f);

		// unlikely
		if (r.d.x == 0.0f && r.d.y == 0.0f && r.d.z == 0.0f)
			continue;

		r.d.normalize();
		float u, v, w;
		int tri = m_Scene.IntersectRay(r, u, v, w);

		// nothing hit
		if (tri == -1)
			continue;

		// convert barycentric to uv coords in the lightmap
		Vector2 uv;
		m_Scene.FindUVsBarycentric(tri, uv, u, v, w);

		// texel address
		int tx = uv.x * m_iWidth;
		int ty = uv.y * m_iHeight;

		// could be off the image
		float * pf = m_Image_L.Get(tx, ty);
		if (!pf)
			continue;
		*pf = 1.0f;
	}
}


