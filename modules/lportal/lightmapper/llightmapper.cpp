#include "llightmapper.h"
#include "ldilate.h"
#include "core/os/os.h"
#include "scene/3d/light.h"

#define LIGHTMAP_STRINGIFY(x) #x
#define LIGHTMAP_TOSTRING(x) LIGHTMAP_STRINGIFY(x)

using namespace LM;

LLightMapper::BakeBeginFunc LLightMapper::bake_begin_function = NULL;
LLightMapper::BakeStepFunc LLightMapper::bake_step_function = NULL;
LLightMapper::BakeEndFunc LLightMapper::bake_end_function = NULL;


void LLightMapper::_bind_methods()
{
	BIND_ENUM_CONSTANT(LLightMapper::MODE_FORWARD);
	BIND_ENUM_CONSTANT(LLightMapper::MODE_BACKWARD);

	// main functions
//	ClassDB::bind_method(D_METHOD("lightmap_mesh", "mesh_instance", "lights_root_node", "output_image"), &LLightMapper::lightmap_mesh);
//	ClassDB::bind_method(D_METHOD("lightmap_set_params", "num_rays", "ray_power", "bounce_power"), &LLightMapper::lightmap_set_params);
	ClassDB::bind_method(D_METHOD("lightmap_bake"), &LLightMapper::lightmap_bake);
	ClassDB::bind_method(D_METHOD("lightmap_bake_to_image", "output_image"), &LLightMapper::lightmap_bake_to_image);


	ClassDB::bind_method(D_METHOD("set_mode", "mode"), &LLightMapper::set_mode);
	ClassDB::bind_method(D_METHOD("get_mode"), &LLightMapper::get_mode);

	ClassDB::bind_method(D_METHOD("set_image_filename", "filename"), &LLightMapper::set_image_filename);
	ClassDB::bind_method(D_METHOD("get_image_filename"), &LLightMapper::get_image_filename);

//	ClassDB::bind_method(D_METHOD("set_mesh_path", "path"), &LLightMapper::set_mesh_path);
//	ClassDB::bind_method(D_METHOD("get_mesh_path"), &LLightMapper::get_mesh_path);
//	ClassDB::bind_method(D_METHOD("set_lights_path", "path"), &LLightMapper::set_lights_path);
//	ClassDB::bind_method(D_METHOD("get_lights_path"), &LLightMapper::get_lights_path);

//	ClassDB::bind_method(D_METHOD("set_num_rays", "num_rays"), &LLightMapper::set_num_rays);
//	ClassDB::bind_method(D_METHOD("get_num_rays"), &LLightMapper::get_num_rays);
//	ClassDB::bind_method(D_METHOD("set_num_bounces", "num_bounces"), &LLightMapper::set_num_bounces);
//	ClassDB::bind_method(D_METHOD("get_num_bounces"), &LLightMapper::get_num_bounces);

//	ClassDB::bind_method(D_METHOD("set_ray_power", "ray_power"), &LLightMapper::set_ray_power);
//	ClassDB::bind_method(D_METHOD("get_ray_power"), &LLightMapper::get_ray_power);
//	ClassDB::bind_method(D_METHOD("set_num_bounces", "num_bounces"), &LLightMapper::set_num_bounces);
//	ClassDB::bind_method(D_METHOD("get_num_bounces"), &LLightMapper::get_num_bounces);

#define LIMPL_PROPERTY(P_TYPE, P_NAME, P_SET, P_GET) ClassDB::bind_method(D_METHOD(LIGHTMAP_TOSTRING(P_SET), LIGHTMAP_TOSTRING(P_NAME)), &LLightMapper::P_SET);\
ClassDB::bind_method(D_METHOD(LIGHTMAP_TOSTRING(P_GET)), &LLightMapper::P_GET);\
ADD_PROPERTY(PropertyInfo(P_TYPE, LIGHTMAP_TOSTRING(P_NAME)), LIGHTMAP_TOSTRING(P_SET), LIGHTMAP_TOSTRING(P_GET));


	ADD_GROUP("Main", "");
	LIMPL_PROPERTY(Variant::NODE_PATH, mesh, set_mesh_path, get_mesh_path);
	LIMPL_PROPERTY(Variant::NODE_PATH, lights, set_lights_path, get_lights_path);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "image_filename", PROPERTY_HINT_FILE, "*.png"), "set_image_filename", "get_image_filename");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Forward,Backward"), "set_mode", "get_mode");

	ADD_GROUP("Size", "");

	LIMPL_PROPERTY(Variant::INT, tex_width, set_tex_width, get_tex_width);
	LIMPL_PROPERTY(Variant::INT, tex_height, set_tex_height, get_tex_height);
	LIMPL_PROPERTY(Variant::VECTOR3, voxel_grid, set_voxel_dims, get_voxel_dims);

	ADD_GROUP("Forward Parameters", "");
	LIMPL_PROPERTY(Variant::INT, f_rays, set_forward_num_rays, get_forward_num_rays);
	LIMPL_PROPERTY(Variant::REAL, f_ray_power, set_forward_ray_power, get_forward_ray_power);
	LIMPL_PROPERTY(Variant::INT, f_bounces, set_forward_num_bounces, get_forward_num_bounces);
	LIMPL_PROPERTY(Variant::REAL, f_bounce_power, set_forward_bounce_power, get_forward_bounce_power);
	LIMPL_PROPERTY(Variant::REAL, f_bounce_directionality, set_forward_bounce_directionality, get_forward_bounce_directionality);

	ADD_GROUP("Backward Parameters", "");
	LIMPL_PROPERTY(Variant::INT, b_initial_rays, set_backward_num_rays, get_backward_num_rays);
	LIMPL_PROPERTY(Variant::REAL, b_ray_power, set_backward_ray_power, get_backward_ray_power);
	LIMPL_PROPERTY(Variant::INT, b_bounce_rays, set_backward_num_bounce_rays, get_backward_num_bounce_rays);
	LIMPL_PROPERTY(Variant::INT, b_bounces, set_backward_num_bounces, get_backward_num_bounces);
	LIMPL_PROPERTY(Variant::REAL, b_bounce_power, set_backward_bounce_power, get_backward_bounce_power);

	ADD_GROUP("Dynamic Range", "");
	LIMPL_PROPERTY(Variant::BOOL, normalize, set_normalize, get_normalize);
	LIMPL_PROPERTY(Variant::REAL, normalize_bias, set_normalize_bias, get_normalize_bias);



