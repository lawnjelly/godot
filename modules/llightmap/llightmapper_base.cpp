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
	data.params[PARAM_GLOW] = 1.0f;

	data.params[PARAM_AO_RANGE] = 2.0f;
	data.params[PARAM_AO_NUM_SAMPLES] = 256;

	settings.mode = LMMODE_BACKWARD;
	settings.bake_mode = LMBAKEMODE_LIGHTMAP;
	settings.quality = LM_QUALITY_MEDIUM;

	// 0 is infinite
	data.params[PARAM_MAX_LIGHT_DISTANCE] = 0;

	data.params[PARAM_VOXEL_DENSITY] = 20;
	data.params[PARAM_SURFACE_BIAS] = 0.005f;

	data.params[PARAM_MATERIAL_SIZE] = 256;

	data.params[PARAM_NORMALIZE] = true;
	data.params[PARAM_NORMALIZE_MULTIPLIER] = 4.0f;
	data.params[PARAM_AO_LIGHT_RATIO] = 0.5f;
	data.params[PARAM_GAMMA] = 2.2f;

	settings.lightmap_is_HDR = false;
	settings.ambient_is_HDR = false;
	settings.combined_is_HDR = false;

	data.params[PARAM_UV_PADDING] = 4;

	data.params[PARAM_PROBE_DENSITY] = 64;
	data.params[PARAM_PROBE_SAMPLES] = 4096;

	data.params[PARAM_NOISE_THRESHOLD] = 0.1f;
	data.params[PARAM_NOISE_REDUCTION] = 1.0f;
	settings.noise_reduction_method = NR_ADVANCED;

	data.params[PARAM_DILATE_ENABLED] = true;

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

	data.params[PARAM_EMISSION_ENABLED] = true;

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
	_image_L.reset();
	_image_L_mirror.reset();

	_image_AO.reset();
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

	as.glow = data.params[PARAM_GLOW];
	as.glow *= as.glow;
	as.smoothness = 1.0f - (float)data.params[PARAM_ROUGHNESS];

	as.backward_num_rays = settings.backward_num_rays;

	as.num_ambient_bounces = data.params[PARAM_NUM_AMBIENT_BOUNCES];
	as.num_ambient_bounce_rays = data.params[PARAM_NUM_AMBIENT_BOUNCE_RAYS];
	as.num_directional_bounces = data.params[PARAM_NUM_BOUNCES];
	as.directional_bounce_power = data.params[PARAM_BOUNCE_POWER];

	as.num_AO_samples = data.params[PARAM_AO_NUM_SAMPLES];
	as.AO_range = data.params[PARAM_AO_RANGE];

	as.max_material_size = data.params[PARAM_MATERIAL_SIZE];
	as.normalize_multiplier = data.params[PARAM_NORMALIZE_MULTIPLIER];

	as.num_sky_samples = data.params[PARAM_SKY_SAMPLES];
	as.sky_brightness = data.params[PARAM_SKY_BRIGHTNESS];
	as.sky_brightness *= as.sky_brightness;

	as.antialias_samples_width = 3; // 16
	as.antialias_samples_per_texel = as.antialias_samples_width * as.antialias_samples_width;

	// overrides
	switch (settings.quality) {
		case LM_QUALITY_LOW: {
			as.num_primary_rays = 1;
			as.forward_num_rays = 1;
			as.backward_num_rays = 4;
			as.num_AO_samples = 1;
			as.max_material_size = 32;
			as.num_ambient_bounces = 0;
			as.num_directional_bounces = 0;
			as.num_sky_samples = 64;
		} break;
		case LM_QUALITY_MEDIUM: {
			as.num_primary_rays /= 2;
			as.num_AO_samples /= 2;
			as.max_material_size /= 4;
			as.num_ambient_bounce_rays /= 2;
			as.num_sky_samples /= 2;
		} break;
		default:
			// high is default
			break;
		case LM_QUALITY_FINAL:
			as.num_primary_rays *= 2;
			as.num_AO_samples *= 2;
			as.num_ambient_bounce_rays *= 2;
			as.num_sky_samples *= 2;
			break;
	}

	// minimums
	as.num_primary_rays = MAX(as.num_primary_rays, 1);

	as.forward_num_rays = (as.num_primary_rays * 16) / 32;
	as.backward_num_rays = (as.num_primary_rays * 128) / 32;

	as.forward_num_rays = MAX(as.forward_num_rays, 1);

	as.num_ambient_bounce_rays = MAX(as.num_ambient_bounce_rays, 1);
	as.num_AO_samples = MAX(as.num_AO_samples, 1);
	as.max_material_size = MAX(as.max_material_size, 32);
	as.emission_density = MAX(as.emission_density, 1);
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

		stitcher.stitch_object_seams(*mi, _image_L, data.params[PARAM_SEAM_DISTANCE_THRESHOLD], data.params[PARAM_SEAM_NORMAL_THRESHOLD], data.params[PARAM_VISUALIZE_SEAMS_ENABLED]);
	}
}

