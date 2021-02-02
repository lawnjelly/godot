#include "llightmapper.h"
#include "ldilate.h"
#include "core/os/os.h"
#include "scene/3d/light.h"

using namespace LM;

LightMapper_Base::BakeBeginFunc LightMapper_Base::bake_begin_function = NULL;
LightMapper_Base::BakeStepFunc LightMapper_Base::bake_step_function = NULL;
LightMapper_Base::BakeEndFunc LightMapper_Base::bake_end_function = NULL;

LightMapper_Base::LightMapper_Base()
{
	m_iNumRays = 1;
	m_Settings_Forward_NumRays = 1;
	m_Settings_Forward_NumBounces = 1;
	m_Settings_Forward_RayPower = 0.01f;
	m_Settings_Forward_BouncePower = 0.5f;
	m_Settings_Forward_BounceDirectionality = 0.5f;

	m_Settings_Backward_NumRays = 1;
	m_Settings_Backward_NumBounceRays = 1;
	m_Settings_Backward_NumBounces = 1;
	m_Settings_Backward_RayPower = 0.01f;
	m_Settings_Backward_BouncePower = 0.5f;

	m_Settings_AO_Range = 24.0f;
	m_Settings_AO_Samples = 64;
	m_Settings_AO_CutRange = 0.05f;

	m_Settings_Mode = LMMODE_FORWARD;

	m_Settings_TexWidth = 128;
	m_Settings_TexHeight = 128;
	m_Settings_VoxelDims.Set(20, 6, 20);

	m_Settings_Normalize = true;
	m_Settings_NormalizeBias = 1.0f;
}


void LightMapper_Base::FindLight(const Node * pNode)
{
	const Light * pLight = Object::cast_to<const Light>(pNode);
	if (!pLight)
		return;

	// visibility or bake mode?
	if (pLight->get_bake_mode() == Light::BAKE_DISABLED)
		return;

	// is it visible?
//	if (!pLight->is_visible_in_tree())
//		return;

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

	const DirectionalLight * pDLight = Object::cast_to<DirectionalLight>(pLight);
	if (pDLight)
		l->type = LLight::LT_DIRECTIONAL;

	const SpotLight * pSLight = Object::cast_to<SpotLight>(pLight);
	if (pSLight)
		l->type = LLight::LT_SPOT;

	const OmniLight * pOLight = Object::cast_to<OmniLight>(pLight);
	if (pOLight)
		l->type = LLight::LT_OMNI;

}


void LightMapper_Base::FindLights_Recursive(const Node * pNode)
{
	FindLight(pNode);

	int nChildren = pNode->get_child_count();

	for (int n=0; n<nChildren; n++)
	{
		Node * pChild = pNode->get_child(n);
		FindLights_Recursive(pChild);
	}
}




void LightMapper_Base::PrepareImageMaps()
{
	m_Image_ID_p1.Blank();
	m_Image_ID2_p1.Blank();

	// rasterize each triangle in turn
	m_Scene.RasterizeTriangleIDs(*this, m_Image_ID_p1, m_Image_ID2_p1, m_Image_Barycentric);

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

void LightMapper_Base::Normalize_AO()
{
	int nPixels = m_Image_AO.GetNumPixels();
	float fmax = 0.0f;

	// first find the max
	for (int n=0; n<nPixels; n++)
	{
		float f = *m_Image_AO.Get(n);
		if (f > fmax)
			fmax = f;
	}

	if (fmax < 0.001f)
	{
		WARN_PRINT_ONCE("LightMapper_Base::Normalize_AO : values too small to normalize");
		return;
	}

	// multiplier to normal is 1.0f / fmax
	float mult = 1.0f / fmax;

	// apply bias
	//mult *= m_Settings_NormalizeBias;

	// apply multiplier
	for (int n=0; n<nPixels; n++)
	{
		float &f = *m_Image_AO.Get(n);
		f *= mult;

		// negate AO
		f = 1.0f - f;
		if (f < 0.0f)
			f = 0.0f;
	}

}

void LightMapper_Base::Normalize()
{
	if (!m_Settings_Normalize)
		return;

	int nPixels = m_Image_L.GetNumPixels();
	float fmax = 0.0f;

	// first find the max
	for (int n=0; n<nPixels; n++)
	{
		float f = *m_Image_L.Get(n);
		if (f > fmax)
			fmax = f;
	}

	if (fmax < 0.001f)
	{
		WARN_PRINT_ONCE("LightMapper_Base::Normalize : values too small to normalize");
		return;
	}

	// multiplier to normal is 1.0f / fmax
	float mult = 1.0f / fmax;

	// apply bias
	mult *= m_Settings_NormalizeBias;

	// apply multiplier
	for (int n=0; n<nPixels; n++)
	{
		float &f = *m_Image_L.Get(n);
		f *= mult;
	}
}

void LightMapper_Base::WriteOutputImage(Image &output_image)
{
	// for testing copy AO to L
	m_Image_AO.CopyTo(m_Image_L);


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

	Normalize();

	////
	// write some debug
//#define LLIGHTMAPPER_OUTPUT_TRIIDS
#ifdef LLIGHTMAPPER_OUTPUT_TRIIDS
	output_image.lock();
	Color cols[1024];
	for (int n=0; n<m_Scene.GetNumTris(); n++)
	{
		if (n == 1024)
			break;

		cols[n] = Color(Math::randf(), Math::randf(), Math::randf(), 1.0f);
	}
	cols[0] = Color(0, 0, 0, 1.0f);

	for (int y=0; y<m_iHeight; y++)
	{
		for (int x=0; x<m_iWidth; x++)
		{
			int coln = m_Image_ID_p1.GetItem(x, y) % 1024;

			output_image.set_pixel(x, y, cols[coln]);
		}
	}

	output_image.unlock();
	output_image.save_png("tri_ids.png");
#endif

	// final version
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


			Color col;
			col = Color(f, f, f, 1);


			// debug mark the dilated pixels
#define MARK_DILATED
#ifdef MARK_DILATED
			if (!m_Image_ID_p1.GetItem(x, y))
			{
				col = Color(1.0f, 0.33f, 0.66f, 1);
			}
#endif
			//			if (m_Image_ID_p1.GetItem(x, y))
//			{
//				output_image.set_pixel(x, y, Color(f, f, f, 255));
//			}
//			else
//			{
//				output_image.set_pixel(x, y, Color(0, 0, 0, 255));
//			}

			// visual cuts
//			if (m_Image_Cuts.GetItem(x, y).num)
//			{
//				col = Color(1.0f, 0.33f, 0.66f, 1);
//			}
//			else
//			{
//				col = Color(0, 0, 0, 1);
//			}

			// visualize concave
//			const MiniList_Cuts &cuts = m_Image_Cuts.GetItem(x, y);
//			if (cuts.num == 2)
//			{
//				if (cuts.convex)
//				{
//					col = Color(1.0f, 0, 0, 1);
//				}
//				else
//				{
//					col = Color(0, 0, 1, 1);
//				}
//			}

			output_image.set_pixel(x, y, col);
		}
	}

	output_image.unlock();
}


