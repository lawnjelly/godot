#include "gdlightmapper.h"

#define LIGHTMAP_STRINGIFY(x) #x
#define LIGHTMAP_TOSTRING(x) LIGHTMAP_STRINGIFY(x)


void LLightmap::_bind_methods()
{
	BIND_ENUM_CONSTANT(LLightmap::MODE_FORWARD);
	BIND_ENUM_CONSTANT(LLightmap::MODE_BACKWARD);

	BIND_ENUM_CONSTANT(LLightmap::BAKEMODE_AO);
	BIND_ENUM_CONSTANT(LLightmap::BAKEMODE_MERGE);
	BIND_ENUM_CONSTANT(LLightmap::BAKEMODE_COMBINED);
	BIND_ENUM_CONSTANT(LLightmap::BAKEMODE_LIGHTMAP);

	// main functions
	ClassDB::bind_method(D_METHOD("lightmap_bake"), &LLightmap::lightmap_bake);
	ClassDB::bind_method(D_METHOD("lightmap_bake_to_image", "output_image"), &LLightmap::lightmap_bake_to_image);


	ClassDB::bind_method(D_METHOD("set_mode", "mode"), &LLightmap::set_mode);
	ClassDB::bind_method(D_METHOD("get_mode"), &LLightmap::get_mode);

	ClassDB::bind_method(D_METHOD("set_bake_mode", "bake_mode"), &LLightmap::set_bake_mode);
	ClassDB::bind_method(D_METHOD("get_bake_mode"), &LLightmap::get_bake_mode);

	ClassDB::bind_method(D_METHOD("set_lightmap_filename", "lightmap_filename"), &LLightmap::set_lightmap_filename);
	ClassDB::bind_method(D_METHOD("get_lightmap_filename"), &LLightmap::get_lightmap_filename);
	ClassDB::bind_method(D_METHOD("set_ao_filename", "ao_filename"), &LLightmap::set_ao_filename);
	ClassDB::bind_method(D_METHOD("get_ao_filename"), &LLightmap::get_ao_filename);
	ClassDB::bind_method(D_METHOD("set_combined_filename", "combined_filename"), &LLightmap::set_combined_filename);
	ClassDB::bind_method(D_METHOD("get_combined_filename"), &LLightmap::get_combined_filename);


#define LIMPL_PROPERTY(P_TYPE, P_NAME, P_SET, P_GET) ClassDB::bind_method(D_METHOD(LIGHTMAP_TOSTRING(P_SET), LIGHTMAP_TOSTRING(P_NAME)), &LLightmap::P_SET);\
ClassDB::bind_method(D_METHOD(LIGHTMAP_TOSTRING(P_GET)), &LLightmap::P_GET);\
ADD_PROPERTY(PropertyInfo(P_TYPE, LIGHTMAP_TOSTRING(P_NAME)), LIGHTMAP_TOSTRING(P_SET), LIGHTMAP_TOSTRING(P_GET));


	ADD_GROUP("Main", "");
	LIMPL_PROPERTY(Variant::NODE_PATH, mesh, set_mesh_path, get_mesh_path);
	LIMPL_PROPERTY(Variant::NODE_PATH, lights, set_lights_path, get_lights_path);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Forward,Backward"), "set_mode", "get_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "bake_mode", PROPERTY_HINT_ENUM, "Lightmap,AO,Merge,Combined"), "set_bake_mode", "get_bake_mode");
	LIMPL_PROPERTY(Variant::REAL, surface_bias, set_surface_bias, get_surface_bias);

	ADD_GROUP("Files", "");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "lightmap_filename", PROPERTY_HINT_FILE, "*.png,*.exr"), "set_lightmap_filename", "get_lightmap_filename");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "ao_filename", PROPERTY_HINT_FILE, "*.png,*.exr"), "set_ao_filename", "get_ao_filename");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "combined_filename", PROPERTY_HINT_FILE, "*.png,*.exr"), "set_combined_filename", "get_combined_filename");


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

	ADD_GROUP("Ambient Occlusion", "");
	LIMPL_PROPERTY(Variant::INT, ao_samples, set_ao_num_samples, get_ao_num_samples);
	LIMPL_PROPERTY(Variant::REAL, ao_range, set_ao_range, get_ao_range);
	LIMPL_PROPERTY(Variant::REAL, ao_cut_range, set_ao_cut_range, get_ao_cut_range);


	ADD_GROUP("Dynamic Range", "");
	LIMPL_PROPERTY(Variant::BOOL, normalize, set_normalize, get_normalize);
	LIMPL_PROPERTY(Variant::REAL, normalize_bias, set_normalize_bias, get_normalize_bias);
}

void LLightmap::set_mode(LLightmap::eMode p_mode) {m_LM.m_Settings_Mode = (LM::LightMapper::eLMMode) p_mode;}
LLightmap::eMode LLightmap::get_mode() const {return (LLightmap::eMode) m_LM.m_Settings_Mode;}

