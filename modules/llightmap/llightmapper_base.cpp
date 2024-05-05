#include "core/math/plane.h"
#include "core/os/os.h"
#include "editor/editor_node.h"
#include "lconvolution.h"
#include "ldilate.h"
#include "llightmapper.h"
#include "lstitcher.h"
#include "modules/denoise/denoise_wrapper.h"
#include "scene/3d/light.h"

using namespace LM;

//LightMapper_Base::BakeBeginFunc LightMapper_Base::bake_begin_function = NULL;
LightMapper_Base::BakeStepFunc LightMapper_Base::bake_step_function = nullptr;
LightMapper_Base::BakeSubStepFunc LightMapper_Base::bake_substep_function = nullptr;
LightMapper_Base::BakeEndFunc LightMapper_Base::bake_end_function = nullptr;

LightMapper_Base::LightMapper_Base() {
	_num_rays = 1;

	settings.backward_ray_power = 1.0f;

	data.params[PARAM_BOUNCE_POWER] = 1.0f;
	data.params[PARAM_AMBIENT_BOUNCE_MIX] = 1.0f;
	//settings.directional_bounce_power = 1.0f;

	data.params[PARAM_TEX_WIDTH] = 512;
	data.params[PARAM_TEX_HEIGHT] = 512;

	data.params[PARAM_NUM_PRIMARY_RAYS] = 32;
	data.params[PARAM_NUM_AMBIENT_BOUNCES] = 0;
	data.params[PARAM_NUM_AMBIENT_BOUNCE_RAYS] = 128;
	data.params[PARAM_NUM_BOUNCES] = 0;
	data.params[PARAM_AMBIENT_BOUNCE_POWER] = 0.5f;
	data.params[PARAM_ROUGHNESS] = 0.5f;

	data.params[PARAM_EMISSION_DENSITY] = 1.0f;
	data.params[PARAM_EMISSION_POWER] = 1.0f;
	data.params[PARAM_EMISSION_FORWARD_BOUNCE_SAMPLES] = 128;
	data.params[PARAM_GLOW] = 1.0f;

	data.params[PARAM_AO_RANGE] = 2.0f;
	data.params[PARAM_AO_NUM_SAMPLES] = 256;
	data.params[PARAM_AO_ERROR_METRIC] = 1.0f;
	data.params[PARAM_AO_ABORT_TIMEOUT] = 8;

	settings.mode = LMMODE_BACKWARD;
	settings.bake_mode = LMBAKEMODE_LIGHTMAP;
	settings.quality = LM_QUALITY_MEDIUM;

	// 0 is infinite
	data.params[PARAM_MAX_LIGHT_DISTANCE] = 0;
	data.params[PARAM_HIGH_SHADOW_QUALITY] = true;
	data.params[PARAM_AA_KERNEL_SIZE] = 16;
	data.params[PARAM_AA_NUM_LIGHT_SAMPLES] = 1;

	data.params[PARAM_VOXEL_DENSITY] = 20;
	data.params[PARAM_SURFACE_BIAS] = 0.005f;

	data.params[PARAM_MATERIAL_SIZE] = 256;
	data.params[PARAM_MATERIAL_KERNEL_SIZE] = 8;

	data.params[PARAM_NORMALIZE] = true;
	data.params[PARAM_NORMALIZE_MULTIPLIER] = 4.0f;
	data.params[PARAM_AO_LIGHT_RATIO] = 0.5f;
	data.params[PARAM_GAMMA] = 2.2f;

	//	settings.lightmap_is_HDR = false;
	//	settings.ambient_is_HDR = false;
	settings.combined_is_HDR = false;

	data.params[PARAM_UV_PADDING] = 4;

	data.params[PARAM_PROBE_DENSITY] = 64;
	data.params[PARAM_PROBE_SAMPLES] = 4096;

	data.params[PARAM_NOISE_THRESHOLD] = 0.1f;
	data.params[PARAM_NOISE_REDUCTION] = 1.0f;
	settings.noise_reduction_method = NR_ADVANCED;

	data.params[PARAM_DILATE_ENABLED] = true;
	// data.params[PARAM_COMBINE_ORIG_MATERIAL_ENABLED] = false;

	data.params[PARAM_SEAM_STITCHING_ENABLED] = true;
	data.params[PARAM_SEAM_DISTANCE_THRESHOLD] = 0.001f;
	data.params[PARAM_SEAM_NORMAL_THRESHOLD] = 45.0f;
	data.params[PARAM_VISUALIZE_SEAMS_ENABLED] = false;

	data.params[PARAM_SKY_BLUR] = 0.18f;
	data.params[PARAM_SKY_SIZE] = 256;
	data.params[PARAM_SKY_SAMPLES] = 512;
	data.params[PARAM_SKY_BRIGHTNESS] = 1.0f;
	//	settings.sky_blur_amount = 0.18f;
	//	settings.sky_size = 256;
	//	settings.sky_num_samples = 512;
	//	settings.sky_brightness = 1.0f;

	//data.params[PARAM_EMISSION_ENABLED] = true;

	data.params[PARAM_MERGE_FLAG_LIGHTS] = true;
	data.params[PARAM_MERGE_FLAG_AO] = true;
	data.params[PARAM_MERGE_FLAG_BOUNCE] = true;
	data.params[PARAM_MERGE_FLAG_EMISSION] = true;
	data.params[PARAM_MERGE_FLAG_GLOW] = true;
	data.params[PARAM_MERGE_FLAG_MATERIAL] = false;

	calculate_quality_adjusted_settings();
}