//	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "mesh"), "set_mesh_path", "get_mesh_path");
//	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "lights"), "set_lights_path", "get_lights_path");
//	ADD_PROPERTY(PropertyInfo(Variant::INT, "num_rays"), "set_num_rays", "get_num_rays");
//	ADD_PROPERTY(PropertyInfo(Variant::REAL, "ray_power"), "set_ray_power", "get_ray_power");
//	ADD_PROPERTY(PropertyInfo(Variant::INT, "num_bounces"), "set_num_bounces", "get_num_bounces");
//	ADD_PROPERTY(PropertyInfo(Variant::REAL, "bounce_power"), "set_bounce_power", "get_bounce_power");
}

LLightMapper::LLightMapper()
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

	m_Settings_Mode = MODE_FORWARD;

	m_Settings_TexWidth = 128;
	m_Settings_TexHeight = 128;
	m_Settings_VoxelDims.Set(20, 6, 20);

	m_Settings_Normalize = true;
	m_Settings_NormalizeBias = 1.0f;
}

void LLightMapper::set_mode(LLightMapper::eMode p_mode) {m_Settings_Mode = p_mode;}
LLightMapper::eMode LLightMapper::get_mode() const {return m_Settings_Mode;}

void LLightMapper::set_mesh_path(const NodePath &p_path) {m_Settings_Path_Mesh = p_path;}
NodePath LLightMapper::get_mesh_path() const {return m_Settings_Path_Mesh;}
void LLightMapper::set_lights_path(const NodePath &p_path)  {m_Settings_Path_Lights = p_path;}
NodePath LLightMapper::get_lights_path() const {return m_Settings_Path_Lights;}

void LLightMapper::set_forward_num_rays(int num_rays) {m_Settings_Forward_NumRays = num_rays;}
int LLightMapper::get_forward_num_rays() const {return m_Settings_Forward_NumRays;}

void LLightMapper::set_forward_num_bounces(int num_bounces) {m_Settings_Forward_NumBounces = num_bounces;}
int LLightMapper::get_forward_num_bounces() const {return m_Settings_Forward_NumBounces;}

void LLightMapper::set_forward_ray_power(float ray_power) {m_Settings_Forward_RayPower = ray_power;}
float LLightMapper::get_forward_ray_power() const {return m_Settings_Forward_RayPower;}

