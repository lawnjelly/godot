#include "gdlightmapper.h"

#define LIGHTMAP_STRINGIFY(x) #x
#define LIGHTMAP_TOSTRING(x) LIGHTMAP_STRINGIFY(x)


void LLightMapper::_bind_methods()
{
	BIND_ENUM_CONSTANT(LLightMapper::MODE_FORWARD);
	BIND_ENUM_CONSTANT(LLightMapper::MODE_BACKWARD);

	// main functions
	ClassDB::bind_method(D_METHOD("lightmap_bake"), &LLightMapper::lightmap_bake);
	ClassDB::bind_method(D_METHOD("lightmap_bake_to_image", "output_image"), &LLightMapper::lightmap_bake_to_image);


	ClassDB::bind_method(D_METHOD("set_mode", "mode"), &LLightMapper::set_mode);
	ClassDB::bind_method(D_METHOD("get_mode"), &LLightMapper::get_mode);

	ClassDB::bind_method(D_METHOD("set_image_filename", "filename"), &LLightMapper::set_image_filename);
	ClassDB::bind_method(D_METHOD("get_image_filename"), &LLightMapper::get_image_filename);


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
}

void LLightMapper::set_mode(LLightMapper::eMode p_mode) {m_LM.m_Settings_Mode = (LM::LightMapper::eLMMode) p_mode;}
LLightMapper::eMode LLightMapper::get_mode() const {return (LLightMapper::eMode) m_LM.m_Settings_Mode;}

void LLightMapper::set_mesh_path(const NodePath &p_path) {m_LM.m_Settings_Path_Mesh = p_path;}
NodePath LLightMapper::get_mesh_path() const {return m_LM.m_Settings_Path_Mesh;}
void LLightMapper::set_lights_path(const NodePath &p_path)  {m_LM.m_Settings_Path_Lights = p_path;}
NodePath LLightMapper::get_lights_path() const {return m_LM.m_Settings_Path_Lights;}

void LLightMapper::set_forward_num_rays(int num_rays) {m_LM.m_Settings_Forward_NumRays = num_rays;}
int LLightMapper::get_forward_num_rays() const {return m_LM.m_Settings_Forward_NumRays;}

void LLightMapper::set_forward_num_bounces(int num_bounces) {m_LM.m_Settings_Forward_NumBounces = num_bounces;}
int LLightMapper::get_forward_num_bounces() const {return m_LM.m_Settings_Forward_NumBounces;}

void LLightMapper::set_forward_ray_power(float ray_power) {m_LM.m_Settings_Forward_RayPower = ray_power;}
float LLightMapper::get_forward_ray_power() const {return m_LM.m_Settings_Forward_RayPower;}

void LLightMapper::set_forward_bounce_power(float bounce_power) {m_LM.m_Settings_Forward_BouncePower = bounce_power;}
float LLightMapper::get_forward_bounce_power() const {return m_LM.m_Settings_Forward_BouncePower;}

void LLightMapper::set_forward_bounce_directionality(float bounce_dir) {m_LM.m_Settings_Forward_BounceDirectionality = bounce_dir;}
float LLightMapper::get_forward_bounce_directionality() const {return m_LM.m_Settings_Forward_BounceDirectionality;}

////////////////////////////
void LLightMapper::set_backward_num_rays(int num_rays) {m_LM.m_Settings_Backward_NumRays = num_rays;}
int LLightMapper::get_backward_num_rays() const {return m_LM.m_Settings_Backward_NumRays;}

void LLightMapper::set_backward_num_bounce_rays(int num_rays) {m_LM.m_Settings_Backward_NumBounceRays = num_rays;}
int LLightMapper::get_backward_num_bounce_rays() const {return m_LM.m_Settings_Backward_NumBounceRays;}

void LLightMapper::set_backward_num_bounces(int num_bounces) {m_LM.m_Settings_Backward_NumBounces = num_bounces;}
int LLightMapper::get_backward_num_bounces() const {return m_LM.m_Settings_Backward_NumBounces;}

void LLightMapper::set_backward_ray_power(float ray_power) {m_LM.m_Settings_Backward_RayPower = ray_power;}
float LLightMapper::get_backward_ray_power() const {return m_LM.m_Settings_Backward_RayPower;}

void LLightMapper::set_backward_bounce_power(float bounce_power) {m_LM.m_Settings_Backward_BouncePower = bounce_power;}
float LLightMapper::get_backward_bounce_power() const {return m_LM.m_Settings_Backward_BouncePower;}
////////////////////////////

void LLightMapper::set_tex_width(int width) {m_LM.m_Settings_TexWidth = width;}
int LLightMapper::get_tex_width() const {return m_LM.m_Settings_TexWidth;}

void LLightMapper::set_tex_height(int height) {m_LM.m_Settings_TexHeight = height;}
int LLightMapper::get_tex_height() const {return m_LM.m_Settings_TexHeight;}

void LLightMapper::set_voxel_dims(const Vector3 &dims) {m_LM.m_Settings_VoxelDims.Set(dims);}
Vector3 LLightMapper::get_voxel_dims() const {Vector3 p; m_LM.m_Settings_VoxelDims.To(p); return p;}

void LLightMapper::set_normalize(bool norm) {m_LM.m_Settings_Normalize = norm;}
bool LLightMapper::get_normalize() const {return m_LM.m_Settings_Normalize;}

void LLightMapper::set_normalize_bias(float bias) {m_LM.m_Settings_NormalizeBias = bias;}
float LLightMapper::get_normalize_bias() const {return m_LM.m_Settings_NormalizeBias;}

void LLightMapper::set_image_filename(const String &p_filename) {m_LM.m_Settings_ImageFilename = p_filename;}
String LLightMapper::get_image_filename() const {return m_LM.m_Settings_ImageFilename;}


bool LLightMapper::lightmap_bake()
{
	if (m_LM.m_Settings_ImageFilename == "")
		return false;

	// bake to a file
	Ref<Image> image = memnew(Image(m_LM.m_Settings_TexWidth, m_LM.m_Settings_TexHeight, false, Image::FORMAT_RGBA8));

	lightmap_bake_to_image(image.ptr());

	// save the image
	image->save_png(m_LM.m_Settings_ImageFilename);

	ResourceLoader::import(m_LM.m_Settings_ImageFilename);

	return true;
}

bool LLightMapper::lightmap_bake_to_image(Object * pOutputImage)
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

	return m_LM.lightmap_mesh(pMI, pLR, pIm);
}