void LightMapper_Base::set_param(Param p_param, Variant p_value) {
	data.params[p_param] = p_value;

	switch (p_param) {
		default:
			break;
	}
}

Variant LightMapper_Base::get_param(Param p_param) {
	return data.params[p_param];
}

void LightMapper_Base::debug_save(LightImage<uint32_t> &p_im, String p_filename) {
	//	Ref<Image> image = memnew(Image(width, height, false, Image::FORMAT_RGBA8));
	Ref<Image> im = memnew(Image(p_im.get_width(), p_im.get_height(), false, Image::FORMAT_RGBA8));
	;
	//im.create(p_im.GetWidth(), p_im.GetHeight(), false, Image::FORMAT_RGBA8);
	im->lock();
	for (uint32_t y = 0; y < p_im.get_height(); y++) {
		for (uint32_t x = 0; x < p_im.get_width(); x++) {
			Color c = Color::hex(p_im.get_item(x, y));
			im->set_pixel(x, y, c);
		}
	}
	im->unlock();

	im->save_png(p_filename);
}

void LightMapper_Base::base_reset() {
	_image_main.reset();
	_image_main_mirror.reset();

	_image_lightmap.reset();
	_image_orig_material.reset();
	_image_AO.reset();
	_image_emission.reset();
	_image_glow.reset();
	_image_bounce.reset();

	_image_tri_ids_p1.reset();
#ifdef LLIGHTMAP_DEBUG_RECLAIMED_TEXELS
	_image_reclaimed_texels.reset();
#endif
	_image_tri_minilists.reset();

	_minilist_tri_ids.clear(true);

	_image_barycentric.reset();
	_scene._image_emission_done.reset();

	_lights.clear(true);

	_QMC.create(0);
}