void LLightMapper::set_forward_bounce_power(float bounce_power) {m_Settings_Forward_BouncePower = bounce_power;}
float LLightMapper::get_forward_bounce_power() const {return m_Settings_Forward_BouncePower;}

void LLightMapper::set_forward_bounce_directionality(float bounce_dir) {m_Settings_Forward_BounceDirectionality = bounce_dir;}
float LLightMapper::get_forward_bounce_directionality() const {return m_Settings_Forward_BounceDirectionality;}

////////////////////////////
void LLightMapper::set_backward_num_rays(int num_rays) {m_Settings_Backward_NumRays = num_rays;}
int LLightMapper::get_backward_num_rays() const {return m_Settings_Backward_NumRays;}

void LLightMapper::set_backward_num_bounce_rays(int num_rays) {m_Settings_Backward_NumBounceRays = num_rays;}
int LLightMapper::get_backward_num_bounce_rays() const {return m_Settings_Backward_NumBounceRays;}

void LLightMapper::set_backward_num_bounces(int num_bounces) {m_Settings_Backward_NumBounces = num_bounces;}
int LLightMapper::get_backward_num_bounces() const {return m_Settings_Backward_NumBounces;}

void LLightMapper::set_backward_ray_power(float ray_power) {m_Settings_Backward_RayPower = ray_power;}
float LLightMapper::get_backward_ray_power() const {return m_Settings_Backward_RayPower;}

void LLightMapper::set_backward_bounce_power(float bounce_power) {m_Settings_Backward_BouncePower = bounce_power;}
float LLightMapper::get_backward_bounce_power() const {return m_Settings_Backward_BouncePower;}
////////////////////////////

void LLightMapper::set_tex_width(int width) {m_Settings_TexWidth = width;}
int LLightMapper::get_tex_width() const {return m_Settings_TexWidth;}

void LLightMapper::set_tex_height(int height) {m_Settings_TexHeight = height;}
int LLightMapper::get_tex_height() const {return m_Settings_TexHeight;}

void LLightMapper::set_voxel_dims(const Vector3 &dims) {m_Settings_VoxelDims.Set(dims);}
Vector3 LLightMapper::get_voxel_dims() const {Vector3 p; m_Settings_VoxelDims.To(p); return p;}

void LLightMapper::set_normalize(bool norm) {m_Settings_Normalize = norm;}
bool LLightMapper::get_normalize() const {return m_Settings_Normalize;}

void LLightMapper::set_normalize_bias(float bias) {m_Settings_NormalizeBias = bias;}
float LLightMapper::get_normalize_bias() const {return m_Settings_NormalizeBias;}

void LLightMapper::set_image_filename(const String &p_filename) {m_Settings_ImageFilename = p_filename;}
String LLightMapper::get_image_filename() const {return m_Settings_ImageFilename;}

//void LLightMapper::lightmap_set_params(int num_rays, float power, float bounce_power)
//{
//	m_Settings_Forward_NumRays = num_rays;
//	m_Settings_Forward_RayPower = power;
//	m_Settings_Forward_BouncePower = bounce_power / num_rays;
//}


bool LLightMapper::lightmap_bake()
{
	if (m_Settings_ImageFilename == "")
		return false;

	// bake to a file
//	Ref<Image> image;
	Ref<Image> image = memnew(Image(m_Settings_TexWidth, m_Settings_TexHeight, false, Image::FORMAT_RGBA8));
//	image.instance();
//	Ref<Image>im = memnew(Image);
//	image->create(128, 128, false, Image::FORMAT_RGBA8);

	lightmap_bake_to_image(image.ptr());

	// save the image
	image->save_png(m_Settings_ImageFilename);

	ResourceLoader::import(m_Settings_ImageFilename);

	return true;
}

bool LLightMapper::lightmap_bake_to_image(Object * pOutputImage)
{
	// get the mesh instance and light root
	if (!has_node(m_Settings_Path_Mesh))
	{
		WARN_PRINT("lightmap_bake : mesh path is invalid");
		return false;
	}

	MeshInstance * pMeshInstance = Object::cast_to<MeshInstance>(get_node(m_Settings_Path_Mesh));
	if (!pMeshInstance)
	{
		WARN_PRINT("lightmap_bake : mesh path is not a mesh instance");
		return false;
	}

	if (!has_node(m_Settings_Path_Lights))
	{
		WARN_PRINT("lightmap_bake : lights path is invalid");
		return false;
	}

	Node * pLightRoot = Object::cast_to<Node>(get_node(m_Settings_Path_Lights));
	if (!pLightRoot)
	{
		WARN_PRINT("lightmap_bake : lights path is not a node");
		return false;
	}

	return lightmap_mesh(pMeshInstance, pLightRoot, pOutputImage);
}


