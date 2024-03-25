#include "gdlightmapper.h"
#include "core/os/os.h"

#define LIGHTMAP_STRINGIFY(x) #x
#define LIGHTMAP_TOSTRING(x) LIGHTMAP_STRINGIFY(x)

void LLightmap::_bind_methods() {
	BIND_ENUM_CONSTANT(LLightmap::MODE_FORWARD);
	BIND_ENUM_CONSTANT(LLightmap::MODE_BACKWARD);

	BIND_ENUM_CONSTANT(LLightmap::BAKEMODE_UVMAP);
	BIND_ENUM_CONSTANT(LLightmap::BAKEMODE_LIGHTMAP);
	BIND_ENUM_CONSTANT(LLightmap::BAKEMODE_AO);
	BIND_ENUM_CONSTANT(LLightmap::BAKEMODE_MERGE);
	BIND_ENUM_CONSTANT(LLightmap::BAKEMODE_PROBES);
	BIND_ENUM_CONSTANT(LLightmap::BAKEMODE_COMBINED);

	BIND_ENUM_CONSTANT(LLightmap::QUALITY_LOW);
	BIND_ENUM_CONSTANT(LLightmap::QUALITY_MEDIUM);
	BIND_ENUM_CONSTANT(LLightmap::QUALITY_HIGH);
	BIND_ENUM_CONSTANT(LLightmap::QUALITY_FINAL);

	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_TEX_WIDTH);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_TEX_HEIGHT);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_MAX_LIGHT_DISTANCE);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_SURFACE_BIAS);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_MATERIAL_SIZE);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_VOXEL_DENSITY);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_NUM_PRIMARY_RAYS);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_NUM_BOUNCES);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_BOUNCE_POWER);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_ROUGHNESS);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_NUM_AMBIENT_BOUNCES);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_NUM_AMBIENT_BOUNCE_RAYS);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_AMBIENT_BOUNCE_POWER);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_EMISSION_ENABLED);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_EMISSION_DENSITY);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_GLOW);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_AO_NUM_SAMPLES);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_AO_RANGE);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_SKY_SIZE);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_SKY_SAMPLES);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_SKY_BLUR);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_SKY_BRIGHTNESS);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_NORMALIZE);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_NORMALIZE_MULTIPLIER);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_AO_LIGHT_RATIO);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_GAMMA);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_DILATE_ENABLED);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_NOISE_REDUCTION);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_NOISE_THRESHOLD);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_SEAM_STITCHING_ENABLED);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_VISUALIZE_SEAMS_ENABLED);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_SEAM_DISTANCE_THRESHOLD);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_SEAM_NORMAL_THRESHOLD);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_PROBE_DENSITY);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_PROBE_SAMPLES);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_UV_PADDING);

	ClassDB::bind_method(D_METHOD("set_param", "param", "value"), &LLightmap::set_param);
	ClassDB::bind_method(D_METHOD("get_param", "param"), &LLightmap::get_param);

	// main functions
	ClassDB::bind_method(D_METHOD("lightmap_bake"), &LLightmap::lightmap_bake);
	ClassDB::bind_method(D_METHOD("lightmap_bake_to_image", "output_image"), &LLightmap::lightmap_bake_to_image);

	ClassDB::bind_method(D_METHOD("set_mode", "mode"), &LLightmap::set_mode);
	ClassDB::bind_method(D_METHOD("get_mode"), &LLightmap::get_mode);

	ClassDB::bind_method(D_METHOD("set_bake_mode", "bake_mode"), &LLightmap::set_bake_mode);
	ClassDB::bind_method(D_METHOD("get_bake_mode"), &LLightmap::get_bake_mode);

	ClassDB::bind_method(D_METHOD("set_quality", "quality"), &LLightmap::set_quality);
	ClassDB::bind_method(D_METHOD("get_quality"), &LLightmap::get_quality);

	ClassDB::bind_method(D_METHOD("set_lightmap_filename", "lightmap_filename"), &LLightmap::set_lightmap_filename);
	ClassDB::bind_method(D_METHOD("get_lightmap_filename"), &LLightmap::get_lightmap_filename);
	ClassDB::bind_method(D_METHOD("set_ao_filename", "ao_filename"), &LLightmap::set_ao_filename);
	ClassDB::bind_method(D_METHOD("get_ao_filename"), &LLightmap::get_ao_filename);
	ClassDB::bind_method(D_METHOD("set_combined_filename", "combined_filename"), &LLightmap::set_combined_filename);
	ClassDB::bind_method(D_METHOD("get_combined_filename"), &LLightmap::get_combined_filename);

	ClassDB::bind_method(D_METHOD("set_uv_filename", "uv_filename"), &LLightmap::set_uv_filename);
	ClassDB::bind_method(D_METHOD("get_uv_filename"), &LLightmap::get_uv_filename);

	ClassDB::bind_method(D_METHOD("set_noise_reduction_method", "method"), &LLightmap::set_noise_reduction_method);
	ClassDB::bind_method(D_METHOD("get_noise_reduction_method"), &LLightmap::get_noise_reduction_method);

	ClassDB::bind_method(D_METHOD("set_sky_filename", "sky_filename"), &LLightmap::set_sky_filename);
	ClassDB::bind_method(D_METHOD("get_sky_filename"), &LLightmap::get_sky_filename);

	//	ClassDB::bind_method(D_METHOD("set_probe_filename", "probe_filename"), &LLightmap::set_probe_filename);
	//	ClassDB::bind_method(D_METHOD("get_probe_filename"), &LLightmap::get_probe_filename);