void LightMapper_Base::calculate_quality_adjusted_settings() {
	// set them initially to the same
	AdjustedSettings &as = adjusted_settings;

	as.tex_width = data.params[PARAM_TEX_WIDTH];
	as.tex_height = data.params[PARAM_TEX_HEIGHT];

	as.num_primary_rays = data.params[PARAM_NUM_PRIMARY_RAYS];
	as.max_light_distance = data.params[PARAM_MAX_LIGHT_DISTANCE];

	as.surface_bias = data.params[PARAM_SURFACE_BIAS];

	//as.m_Forward_NumRays = settings.Forward_NumRays;
	as.emission_density = data.params[PARAM_EMISSION_DENSITY];
	as.emission_power = data.params[PARAM_EMISSION_POWER];
	as.emission_forward_bounce_samples = (int)data.params[PARAM_EMISSION_FORWARD_BOUNCE_SAMPLES] * 8 * 8;

	// New .. Scale emission power by the final lightmap size.
	// This is because emission is more per texel that emits, and the number of texels
	// scales with overall lightmap size.
#if 0
	uint32_t num_texels = _width * _height;
	float overall_lightmap_size_scaling = num_texels / (float)(512 * 512);

	if (overall_lightmap_size_scaling) {
		as.emission_power *= 1.0 / overall_lightmap_size_scaling;
	} else {
		WARN_PRINT("overall_lightmap_size_scaling is zero.");
	}
#endif

	as.glow = data.params[PARAM_GLOW];
	as.glow *= as.glow;
	as.smoothness = 1.0f - (float)data.params[PARAM_ROUGHNESS];

	as.backward_num_rays = settings.backward_num_rays;

	as.num_ambient_bounces = data.params[PARAM_NUM_AMBIENT_BOUNCES];
	as.num_ambient_bounce_rays = data.params[PARAM_NUM_AMBIENT_BOUNCE_RAYS];
	as.num_directional_bounces = data.params[PARAM_NUM_BOUNCES];
	as.directional_bounce_power = data.params[PARAM_BOUNCE_POWER];

	as.AO_num_samples = data.params[PARAM_AO_NUM_SAMPLES];
	as.AO_range = data.params[PARAM_AO_RANGE];
	as.AO_error_metric = (float)data.params[PARAM_AO_ERROR_METRIC] * 0.002f;
	as.AO_abort_timeout = (int)data.params[PARAM_AO_ABORT_TIMEOUT] * 8;

	as.max_material_size = data.params[PARAM_MATERIAL_SIZE];
	as.normalize_multiplier = data.params[PARAM_NORMALIZE_MULTIPLIER];

	as.num_sky_samples = data.params[PARAM_SKY_SAMPLES];
	as.sky_brightness = data.params[PARAM_SKY_BRIGHTNESS];
	as.sky_brightness *= as.sky_brightness;

	as.antialias_samples_width = data.params[PARAM_AA_KERNEL_SIZE]; // 16
	as.antialias_samples_per_texel = as.antialias_samples_width * as.antialias_samples_width;
	as.antialias_num_light_samples_per_subtexel = data.params[PARAM_AA_NUM_LIGHT_SAMPLES];

	as.material_kernel_size = data.params[PARAM_MATERIAL_KERNEL_SIZE];

	// overrides
	switch (settings.quality) {
		case LM_QUALITY_LOW: {
			as.num_primary_rays = 1;
			as.forward_num_rays = 1;
			as.backward_num_rays = 4;
			as.AO_num_samples = 1;
			as.max_material_size = 32;
			as.num_ambient_bounces = 0;
			as.num_directional_bounces = 0;
			as.num_sky_samples = 64;
			as.antialias_samples_per_texel = 1;
			as.antialias_num_light_samples_per_subtexel = 1;
			as.material_kernel_size = 1;
			as.emission_forward_bounce_samples = 1;
		} break;
		case LM_QUALITY_MEDIUM: {
			as.num_primary_rays /= 2;
			as.AO_num_samples /= 2;
			as.max_material_size /= 4;
			as.num_ambient_bounce_rays /= 2;
			as.num_sky_samples /= 2;
			as.antialias_samples_width /= 2;
			as.antialias_samples_per_texel /= 2;
			as.material_kernel_size /= 2;
			as.emission_forward_bounce_samples /= 2;
		} break;
		default:
			// high is default
			break;
		case LM_QUALITY_FINAL:
			as.num_primary_rays *= 2;
			as.AO_num_samples *= 2;
			as.AO_error_metric *= 0.5f;
			as.num_ambient_bounce_rays *= 2;
			as.num_sky_samples *= 2;
			as.antialias_num_light_samples_per_subtexel *= 2;

			as.antialias_samples_per_texel *= 2;
			as.emission_forward_bounce_samples *= 2;
			as.material_kernel_size += 2;
			break;
	}

	// minimums
	as.num_primary_rays = MAX(as.num_primary_rays, 1);

	as.forward_num_rays = (as.num_primary_rays * 16) / 32;
	as.backward_num_rays = (as.num_primary_rays * 128) / 32;

	as.forward_num_rays = MAX(as.forward_num_rays, 1);

	as.num_ambient_bounce_rays = MAX(as.num_ambient_bounce_rays, 1);
	as.AO_num_samples = MAX(as.AO_num_samples, 1);
	as.max_material_size = MAX(as.max_material_size, 32);

	as.emission_density = MAX(as.emission_density, 1);
	as.emission_forward_bounce_samples = MAX(as.emission_forward_bounce_samples, 1);

	as.antialias_samples_per_texel = MAX(as.antialias_samples_per_texel, 1);
	as.antialias_samples_width = MAX(as.antialias_samples_width, 1);

	as.antialias_total_light_samples_per_texel = as.antialias_samples_per_texel * as.antialias_num_light_samples_per_subtexel;

	as.material_kernel_size = MAX(as.material_kernel_size, 1);
}

void LightMapper_Base::find_light(const Node *pNode) {
	const Light *pLight = Object::cast_to<const Light>(pNode);
	if (!pLight)
		return;

	// visibility or bake mode?
	if (pLight->get_bake_mode() == Light::BAKE_DISABLED)
		return;

	// is it visible?
	//	if (!pLight->is_visible_in_tree())
	//		return;

	LLight *l = _lights.request();

	// blank
	memset(l, 0, sizeof(LLight));

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
	l->spot_angle_radians = Math::deg2rad(pLight->get_param(Light::PARAM_SPOT_ANGLE));
	l->spot_dot_max = Math::cos(l->spot_angle_radians);
	l->spot_dot_max = MIN(l->spot_dot_max, 0.9999f); // just to prevent divide by zero in cone of spotlight

	// the spot emanation point is used for spotlight cone culling.
	// if we used the dot from the pos to cull, we would miss cases where
	// the sample origin is offset by scale from pos. So we push back the pos
	// in order to account for the scale 'cloud' of origins.
	float radius = MAX(l->scale.x, MAX(l->scale.y, l->scale.z));
	l->spot_emanation_point = l->pos - (l->dir * radius);

	// pre apply intensity
	l->color = pLight->get_color() * l->energy;

	const DirectionalLight *pDLight = Object::cast_to<DirectionalLight>(pLight);
	if (pDLight)
		l->type = LLight::LT_DIRECTIONAL;

	const SpotLight *pSLight = Object::cast_to<SpotLight>(pLight);
	if (pSLight)
		l->type = LLight::LT_SPOT;

	const OmniLight *pOLight = Object::cast_to<OmniLight>(pLight);
	if (pOLight)
		l->type = LLight::LT_OMNI;
}

void LightMapper_Base::prepare_lights() {
	for (int n = 0; n < _lights.size(); n++) {
		LLight &light = _lights[n];

		if (light.type == LLight::LT_DIRECTIONAL)
			light_to_plane(light);
	}
}