void LLightmap::set_bake_mode(LLightmap::eBakeMode p_mode) {m_LM.m_Settings_Mode = (LM::LightMapper::eLMMode) p_mode;}
LLightmap::eBakeMode LLightmap::get_bake_mode() const {return (LLightmap::eBakeMode) m_LM.m_Settings_Mode;}

void LLightmap::set_mesh_path(const NodePath &p_path) {m_LM.m_Settings_Path_Mesh = p_path;}
NodePath LLightmap::get_mesh_path() const {return m_LM.m_Settings_Path_Mesh;}
void LLightmap::set_lights_path(const NodePath &p_path)  {m_LM.m_Settings_Path_Lights = p_path;}
NodePath LLightmap::get_lights_path() const {return m_LM.m_Settings_Path_Lights;}

void LLightmap::set_forward_num_rays(int num_rays) {m_LM.m_Settings_Forward_NumRays = num_rays;}
int LLightmap::get_forward_num_rays() const {return m_LM.m_Settings_Forward_NumRays;}

void LLightmap::set_forward_num_bounces(int num_bounces) {m_LM.m_Settings_Forward_NumBounces = num_bounces;}
int LLightmap::get_forward_num_bounces() const {return m_LM.m_Settings_Forward_NumBounces;}

void LLightmap::set_forward_ray_power(float ray_power) {m_LM.m_Settings_Forward_RayPower = ray_power;}
float LLightmap::get_forward_ray_power() const {return m_LM.m_Settings_Forward_RayPower;}

void LLightmap::set_forward_bounce_power(float bounce_power) {m_LM.m_Settings_Forward_BouncePower = bounce_power;}
float LLightmap::get_forward_bounce_power() const {return m_LM.m_Settings_Forward_BouncePower;}

void LLightmap::set_forward_bounce_directionality(float bounce_dir) {m_LM.m_Settings_Forward_BounceDirectionality = bounce_dir;}
float LLightmap::get_forward_bounce_directionality() const {return m_LM.m_Settings_Forward_BounceDirectionality;}

////////////////////////////
void LLightmap::set_backward_num_rays(int num_rays) {m_LM.m_Settings_Backward_NumRays = num_rays;}
int LLightmap::get_backward_num_rays() const {return m_LM.m_Settings_Backward_NumRays;}

void LLightmap::set_backward_num_bounce_rays(int num_rays) {m_LM.m_Settings_Backward_NumBounceRays = num_rays;}
int LLightmap::get_backward_num_bounce_rays() const {return m_LM.m_Settings_Backward_NumBounceRays;}

void LLightmap::set_backward_num_bounces(int num_bounces) {m_LM.m_Settings_Backward_NumBounces = num_bounces;}
int LLightmap::get_backward_num_bounces() const {return m_LM.m_Settings_Backward_NumBounces;}

void LLightmap::set_backward_ray_power(float ray_power) {m_LM.m_Settings_Backward_RayPower = ray_power;}
float LLightmap::get_backward_ray_power() const {return m_LM.m_Settings_Backward_RayPower;}

void LLightmap::set_backward_bounce_power(float bounce_power) {m_LM.m_Settings_Backward_BouncePower = bounce_power;}
float LLightmap::get_backward_bounce_power() const {return m_LM.m_Settings_Backward_BouncePower;}
////////////////////////////

void LLightmap::set_ao_range(float ao_range) {m_LM.m_Settings_AO_Range = ao_range;}
float LLightmap::get_ao_range() const {return m_LM.m_Settings_AO_Range;}

void LLightmap::set_ao_cut_range(float ao_cut_range) {m_LM.m_Settings_AO_CutRange = ao_cut_range;}
float LLightmap::get_ao_cut_range() const {return m_LM.m_Settings_AO_CutRange;}

void LLightmap::set_ao_num_samples(int ao_num_samples) {m_LM.m_Settings_AO_Samples = ao_num_samples;}
int LLightmap::get_ao_num_samples() const {return m_LM.m_Settings_AO_Samples;}

////////////////////////////

void LLightmap::set_tex_width(int width) {m_LM.m_Settings_TexWidth = width;}
int LLightmap::get_tex_width() const {return m_LM.m_Settings_TexWidth;}

void LLightmap::set_tex_height(int height) {m_LM.m_Settings_TexHeight = height;}
int LLightmap::get_tex_height() const {return m_LM.m_Settings_TexHeight;}

void LLightmap::set_voxel_dims(const Vector3 &dims) {m_LM.m_Settings_VoxelDims.Set(dims);}
Vector3 LLightmap::get_voxel_dims() const {Vector3 p; m_LM.m_Settings_VoxelDims.To(p); return p;}

void LLightmap::set_surface_bias(float bias) {m_LM.m_Settings_SurfaceBias = bias;}
float LLightmap::get_surface_bias() const {return m_LM.m_Settings_SurfaceBias;}

void LLightmap::set_normalize(bool norm) {m_LM.m_Settings_Normalize = norm;}
bool LLightmap::get_normalize() const {return m_LM.m_Settings_Normalize;}