void LightMapper_Base::apply_noise_reduction() {
	switch (settings.noise_reduction_method) {
		case NR_DISABLE: {
		} break;
		case NR_SIMPLE: {
			// simple
			Convolution<FColor> conv;
			conv.run(_image_L, data.params[PARAM_NOISE_THRESHOLD], data.params[PARAM_NOISE_REDUCTION]);

			//	Convolution<float> conv_ao;
			//	conv_ao.Run(m_Image_AO, settings.NoiseThreshold, settings.NoiseReduction);
		} break;
		case NR_ADVANCED: {
			// use open image denoise
			void *device = oidn_denoiser_init();

			if (!oidn_denoise(device, (float *)_image_L.get(0), _image_L.get_width(), _image_L.get_height())) {
				WARN_PRINT("open image denoise error");
			}

			oidn_denoiser_finish(device);
		} break;
	}
}

void LightMapper_Base::normalize() {
	if (data.params[PARAM_NORMALIZE] != Variant(true))
		return;

	int nPixels = _image_L.get_num_pixels();
	float fmax = 0.0f;

	// first find the max
	for (int n = 0; n < nPixels; n++) {
		float f = _image_L.get(n)->max();
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
	mult *= adjusted_settings.normalize_multiplier;

	// apply multiplier
	for (int n = 0; n < nPixels; n++) {
		FColor &col = *_image_L.get(n);
		col = col * mult;
	}
}

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
			_image_L.get_item(x, y).set(image.get_pixel(x, y));
		}
	}
	image.unlock();

	return true;
}