void LightMapper_Base::find_lights_recursive(const Node *pNode) {
	find_light(pNode);

	int nChildren = pNode->get_child_count();

	for (int n = 0; n < nChildren; n++) {
		Node *pChild = pNode->get_child(n);
		find_lights_recursive(pChild);
	}
}

Plane LightMapper_Base::find_containment_plane(const Vector3 &dir, Vector3 pts[8], float &range, float padding) {
	// construct a plane with one of the points
	Plane pl(pts[0], dir);

	float furthest_dist = 0.0f;
	int furthest = 0;

	// find the furthest point
	for (int n = 0; n < 8; n++) {
		float d = pl.distance_to(pts[n]);

		if (d < furthest_dist) {
			furthest_dist = d;
			furthest = n;
		}
	}

	// reconstruct the plane based on the furthest point
	pl = Plane(pts[furthest], dir);

	// find the range
	range = 0.0f;

	for (int n = 0; n < 8; n++) {
		float d = pl.distance_to(pts[n]);

		if (d > range) {
			range = d;
		}
	}

	//	const float padding = 8.0f;

	// move plane backward a bit for luck
	pl.d -= padding;

	// add a boost to the range
	range += padding * 2.0f;

	return pl;
}

void LightMapper_Base::light_to_plane(LLight &light) {
	AABB bb = _scene._tracer.get_world_bound_expanded();
	Vector3 minmax[2];
	minmax[0] = bb.position;
	minmax[1] = bb.position + bb.size;

	if (light.dir.y == 0.0f)
		return;

	// find the shift in x and z caused by y offset to top of scene
	float units = bb.size.y / light.dir.y;
	Vector3 offset = light.dir * -units;

	// add to which side of scene
	if (offset.x >= 0.0f)
		minmax[1].x += offset.x;
	else
		minmax[0].x += offset.x;

	if (offset.z >= 0.0f)
		minmax[1].z += offset.z;
	else
		minmax[0].z += offset.z;

	light.dl_plane_pt = minmax[0];
	light.dl_tangent = Vector3(1, 0, 0);
	light.dl_bitangent = Vector3(0, 0, 1);
	light.dl_tangent_range = minmax[1].x - minmax[0].x;
	light.dl_bitangent_range = minmax[1].z - minmax[0].z;

	print_line("plane mins : " + String(Variant(minmax[0])));
	print_line("plane maxs : " + String(Variant(minmax[1])));

	return;

	Vector3 pts[8];
	for (int n = 0; n < 8; n++) {
		// which x etc. either 0 or 1 for each axis
		int wx = MIN(n & 1, 1);
		int wy = MIN(n & 2, 1);
		int wz = MIN(n & 4, 1);

		pts[n].x = minmax[wx].x;
		pts[n].y = minmax[wy].y;
		pts[n].z = minmax[wz].z;
	}

	// new .. don't use light direction as plane normal, always use
	// ceiling type plane (for sky) or from below.
	// This will deal with most common cases .. for side lights,
	// area light is better.
	Vector3 plane_normal = light.dir;
	if (light.dir.y < 0.0f) {
		plane_normal = Vector3(0, -1, 0);
	} else {
		plane_normal = Vector3(0, 1, 0);
	}

	const float PLANE_PUSH = 2.0f;

	float main_range;
	Plane pl = find_containment_plane(plane_normal, pts, main_range, PLANE_PUSH);

	// push it back even further for safety
	//pl.d -= 2.0f;

	// now create a bound on this plane

	// find a good tangent
	Vector3 cross[3];
	cross[0] = plane_normal.cross(Vector3(1, 0, 0));
	cross[1] = plane_normal.cross(Vector3(0, 1, 0));
	cross[2] = plane_normal.cross(Vector3(0, 0, 1));

	float lx = cross[0].length();
	float ly = cross[1].length();
	float lz = cross[2].length();

	int best_cross = 0;
	if (ly > lx) {
		best_cross = 1;
		if (lz > ly)
			best_cross = 2;
	}
	if (lz > lx)
		best_cross = 2;

	Vector3 tangent = cross[best_cross];
	tangent.normalize();

	Vector3 bitangent = plane_normal.cross(tangent);
	bitangent.normalize();

	float tangent_range;
	Plane pl_tangent = find_containment_plane(tangent, pts, tangent_range, 0.0f);

	float bitangent_range;
	Plane pl_bitangent = find_containment_plane(bitangent, pts, bitangent_range, 0.0f);

	// find point at mins of the planes
	Vector3 ptPlaneMins;
	if (!pl.intersect_3(pl_tangent, pl_bitangent, &ptPlaneMins)) {
		DEV_ASSERT("intersect3 failed");
	}

	// for flat sky, adjust the point to account for the incoming light direction
	// so as not to have part of the mesh in shadow
	//	Vector3 offset = light.dir * -PLANE_PUSH;
	//	ptPlaneMins.x += offset.x;
	//	ptPlaneMins.z += offset.z;

	// we now have a point, 2 vectors (tangent and bitangent) and ranges,
	// all that is needed for a random distribution!

	//	Vector3 dl_plane_pt;
	//	Vector3 dl_plane_tangent;
	//	Vector3 dl_plane_bitangent;
	//	float dl_plane_tangent_range;
	//	float dl_plane_bitangent_range;
	light.dl_plane_pt = ptPlaneMins;
	light.dl_tangent = tangent;
	light.dl_bitangent = bitangent;
	light.dl_tangent_range = tangent_range;
	light.dl_bitangent_range = bitangent_range;

	// debug output the positions
	Vector3 pA = ptPlaneMins;
	Vector3 pB = ptPlaneMins + (tangent * tangent_range);
	Vector3 pC = ptPlaneMins + (tangent * tangent_range) + (bitangent * bitangent_range);
	Vector3 pD = ptPlaneMins + (bitangent * bitangent_range);

	print_line("dir light A : " + String(Variant(pA)));
	print_line("dir light B : " + String(Variant(pB)));
	print_line("dir light C : " + String(Variant(pC)));
	print_line("dir light D : " + String(Variant(pD)));
}