void LLightmap::set_normalize_bias(float bias) {m_LM.m_Settings_NormalizeBias = bias;}
float LLightmap::get_normalize_bias() const {return m_LM.m_Settings_NormalizeBias;}

#define LLIGHTMAP_IMPLEMENT_SETGET_FILENAME(SET_FUNC_NAME, GET_FUNC_NAME, SETTING, SETTING_HDR) void LLightmap::SET_FUNC_NAME(const String &p_filename)\
{\
m_LM.SETTING = p_filename;\
if (p_filename.get_extension() == "exr")\
{m_LM.SETTING_HDR = true;}\
else\
{m_LM.SETTING_HDR = false;}\
}\
String LLightmap::GET_FUNC_NAME() const {return m_LM.SETTING;}


LLIGHTMAP_IMPLEMENT_SETGET_FILENAME(set_lightmap_filename, get_lightmap_filename, m_Settings_LightmapFilename, m_Settings_LightmapIsHDR)
LLIGHTMAP_IMPLEMENT_SETGET_FILENAME(set_ao_filename, get_ao_filename, m_Settings_AmbientFilename, m_Settings_AmbientIsHDR)
LLIGHTMAP_IMPLEMENT_SETGET_FILENAME(set_combined_filename, get_combined_filename, m_Settings_CombinedFilename, m_Settings_CombinedIsHDR)


#undef LLIGHTMAP_IMPLEMENT_SETGET_FILENAME
//void LLightmap::set_lightmap_filename(const String &p_filename)
//{
//	m_LM.m_Settings_LightmapFilename = p_filename;
//	// HDR?
//	if (p_filename.get_extension() == "exr")
//		m_LM.m_Settings_LightmapIsHDR = true;
//	else
//		m_LM.m_Settings_LightmapIsHDR = false;
//}
//String LLightmap::get_lightmap_filename() const {return m_LM.m_Settings_LightmapFilename;}



bool LLightmap::lightmap_bake()
{
	if (m_LM.m_Settings_LightmapFilename == "")
		return false;

	// bake to a file
	Ref<Image> image_lightmap;
	Ref<Image> image_ao;
	Ref<Image> image_combined;

	int w = m_LM.m_Settings_TexWidth;
	int h = m_LM.m_Settings_TexHeight;

	// create either low or HDR images
	if (m_LM.m_Settings_LightmapIsHDR)
	{
		Ref<Image> image = memnew(Image(w, h, false, Image::FORMAT_RGBAF));
		image_lightmap = image;
	}
	else
	{
		Ref<Image> image = memnew(Image(w, h, false, Image::FORMAT_RGBA8));
		image_lightmap = image;
	}

	if (m_LM.m_Settings_AmbientIsHDR)
	{
		Ref<Image> image = memnew(Image(w, h, false, Image::FORMAT_RF));
		image_ao = image;
	}
	else
	{
		Ref<Image> image = memnew(Image(w, h, false, Image::FORMAT_L8));
		image_ao = image;
	}

	if (m_LM.m_Settings_CombinedIsHDR)
	{
		Ref<Image> image = memnew(Image(w, h, false, Image::FORMAT_RGBAF));
		image_combined = image;
	}
	else
	{
		Ref<Image> image = memnew(Image(w, h, false, Image::FORMAT_RGBA8));
		image_combined = image;
	}

	lightmap_bake_to_image(image_lightmap.ptr());

	// save the images, png or exr
	if (m_LM.m_Settings_LightmapIsHDR)
	{
		String szGlobalPath = ProjectSettings::get_singleton()->globalize_path(m_LM.m_Settings_LightmapFilename);
		image_lightmap->save_exr(szGlobalPath, false);
	}
	else
	{
		image_lightmap->save_png(m_LM.m_Settings_LightmapFilename);
	}

	ResourceLoader::import(m_LM.m_Settings_LightmapFilename);

	return true;
}

bool LLightmap::lightmap_bake_to_image(Object * pOutputImage)
{
	// get the mesh instance and light root
	if (!has_node(m_LM.m_Settings_Path_Mesh))
	{
		WARN_PRINT("lightmap_bake : mesh path is invalid");
		return false;
	}

	MeshInstance * pMeshInstance = Object::cast_to<MeshInstance>(get_node(m_LM.m_Settings_Path_Mesh));
	if (!pMeshInstance)
	{
		WARN_PRINT("lightmap_bake : mesh path is not a mesh instance");
		return false;
	}

	if (!has_node(m_LM.m_Settings_Path_Lights))
	{
		WARN_PRINT("lightmap_bake : lights path is invalid");
		return false;
	}

	Node * pLightRoot = Object::cast_to<Node>(get_node(m_LM.m_Settings_Path_Lights));
	if (!pLightRoot)
	{
		WARN_PRINT("lightmap_bake : lights path is not a node");
		return false;
	}

	return lightmap_mesh(pMeshInstance, pLightRoot, pOutputImage);
}


bool LLightmap::lightmap_mesh(Node * pMeshInstance, Node * pLightRoot, Object * pOutputImage)
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

	return m_LM.lightmap_mesh(pMI, pLR, pIm);
}