bool LLightMapper::lightmap_mesh(Node * pMeshInstance, Node * pLightRoot, Object * pOutputImage)
{

	MeshInstance * pMI = Object::cast_to<MeshInstance>(pMeshInstance);
	if (!pMI)
	{
		WARN_PRINT("lightmap_mesh : not a mesh instance");
		return false;
	}

	Spatial * pLR = Object::cast_to<Spatial>(pLightRoot);
	if (!pLR)
	{
		WARN_PRINT("lightmap_mesh : lights root is not a spatial");
		return false;
	}

	Image * pIm = Object::cast_to<Image>(pOutputImage);
	if (!pIm)
	{
		WARN_PRINT("lightmap_mesh : not an image");
		return false;
	}

	// get the output dimensions before starting, because we need this
	// to determine number of rays, and the progress range
	m_iWidth = pIm->get_width();
	m_iHeight = pIm->get_height();
	m_iNumRays = m_Settings_Forward_NumRays;

	int nTexels = m_iWidth * m_iHeight;
	int progress_range = m_iHeight;

	// set num rays depending on method
	if (m_Settings_Mode == MODE_FORWARD)
	{
		// the num rays / texel. This is per light!
		m_iNumRays *= nTexels;
		progress_range = m_iNumRays;
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

void LLightMapper::FindLight(const Node * pNode)
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


void LLightMapper::FindLights_Recursive(const Node * pNode)
{
	FindLight(pNode);

	int nChildren = pNode->get_child_count();

	for (int n=0; n<nChildren; n++)
	{
		Node * pChild = pNode->get_child(n);
		FindLights_Recursive(pChild);
	}
}


void LLightMapper::Reset()
{
	m_Lights.clear(true);
	m_Scene.Reset();
	m_RayBank.Reset();
}

bool LLightMapper::LightmapMesh(const MeshInstance &mi, const Spatial &light_root, Image &output_image)
{
	// print out settings
	print_line("Lightmap mesh");
	print_line("\tnum_bounces " + itos(m_Settings_Forward_NumBounces));
	print_line("\tbounce_power " + String(Variant(m_Settings_Forward_BouncePower)));

	Reset();
	m_bCancel = false;

	uint32_t before, after;
	FindLights_Recursive(&light_root);
	print_line("Found " + itos (m_Lights.size()) + " lights.");

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
	if (!m_Scene.Create(mi, m_iWidth, m_iHeight, m_Settings_VoxelDims))
		return false;

	m_RayBank.Create(m_Settings_VoxelDims, m_Scene);

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

	print_line("ProcessTexels");
	before = OS::get_singleton()->get_ticks_msec();
	if (m_Settings_Mode == MODE_BACKWARD)
		ProcessTexels();
	else
	{
		for (int n=0; n<m_Lights.size(); n++)
		{
			ProcessLight(n);
		}
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

void LLightMapper::Normalize()
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
		WARN_PRINT_ONCE("LLightMapper::Normalize : values too small to normalize");
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


void LLightMapper::ProcessTexels()
{
	m_iNumTests = 0;


#ifdef _OPENMP
#pragma message ("_OPENMP defined")
//#pragma omp parallel
#endif
//    #pragma omp parallel for
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

float LLightMapper::ProcessTexel_Light(int light_id, const Vector3 &ptDest, const Vector3 &ptNormal, uint32_t tri_ignore)
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
		int tri = m_Scene.IntersectRay(r, u, v, w, t, m_iNumTests);
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

	return fTotal / nSamples;
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
	power *= m_Settings_Forward_RayPower;



	// each ray
	for (int n=0; n<m_iNumRays; n++)
	{
		if ((n % 10000) == 0)
		{
			if (bake_step_function)
			{
				m_bCancel = bake_step_function(n, String("Process Rays: ") + " (" + itos(n) + ")");
				if (m_bCancel)
					return;
			}
		}

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

		ProcessRay(r, 0, power);
	}
}