bool LightMapper_Base::prepare_image_maps() {
	_image_tri_ids_p1.blank();
#ifdef LLIGHTMAP_DEBUG_RECLAIMED_TEXELS
	_image_reclaimed_texels.blank();
#endif

	// rasterize each triangle in turn
	//m_Scene.RasterizeTriangleIDs(*this, m_Image_ID_p1, m_Image_ID2_p1, m_Image_Barycentric);
	if (!_scene.rasterize_triangles_ids(*this, _image_tri_ids_p1, _image_barycentric))
		return false;

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

	return true;
}

void LightMapper_Base::normalize_AO() {
	int nPixels = _image_AO.get_num_pixels();
	float fmax = 0.0f;

	// first find the max
	for (int n = 0; n < nPixels; n++) {
		float f = *_image_AO.get(n);
		if (f > fmax)
			fmax = f;
	}

	if (fmax < 0.001f) {
		WARN_PRINT_ONCE("LightMapper_Base::Normalize_AO : values too small to normalize");
		return;
	}

	// multiplier to normal is 1.0f / fmax
	float mult = 1.0f / fmax;

	// apply bias
	//mult *= settings.NormalizeBias;

	// apply multiplier
	for (int n = 0; n < nPixels; n++) {
		float &f = *_image_AO.get(n);
		f *= mult;

		// negate AO
		f = 1.0f - f;
		if (f < 0.0f)
			f = 0.0f;
	}
}

void LightMapper_Base::stitch_seams() {
	if (!data.params[PARAM_SEAM_STITCHING_ENABLED])
		return;

	Stitcher stitcher;

	// stitch seams one mesh at a time
	for (int n = 0; n < _scene.get_num_meshes(); n++) {
		MeshInstance *mi = _scene.get_mesh(n);

		stitcher.stitch_object_seams(*mi, _image_main, data.params[PARAM_SEAM_DISTANCE_THRESHOLD], data.params[PARAM_SEAM_NORMAL_THRESHOLD], data.params[PARAM_VISUALIZE_SEAMS_ENABLED]);
	}
}

void LightMapper_Base::apply_noise_reduction() {
	switch (settings.noise_reduction_method) {
		case NR_DISABLE: {
		} break;
		case NR_SIMPLE: {
			// simple
			Convolution<FColor> conv;
			conv.run(_image_main, data.params[PARAM_NOISE_THRESHOLD], data.params[PARAM_NOISE_REDUCTION]);

			//	Convolution<float> conv_ao;
			//	conv_ao.Run(m_Image_AO, settings.NoiseThreshold, settings.NoiseReduction);
		} break;
		case NR_ADVANCED: {
			// use open image denoise
			void *device = oidn_denoiser_init();

			if (!oidn_denoise(device, (float *)_image_main.get(0), _image_main.get_width(), _image_main.get_height())) {
				WARN_PRINT("open image denoise error");
			}

			oidn_denoiser_finish(device);
		} break;
	}
}

void LightMapper_Base::_normalize(LightImage<FColor> &r_image, float p_normalize_multiplier) {
	int nPixels = _image_main.get_num_pixels();
	float fmax = 0.0f;

	// first find the max
	for (int n = 0; n < nPixels; n++) {
		float f = r_image.get(n)->max();
		if (f > fmax)
			fmax = f;
	}

	if (fmax < 0.001f) {
		WARN_PRINT_ONCE("LightMapper_Base::Normalize : values too small to normalize");
		return;
	}

	// multiplier to normal is 1.0f / fmax
	float mult = 1.0f / fmax;

	// apply bias
	mult *= p_normalize_multiplier;

	// apply multiplier
	for (int n = 0; n < nPixels; n++) {
		FColor &col = *r_image.get(n);
		col = col * mult;
	}
}