#define LIMPL_PROPERTY(P_TYPE, P_NAME, P_SET, P_GET)                                                        \
	ClassDB::bind_method(D_METHOD(LIGHTMAP_TOSTRING(P_SET), LIGHTMAP_TOSTRING(P_NAME)), &LLightmap::P_SET); \
	ClassDB::bind_method(D_METHOD(LIGHTMAP_TOSTRING(P_GET)), &LLightmap::P_GET);                            \
	ADD_PROPERTY(PropertyInfo(P_TYPE, LIGHTMAP_TOSTRING(P_NAME)), LIGHTMAP_TOSTRING(P_SET), LIGHTMAP_TOSTRING(P_GET));

#define LIMPL_PROPERTY_RANGE(P_TYPE, P_NAME, P_SET, P_GET, P_RANGE_STRING)                                  \
	ClassDB::bind_method(D_METHOD(LIGHTMAP_TOSTRING(P_SET), LIGHTMAP_TOSTRING(P_NAME)), &LLightmap::P_SET); \
	ClassDB::bind_method(D_METHOD(LIGHTMAP_TOSTRING(P_GET)), &LLightmap::P_GET);                            \
	ADD_PROPERTY(PropertyInfo(P_TYPE, LIGHTMAP_TOSTRING(P_NAME), PROPERTY_HINT_RANGE, P_RANGE_STRING), LIGHTMAP_TOSTRING(P_SET), LIGHTMAP_TOSTRING(P_GET));

#define LIMPL_PROPERTY_PARAM_RANGE(P_TYPE, P_NAME, P_RANGE_STRING, P_INDEX) \
	ADD_PROPERTYI(PropertyInfo(P_TYPE, LIGHTMAP_TOSTRING(P_NAME), PROPERTY_HINT_RANGE, P_RANGE_STRING), "set_param", "get_param", P_INDEX);

