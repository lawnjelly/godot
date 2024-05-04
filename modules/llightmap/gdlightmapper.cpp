#include "gdlightmapper.h"
#include "core/os/os.h"

#define LIGHTMAP_STRINGIFY(x) #x
#define LIGHTMAP_TOSTRING(x) LIGHTMAP_STRINGIFY(x)

void LLightmap::_bind_methods() {
	BIND_ENUM_CONSTANT(LLightmap::MODE_FORWARD);
	BIND_ENUM_CONSTANT(LLightmap::MODE_BACKWARD);

	BIND_ENUM_CONSTANT(LLightmap::BAKEMODE_UVMAP);
	BIND_ENUM_CONSTANT(LLightmap::BAKEMODE_LIGHTMAP);
	BIND_ENUM_CONSTANT(LLightmap::BAKEMODE_BOUNCE);
	BIND_ENUM_CONSTANT(LLightmap::BAKEMODE_AO);
	BIND_ENUM_CONSTANT(LLightmap::BAKEMODE_MERGE);
	BIND_ENUM_CONSTANT(LLightmap::BAKEMODE_PROBES);
	BIND_ENUM_CONSTANT(LLightmap::BAKEMODE_MATERIAL);
	BIND_ENUM_CONSTANT(LLightmap::BAKEMODE_COMBINED);

	BIND_ENUM_CONSTANT(LLightmap::QUALITY_LOW);
	BIND_ENUM_CONSTANT(LLightmap::QUALITY_MEDIUM);
	BIND_ENUM_CONSTANT(LLightmap::QUALITY_HIGH);
	BIND_ENUM_CONSTANT(LLightmap::QUALITY_FINAL);

	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_TEX_WIDTH);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_TEX_HEIGHT);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_MAX_LIGHT_DISTANCE);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_AA_KERNEL_SIZE);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_MATERIAL_KERNEL_SIZE);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_AA_NUM_LIGHT_SAMPLES);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_HIGH_SHADOW_QUALITY);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_SURFACE_BIAS);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_MATERIAL_SIZE);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_VOXEL_DENSITY);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_NUM_PRIMARY_RAYS);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_NUM_BOUNCES);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_BOUNCE_POWER);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_AMBIENT_BOUNCE_MIX);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_ROUGHNESS);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_NUM_AMBIENT_BOUNCES);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_NUM_AMBIENT_BOUNCE_RAYS);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_AMBIENT_BOUNCE_POWER);
	//BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_EMISSION_ENABLED);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_EMISSION_DENSITY);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_EMISSION_POWER);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_GLOW);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_AO_NUM_SAMPLES);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_AO_RANGE);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_AO_ERROR_METRIC);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_AO_ABORT_TIMEOUT);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_SKY_SIZE);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_SKY_SAMPLES);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_SKY_BLUR);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_SKY_BRIGHTNESS);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_NORMALIZE);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_NORMALIZE_MULTIPLIER);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_AO_LIGHT_RATIO);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_GAMMA);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_DILATE_ENABLED);
	//BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_COMBINE_ORIG_MATERIAL_ENABLED);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_NOISE_REDUCTION);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_NOISE_THRESHOLD);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_SEAM_STITCHING_ENABLED);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_VISUALIZE_SEAMS_ENABLED);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_SEAM_DISTANCE_THRESHOLD);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_SEAM_NORMAL_THRESHOLD);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_PROBE_DENSITY);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_PROBE_SAMPLES);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_UV_PADDING);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_MERGE_FLAG_LIGHTS);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_MERGE_FLAG_AO);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_MERGE_FLAG_BOUNCE);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_MERGE_FLAG_EMISSION);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_MERGE_FLAG_GLOW);
	BIND_ENUM_CONSTANT(LM::LightMapper::PARAM_MERGE_FLAG_MATERIAL);

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

	//	ClassDB::bind_method(D_METHOD("set_lightmap_filename", "lightmap_filename"), &LLightmap::set_lightmap_filename);
	//	ClassDB::bind_method(D_METHOD("get_lightmap_filename"), &LLightmap::get_lightmap_filename);
	//	ClassDB::bind_method(D_METHOD("set_ao_filename", "ao_filename"), &LLightmap::set_ao_filename);
	//	ClassDB::bind_method(D_METHOD("get_ao_filename"), &LLightmap::get_ao_filename);

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
	ADD_PROPERTY(PropertyInfo(Variant::INT, "bake_mode", PROPERTY_HINT_ENUM, "Lightmap,AO,Material,Bounce,LightProbes,UVMap,Combined,Merge"), "set_bake_mode", "get_bake_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Forward,Backward"), "set_mode", "get_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "quality", PROPERTY_HINT_ENUM, "Low,Medium,High,Final"), "set_quality", "get_quality");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "noise method", PROPERTY_HINT_ENUM, "Disabled,Simple,Advanced"), "set_noise_reduction_method", "get_noise_reduction_method");

	ADD_GROUP("Options", "");
	LIMPL_PROPERTY_PARAM(Variant::BOOL, dilate, LM::LightMapper::PARAM_DILATE_ENABLED);
	LIMPL_PROPERTY_PARAM(Variant::BOOL, seam_stitching, LM::LightMapper::PARAM_SEAM_STITCHING_ENABLED);
	LIMPL_PROPERTY_PARAM(Variant::BOOL, visualize_seams, LM::LightMapper::PARAM_VISUALIZE_SEAMS_ENABLED);

	ADD_GROUP("Merge Flags", "merge_flag");
	LIMPL_PROPERTY_PARAM(Variant::BOOL, merge_flag_lights, LM::LightMapper::PARAM_MERGE_FLAG_LIGHTS);
	LIMPL_PROPERTY_PARAM(Variant::BOOL, merge_flag_ambient_occlusion, LM::LightMapper::PARAM_MERGE_FLAG_AO);
	LIMPL_PROPERTY_PARAM(Variant::BOOL, merge_flag_ambient_bounce, LM::LightMapper::PARAM_MERGE_FLAG_BOUNCE);
	LIMPL_PROPERTY_PARAM(Variant::BOOL, merge_flag_emission, LM::LightMapper::PARAM_MERGE_FLAG_EMISSION);
	LIMPL_PROPERTY_PARAM(Variant::BOOL, merge_flag_glow, LM::LightMapper::PARAM_MERGE_FLAG_GLOW);
	LIMPL_PROPERTY_PARAM(Variant::BOOL, merge_flag_material, LM::LightMapper::PARAM_MERGE_FLAG_MATERIAL);

	ADD_GROUP("Size", "");

	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, tex_width, "128,8192,128", LM::LightMapper::PARAM_TEX_WIDTH);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, tex_height, "128,8192,128", LM::LightMapper::PARAM_TEX_HEIGHT);

	ADD_GROUP("Dynamic Range", "");
	//	LIMPL_PROPERTY(Variant::BOOL, normalize, set_normalize, get_normalize);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::REAL, normalize_multiplier, "0.0,16.0", LM::LightMapper::PARAM_NORMALIZE_MULTIPLIER);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::REAL, ao_light_ratio, "0.0,1.0,0.01", LM::LightMapper::PARAM_AO_LIGHT_RATIO);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::REAL, gamma, "0.01,10.0,0.01", LM::LightMapper::PARAM_GAMMA);

	ADD_GROUP("Post Processing", "");
	//LIMPL_PROPERTY(Variant::BOOL, dilate, set_dilate, get_dilate);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::REAL, noise_reduction, "0.0,1.0,0.01", LM::LightMapper::PARAM_NOISE_REDUCTION);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::REAL, noise_threshold, "0.0,1.0,0.01", LM::LightMapper::PARAM_NOISE_THRESHOLD);

	LIMPL_PROPERTY_PARAM_RANGE(Variant::REAL, seam_distance_threshold, "0.0,0.01,0.0001", LM::LightMapper::PARAM_SEAM_DISTANCE_THRESHOLD);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::REAL, seam_normal_threshold, "0.0,180.0,1.0", LM::LightMapper::PARAM_SEAM_NORMAL_THRESHOLD);

	ADD_GROUP("Backward", "");
	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, aa_kernel_size, "1,64,1", LM::LightMapper::PARAM_AA_KERNEL_SIZE);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, aa_num_light_samples, "1,64,1", LM::LightMapper::PARAM_AA_NUM_LIGHT_SAMPLES);
	LIMPL_PROPERTY_PARAM(Variant::BOOL, high_shadow_quality, LM::LightMapper::PARAM_HIGH_SHADOW_QUALITY);

	ADD_GROUP("Forward", "");
	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, num_bounces, "0,16,1", LM::LightMapper::PARAM_NUM_BOUNCES);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::REAL, bounce_power, "0.0,8.0,0.05", LM::LightMapper::PARAM_BOUNCE_POWER);

	ADD_GROUP("Common", "");
	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, primary_rays, "1,4096,1", LM::LightMapper::PARAM_NUM_PRIMARY_RAYS);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, max_light_distance, "0,999999,1", LM::LightMapper::PARAM_MAX_LIGHT_DISTANCE);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, voxel_density, "1,512,1", LM::LightMapper::PARAM_VOXEL_DENSITY);

	ADD_GROUP("Ambient", "ambient");

	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, ambient_bounces, "0,16,1", LM::LightMapper::PARAM_NUM_AMBIENT_BOUNCES);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, ambient_bounce_samples, "0, 1024, 1", LM::LightMapper::PARAM_NUM_AMBIENT_BOUNCE_RAYS);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::REAL, ambient_bounce_power, "0.0, 1.0", LM::LightMapper::PARAM_AMBIENT_BOUNCE_POWER);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::REAL, ambient_bounce_mix, "0.0,4.0", LM::LightMapper::PARAM_AMBIENT_BOUNCE_MIX);

	ADD_GROUP("Miscellaneous", "");
	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, material_size, "128,2048,128", LM::LightMapper::PARAM_MATERIAL_SIZE);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, material_kernel_size, "1,64,1", LM::LightMapper::PARAM_MATERIAL_KERNEL_SIZE);
	//LIMPL_PROPERTY_PARAM(Variant::BOOL, combine_original_material, LM::LightMapper::PARAM_COMBINE_ORIG_MATERIAL_ENABLED);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::REAL, roughness, "0.0,1.0,0.05", LM::LightMapper::PARAM_ROUGHNESS);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::REAL, surface_bias, "0.0,1.0", LM::LightMapper::PARAM_SURFACE_BIAS);

	ADD_GROUP("Emission", "");
	//LIMPL_PROPERTY_PARAM(Variant::BOOL, emission_enabled, LM::LightMapper::PARAM_EMISSION_ENABLED);

	LIMPL_PROPERTY_PARAM_RANGE(Variant::REAL, emission_density, "0.0,8.0,0.05", LM::LightMapper::PARAM_EMISSION_DENSITY);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::REAL, emission_power, "0.0,100.0", LM::LightMapper::PARAM_EMISSION_POWER);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::REAL, glow, "0.0,4.0", LM::LightMapper::PARAM_GLOW);

	//	ADD_GROUP("Forward Parameters", "");
	//LIMPL_PROPERTY_RANGE(Variant::REAL, f_ray_power, set_forward_ray_power, get_forward_ray_power, "0.0,0.1,0.01");

	//	ADD_GROUP("Backward Parameters", "");
	//	LIMPL_PROPERTY(Variant::INT, b_initial_rays, set_backward_num_rays, get_backward_num_rays);
	//LIMPL_PROPERTY(Variant::REAL, b_ray_power, set_backward_ray_power, get_backward_ray_power);

	ADD_GROUP("Ambient Occlusion", "");
	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, ao_samples, "1,2048,1", LM::LightMapper::PARAM_AO_NUM_SAMPLES);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::REAL, ao_range, "0.0,1000.0", LM::LightMapper::PARAM_AO_RANGE);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::REAL, ao_error_metric, "0.0,1.0", LM::LightMapper::PARAM_AO_ERROR_METRIC);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, ao_abort_timeout, "1,64,1", LM::LightMapper::PARAM_AO_ABORT_TIMEOUT);
	//	LIMPL_PROPERTY(Variant::REAL, ao_cut_range, set_ao_cut_range, get_ao_cut_range);

	ADD_GROUP("Sky", "");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "sky_filename", PROPERTY_HINT_FILE, "*.exr,*.png,*.jpg"), "set_sky_filename", "get_sky_filename");
	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, sky_size, "64,2048,64", LM::LightMapper::PARAM_SKY_SIZE);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, sky_samples, "128,8192,128", LM::LightMapper::PARAM_SKY_SAMPLES);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::REAL, sky_blur, "0.0,0.5,0.01", LM::LightMapper::PARAM_SKY_BLUR);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::REAL, sky_brightness, "0.0,4.0,0.01", LM::LightMapper::PARAM_SKY_BRIGHTNESS);

	ADD_GROUP("Light Probes", "");
	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, probe_density, "1,512,1", LM::LightMapper::PARAM_PROBE_DENSITY);
	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, probe_samples, "512,4096*8,512", LM::LightMapper::PARAM_PROBE_SAMPLES);
	//	ADD_PROPERTY(PropertyInfo(Variant::STRING, "probe_filename", PROPERTY_HINT_SAVE_FILE, "*.probe"), "set_probe_filename", "get_probe_filename");

	ADD_GROUP("UV Unwrap", "");
	LIMPL_PROPERTY_PARAM_RANGE(Variant::INT, uv_padding, "0,256,1", LM::LightMapper::PARAM_UV_PADDING);

	ADD_GROUP("Paths", "");
	LIMPL_PROPERTY(Variant::NODE_PATH, meshes, set_mesh_path, get_mesh_path);
	LIMPL_PROPERTY(Variant::NODE_PATH, lights, set_lights_path, get_lights_path);
	//	ADD_PROPERTY(PropertyInfo(Variant::STRING, "lightmap_filename", PROPERTY_HINT_SAVE_FILE, "*.exr"), "set_lightmap_filename", "get_lightmap_filename");
	//	ADD_PROPERTY(PropertyInfo(Variant::STRING, "ao_filename", PROPERTY_HINT_SAVE_FILE, "*.exr"), "set_ao_filename", "get_ao_filename");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "combined_filename", PROPERTY_HINT_SAVE_FILE, "*.png,*.exr"), "set_combined_filename", "get_combined_filename");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "uv_filename", PROPERTY_HINT_SAVE_FILE, "*.tscn"), "set_uv_filename", "get_uv_filename");

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