void LightMapper_Base::Settings::set_images_filename(String p_filename) {
	String ext = p_filename.get_extension();

	combined_filename = p_filename;
	combined_is_HDR = ext == "exr";

	int l = ext.length();
	image_filename_base = p_filename.substr(0, p_filename.length() - l);

	print_line("Base image filename: " + image_filename_base);

	lightmap_filename = image_filename_base + "lightmap.exr";
	AO_filename = image_filename_base + "ao.exr";
	AO_bitimage_clear_filename = image_filename_base + "ao_clear.biti";
	AO_bitimage_black_filename = image_filename_base + "ao_black.biti";
	emission_filename = image_filename_base + "emission.exr";
	glow_filename = image_filename_base + "glow.exr";
	orig_material_filename = image_filename_base + "material.exr";
	bounce_filename = image_filename_base + "bounce.exr";
	tri_ids_filename = image_filename_base + "coverage.png";
}

#if 0
bool LightMapper_Base::load_lightmap(Image &image) {
	//	assert (image.get_width() == m_iWidth);
	//	assert (image.get_height() == m_iHeight);

	Error res = image.load(settings.lightmap_filename);
	if (res != OK) {
		show_warning("Loading lights EXR failed.\n\n" + settings.lightmap_filename);
		return false;
	}

	if ((image.get_width() != _width) || (image.get_height() != _height)) {
		show_warning("Loaded Lights texture file dimensions do not match project, ignoring.");
		return false;
	}

	image.lock();
	for (int y = 0; y < _height; y++) {
		for (int x = 0; x < _width; x++) {
			_image_main.get_item(x, y).set(image.get_pixel(x, y));
		}
	}
	image.unlock();

	// Dilate after loading to allow merging.
	if (data.params[PARAM_DILATE_ENABLED] == Variant(true)) {
		Dilate<FColor> dilate;
		dilate.dilate_image(_image_main, _image_tri_ids_p1, 256);
	}

	return true;
}

bool LightMapper_Base::load_AO(Image &image) {
	//	assert (image.get_width() == m_iWidth);
	//	assert (image.get_height() == m_iHeight);

	Error res = image.load(settings.AO_filename);
	if (res != OK) {
		show_warning("Loading AO EXR failed.\n\n" + settings.AO_filename);
		return false;
	}

	if ((image.get_width() != _width) || (image.get_height() != _height)) {
		show_warning("Loaded AO texture file dimensions do not match project, ignoring.");
		return false;
	}

	image.lock();
	for (int y = 0; y < _height; y++) {
		for (int x = 0; x < _width; x++) {
			_image_AO.get_item(x, y) = image.get_pixel(x, y).r;
		}
	}
	image.unlock();

	// Dilate after loading to allow merging.
	if (data.params[PARAM_DILATE_ENABLED] == Variant(true)) {
		Dilate<float> dilate;
		dilate.dilate_image(_image_AO, _image_tri_ids_p1, 256);
	}

	return true;
}
#endif

void LightMapper_Base::merge_for_ambient_bounces() {
	float mult_emission = data.params[PARAM_EMISSION_POWER];
	float mult_glow = data.params[PARAM_GLOW];

	for (int y = 0; y < _height; y++) {
		for (int x = 0; x < _width; x++) {
			FColor total;
			const FColor &light = _image_lightmap.get_item(x, y);
			const FColor &emission = _image_emission.get_item(x, y);
			const FColor &glow = _image_glow.get_item(x, y);

			total = light;
			total += emission * mult_emission;
			total += glow * mult_glow;

			_image_main.get_item(x, y) = total;
		}
	}
}