#define LIMPL_PROPERTY_PARAM(P_TYPE, P_NAME, P_INDEX) \
	ADD_PROPERTYI(PropertyInfo(P_TYPE, LIGHTMAP_TOSTRING(P_NAME)), "set_param", "get_param", P_INDEX)

	//	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "light_specular", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_param", "get_param", PARAM_SPECULAR);

	ADD_GROUP("Main", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "bake_mode", PROPERTY_HINT_ENUM, "UVMap,Lightmap,AO,Merge,LightProbes,Combined"), "set_bake_mode", "get_bake_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Forward,Backward"), "set_mode", "get_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "quality", PROPERTY_HINT_ENUM, "Low,Medium,High,Final"), "set_quality", "get_quality");
	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, max_light_distance, "0,999999,1", LM::LightMapper::PARAM_MAX_LIGHT_DISTANCE);

	ADD_GROUP("Paths", "");
	LIMPL_PROPERTY(Variant::NODE_PATH, meshes, set_mesh_path, get_mesh_path);
	LIMPL_PROPERTY(Variant::NODE_PATH, lights, set_lights_path, get_lights_path);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "lightmap_filename", PROPERTY_HINT_SAVE_FILE, "*.exr"), "set_lightmap_filename", "get_lightmap_filename");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "ao_filename", PROPERTY_HINT_SAVE_FILE, "*.exr"), "set_ao_filename", "get_ao_filename");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "combined_filename", PROPERTY_HINT_SAVE_FILE, "*.png,*.exr"), "set_combined_filename", "get_combined_filename");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "uv_filename", PROPERTY_HINT_SAVE_FILE, "*.tscn"), "set_uv_filename", "get_uv_filename");

	ADD_GROUP("Size", "");

	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, tex_width, "128,8192,128", LM::LightMapper::PARAM_TEX_WIDTH);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, tex_height, "128,8192,128", LM::LightMapper::PARAM_TEX_HEIGHT);

	//	LIMPL_PROPERTY(Variant::VECTOR3, voxel_grid, set_voxel_dims, get_voxel_dims);
	LIMPL_PROPERTY(Variant::REAL, surface_bias, set_surface_bias, get_surface_bias);
	LIMPL_PROPERTY_RANGE(Variant::INT, material_size, set_material_size, get_material_size, "128,2048,128");
	LIMPL_PROPERTY_RANGE(Variant::INT, voxel_density, set_voxel_density, get_voxel_density, "1,512,1");

	ADD_GROUP("Common", "");
	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, primary_rays, "1,4096,1", LM::LightMapper::PARAM_NUM_PRIMARY_RAYS);

	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, num_bounces, "0,16,1", LM::LightMapper::PARAM_NUM_BOUNCES);
	LIMPL_PROPERTY_RANGE(Variant::REAL, bounce_power, set_bounce_power, get_bounce_power, "0.0,8.0,0.05");
	LIMPL_PROPERTY_PARAM_RANGE(Variant::REAL, roughness, "0.0,1.0,0.05", LM::LightMapper::PARAM_ROUGHNESS);

	ADD_GROUP("Ambient", "");

	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, a_bounces, "0,16,1", LM::LightMapper::PARAM_NUM_AMBIENT_BOUNCES);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, a_bounce_samples, "0, 1024, 1", LM::LightMapper::PARAM_NUM_AMBIENT_BOUNCE_RAYS);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::REAL, a_bounce_power, "0.0, 1.0", LM::LightMapper::PARAM_AMBIENT_BOUNCE_POWER);

	ADD_GROUP("Emission", "");
	LIMPL_PROPERTY_PARAM(Variant::BOOL, emission_enabled, LM::LightMapper::PARAM_EMISSION_ENABLED);

	LIMPL_PROPERTY_RANGE(Variant::REAL, emission_density, set_emission_density, get_emission_density, "0.0,8.0,0.05");
	LIMPL_PROPERTY_RANGE(Variant::REAL, glow, set_glow, get_glow, "0.0,16.0,0.05");

	//	ADD_GROUP("Forward Parameters", "");
	//LIMPL_PROPERTY_RANGE(Variant::REAL, f_ray_power, set_forward_ray_power, get_forward_ray_power, "0.0,0.1,0.01");

	//	ADD_GROUP("Backward Parameters", "");
	//	LIMPL_PROPERTY(Variant::INT, b_initial_rays, set_backward_num_rays, get_backward_num_rays);
	//LIMPL_PROPERTY(Variant::REAL, b_ray_power, set_backward_ray_power, get_backward_ray_power);

	ADD_GROUP("Ambient Occlusion", "");
	LIMPL_PROPERTY_RANGE(Variant::INT, ao_samples, set_ao_num_samples, get_ao_num_samples, "1,2048,1");
	LIMPL_PROPERTY(Variant::REAL, ao_range, set_ao_range, get_ao_range);
	//	LIMPL_PROPERTY(Variant::REAL, ao_cut_range, set_ao_cut_range, get_ao_cut_range);

	ADD_GROUP("Sky", "");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "sky_filename", PROPERTY_HINT_FILE, "*.exr,*.png,*.jpg"), "set_sky_filename", "get_sky_filename");
	LIMPL_PROPERTY_RANGE(Variant::INT, sky_size, set_sky_size, get_sky_size, "64,2048,64");
	LIMPL_PROPERTY_RANGE(Variant::INT, sky_samples, set_sky_samples, get_sky_samples, "128,8192,128");
	LIMPL_PROPERTY_RANGE(Variant::REAL, sky_blur, set_sky_blur, get_sky_blur, "0.0,0.5,0.01");
	LIMPL_PROPERTY_RANGE(Variant::REAL, sky_brightness, set_sky_brightness, get_sky_brightness, "0.0,4.0,0.01");

	ADD_GROUP("Dynamic Range", "");
	//	LIMPL_PROPERTY(Variant::BOOL, normalize, set_normalize, get_normalize);
	LIMPL_PROPERTY(Variant::REAL, normalize_multiplier, set_normalize_multiplier, get_normalize_multiplier);
	LIMPL_PROPERTY_RANGE(Variant::REAL, ao_light_ratio, set_light_ao_ratio, get_light_ao_ratio, "0.0,1.0,0.01");
	LIMPL_PROPERTY_RANGE(Variant::REAL, gamma, set_gamma, get_gamma, "0.01,10.0,0.01");

	ADD_GROUP("Post Processing", "");
	LIMPL_PROPERTY_PARAM(Variant::BOOL, dilate, LM::LightMapper::PARAM_DILATE_ENABLED);
	//LIMPL_PROPERTY(Variant::BOOL, dilate, set_dilate, get_dilate);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "noise method", PROPERTY_HINT_ENUM, "Disabled,Simple,Advanced"), "set_noise_reduction_method", "get_noise_reduction_method");
	LIMPL_PROPERTY_RANGE(Variant::REAL, noise_reduction, set_noise_reduction, get_noise_reduction, "0.0,1.0,0.01");
	LIMPL_PROPERTY_RANGE(Variant::REAL, noise_threshold, set_noise_threshold, get_noise_threshold, "0.0,1.0,0.01");
	LIMPL_PROPERTY(Variant::BOOL, seam_stitching, set_seam_stitching, get_seam_stitching);
	//LIMPL_PROPERTY(Variant::BOOL, visualize_seams, set_visualize_seams, get_visualize_seams);
	LIMPL_PROPERTY_PARAM(Variant::BOOL, visualize_seams, LM::LightMapper::PARAM_VISUALIZE_SEAMS_ENABLED);

	LIMPL_PROPERTY_RANGE(Variant::REAL, seam_distance_threshold, set_seam_distance_threshold, get_seam_distance_threshold, "0.0,0.01,0.0001");
	LIMPL_PROPERTY_RANGE(Variant::REAL, seam_normal_threshold, set_seam_normal_threshold, get_seam_normal_threshold, "0.0,180.0,1.0");

	ADD_GROUP("Light Probes", "");
	LIMPL_PROPERTY_RANGE(Variant::INT, probe_density, set_probe_density, get_probe_density, "1,512,1");
	LIMPL_PROPERTY_RANGE(Variant::INT, probe_samples, set_probe_samples, get_probe_samples, "512,4096*8,512");
	//	ADD_PROPERTY(PropertyInfo(Variant::STRING, "probe_filename", PROPERTY_HINT_SAVE_FILE, "*.probe"), "set_probe_filename", "get_probe_filename");

	ADD_GROUP("UV Unwrap", "");
	LIMPL_PROPERTY_RANGE(Variant::INT, uv_padding, set_uv_padding, get_uv_padding, "0,256,1");