////////////////////////////
void LLightmap::set_backward_num_rays(int num_rays) {
	m_LM.settings.backward_num_rays = num_rays;
}
int LLightmap::get_backward_num_rays() const {
	return m_LM.settings.backward_num_rays;
}

void LLightmap::set_uv_filename(const String &p_filename) {
	m_LM.settings.UV_filename = p_filename;
}
String LLightmap::get_uv_filename() const {
	return m_LM.settings.UV_filename;
}

void LLightmap::set_noise_reduction_method(int method) {
	m_LM.settings.noise_reduction_method = (LM::LightMapper_Base::eNRMethod)method;
}

int LLightmap::get_noise_reduction_method() const {
	return m_LM.settings.noise_reduction_method;
}

void LLightmap::set_sky_filename(const String &p_filename) {
	m_LM.settings.sky_filename = p_filename;
}
String LLightmap::get_sky_filename() const {
	return m_LM.settings.sky_filename;
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

//LLIGHTMAP_IMPLEMENT_SETGET_FILENAME(set_lightmap_filename, get_lightmap_filename, settings.lightmap_filename, settings.lightmap_is_HDR)
//LLIGHTMAP_IMPLEMENT_SETGET_FILENAME(set_ao_filename, get_ao_filename, settings.ambient_filename, settings.ambient_is_HDR)
//LLIGHTMAP_IMPLEMENT_SETGET_FILENAME(set_combined_filename, get_combined_filename, settings.CombinedFilename, settings.CombinedIsHDR)

void LLightmap::set_combined_filename(const String &p_filename) {
	String new_filename = p_filename;
	String ext = new_filename.get_extension();

	// no extension? default to png
	if (ext == "")
		new_filename += ".png";

	m_LM.settings.set_images_filename(new_filename);
	/*
	m_LM.settings.combined_filename = new_filename;

	if (ext == "exr") {
		m_LM.settings.combined_is_HDR = true;
	} else {
		m_LM.settings.combined_is_HDR = false;
	}
*/
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
	//MutexLock guard(baking_mutex);

	m_LM.calculate_quality_adjusted_settings();

	if (m_LM.settings.bake_mode == LM::LightMapper_Base::LMBAKEMODE_UVMAP) {
		return uvmap();
	}

	if (m_LM.settings.lightmap_filename == "")
		return false;
	if (m_LM.settings.combined_filename == "")
		return false;

	// bake to a file
	//Ref<Image> image_lightmap;
	//Ref<Image> image_ao;
	Ref<Image> image_combined;

	int w = m_LM.adjusted_settings.tex_width;
	int h = m_LM.adjusted_settings.tex_height;

	// create either low or HDR images
	Ref<Image> image_lightmap = memnew(Image(w, h, false, Image::FORMAT_RGBAF));
	Ref<Image> image_ao = memnew(Image(w, h, false, Image::FORMAT_RGBAF));
	//	if (m_LM.settings.lightmap_is_HDR) {
	//		Ref<Image> image = memnew(Image(w, h, false, Image::FORMAT_RGBAF));
	//		image_lightmap = image;
	//	} else {
	//		Ref<Image> image = memnew(Image(w, h, false, Image::FORMAT_RGBA8));
	//		image_lightmap = image;
	//	}

	//	if (m_LM.settings.ambient_is_HDR) {
	//		Ref<Image> image = memnew(Image(w, h, false, Image::FORMAT_RF));
	//		image_ao = image;
	//	} else {
	//		Ref<Image> image = memnew(Image(w, h, false, Image::FORMAT_L8));
	//		image_ao = image;
	//	}

	if (m_LM.settings.combined_is_HDR) {
		Ref<Image> image = memnew(Image(w, h, false, Image::FORMAT_RGBAF));
		image_combined = image;
	} else {
		Ref<Image> image = memnew(Image(w, h, false, Image::FORMAT_RGBA8));
		image_combined = image;
	}

	lightmap_bake_to_image(image_lightmap.ptr(), image_ao.ptr(), image_combined.ptr());

#if 0
	   // save the images, png or exr
#define LLIGHTMAP_DEFINE_SAVE_IMAGE_MAP(PROCESS_BOOL, LM_FILENAME, LIGHTIMAGE)                                          \
	if (m_LM.logic.PROCESS_BOOL) {                                                                                      \
		String global_path = ProjectSettings::get_singleton()->globalize_path(m_LM.settings.LM_FILENAME);               \
		print_line("saving EXR .. global path : " + global_path);                                                       \
		Error err = LIGHTIMAGE->save_exr(global_path, false);                                                           \
		if (err != OK)                                                                                                  \
			OS::get_singleton()->alert("Error writing EXR file. Does this folder exist?\n\n" + global_path, "WARNING"); \
	}
	
	LLIGHTMAP_DEFINE_SAVE_IMAGE_MAP(process_lightmap, lightmap_filename, image_lightmap)
	LLIGHTMAP_DEFINE_SAVE_IMAGE_MAP(process_AO, AO_filename, image_ao)
	LLIGHTMAP_DEFINE_SAVE_IMAGE_MAP(process_emission, emission_filename, image_emission)
	LLIGHTMAP_DEFINE_SAVE_IMAGE_MAP(process_glow, glow_filename, image_glow)
	
//	if (m_LM.logic.process_lightmap) {
//		if (true) {
//			//		if (m_LM.settings.lightmap_is_HDR) {
//			String szGlobalPath = ProjectSettings::get_singleton()->globalize_path(m_LM.settings.lightmap_filename);
//			print_line("saving lights EXR .. global path : " + szGlobalPath);
//			Error err = image_lightmap->save_exr(szGlobalPath, false);
			
//			if (err != OK)
//				OS::get_singleton()->alert("Error writing EXR file. Does this folder exist?\n\n" + m_LM.settings.lightmap_filename, "WARNING");
//		} else {
//			image_lightmap->save_png(m_LM.settings.lightmap_filename);
//		}
//	}
	
	
//	if (m_LM.logic.process_AO) {
//		if (true) {
//			//		if (m_LM.settings.ambient_is_HDR) {
//			String szGlobalPath = ProjectSettings::get_singleton()->globalize_path(m_LM.settings.AO_filename);
//			print_line("saving ao EXR .. global path : " + szGlobalPath);
//			Error err = image_ao->save_exr(szGlobalPath, false);

//			if (err != OK)
//				OS::get_singleton()->alert("Error writing EXR file. Does this folder exist?\n\n" + m_LM.settings.AO_filename, "WARNING");
//		} else {
//			image_ao->save_png(m_LM.settings.AO_filename);
//		}
//	}

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
#endif

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

	return m_LM.lightmap_meshes(pMI, pLR, pIm_Lightmap, pIm_AO, pIm_Combined);
}