void LightMapper_Base::merge_to_combined() {
	float light_AO_ratio = data.params[PARAM_AO_LIGHT_RATIO];

	bool flag_lights = data.params[PARAM_MERGE_FLAG_LIGHTS] == Variant(true);
	bool flag_ao = data.params[PARAM_MERGE_FLAG_AO] == Variant(true);
	bool flag_bounce = data.params[PARAM_MERGE_FLAG_BOUNCE] == Variant(true);
	bool flag_emission = data.params[PARAM_MERGE_FLAG_EMISSION] == Variant(true);
	bool flag_glow = data.params[PARAM_MERGE_FLAG_GLOW] == Variant(true);
	bool flag_material = (data.params[PARAM_MERGE_FLAG_MATERIAL] == Variant(true)) && _image_orig_material.get_num_pixels();

	bool material_only = flag_material && (!(flag_ao || flag_bounce || flag_emission || flag_lights));

	// merge them both before applying noise reduction and seams
	float gamma = 1.0f / (float)data.params[PARAM_GAMMA];

	// First light and emission, so noise reduction can be applied.
	if (true) {
		//	if (settings.bake_mode != LMBAKEMODE_AO) {
		float mult_emission = data.params[PARAM_EMISSION_POWER];
		float mult_glow = data.params[PARAM_GLOW];
		float mult_bounce = data.params[PARAM_AMBIENT_BOUNCE_MIX];

		for (int y = 0; y < _height; y++) {
			if ((y % 256 == 0) && bake_step_function) {
				bake_step_function(y / (float)_height, String("Merging Light Texels"));
			}

			for (int x = 0; x < _width; x++) {
				FColor total;
				if (settings.bake_mode != LMBAKEMODE_AO) {
					const FColor &light = _image_lightmap.get_item(x, y);
					const FColor &emission = _image_emission.get_item(x, y);

					total = flag_lights ? light : FColor::create(0);
					if (flag_emission) {
						total += emission * mult_emission;
					}
				}

				if (_image_bounce.get_num_pixels() && flag_bounce) {
					total += _image_bounce.get_item(x, y) * mult_bounce;
				}

				float ao = 0.0f;

				if (_image_AO.get_num_pixels()) {
					ao = _image_AO.get_item(x, y);
					if (!settings.combined_is_HDR) {
						ao = Math::pow((double)ao, 1.0 / gamma);
					}
				}

				switch (settings.bake_mode) {
					case LMBAKEMODE_EMISSION:
					case LMBAKEMODE_LIGHTMAP: {
					} break;
					case LMBAKEMODE_AO: {
						total.set(ao);
					} break;
					default: {
						if (flag_ao) {
							if (flag_lights || flag_emission || flag_bounce) {
								FColor applied = total * ao;
								total.lerp(applied, light_AO_ratio);
							} else {
								total.set(ao);
							}
						}
					} break;
				}

				_image_main.get_item(x, y) = total;
			}
		}

		if (settings.noise_reduction_method != NR_DISABLE) {
			if (bake_step_function) {
				bake_step_function(0, String("Applying Noise Reduction"));
			}

			// Normalize before noise reduction?
			_normalize(_image_main);

			// Dilate to make sure noise reduction has good stuff to work with.
			Dilate<FColor> dilate;
			dilate.dilate_image(_image_main, _image_tri_ids_p1, 256);

			// Noise Reduction.
			apply_noise_reduction();
		}

		// Glow and orig material.
		if (flag_glow || flag_material) {
			for (int y = 0; y < _height; y++) {
				if ((y % 256 == 0) && bake_step_function) {
					bake_step_function(y / (float)_height, String("Merging Glow and Material"));
				}

				for (int x = 0; x < _width; x++) {
					FColor &total = _image_main.get_item(x, y);
					const FColor &glow = _image_glow.get_item(x, y);

					if (flag_material) {
						const Color &alb = _image_orig_material.get_item(x, y);

						if (!material_only) {
							total.r *= alb.r;
							total.g *= alb.g;
							total.b *= alb.b;
						} else {
							total.set(alb);
						}
					}

					if (flag_glow) {
						total += glow * mult_glow;
					}
				}
			} // for y

		} // if glow or material
	}

	// normalize lightmap on combine
	if (data.params[PARAM_NORMALIZE] == Variant(true) && !material_only) {
		_normalize(_image_main, adjusted_settings.normalize_multiplier);
	}

	if (bake_step_function) {
		bake_step_function(0, String("Applying Gamma"));
	}

	if (!material_only) {
		for (int y = 0; y < _height; y++) {
			for (int x = 0; x < _width; x++) {
#if 0
			float ao = 0.0f;

			if (_image_AO.get_num_pixels())
				ao = _image_AO.get_item(x, y);

			const FColor &light = _image_main.get_item(x, y);

			// combined
			FColor total;
			switch (settings.bake_mode) {
				case LMBAKEMODE_LIGHTMAP: {
					total = light;
				} break;
				case LMBAKEMODE_AO: {
					total.set(ao);
				} break;
				default: {
					FColor applied = light * ao;
					total = light;
					total.lerp(applied, light_AO_ratio);

#if 0					
					FColor mid = light * ao;

					if (light_AO_ratio < 0.5f) {
						float r = light_AO_ratio / 0.5f;
						total.set((1.0f - r) * ao);
						total += mid * r;
					} else {
						float r = (light_AO_ratio - 0.5f) / 0.5f;
						total = mid * (1.0f - r);
						total += light * r;
					}
#endif
				} break;
			}

#endif
				FColor &total = _image_main.get_item(x, y);

				// gamma correction
				if (!settings.combined_is_HDR) {
					total.r = Math::pow((double)total.r, (double)gamma);
					total.g = Math::pow((double)total.g, (double)gamma);
					total.b = Math::pow((double)total.b, (double)gamma);
				}

				/*
				Color col;
				col = Color(f.r, f.g, f.b, 1);

				// new... RGBM .. use a multiplier in the alpha to get increased dynamic range!
				//ColorToRGBM(col);

				image.set_pixel(x, y, col);
				*/
			} // for x
		} // for y
	} // if not material only

#if 0
	// Test fill with random colors.
	for (int n=0; n<_image_main.get_num_pixels(); n++)
	{
		_image_main.get(n)->set(Math::randf(), Math::randf(), Math::randf());
	}
#endif

	// One more dilate?
	if (data.params[PARAM_DILATE_ENABLED] == Variant(true)) {
		if (bake_step_function) {
			bake_step_function(0, String("Dilate"));
		}

		Dilate<FColor> dilate;
		dilate.dilate_image(_image_main, _image_tri_ids_p1, 256);
	}

	if (data.params[PARAM_SEAM_STITCHING_ENABLED]) {
		if (bake_step_function) {
			bake_step_function(0, String("Stitch Seams"));
		}
		stitch_seams();
	}

	// mark magenta
	if (data.params[PARAM_DILATE_ENABLED] != Variant(true)) {
		_mark_dilated_area(_image_main);
	}
}