#undef LIMPL_PROPERTY
#undef LIMPL_PROPERTY_RANGE
}

void LLightmap::set_mode(LLightmap::eMode p_mode) {
	m_LM.settings.mode = (LM::LightMapper::eLMMode)p_mode;
}
LLightmap::eMode LLightmap::get_mode() const {
	return (LLightmap::eMode)m_LM.settings.mode;
}

void LLightmap::set_bake_mode(LLightmap::eBakeMode p_mode) {
	m_LM.settings.bake_mode = (LM::LightMapper::eLMBakeMode)p_mode;
}
LLightmap::eBakeMode LLightmap::get_bake_mode() const {
	return (LLightmap::eBakeMode)m_LM.settings.bake_mode;
}

void LLightmap::set_quality(LLightmap::eQuality p_quality) {
	m_LM.settings.quality = (LM::LightMapper::eLMBakeQuality)p_quality;
}
LLightmap::eQuality LLightmap::get_quality() const {
	return (LLightmap::eQuality)m_LM.settings.quality;
}

//void LLightmap::set_max_light_distance(int dist) {
//	m_LM.settings.max_light_dist = dist;
//}

//int LLightmap::get_max_light_distance() const {
//	return m_LM.settings.max_light_dist;
//}

void LLightmap::set_mesh_path(const NodePath &p_path) {
	m_LM.settings.path_mesh = p_path;
}
NodePath LLightmap::get_mesh_path() const {
	return m_LM.settings.path_mesh;
}
void LLightmap::set_lights_path(const NodePath &p_path) {
	m_LM.settings.path_lights = p_path;
}
NodePath LLightmap::get_lights_path() const {
	return m_LM.settings.path_lights;
}