bool LightMapper_Base::load_AO(Image &image) {
	//	assert (image.get_width() == m_iWidth);
	//	assert (image.get_height() == m_iHeight);

	Error res = image.load(settings.ambient_filename);
	if (res != OK) {
		show_warning("Loading AO EXR failed.\n\n" + settings.ambient_filename);
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

	return true;
}

void LightMapper_Base::merge_and_write_output_image_combined(Image &image) {
	// normalize lightmap on combine
	normalize();

	// merge them both before applying noise reduction and seams
	float gamma = 1.0f / (float)data.params[PARAM_GAMMA];
	float light_AO_ratio = data.params[PARAM_AO_LIGHT_RATIO];

	for (int y = 0; y < _height; y++) {
		for (int x = 0; x < _width; x++) {
			float ao = 0.0f;

			if (_image_AO.get_num_pixels())
				ao = _image_AO.get_item(x, y);

			FColor lum = _image_L.get_item(x, y);

			// combined
			FColor f;
			switch (settings.bake_mode) {
				case LMBAKEMODE_LIGHTMAP: {
					f = lum;
				} break;
				case LMBAKEMODE_AO: {
					f.set(ao);
				} break;
				default: {
					FColor mid = lum * ao;

					if (light_AO_ratio < 0.5f) {
						float r = light_AO_ratio / 0.5f;
						f.set((1.0f - r) * ao);
						f += mid * r;
					} else {
						float r = (light_AO_ratio - 0.5f) / 0.5f;
						f = mid * (1.0f - r);
						f += lum * r;
					}
				} break;
			}

			// gamma correction
			if (!settings.combined_is_HDR) {
				f.r = powf(f.r, gamma);
				f.g = powf(f.g, gamma);
				f.b = powf(f.b, gamma);
			}

			/*
			Color col;
			col = Color(f.r, f.g, f.b, 1);

			// new... RGBM .. use a multiplier in the alpha to get increased dynamic range!
			//ColorToRGBM(col);

			image.set_pixel(x, y, col);
			*/
			// write back to L
			*_image_L.get(x, y) = f;
		}
	}

	apply_noise_reduction();

	stitch_seams();

	// assuming both lightmap and AO are already dilated
	// final version
	image.lock();

	for (int y = 0; y < _height; y++) {
		for (int x = 0; x < _width; x++) {
			FColor f = _image_L.get_item(x, y);

			Color col;
			col = Color(f.r, f.g, f.b, 1);

			// new... RGBM .. use a multiplier in the alpha to get increased dynamic range!
			//ColorToRGBM(col);

			image.set_pixel(x, y, col);
		}
	}

	// mark magenta
	//	if (settings.bake_mode == LMBAKEMODE_LIGHTMAP || settings.bake_mode == LMBAKEMODE_AO) {
	if (_image_tri_ids_p1.get_num_pixels()) {
		if (data.params[PARAM_DILATE_ENABLED] != Variant(true)) {
			for (uint32_t y = 0; y < _height; y++) {
				for (uint32_t x = 0; x < _width; x++) {
					if (_image_tri_ids_p1.get_item(x, y) == 0) {
#ifdef LLIGHTMAP_DEBUG_RECLAIMED_TEXELS
						if (_image_reclaimed_texels.get_item(x, y) == 0) {
							image.set_pixel(x, y, Color(1, 0, 1));
						} else {
							image.set_pixel(x, y, Color(1, 0, 0));
						}
#else
						image.set_pixel(x, y, Color(1, 0, 1));
#endif
					}
				}
			}
		}
	}

	/*
	float gamma = 1.0f / settings.Gamma;

	for (int y = 0; y < m_iHeight; y++) {
		for (int x = 0; x < m_iWidth; x++) {
			float ao = 0.0f;

			if (m_Image_AO.GetNumPixels())
				ao = m_Image_AO.GetItem(x, y);

			FColor lum = m_Image_L.GetItem(x, y);

			// combined
			FColor f;
			switch (settings.BakeMode) {
				case LMBAKEMODE_LIGHTMAP: {
					f = lum;
				} break;
				case LMBAKEMODE_AO: {
					f.Set(ao);
				} break;
				default: {
					FColor mid = lum * ao;

					if (settings.Light_AO_Ratio < 0.5f) {
						float r = settings.Light_AO_Ratio / 0.5f;
						f.Set((1.0f - r) * ao);
						f += mid * r;
					} else {
						float r = (settings.Light_AO_Ratio - 0.5f) / 0.5f;
						f = mid * (1.0f - r);
						f += lum * r;
					}
				} break;
			}

			// gamma correction
			if (!settings.CombinedIsHDR) {
				f.r = powf(f.r, gamma);
				f.g = powf(f.g, gamma);
				f.b = powf(f.b, gamma);
			}

			Color col;
			col = Color(f.r, f.g, f.b, 1);

			// new... RGBM .. use a multiplier in the alpha to get increased dynamic range!
			//ColorToRGBM(col);

			image.set_pixel(x, y, col);
		}
	}
	*/

	image.unlock();
}

void LightMapper_Base::write_output_image_AO(Image &image) {
	if (!_image_AO.get_num_pixels())
		return;

	if (data.params[PARAM_DILATE_ENABLED] == Variant(true)) {
		Dilate<float> dilate;
		dilate.dilate_image(_image_AO, _image_tri_ids_p1, 256);
	}

	// final version
	image.lock();

	for (int y = 0; y < _height; y++) {
		for (int x = 0; x < _width; x++) {
			const float *pf = _image_AO.get(x, y);
			assert(pf);
			float f = *pf;

			// gamma correction
			if (!settings.ambient_is_HDR) {
				float gamma = 1.0f / 2.2f;
				f = powf(f, gamma);
			}

			Color col;
			col = Color(f, f, f, 1);

			// debug mark the dilated pixels
//#define MARK_AO_DILATED
#ifdef MARK_AO_DILATED
			if (!dilate && !_image_tri_ids_p1.get_item(x, y)) {
				col = Color(1, 0, 1);
			}
#endif
			image.set_pixel(x, y, col);
		}
	}

	image.unlock();
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

void LightMapper_Base::write_output_image_lightmap(Image &image) {
	if (data.params[PARAM_DILATE_ENABLED] == Variant(true)) {
		Dilate<FColor> dilate;
		dilate.dilate_image(_image_L, _image_tri_ids_p1, 256);
		//	} else {
		//		// mark magenta
		//		for (uint32_t y = 0; y < _height; y++) {
		//			for (uint32_t x = 0; x < _width; x++) {
		//				if (_image_tri_ids_p1.get_item(x, y) == 0) {
		//					_image_L.get_item(x, y).set(1, 0, 1);
		//				}
		//			}
		//		}
	}

	////
	// write some debug
//#define LLIGHTMAPPER_OUTPUT_TRIIDS
#ifdef LLIGHTMAPPER_OUTPUT_TRIIDS
	output_image.lock();
	Color cols[1024];
	for (int n = 0; n < m_Scene.GetNumTris(); n++) {
		if (n == 1024)
			break;

		cols[n] = Color(Math::randf(), Math::randf(), Math::randf(), 1.0f);
	}
	cols[0] = Color(0, 0, 0, 1.0f);

	for (int y = 0; y < m_iHeight; y++) {
		for (int x = 0; x < m_iWidth; x++) {
			int coln = m_Image_ID_p1.GetItem(x, y) % 1024;

			output_image.set_pixel(x, y, cols[coln]);
		}
	}

	output_image.unlock();
	output_image.save_png("tri_ids.png");
#endif

	// final version
	image.lock();

	for (int y = 0; y < _height; y++) {
		for (int x = 0; x < _width; x++) {
			FColor f = *_image_L.get(x, y);

			// gamma correction
			if (!settings.lightmap_is_HDR) {
				float gamma = 1.0f / 2.2f;
				f.r = powf(f.r, gamma);
				f.g = powf(f.g, gamma);
				f.b = powf(f.b, gamma);
			}

			Color col;
			col = Color(f.r, f.g, f.b, 1);

			// debug mark the dilated pixels
//#define MARK_DILATED
#ifdef MARK_DILATED
			if (!m_Image_ID_p1.GetItem(x, y)) {
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

			image.set_pixel(x, y, col);
		}
	}

	image.unlock();
}