void LightMapper_Base::_mark_dilated_area(LightImage<FColor> &r_image) {
	if (!_image_tri_ids_p1.get_num_pixels()) {
		return;
	}
	for (uint32_t y = 0; y < _height; y++) {
		for (uint32_t x = 0; x < _width; x++) {
			if (_image_tri_ids_p1.get_item(x, y) == 0) {
				FColor &fcol = r_image.get_item(x, y);
#ifdef LLIGHTMAP_DEBUG_RECLAIMED_TEXELS
				if (_image_reclaimed_texels.get_item(x, y) == 0) {
					fcol.set(1, 0, 1);
				} else {
					fcol.set(1, 0, 0);
				}
#else
				fcol.set(1, 0, 1);
#endif
			}
		} // for x
	} // for y
}

void LightMapper_Base::_load_texel_data(uint32_t p_tri_id, const Vector3 &p_bary, Vector3 &r_pos_pushed, Vector3 &r_normal, const Vector3 **r_plane_normal) const {
	// new .. cap the barycentric to prevent edge artifacts
	const float clamp_margin = 0.0001f;
	const float clamp_margin_high = 1.0f - clamp_margin;

	Vector3 bary_clamped;
	bary_clamped.x = CLAMP(p_bary.x, clamp_margin, clamp_margin_high);
	bary_clamped.y = CLAMP(p_bary.y, clamp_margin, clamp_margin_high);
	bary_clamped.z = CLAMP(p_bary.z, clamp_margin, clamp_margin_high);

	// we will trace
	// FROM THE SURFACE TO THE LIGHT!!
	// this is very important, because the ray is origin and direction,
	// and there will be precision errors at the destination.
	// At the light end this doesn't matter, but if we trace the other way
	// we get artifacts due to precision loss due to normalized direction.
	_scene._tris[p_tri_id].interpolate_barycentric(r_pos_pushed, bary_clamped);

	// add epsilon to pos to prevent self intersection and neighbour intersection
	const Vector3 &plane_normal = _scene._tri_planes[p_tri_id].normal;
	*r_plane_normal = &plane_normal;

	r_pos_pushed += plane_normal * adjusted_settings.surface_bias;

	_scene._tri_normals[p_tri_id].interpolate_barycentric(r_normal, p_bary.x, p_bary.y, p_bary.z);
}

bool LightMapper_Base::load_texel_data(int32_t p_x, int32_t p_y, uint32_t &r_tri_id, const Vector3 **r_bary, Vector3 &r_pos_pushed, Vector3 &r_normal, const Vector3 **r_plane_normal) const {
	// find triangle
	r_tri_id = *_image_tri_ids_p1.get(p_x, p_y);
	if (!r_tri_id)
		return false;
	r_tri_id--; // plus one based

	// barycentric
	const Vector3 &bary = _image_barycentric.get_item(p_x, p_y);
	*r_bary = &bary;

	_load_texel_data(r_tri_id, bary, r_pos_pushed, r_normal, r_plane_normal);
	return true;

	/*
	// new .. cap the barycentric to prevent edge artifacts
	const float clamp_margin = 0.0001f;
	const float clamp_margin_high = 1.0f - clamp_margin;

	 Vector3 bary_clamped;
	 bary_clamped.x = CLAMP(bary.x, clamp_margin, clamp_margin_high);
	 bary_clamped.y = CLAMP(bary.y, clamp_margin, clamp_margin_high);
	 bary_clamped.z = CLAMP(bary.z, clamp_margin, clamp_margin_high);

	  // we will trace
	  // FROM THE SURFACE TO THE LIGHT!!
	  // this is very important, because the ray is origin and direction,
	  // and there will be precision errors at the destination.
	  // At the light end this doesn't matter, but if we trace the other way
	  // we get artifacts due to precision loss due to normalized direction.
	  _scene._tris[r_tri_id].interpolate_barycentric(r_pos_pushed, bary_clamped);

	 // add epsilon to pos to prevent self intersection and neighbour intersection
	 const Vector3 &plane_normal = _scene._tri_planes[r_tri_id].normal;
	 *r_plane_normal = &plane_normal;

	  r_pos_pushed += plane_normal * adjusted_settings.surface_bias;

	 _scene._tri_normals[r_tri_id].interpolate_barycentric(r_normal, bary.x, bary.y, bary.z);

	  return true;
	  */
}

void LightMapper_Base::show_warning(String sz, bool bAlert) {
#ifdef TOOLS_ENABLED
	EditorNode::get_singleton()->show_warning(TTR(sz));
	WARN_PRINT(sz);

//	if (bAlert)
//		OS::get_singleton()->alert(sz, "WARNING");
#else
	WARN_PRINT(sz);
#endif
}