void LLightmap::set_bounce_power(float bounce_power) {
	m_LM.settings.directional_bounce_power = bounce_power;
}
float LLightmap::get_bounce_power() const {
	return m_LM.settings.directional_bounce_power;
}

//void LLightmap::set_roughness(float roughness) {
//	m_LM.settings.smoothness = 1.0f - roughness;
//}
//float LLightmap::get_roughness() const {
//	return 1.0f - m_LM.settings.smoothness;
//}

void LLightmap::set_emission_density(float density) {
	m_LM.settings.emission_density = density;
}
float LLightmap::get_emission_density() const {
	return m_LM.settings.emission_density;
}

void LLightmap::set_glow(float glow) {
	m_LM.settings.glow = glow;
}
float LLightmap::get_glow() const {
	return m_LM.settings.glow;
}

////////////////////////////
void LLightmap::set_backward_num_rays(int num_rays) {
	m_LM.settings.backward_num_rays = num_rays;
}
int LLightmap::get_backward_num_rays() const {
	return m_LM.settings.backward_num_rays;
}

//void LLightmap::set_num_ambient_bounce_samples(int num_samples) {
//	m_LM.settings.num_ambient_bounce_rays = num_samples;
//}
//int LLightmap::get_num_ambient_bounce_samples() const {
//	return m_LM.settings.num_ambient_bounce_rays;
//}

//void LLightmap::set_backward_num_bounces(int num_bounces) {m_LM.settings.Backward_NumBounces = num_bounces;}
//int LLightmap::get_backward_num_bounces() const {return m_LM.settings.Backward_NumBounces;}

//void LLightmap::set_backward_ray_power(float ray_power) {m_LM.settings.Backward_RayPower = ray_power;}
//float LLightmap::get_backward_ray_power() const {return m_LM.settings.Backward_RayPower;}

//void LLightmap::set_ambient_bounce_power(float bounce_power) {
//	m_LM.settings.ambient_bounce_power = bounce_power;
//}
//float LLightmap::get_ambient_bounce_power() const {
//	return m_LM.settings.ambient_bounce_power;
//}
////////////////////////////

//void LLightmap::set_num_ambient_bounces(int num_bounces) {
//	m_LM.settings.num_ambient_bounces = num_bounces;
//}
//int LLightmap::get_num_ambient_bounces() const {
//	return m_LM.settings.num_ambient_bounces;
//}

void LLightmap::set_ao_range(float ao_range) {
	m_LM.settings.AO_range = ao_range;
}
float LLightmap::get_ao_range() const {
	return m_LM.settings.AO_range;
}

void LLightmap::set_ao_cut_range(float ao_cut_range) {
	m_LM.settings.AO_cut_range = ao_cut_range;
}
float LLightmap::get_ao_cut_range() const {
	return m_LM.settings.AO_cut_range;
}

void LLightmap::set_ao_num_samples(int ao_num_samples) {
	m_LM.settings.AO_samples = ao_num_samples;
}
int LLightmap::get_ao_num_samples() const {
	return m_LM.settings.AO_samples;
}

////////////////////////////

//void LLightmap::set_tex_width(int width) {
//	m_LM.settings.tex_width = width;
//}
//int LLightmap::get_tex_width() const {
//	return m_LM.settings.tex_width;
//}

//void LLightmap::set_tex_height(int height) {
//	m_LM.settings.tex_height = height;
//}
//int LLightmap::get_tex_height() const {
//	return m_LM.settings.tex_height;
//}

void LLightmap::set_material_size(int size) {
	m_LM.settings.max_material_size = size;
}
int LLightmap::get_material_size() const {
	return m_LM.settings.max_material_size;
}

void LLightmap::set_voxel_density(int density) {
	m_LM.settings.voxel_density = density;
}
int LLightmap::get_voxel_density() const {
	return m_LM.settings.voxel_density;
}

void LLightmap::set_surface_bias(float bias) {
	m_LM.settings.surface_bias = bias;
}
float LLightmap::get_surface_bias() const {
	return m_LM.settings.surface_bias;
}

//void LLightmap::set_normalize(bool norm) {
//	m_LM.settings.normalize = norm;
//}
//bool LLightmap::get_normalize() const {
//	return m_LM.settings.normalize;
//}

void LLightmap::set_normalize_multiplier(float bias) {
	m_LM.settings.normalize_bias = bias;
}
float LLightmap::get_normalize_multiplier() const {
	return m_LM.settings.normalize_bias;
}

void LLightmap::set_light_ao_ratio(float ratio) {
	m_LM.settings.light_AO_ratio = ratio;
}
float LLightmap::get_light_ao_ratio() const {
	return m_LM.settings.light_AO_ratio;
}

void LLightmap::set_gamma(float gamma) {
	m_LM.settings.gamma = gamma;
}
float LLightmap::get_gamma() const {
	return m_LM.settings.gamma;
}

void LLightmap::set_uv_filename(const String &p_filename) {
	m_LM.settings.UV_filename = p_filename;
}
String LLightmap::get_uv_filename() const {
	return m_LM.settings.UV_filename;
}

void LLightmap::set_uv_padding(int pad) {
	m_LM.settings.UV_padding = pad;
}
int LLightmap::get_uv_padding() const {
	return m_LM.settings.UV_padding;
}

// probes
void LLightmap::set_probe_density(int density) {
	m_LM.settings.probe_density = density;
}
int LLightmap::get_probe_density() const {
	return m_LM.settings.probe_density;
}

void LLightmap::set_probe_samples(int samples) {
	m_LM.settings.num_probe_samples = samples;
}
int LLightmap::get_probe_samples() const {
	return m_LM.settings.num_probe_samples;
}

void LLightmap::set_noise_reduction(float nr) {
	m_LM.settings.noise_reduction = nr;
}
float LLightmap::get_noise_reduction() const {
	return m_LM.settings.noise_reduction;
}

void LLightmap::set_noise_threshold(float threshold) {
	m_LM.settings.noise_threshold = threshold;
}
float LLightmap::get_noise_threshold() const {
	return m_LM.settings.noise_threshold;
}

void LLightmap::set_noise_reduction_method(int method) {
	m_LM.settings.noise_reduction_method = (LM::LightMapper_Base::eNRMethod)method;
}

int LLightmap::get_noise_reduction_method() const {
	return m_LM.settings.noise_reduction_method;
}

void LLightmap::set_seam_stitching(bool active) {
	m_LM.settings.use_seam_stitching = active;
}

bool LLightmap::get_seam_stitching() const {
	return m_LM.settings.use_seam_stitching;
}

void LLightmap::set_seam_distance_threshold(float threshold) {
	m_LM.settings.seam_distance_threshold = threshold;
}

float LLightmap::get_seam_distance_threshold() const {
	return m_LM.settings.seam_distance_threshold;
}

void LLightmap::set_seam_normal_threshold(float threshold) {
	m_LM.settings.seam_normal_threshold = threshold;
}

float LLightmap::get_seam_normal_threshold() const {
	return m_LM.settings.seam_normal_threshold;
}

//void LLightmap::set_visualize_seams(bool active) {
//	m_LM.settings.visualize_seams_enabled = active;
//}

//bool LLightmap::get_visualize_seams() const {
//	return m_LM.settings.visualize_seams_enabled;
//}

//void LLightmap::set_dilate(bool active) {
//	m_LM.settings.dilate_enabled = active;
//}

//bool LLightmap::get_dilate() const {
//	return m_LM.settings.dilate_enabled;
//}

void LLightmap::set_sky_filename(const String &p_filename) {
	m_LM.settings.sky_filename = p_filename;
}
String LLightmap::get_sky_filename() const {
	return m_LM.settings.sky_filename;
}

void LLightmap::set_sky_blur(float p_blur) {
	m_LM.settings.sky_blur_amount = p_blur;
}

float LLightmap::get_sky_blur() const {
	return m_LM.settings.sky_blur_amount;
}

void LLightmap::set_sky_size(int p_size) {
	m_LM.settings.sky_size = p_size;
}

int LLightmap::get_sky_size() const {
	return m_LM.settings.sky_size;
}

void LLightmap::set_sky_samples(int p_samples) {
	m_LM.settings.sky_num_samples = p_samples;
}

int LLightmap::get_sky_samples() const {
	return m_LM.settings.sky_num_samples;
}

void LLightmap::set_sky_brightness(float p_brightness) {
	m_LM.settings.sky_brightness = p_brightness;
}

float LLightmap::get_sky_brightness() const {
	return m_LM.settings.sky_brightness;
}

void LLightmap::set_param(LM::LightMapper::Param p_param, Variant p_value) {
	m_LM.set_param(p_param, p_value);
}

Variant LLightmap::get_param(LM::LightMapper::Param p_param) {
	return m_LM.get_param(p_param);
}

//void LLightmap::set_probe_filename(const String &p_filename) {m_LM.settings.ProbeFilename = p_filename;}
//String LLightmap::get_probe_filename() const {return m_LM.settings.ProbeFilename;}

#define LLIGHTMAP_IMPLEMENT_SETGET_FILENAME(SET_FUNC_NAME, GET_FUNC_NAME, SETTING, SETTING_HDR) \
	void LLightmap::SET_FUNC_NAME(const String &p_filename) {                                   \
		m_LM.SETTING = p_filename;                                                              \
		if (p_filename.get_extension() == "exr") {                                              \
			m_LM.SETTING_HDR = true;                                                            \
		} else {                                                                                \
			m_LM.SETTING_HDR = false;                                                           \
		}                                                                                       \
	}                                                                                           \
	String LLightmap::GET_FUNC_NAME() const { return m_LM.SETTING; }

LLIGHTMAP_IMPLEMENT_SETGET_FILENAME(set_lightmap_filename, get_lightmap_filename, settings.lightmap_filename, settings.lightmap_is_HDR)
LLIGHTMAP_IMPLEMENT_SETGET_FILENAME(set_ao_filename, get_ao_filename, settings.ambient_filename, settings.ambient_is_HDR)
//LLIGHTMAP_IMPLEMENT_SETGET_FILENAME(set_combined_filename, get_combined_filename, settings.CombinedFilename, settings.CombinedIsHDR)

void LLightmap::set_combined_filename(const String &p_filename) {
	String new_filename = p_filename;
	String ext = new_filename.get_extension();

	// no extension? default to png
	if (ext == "")
		new_filename += ".png";

	m_LM.settings.combined_filename = new_filename;

	if (ext == "exr") {
		m_LM.settings.combined_is_HDR = true;
	} else {
		m_LM.settings.combined_is_HDR = false;
	}
}

String LLightmap::get_combined_filename() const {
	return m_LM.settings.combined_filename;
}

#undef LLIGHTMAP_IMPLEMENT_SETGET_FILENAME

//void LLightmap::ShowWarning(String sz, bool bAlert)
//{
//#ifdef TOOLS_ENABLED
//	EditorNode::get_singleton()->show_warning(TTR(sz));

//	if (bAlert)
//		OS::get_singleton()->alert(sz, "WARNING");
//#else
//	WARN_PRINT(sz);
//#endif
//}

bool LLightmap::uvmap() {
	if (!has_node(m_LM.settings.path_mesh)) {
		ShowWarning("Meshes nodepath is invalid.");
		return false;
	}

	Spatial *pRoot = Object::cast_to<Spatial>(get_node(m_LM.settings.path_mesh));
	if (!pRoot) {
		ShowWarning("Meshes nodepath is not a spatial.");
		return false;
	}

#ifndef TOOLS_ENABLED
	ShowWarning("UVMapping only possible in TOOLS build.");
	return false;
#else
	return m_LM.uv_map_meshes(pRoot);
#endif
}

bool LLightmap::lightmap_bake() {
	if (m_LM.settings.bake_mode == LM::LightMapper_Base::LMBAKEMODE_UVMAP) {
		return uvmap();
	}

	if (m_LM.settings.lightmap_filename == "")
		return false;
	if (m_LM.settings.combined_filename == "")
		return false;

	// bake to a file
	Ref<Image> image_lightmap;
	Ref<Image> image_ao;
	Ref<Image> image_combined;

	int w = m_LM.settings.tex_width;
	int h = m_LM.settings.tex_height;

	// create either low or HDR images
	if (m_LM.settings.lightmap_is_HDR) {
		Ref<Image> image = memnew(Image(w, h, false, Image::FORMAT_RGBAF));
		image_lightmap = image;
	} else {
		Ref<Image> image = memnew(Image(w, h, false, Image::FORMAT_RGBA8));
		image_lightmap = image;
	}

	if (m_LM.settings.ambient_is_HDR) {
		Ref<Image> image = memnew(Image(w, h, false, Image::FORMAT_RF));
		image_ao = image;
	} else {
		Ref<Image> image = memnew(Image(w, h, false, Image::FORMAT_L8));
		image_ao = image;
	}

	if (m_LM.settings.combined_is_HDR) {
		Ref<Image> image = memnew(Image(w, h, false, Image::FORMAT_RGBAF));
		image_combined = image;
	} else {
		Ref<Image> image = memnew(Image(w, h, false, Image::FORMAT_RGBA8));
		image_combined = image;
	}

	lightmap_bake_to_image(image_lightmap.ptr(), image_ao.ptr(), image_combined.ptr());

	// save the images, png or exr
	if (m_LM.logic.process_lightmap) {
		if (m_LM.settings.lightmap_is_HDR) {
			String szGlobalPath = ProjectSettings::get_singleton()->globalize_path(m_LM.settings.lightmap_filename);
			print_line("saving lights EXR .. global path : " + szGlobalPath);
			Error err = image_lightmap->save_exr(szGlobalPath, false);

			if (err != OK)
				OS::get_singleton()->alert("Error writing EXR file. Does this folder exist?\n\n" + m_LM.settings.lightmap_filename, "WARNING");
		} else {
			image_lightmap->save_png(m_LM.settings.lightmap_filename);
		}
	}

	if (m_LM.logic.process_AO) {
		if (m_LM.settings.ambient_is_HDR) {
			String szGlobalPath = ProjectSettings::get_singleton()->globalize_path(m_LM.settings.ambient_filename);
			print_line("saving ao EXR .. global path : " + szGlobalPath);
			Error err = image_ao->save_exr(szGlobalPath, false);

			if (err != OK)
				OS::get_singleton()->alert("Error writing EXR file. Does this folder exist?\n\n" + m_LM.settings.ambient_filename, "WARNING");
		} else {
			image_ao->save_png(m_LM.settings.ambient_filename);
		}
	}

	// only if making final output
	if (m_LM.logic.output_final) {
		Error err;

		if (m_LM.settings.combined_is_HDR) {
			String szGlobalPath = ProjectSettings::get_singleton()->globalize_path(m_LM.settings.combined_filename);
			err = image_combined->save_exr(szGlobalPath, false);
		} else {
			err = image_combined->save_png(m_LM.settings.combined_filename);
		}

		if (err == OK)
			ResourceLoader::import(m_LM.settings.combined_filename);
		else
			OS::get_singleton()->alert("Error writing combined file. Does this folder exist?\n\n" + m_LM.settings.combined_filename, "WARNING");
	}

	return true;
}

bool LLightmap::lightmap_bake_to_image(Object *pOutputLightmapImage, Object *pOutputAOImage, Object *pOutputCombinedImage) {
	// get the mesh instance and light root
	if (!has_node(m_LM.settings.path_mesh)) {
		ShowWarning("Meshes nodepath is invalid.");
		return false;
	}

	Spatial *pMeshInstance = Object::cast_to<Spatial>(get_node(m_LM.settings.path_mesh));
	if (!pMeshInstance) {
		ShowWarning("Meshes nodepath is not a spatial.");
		return false;
	}

	if (!has_node(m_LM.settings.path_lights)) {
		ShowWarning("Lights nodepath is invalid.");
		return false;
	}

	Node *pLightRoot = Object::cast_to<Node>(get_node(m_LM.settings.path_lights));
	if (!pLightRoot) {
		ShowWarning("Lights nodepath is not a node.");
		return false;
	}

	return lightmap_mesh(pMeshInstance, pLightRoot, pOutputLightmapImage, pOutputAOImage, pOutputCombinedImage);
}

bool LLightmap::lightmap_mesh(Node *pMeshRoot, Node *pLightRoot, Object *pOutputImage_Lightmap, Object *pOutputImage_AO, Object *pOutputImage_Combined) {
	Spatial *pMI = Object::cast_to<Spatial>(pMeshRoot);
	if (!pMI) {
		ShowWarning("lightmap_mesh : pMeshRoot not a spatial");
		return false;
	}

	Spatial *pLR = Object::cast_to<Spatial>(pLightRoot);
	if (!pLR) {
		ShowWarning("lightmap_mesh : lights root is not a spatial");
		return false;
	}

	Image *pIm_Lightmap = Object::cast_to<Image>(pOutputImage_Lightmap);
	if (!pIm_Lightmap) {
		ShowWarning("lightmap_mesh : lightmap not an image");
		return false;
	}

	Image *pIm_AO = Object::cast_to<Image>(pOutputImage_AO);
	if (!pIm_AO) {
		ShowWarning("lightmap_mesh : AO not an image");
		return false;
	}

	Image *pIm_Combined = Object::cast_to<Image>(pOutputImage_Combined);
	if (!pIm_Combined) {
		ShowWarning("lightmap_mesh : combined not an image");
		return false;
	}

	return m_LM.lightmap_mesh(pMI, pLR, pIm_Lightmap, pIm_AO, pIm_Combined);
}
