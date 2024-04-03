#include "llightmapper.h"
#include "core/os/threaded_array_processor.h"
#include "llightprobe.h"
#include "llightscene.h"
#include "lmerger.h"
#include "lscene_saver.h"
#include "lunmerger.h"
#include "scene/resources/packed_scene.h"

namespace LM {

bool LightMapper::uv_map_meshes(Spatial *pRoot) {
	if (settings.UV_filename == "") {
		show_warning("UV Filename is not set, aborting.");
		return false;
	}

	calculate_quality_adjusted_settings();

	bool replace_mesh_scene = false;

	if (!pRoot)
		return false;

	// can't do uv mapping if not tools build, as xatlas isn't compiled
	float steps = 4;
	//	if (bake_begin_function) {
	//		bake_begin_function(4);
	//	}

	if (bake_step_function) {
		bake_step_function(0 / steps, String("Saving uvmap_backup.tscn"));
	}

	// first back up the existing meshes scene.
	SceneSaver saver;
	saver.save_scene(pRoot, "res://uvmap_backup.tscn");

	if (bake_step_function) {
		bake_step_function(1 / steps, String("Merging to proxy"));
	}

	Merger m;
	MeshInstance *pMerged = m.merge(pRoot, data.params[PARAM_UV_PADDING]);
	if (!pMerged) {
		if (bake_end_function) {
			bake_end_function();
		}
		return false;
	}

	// test save the merged mesh
	//saver.SaveScene(pMerged, "res://merged_test.tscn");

	if (bake_step_function) {
		bake_step_function(2 / steps, String("Unmerging"));
	}

	// unmerge
	UnMerger u;
	bool res = u.unmerge(m, *pMerged);

	// for debug save the merged version
	saver.save_scene(pMerged, "res://merged_proxy.tscn");

	pMerged->queue_delete();

	if (bake_step_function) {
		bake_step_function(2 / steps, String("Saving"));
	}

	// if we specified an output file, save
	if (settings.UV_filename != "") {
		Node *pOrigOwner = pRoot->get_owner();

		if (!saver.save_scene(pRoot, settings.UV_filename, true)) {
			show_warning("Error saving UV mapped scene. Does the folder exist?\n\n" + settings.UV_filename);
		}

		if (replace_mesh_scene) {
			// delete the orig
			Node *pParent = pRoot->get_parent();

			// rename the old scene to prevent naming conflict
			pRoot->set_name("ToBeDeleted");
			pRoot->queue_delete();

			// now load the new file
			//ResourceLoader::import(settings.UVFilename);

			Ref<PackedScene> ps = ResourceLoader::load(settings.UV_filename, "PackedScene");
			if (ps.is_null()) {
				if (bake_end_function) {
					bake_end_function();
				}
				return res;
			}

			Node *pFinalScene = ps->instance();
			if (pFinalScene) {
				pParent->add_child(pFinalScene);

				// set owners
				saver.set_owner_recursive(pFinalScene, pFinalScene);
				pFinalScene->set_owner(pOrigOwner);
			}

		} // if replace the scene
		else {
			// delete, to save confusion
			pRoot->queue_delete();
		}
	}

	if (bake_end_function) {
		bake_end_function();
	}

	return res;
}

bool LightMapper::lightmap_meshes(Spatial *pMeshesRoot, Spatial *pLR, Image *pIm_Lightmap, Image *pIm_AO, Image *pIm_Combined) {
	calculate_quality_adjusted_settings();

	// get the output dimensions before starting, because we need this
	// to determine number of rays, and the progress range
	_width = pIm_Combined->get_width();
	_height = pIm_Combined->get_height();
	_num_rays = adjusted_settings.forward_num_rays;

	int nTexels = _width * _height;
	_num_rays_forward = _num_rays * nTexels;

	// set num rays depending on method
	//	if (settings.mode == LMMODE_FORWARD) {
	//		// the num rays / texel. This is per light!
	//		_num_rays *= nTexels;
	//	}

	// do twice to test SIMD
	uint32_t beforeA = OS::get_singleton()->get_ticks_msec();
	_scene._use_SIMD = true;
	_scene._tracer._use_SIMD = true;

	bool res = _lightmap_meshes(pMeshesRoot, *pLR, *pIm_Lightmap, *pIm_AO, *pIm_Combined);

	uint32_t afterA = OS::get_singleton()->get_ticks_msec();
	print_line("Overall took : " + itos(afterA - beforeA));

	return res;
}

void LightMapper::reset() {
	_lights.clear(true);
	_scene.reset();
	ray_bank_reset();
	base_reset();
}

void LightMapper::refresh_process_state() {
	// defaults
	logic.process_lightmap = true;
	logic.process_AO = true;
	logic.reserve_AO = true;
	logic.process_probes = false;
	logic.output_final = true;
	logic.rasterize_mini_lists = true;

	// process states
	switch (settings.bake_mode) {
		case LMBAKEMODE_PROBES: {
			logic.process_probes = true;
			logic.process_lightmap = false;
			logic.process_AO = false;
			logic.output_final = false;
		} break;
		case LMBAKEMODE_LIGHTMAP: {
			logic.process_AO = false;
			logic.reserve_AO = false;
		} break;
		case LMBAKEMODE_AO: {
			logic.process_lightmap = false;
		} break;
		case LMBAKEMODE_MERGE: {
			logic.process_lightmap = false;
			logic.process_AO = false;
		} break;
		case LMBAKEMODE_UVMAP: {
			logic.process_lightmap = false;
			logic.process_AO = false;
			logic.reserve_AO = false;
			logic.output_final = false;
		} break;
		default: {
		} break;
	}

	// logic.rasterize_mini_lists = logic.process_AO;
}

bool LightMapper::_lightmap_meshes(Spatial *pMeshesRoot, const Spatial &light_root, Image &out_image_lightmap, Image &out_image_ao, Image &out_image_combined) {
	// print out settings
	print_line("Lightmap mesh");
	print_line("\tnum_directional_bounces " + itos(adjusted_settings.num_directional_bounces));
	print_line("\tdirectional_bounce_power " + String(data.params[PARAM_BOUNCE_POWER]));

	refresh_process_state();

	reset();
	_pressed_cancel = false;

	if (_width <= 0)
		return false;
	if (_height <= 0)
		return false;

	// create stuff used by everything
	_image_L.create(_width, _height);
	_image_L_mirror.create(_width, _height);

	_scene._image_emission_done.create(_width, _height);

	// whether we need storage
	if (logic.reserve_AO)
		_image_AO.create(_width, _height);

	if (settings.bake_mode != LMBAKEMODE_MERGE) {
		if (logic.process_AO) {
			if (bake_step_function) {
				bake_step_function(0, String("QMC Create"));
			}
			_QMC.create(adjusted_settings.num_AO_samples);
		}

		uint32_t before, after;
		find_lights_recursive(&light_root);
		print_line("Found " + itos(_lights.size()) + " lights.");

		_image_ID_p1.create(_width, _height);
		//m_Image_ID2_p1.Create(m_iWidth, m_iHeight);

		if (logic.rasterize_mini_lists) {
			_image_tri_ids.create(_width, _height);
			_tri_ids.clear(true);
		}

		if (bake_step_function) {
			bake_step_function(0, String("Barycentric Create"));
		}
		_image_barycentric.create(_width, _height);

		//		if (bake_end_function) {
		//			bake_end_function();
		//		}

		//m_Image_Cuts.Create(m_iWidth, m_iHeight);
		//m_CuttingTris.clear(true);

		if (bake_step_function) {
			bake_step_function(0, String("Scene Create"));
		}
		print_line("Scene Create");
		before = OS::get_singleton()->get_ticks_msec();
		if (!_scene.create(pMeshesRoot, _width, _height, data.params[PARAM_VOXEL_DENSITY], adjusted_settings.max_material_size, adjusted_settings.emission_density))
			return false;

		if (bake_step_function) {
			bake_step_function(0, String("Prepare Lights"));
		}

		prepare_lights();

		ray_bank_create();

		after = OS::get_singleton()->get_ticks_msec();
		print_line("SceneCreate took " + itos(after - before) + " ms");

		if (_pressed_cancel)
			return false;

		print_line("PrepareImageMaps");
		before = OS::get_singleton()->get_ticks_msec();
		if (!prepare_image_maps()) {
			_pressed_cancel = true;
		}
		after = OS::get_singleton()->get_ticks_msec();
		print_line("PrepareImageMaps took " + itos(after - before) + " ms");

		if (_pressed_cancel)
			return false;

		if (logic.process_AO) {
			print_line("ProcessAO");
			before = OS::get_singleton()->get_ticks_msec();
			process_AO();
			after = OS::get_singleton()->get_ticks_msec();
			print_line("ProcessAO took " + itos(after - before) + " ms");

			//Convolve_AO();
		}

		if (logic.process_lightmap) {
			print_line("ProcessTexels");
			before = OS::get_singleton()->get_ticks_msec();
			if (settings.mode == LMMODE_BACKWARD)
				backward_process_texels();
			else {
				process_lights();
			}
			if (data.params[PARAM_EMISSION_ENABLED] == Variant(true)) {
#define LLIGHTMAP_EMISSION_USE_PIXELS
#ifdef LLIGHTMAP_EMISSION_USE_PIXELS
				process_emission_pixels();
#else
				process_emission_tris();
#endif
			}
			do_ambient_bounces();
			after = OS::get_singleton()->get_ticks_msec();
			print_line("ProcessTexels took " + itos(after - before) + " ms");
		}

		if (logic.process_probes) {
			// calculate probes
			print_line("ProcessProbes");
			if (load_lightmap(out_image_lightmap)) {
				process_light_probes();
			}
		}

		if (_pressed_cancel)
			return false;

		if (!logic.process_probes) {
			write_output_image_lightmap(out_image_lightmap);
			write_output_image_AO(out_image_ao);

			// test convolution
			//			Merge_AndWriteOutputImage_Combined(out_image_combined);
			//			out_image_combined.save_png("before_convolve.png");

			//			WriteOutputImage_AO(out_image_ao, true);
		}

	} // if not just merging
	else {
		// merging
		// need the meshes list for seam stitching
		_scene.find_meshes(pMeshesRoot);

		// load the lightmap and ao from disk
		load_lightmap(out_image_lightmap);
		load_AO(out_image_ao);
	}

	//	print_line("WriteOutputImage");
	//	before = OS::get_singleton()->get_ticks_msec();
	if (logic.output_final)
		merge_and_write_output_image_combined(out_image_combined);
	//	after = OS::get_singleton()->get_ticks_msec();
	//	print_line("WriteOutputImage took " + itos(after -before) + " ms");

	// clear everything out of ram as no longer needed
	reset();

	return true;
}

void LightMapper::process_texels_ambient_bounce_line_MT(uint32_t offset_y, int start_y) {
	int y = offset_y + start_y;

	for (int x = 0; x < _width; x++) {
		// find triangle
		uint32_t tri_source = *_image_ID_p1.get(x, y);
		if (tri_source) {
			tri_source--; // plus one based
			FColor power = process_texel_ambient_bounce(x, y, tri_source);

			// save the incoming light power in the mirror image (as the source is still being used)
			_image_L_mirror.get_item(x, y) = power;
		}
	}
}

void LightMapper::process_texels_ambient_bounce(int section_size, int num_sections) {
	_image_L_mirror.blank();

	// disable multithread
	//num_sections = 0;

	for (int s = 0; s < num_sections; s++) {
		int section_start = s * section_size;

		if (bake_step_function) {
			_pressed_cancel = bake_step_function(section_start / (float)_height, String("Process TexelsBounce: ") + " (" + itos(section_start) + ")");
			if (_pressed_cancel) {
				if (bake_end_function)
					bake_end_function();
				return;
			}
		}

		thread_process_array(section_size, this, &LightMapper::process_texels_ambient_bounce_line_MT, section_start);

		//		for (int n=0; n<section_size; n++)
		//		{
		//			ProcessTexel_Line_MT(n, section_start);
		//		}
	}

	int leftover_start = num_sections * section_size;

	for (int y = leftover_start; y < _height; y++) {
		if ((y % 10) == 0) {
			//			print_line("\tTexels bounce line " + itos(y));
			//			OS::get_singleton()->delay_usec(1);

			if (bake_step_function) {
				_pressed_cancel = bake_step_function(y / (float)_height, String("Process TexelsBounce: ") + " (" + itos(y) + ")");
				if (_pressed_cancel)
					return;
			}
		}

		process_texels_ambient_bounce_line_MT(y, 0);
		//		for (int x=0; x<m_iWidth; x++)
		//		{
		//			FColor power = ProcessTexel_Bounce(x, y);

		//			// save the incoming light power in the mirror image (as the source is still being used)
		//			m_Image_L_mirror.GetItem(x, y) = power;
		//		}
	}

	// merge the 2 luminosity maps
	float ambient_bounce_power = data.params[PARAM_AMBIENT_BOUNCE_POWER];

	for (int y = 0; y < _height; y++) {
		for (int x = 0; x < _width; x++) {
			FColor col = _image_L.get_item(x, y);

			FColor col_add = _image_L_mirror.get_item(x, y) * ambient_bounce_power;

			//			assert (col_add.r >= 0.0f);
			//			assert (col_add.g >= 0.0f);
			//			assert (col_add.b >= 0.0f);

			col += col_add;

			_image_L.get_item(x, y) = col;
		}
	}
}

void LightMapper::backward_trace_triangles() {
	int nTris = _scene._tris.size();

	//	if (bake_begin_function) {
	//		int progress_range = nTris;
	//		bake_begin_function(progress_range);
	//	}

	for (int n = 0; n < nTris; n++) {
		if ((n % 128) == 0) {
			if (bake_step_function) {
				_pressed_cancel = bake_step_function(n / (float)nTris, String("Process Backward Tris: ") + " (" + itos(n) + ")");
				if (_pressed_cancel) {
					if (bake_end_function)
						bake_end_function();
					return;
				}
			}
		}

		backward_trace_triangle(n);
	}

	if (bake_end_function) {
		bake_end_function();
	}
}

void LightMapper::backward_trace_triangle(int tri_id) {
	const UVTri &tri = _scene._uv_tris[tri_id];

	float area = tri.calculate_twice_area();
	if (area < 0.0f)
		area = -area;

	int nSamples = area * 400000.0f * adjusted_settings.backward_num_rays;

	for (int n = 0; n < nSamples; n++) {
		backward_trace_sample(tri_id);
	}
}

void LightMapper::backward_trace_sample(int tri_id) {
	const UVTri &uv_tri = _scene._uv_tris[tri_id];
	const Tri &tri = _scene._tris[tri_id];

	// choose random point on triangle
	Vector3 bary;
	generate_random_barycentric(bary);

	// test, clamp the barycentric
	//	bary *= 0.998f;
	//	bary += Vector3(0.001f, 0.001f, 0.001f);

	// get position in world space
	Vector3 pos;
	tri.interpolate_barycentric(pos, bary);

	// test, pull in a little
	//pos = pos.linear_interpolate(tri.GetCentre(), 0.001f);

	// get uv / texel position
	Vector2 uv;
	uv_tri.find_uv_barycentric(uv, bary);
	uv.x *= _width;
	uv.y *= _height;

	// round?
	int tx = uv.x;
	int ty = uv.y;
	//	int tx = Math::round(uv.x);
	//	int ty = Math::round(uv.y);

	// could be off the image
	FColor *pTexel = _image_L.get(tx, ty);
	if (!pTexel)
		return;

	// apply bias
	// add epsilon to pos to prevent self intersection and neighbour intersection
	const Vector3 &plane_normal = _scene._tri_planes[tri_id].normal;
	pos += plane_normal * adjusted_settings.surface_bias;

	// interpolated normal
	Vector3 normal;
	_scene._tri_normals[tri_id].interpolate_barycentric(normal, bary.x, bary.y, bary.z);

	FColor temp;
	for (int l = 0; l < _lights.size(); l++) {
		BF_process_texel_light(Color(), l, pos, plane_normal, normal, temp, 1);
		*pTexel += temp;
	}

	// add emission
	Color emission_tex_color;
	Color emission_color;
	if (_scene.find_emission_color(tri_id, bary, emission_tex_color, emission_color)) {
		FColor femm;
		femm.set(emission_tex_color);

		//		float power = settings.Backward_RayPower * m_AdjustedSettings.m_Backward_NumRays * 128.0f;
		float power = settings.backward_ray_power * 128.0f;

		*pTexel += femm * power;
	}
}

void LightMapper::backward_process_texels() {
	//Backward_TraceTriangles();
	//return;

	//int nCores = OS::get_singleton()->get_processor_count();

	int section_size = _height / 64; //nCores;
	int num_sections = _height / section_size;
	int leftover_start = 0;

	//	if (bake_begin_function) {
	//		int progress_range = m_iHeight;
	//		bake_begin_function(progress_range);
	//	}

	_num_tests = 0;

	// load sky
	_sky.load_sky(settings.sky_filename, data.params[PARAM_SKY_BLUR], data.params[PARAM_SKY_SIZE]);

	// prevent multithread
	//#ifndef BACKWARD_TRACE_MULTITHEADED
	//	num_sections = 0;
	//#endif

//#if 1
#if (defined NO_THREADS) || (!defined BACKWARD_TRACE_MULTITHEADED)
	num_sections = 0;
#else
	for (int s = 0; s < num_sections; s++) {
		int section_start = s * section_size;

		if (bake_step_function) {
			_pressed_cancel = bake_step_function(section_start / (float)_height, String("Process Texels: ") + " (" + itos(section_start) + ")");
			if (_pressed_cancel) {
				if (bake_end_function)
					bake_end_function();
				return;
			}
		}

		thread_process_array(section_size, this, &LightMapper::backward_process_texel_line_MT, section_start);

		//		for (int n=0; n<section_size; n++)
		//		{
		//			ProcessTexel_Line_MT(n, section_start);
		//		}
	}
#endif

	leftover_start = num_sections * section_size;

	for (int y = leftover_start; y < _height; y++) {
		if ((y % 10) == 0) {
			//print_line("\tTexels line " + itos(y));
			//OS::get_singleton()->delay_usec(1);

			if (bake_step_function) {
				_pressed_cancel = bake_step_function(y / (float)_height, String("Process Texels: ") + " (" + itos(y) + ")");
				if (_pressed_cancel) {
					if (bake_end_function)
						bake_end_function();
					return;
				}
			}
		}

		backward_process_texel_line_MT(y, 0);
	}

	//	m_iNumTests /= (m_iHeight * m_iWidth);
	print_line("num tests : " + itos(_num_tests));

	_sky.unload_sky();

	if (bake_end_function) {
		bake_end_function();
	}
}

void LightMapper::do_ambient_bounces() {
	if (!adjusted_settings.num_ambient_bounces)
		return;

	int section_size = _height / 64; //nCores;
	int num_sections = _height / section_size;

	//	if (bake_begin_function) {
	//		int progress_range = m_iHeight;
	//		bake_begin_function(progress_range);
	//	}

	for (int b = 0; b < adjusted_settings.num_ambient_bounces; b++) {
		if (!_pressed_cancel)
			process_texels_ambient_bounce(section_size, num_sections);
	}

	if (bake_end_function) {
		bake_end_function();
	}
}

void LightMapper::backward_process_texel_line_MT(uint32_t offset_y, int start_y) {
	int y = offset_y + start_y;

	for (int x = 0; x < _width; x++) {
		BF_process_texel(x, y);
	}
}

bool LightMapper::light_random_sample(const LLight &light, const Vector3 &ptSurf, Ray &ray, Vector3 &ptLight, float &ray_length, float &multiplier) const {
	// for a spotlight, we can cull completely in a lot of cases.
	if (light.type == LLight::LT_SPOT) {
		Ray r;
		r.o = light.spot_emanation_point;
		r.d = ptSurf - r.o;
		r.d.normalize();
		float dot = r.d.dot(light.dir);
		//float angle = safe_acosf(dot);
		//if (angle >= light.spot_angle_radians)

		dot -= light.spot_dot_max;

		if (dot <= 0.0f)
			return false;
	}

	// default
	multiplier = 1.0f;

	switch (light.type) {
		case LLight::LT_DIRECTIONAL: {
			// we set this to maximum, better not to check at all but makes code simpler
			ray_length = FLT_MAX;

			ray.o = ptSurf;
			//r.d = -light.dir;

			// perturb direction depending on light scale
			//Vector3 ptTarget = light.dir * -2.0f;

			Vector3 offset;
			generate_random_unit_dir(offset);
			offset *= light.scale;

			offset += (light.dir * -2.0f);
			ray.d = offset.normalized();

			// disallow zero length (should be rare)
			if (ray.d.length_squared() < 0.00001f)
				return false;

			// don't allow from opposite direction
			if (ray.d.dot(light.dir) > 0.0f)
				ray.d = -ray.d;

			// this is used for intersection test to see
			// if e.g. sun is obscured. So we make the light position a long way from the origin
			ptLight = ptSurf + (ray.d * 10000000.0f);
		} break;
		case LLight::LT_SPOT: {
			// source
			float dot;

			while (true) {
				Vector3 offset;
				generate_random_unit_dir(offset);
				offset *= light.scale;

				ray.o = light.pos;
				ray.o += offset;

				// offset from origin to destination texel
				ray.d = ptSurf - ray.o;

				// normalize and find length
				ray_length = normalize_and_find_length(ray.d);

				dot = ray.d.dot(light.dir);
				//float angle = safe_acosf(dot);
				//if (angle >= light.spot_angle_radians)

				dot -= light.spot_dot_max;

				// if within cone, it is ok
				if (dot > 0.0f)
					break;
			}

			// store the light sample point
			ptLight = ray.o;

			// reverse ray for precision reasons
			ray.d = -ray.d;
			ray.o = ptSurf;

			dot *= 1.0f / (1.0f - light.spot_dot_max);
			multiplier = dot * dot;
			multiplier *= multiplier;
		} break;
		default: {
			Vector3 offset;
			generate_random_unit_dir(offset);
			offset *= light.scale;
			ptLight = light.pos + offset;

			// offset from origin to destination texel
			ray.o = ptSurf;
			ray.d = ptLight - ray.o;

			// normalize and find length
			ray_length = normalize_and_find_length(ray.d);

			// safety
			//assert (r.d.length() > 0.0f);

		} break;
	}

	// by this point...
	// ray should be set, ray_length, and ptLight
	return true;
}

bool LightMapper::BF_process_texel_sky(const Color &orig_albedo, const Vector3 &ptSource, const Vector3 &orig_face_normal, const Vector3 &orig_vertex_normal, FColor &color) {
	color.set(0.0);

	if (!_sky.is_active())
		return false;

	if (adjusted_settings.sky_brightness == 0.0f)
		return false;

	Ray r;

	// we need more samples for sky, than for a normal light
	//nSamples *= 4;

	int nSamples = adjusted_settings.num_sky_samples;

	FColor total;
	total.set(0.0);

	for (int s = 0; s < nSamples; s++) {
		r.o = ptSource;
		generate_random_hemi_unit_dir(r.d, orig_face_normal);

		// ray test
		if (!_scene.test_intersect_ray(r, FLT_MAX)) {
			_sky.read_sky(r.d, total);
		}

	} // for s

	color = total * (adjusted_settings.sky_brightness * (64.0 / nSamples));
	return true;
}

// trace from the poly TO the light, not the other way round, to avoid precision errors
void LightMapper::BF_process_texel_light(const Color &orig_albedo, int light_id, const Vector3 &ptSource, const Vector3 &orig_face_normal, const Vector3 &orig_vertex_normal, FColor &color, int nSamples) //, uint32_t tri_ignore)
{
	const LLight &light = _lights[light_id];

	Ray r;

	float power = light.energy;
	power *= settings.backward_ray_power;

	//int nSamples = m_AdjustedSettings.m_Backward_NumRays;

	// total light hitting texel
	color.set(0.0f);

	// for a spotlight, we can cull completely in a lot of cases.
	if (light.type == LLight::LT_SPOT) {
		r.o = light.spot_emanation_point;
		r.d = ptSource - r.o;
		r.d.normalize();
		float dot = r.d.dot(light.dir);
		//float angle = safe_acosf(dot);
		//if (angle >= light.spot_angle_radians)

		dot -= light.spot_dot_max;

		if (dot <= 0.0f)
			return;

		// cull by surface back facing NYI
	}

	// lower power of directionals - no distance falloff
	if (light.type == LLight::LT_DIRECTIONAL) {
		power *= 0.08f;

		// cull by back face determined by the sky softness ... NYI
	}

	// omni cull by distance?

	// cull by surface normal .. anything facing away gets no light (plus a bit for wraparound)
	// about 5% faster with this cull with a lot of omnis
	if (light.type == LLight::LT_OMNI) {
		Vector3 light_vec = light.pos - ptSource;
		float light_dist = light_vec.length();

		// cull by dist test
		if (adjusted_settings.max_light_distance) {
			if ((int)light_dist > adjusted_settings.max_light_distance)
				return;
		}

		float radius = MAX(light.scale.x, light.scale.y);
		radius = MAX(radius, light.scale.z);

		// we can only cull if outside the radius of the light
		if (light_dist > radius) {
			// normalize light vec
			light_vec *= 1.0f / light_dist;

			// angle in radians from the surface to the furthest points on the sphere
			float theta = Math::asin(CLAMP(radius / light_dist, 0.0f, 1.0f));

			float dot_threshold = Math::cos(theta + Math_PI / 2.0);
			float dot_light_surf = orig_vertex_normal.dot(light_vec);

			// doesn't need an epsilon, because at 90 degrees the dot will be zero
			// and the lighting will be zero.
			if (dot_light_surf < dot_threshold)
				return;
		}
	}

	// for quick primary test on the last blocking triangle
	int quick_reject_tri_id = -1;

	// each ray
	for (int n = 0; n < nSamples; n++) {
		Vector3 ptDest = light.pos;

		// allow falloff for cones
		float multiplier = 1.0f;

		// if the hit point is further that the distance to the light, then it doesn't count
		// must be set by the light type
		float ray_length;

		switch (light.type) {
			case LLight::LT_DIRECTIONAL: {
				// we set this to maximum, better not to check at all but makes code simpler
				ray_length = FLT_MAX;

				r.o = ptSource;
				//r.d = -light.dir;

				// perturb direction depending on light scale
				//Vector3 ptTarget = light.dir * -2.0f;

				Vector3 offset;
				generate_random_unit_dir(offset);
				offset *= light.scale;

				offset += (light.dir * -2.0f);
				r.d = offset.normalized();

				// disallow zero length (should be rare)
				if (r.d.length_squared() < 0.00001f)
					continue;

				// don't allow from opposite direction
				if (r.d.dot(light.dir) > 0.0f)
					r.d = -r.d;
			} break;
			case LLight::LT_SPOT: {
				// source
				float dot;

				while (true) {
					Vector3 offset;
					generate_random_unit_dir(offset);
					offset *= light.scale;

					r.o = light.pos;
					r.o += offset;

					// offset from origin to destination texel
					r.d = ptSource - r.o;

					// normalize and find length
					ray_length = normalize_and_find_length(r.d);

					dot = r.d.dot(light.dir);
					//float angle = safe_acosf(dot);
					//if (angle >= light.spot_angle_radians)

					dot -= light.spot_dot_max;

					// if within cone, it is ok
					if (dot > 0.0f)
						break;
				}

				// reverse ray for precision reasons
				r.d = -r.d;
				r.o = ptSource;

				dot *= 1.0f / (1.0f - light.spot_dot_max);
				multiplier = dot * dot;
				multiplier *= multiplier;
			} break;
			default: {
				Vector3 offset;
				generate_random_unit_dir(offset);
				offset *= light.scale;
				ptDest += offset;

				// offset from origin to destination texel
				r.o = ptSource;
				r.d = ptDest - r.o;

				// normalize and find length
				ray_length = normalize_and_find_length(r.d);

				// safety
				//assert (r.d.length() > 0.0f);

			} break;
		}

		// only bother tracing if the light is in front of the surface normal
		float dot_light_surf = orig_vertex_normal.dot(r.d);
		if (dot_light_surf <= 0.0f)
			continue;

		//Vector3 ray_origin = r.o;
		FColor sample_color = light.color;
		int panic_count = 32;

		// for bounces
		Vector3 hit_face_normal = orig_face_normal;

		bool keep_tracing = true;

		// quick reject triangle
		if (quick_reject_tri_id != -1) {
			if (_scene.test_intersect_ray_triangle(r, ray_length, quick_reject_tri_id)) {
				keep_tracing = false;
			}
		}

		while (keep_tracing) {
			keep_tracing = false;

			// collision detect
			float u, v, w, t;

			_scene._tracer._use_SDF = true;
			int tri = _scene.find_intersect_ray(r, u, v, w, t);
			//		m_Scene.m_Tracer.m_bUseSDF = false;
			//		int tri2 = m_Scene.IntersectRay(r, u, v, w, t, m_iNumTests);
			//		if (tri != tri2)
			//		{
			//			// repeat SDF version
			//			m_Scene.m_Tracer.m_bUseSDF = true;
			//			int tri = m_Scene.IntersectRay(r, u, v, w, t, m_iNumTests);
			//		}

			// further away than the destination?
			if (tri != -1) {
				if (t > ray_length)
					tri = -1;
				//				else
				//				{
				//					// we hit something, move the ray origin to the thing hit
				//					r.o += r.d * t;
				//				}
			}

			// nothing hit
			if (tri == -1)
			//if ((tri == -1) || (tri == (int) tri_ignore))
			{
				// for backward tracing, first pass, this is a special case, because we DO
				// take account of distance to the light, and normal, in order to simulate the effects
				// of the likelihood of 'catching' a ray. In forward tracing this happens by magic.
				float local_power;

				// no drop off for directional lights
				//float dist = (ptDest - ray_origin).length();
				float dist = ray_length;
				local_power = light_distance_drop_off(dist, light, power);

				// take into account normal
				float dot = r.d.dot(orig_vertex_normal);
				dot = fabs(dot);

				//local_power *= dot;

				// cone falloff
				local_power *= multiplier;

				// total color // this was incorrect because it also reduced the bounce power.
				sample_color *= local_power;
				color += (sample_color * dot);

				// new .. only apply the local power for the first hit, not for the bounces
				//				color += sample_color * (local_power * dot);

				// ONLY BOUNCE IF WE HIT THE TARGET SURFACE!!
				// start the bounces

				// probability of bounce based on alpha? NYI
				if (adjusted_settings.num_directional_bounces) {
					// pass the direction as incoming (i.e. from the light to the surface, not the other way around)

					// the source ray is always the target surface (no matter whether we hit transparent or blockers)
					r.o = ptSource;
					r.d = -r.d;

					// we need to apply the color from the original target surface
					sample_color.r *= orig_albedo.r;
					sample_color.g *= orig_albedo.g;
					sample_color.b *= orig_albedo.b;

					// bounce ray direction
					if (bounce_ray(r, orig_face_normal, false)) {
						// should this normal be plane normal or vertex normal?
						BF_process_texel_light_bounce(adjusted_settings.num_directional_bounces, r, sample_color);
					}
				}
			} else {
				// hit something, could be transparent

				// back face?
				bool bBackFace = hit_back_face(r, tri, Vector3(u, v, w), hit_face_normal);

				// first get the texture details
				/*
				Color albedo, emission;
				bool bTransparent;
				_scene.find_primary_texture_colors(tri, Vector3(u, v, w), albedo, emission, bTransparent);
				*/

				ColorSample cols;
				_scene.take_triangle_color_sample(tri, Vector3(u, v, w), cols);

				if (!cols.is_opaque) {
					bool opaque = cols.albedo.a > 0.999f;

					// push ray past the surface
					if (!opaque) {
						// hard code
						//						if (albedo.a > 0.0f)
						//							albedo.a = 0.3f;

						// position of potential hit
						Vector3 pos;
						const Tri &triangle = _scene._tris[tri];
						triangle.interpolate_barycentric(pos, u, v, w);

						float push = -adjusted_settings.surface_bias;
						if (bBackFace)
							push = -push;

						r.o = pos + (hit_face_normal * push);

						// apply the color to the ray
						calculate_transmittance(cols.albedo, sample_color);

						// this shouldn't happen, but if it did we'd get an infinite loop if we didn't break out
						panic_count--;
						if (!panic_count)
							break; // break out of while .. does this work?

						keep_tracing = true;
					}
				} else {
					quick_reject_tri_id = tri;
				}
			}

		} // while keep tracing

	} // for n through samples

	// the color is returned in color

	// scale color by number of samples
	//	color *= 1024.0 / nSamples;
}

bool LightMapper::bounce_ray(Ray &r, const Vector3 &face_normal, bool apply_epsilon) {
	float face_dot = face_normal.dot(r.d);

	// back face, don't bounce
	if (face_dot >= 0.0f)
		return false;

	// BOUNCING - mirror
	Vector3 mirror_dir = r.d - (2.0f * (face_dot * face_normal));

	// random hemisphere
	Vector3 hemi_dir;
	generate_random_hemi_unit_dir(hemi_dir, face_normal);

	r.d = hemi_dir.linear_interpolate(mirror_dir, adjusted_settings.smoothness);

	// standard epsilon? NYI
	if (apply_epsilon)
		r.o += (face_normal * adjusted_settings.surface_bias); //0.01f);

	return true;
}

void LightMapper::BF_process_texel_light_bounce(int bounces_left, Ray r, FColor ray_color) {
	// apply bounce power
	ray_color *= adjusted_settings.directional_bounce_power;

	float u, v, w, t;
	int tri = _scene.find_intersect_ray(r, u, v, w, t);

	// nothing hit
	if (tri == -1) {
		// terminate bounces (or detect sky)
		return;
	}

	//	if (bounces_left == 1)
	//	{
	//		print_line("test");
	//	}

	// hit the back of a face? if so terminate ray
	Vector3 vertex_normal;
	const Tri &triangle_normal = _scene._tri_normals[tri];
	triangle_normal.interpolate_barycentric(vertex_normal, u, v, w);
	vertex_normal.normalize(); // is this necessary as we are just checking a dot product polarity?

	// first get the texture details
	/*
	Color albedo, emission;
	bool bTransparent;
	_scene.find_primary_texture_colors(tri, Vector3(u, v, w), albedo, emission, bTransparent);
	*/

	ColorSample cols;
	_scene.take_triangle_color_sample(tri, Vector3(u, v, w), cols);

	bool pass_through = !cols.is_opaque && (cols.albedo.a < 0.001f);

	bool bBackFace = false;
	const Vector3 &face_normal = _scene._tri_planes[tri].normal;

	//float face_dot = face_normal.dot(r.d);
	//	if (face_dot >= 0.0f)
	//		bBackFace = true;

	float vertex_dot = vertex_normal.dot(r.d);
	if (vertex_dot >= 0.0f)
		bBackFace = true;

	// if not transparent and backface, then terminate ray
	if (bBackFace && cols.is_opaque)
		return;

	// convert barycentric to uv coords in the lightmap
	Vector2 uv;
	_scene.find_uvs_barycentric(tri, uv, u, v, w);

	// texel address
	int tx = uv.x * _width;
	int ty = uv.y * _height;

	// could be off the image
	if (!_image_L.is_within(tx, ty))
		return;

	// position of potential hit
	Vector3 pos;
	const Tri &triangle = _scene._tris[tri];
	triangle.interpolate_barycentric(pos, u, v, w);

	// deal with tranparency
	if (!cols.is_opaque) {
		// if not passing through, because clear, chance of pass through
		if (!pass_through && !bBackFace) {
			pass_through = Math::randf() > cols.albedo.a;
		}

		// if the ray is passing through, we want to calculate the color modified by the surface
		if (pass_through)
			calculate_transmittance(cols.albedo, ray_color);

		// if pass through
		if (bBackFace || pass_through) {
			// push the ray origin through the hit surface
			float push = -0.001f; // 0.001
			if (bBackFace)
				push = -push;

			r.o = pos + (face_normal * push);

			// call recursively
			BF_process_texel_light_bounce(bounces_left, r, ray_color);
			return;
		}

	} // if transparent

	///////

	// if we got here, it is front face and either solid or no pass through,
	// so there is a hit
	float lambert = MAX(0.0f, -vertex_dot);
	// apply lambert diffuse
	ray_color *= lambert;

	// apply
	MT_safe_add_to_texel(tx, ty, ray_color);

	// any more bounces to go?
	bounces_left--;
	if (bounces_left <= 0)
		return;

	// move the ray origin up to the hit point
	// (we will use the barycentric derived pos here, rather than r.o + (r.d * t)
	// for more accuracy relative to the surface
	r.o = pos;

	// bounce and lower power
	FColor falbedo;
	falbedo.set(cols.albedo);

	// bounce color
	ray_color = ray_color * falbedo;

	// find the bounced direction from the face normal
	bounce_ray(r, face_normal);

	// call recursively
	BF_process_texel_light_bounce(bounces_left, r, ray_color);
}

void LightMapper::process_light_probes() {
	LightProbes probes;
	int stages = probes.create(*this);
	if (stages != -1) {
		//		if (bake_begin_function) {
		//			bake_begin_function(stages);
		//		}

		for (int n = 0; n < stages; n++) {
			if (bake_step_function) {
				_pressed_cancel = bake_step_function(n / (float)stages, String("Process LightProbes: ") + " (" + itos(n) + ")");
				if (_pressed_cancel)
					break;
			}

			probes.process(n);
		}

		if (!_pressed_cancel)
			probes.save();

		if (bake_end_function) {
			bake_end_function();
		}
	}
}

FColor LightMapper::process_texel_ambient_bounce(int x, int y, uint32_t tri_source) {
	FColor total;
	total.set(0.0f);

	// barycentric
	const Vector3 &bary = *_image_barycentric.get(x, y);

	Vector3 pos;
	_scene._tris[tri_source].interpolate_barycentric(pos, bary.x, bary.y, bary.z);

	Vector3 norm;
	const Tri &triangle_normal = _scene._tri_normals[tri_source];
	triangle_normal.interpolate_barycentric(norm, bary.x, bary.y, bary.z);
	norm.normalize();

	// precalculate normal push and ray origin on top of the surface
	const Vector3 &plane_norm = _scene._tri_planes[tri_source].normal;
	Vector3 normal_push = plane_norm * adjusted_settings.surface_bias;
	Vector3 ray_origin = pos + normal_push;

	int nSamples = adjusted_settings.num_ambient_bounce_rays;
	int samples_counted = nSamples;

	for (int n = 0; n < nSamples; n++) {
		if (!process_texel_ambient_bounce_sample(plane_norm, ray_origin, total)) {
			samples_counted--;
		}
	}

	// some samples may have been missed due to transparency
	if (samples_counted)
		return total / samples_counted;

	return total;
}

FColor LightMapper::probe_calculate_indirect_light(const Vector3 &pos) {
	FColor total;
	total.set(0.0f);

	// we will reuse the same routine from texel bounce
	int nSamples = data.params[PARAM_PROBE_SAMPLES];
	int samples_counted = nSamples;

	Vector3 ray_origin = pos;

	for (int n = 0; n < nSamples; n++) {
		// random normal
		Vector3 norm;
		generate_random_unit_dir(norm);

		if (!process_texel_ambient_bounce_sample(norm, ray_origin, total)) {
			samples_counted--;
		}
	}

	// some samples may have been missed due to transparency
	if (samples_counted)
		return total / samples_counted;

	return total;
}

bool LightMapper::process_texel_ambient_bounce_sample(const Vector3 &plane_norm, const Vector3 &ray_origin, FColor &total_col) {
	// first dot
	Ray r;
	r.o = ray_origin;

	// SLIDING
	//			Vector3 temp = r.d.cross(norm);
	//			new_ray.d = norm.cross(temp);

	// BOUNCING - mirror
	//new_ray.d = r.d - (2.0f * (dot * norm));

	// random hemisphere
	generate_random_hemi_unit_dir(r.d, plane_norm);

	// loop here just in case transparent
	while (true) {
		// collision detect
		Vector3 bary;
		float t;
		int tri_hit = _scene.find_intersect_ray(r, bary, t);

		// nothing hit
		//	if ((tri_hit != -1) && (tri_hit != (int) tri_source))
		if (tri_hit != -1) {
			// look up the UV of the tri hit
			Vector2 uvs;
			_scene.find_uvs_barycentric(tri_hit, uvs, bary);

			// find texel
			int dx = (uvs.x * _width); // round?
			int dy = (uvs.y * _height);

			// texel not on the UV map
			if (!_image_L.is_within(dx, dy))
				return true;

			// back face?
			Vector3 face_normal;
			bool is_backface = hit_back_face(r, tri_hit, bary, face_normal);

			// the contribution is the luminosity at that spot and the albedo
			/*
			Color albedo, emission;
			bool bTransparent;
			_scene.find_primary_texture_colors(tri_hit, bary, albedo, emission, bTransparent);
			*/
			ColorSample cols;
			_scene.take_triangle_color_sample(tri_hit, bary, cols);

			FColor falbedo;
			falbedo.set(cols.albedo);

			//bool opaque = !(!cols.is_opaque && (cols.albedo.a < 0.5f));
			bool opaque = cols.is_opaque || (cols.albedo.a >= 0.5f);

			// if the surface transparent we may want to downgrade the influence a little
			if (!cols.is_opaque)
				falbedo *= cols.albedo.a;

			// see through has no effect on colour
			if (!opaque || (is_backface && !cols.is_opaque)) {
				// if it is front facing, still apply the 'haze' from the colour
				// (this all counts as 1 sample, so could get super light in theory, if passing through several hazes...)
				if (!is_backface)
					total_col += (_image_L.get_item(dx, dy) * falbedo);

				// move the ray origin and do again
				// position of potential hit
				Vector3 pos;
				const Tri &triangle = _scene._tris[tri_hit];
				triangle.interpolate_barycentric(pos, bary);

				float push = -adjusted_settings.surface_bias;
				if (is_backface)
					push = -push;

				r.o = pos + (face_normal * push);

				// and repeat the loop
			} else {
				total_col += (_image_L.get_item(dx, dy) * falbedo);

				// If we want emission on ambient bounces.
				// FColor femission;
				// femission.set(cols.emission);
				// total_col += femission;

				//assert (total_col.r >= 0.0f);
				break;
			}
		} else {
			// nothing hit, exit loop
			break;
		}

	} // while true (loop while transparent)

	return true;
}

//	if ((tx == 3) && (ty == 17))
//	{
//		print_line("test");
//	}

// find triangle
//	uint32_t tri = *m_Image_ID_p1.Get(tx, ty);
//	if (!tri)
//		return;
//	tri--; // plus one based

// may be more than 1 triangle on this texel
//	uint32_t tri2 = *m_Image_ID2_p1.Get(tx, ty);

// barycentric
//	const Vector3 &bary = *m_Image_Barycentric.Get(tx, ty);

//	Vector3 pos;
//	m_Scene.m_Tris[tri].InterpolateBarycentric(pos, bary.x, bary.y, bary.z);

//	Vector3 normal;
//	m_Scene.m_TriNormals[tri].InterpolateBarycentric(normal, bary.x, bary.y, bary.z);

void LightMapper::_load_texel_data(uint32_t p_tri_id, const Vector3 &p_bary, Vector3 &r_pos_pushed, Vector3 &r_normal, const Vector3 **r_plane_normal) const {
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

bool LightMapper::load_texel_data(int32_t p_x, int32_t p_y, uint32_t &r_tri_id, const Vector3 **r_bary, Vector3 &r_pos_pushed, Vector3 &r_normal, const Vector3 **r_plane_normal) const {
	// find triangle
	r_tri_id = *_image_ID_p1.get(p_x, p_y);
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

bool LightMapper::AA_BF_process_sub_texel(float p_fx, float p_fy, const MiniList &p_ml, Color &r_col) {
	// find which triangle on the minilist we are inside (if any)
	uint32_t tri_id;
	Vector3 bary;

	// has to be ranged 0 to 1
	Vector2 st(p_fx, p_fy);
	st.x /= _width;
	st.y /= _height;

	if (!AO_find_texel_triangle(p_ml, st, tri_id, bary))
		return false;

	Vector3 pos;
	Vector3 normal;
	//const Vector3 *bary = nullptr;
	const Vector3 *plane_normal = nullptr;
	_load_texel_data(tri_id, bary, pos, normal, &plane_normal);

	// find the colors of this texel
	ColorSample cols;
	_scene.take_triangle_color_sample(tri_id, bary, cols);

	//r_col.r += 1.0f;

	int num_samples = adjusted_settings.backward_num_rays / adjusted_settings.antialias_samples_per_texel;
	//	int num_samples = 1;

	FColor texel_add;
	texel_add.set(0);
	FColor temp;

	for (int l = 0; l < _lights.size(); l++) {
		BF_process_texel_light(cols.albedo, l, pos, *plane_normal, normal, temp, num_samples);
		texel_add += temp;
	}

	// sky (if present)
	if (BF_process_texel_sky(cols.albedo, pos, *plane_normal, normal, temp)) {
		// scale sky by number of samples in the normal lights
		float descale_to_match_normal_lights = num_samples / 1024.0;
		texel_add += temp * descale_to_match_normal_lights;
	}

	// add emission
	//	Color emission_tex_color;
	//	Color emission_color;
	//	if (m_Scene.FindEmissionColor(tri_id, bary, emission_tex_color, emission_color))
	if (cols.is_emitter && (data.params[PARAM_EMISSION_ENABLED] == Variant(true))) {
		// Glow determines how much the surface itself is lighted (and thus the ratio between glow and emission)
		// emission density determines the number of rays and lighting effect
		FColor femm;
		femm.set(cols.emission);

		// float power = settings.backward_ray_power * adjusted_settings.backward_num_rays * 32.0f;
		// power *= adjusted_settings.glow;
		// texel_add += femm * power;

		texel_add += (femm * adjusted_settings.glow * adjusted_settings.emission_power) / adjusted_settings.antialias_samples_per_texel;

		// only if directional bounces are being used (could use ambient bounces for emission)
		if (adjusted_settings.num_directional_bounces) {
			// needs to be adjusted according to size of texture .. as there will be more emissive texels
			int nSamples = adjusted_settings.backward_num_rays * 2 * adjusted_settings.emission_density;

			// apply the albedo to the emission color to get the color emanating
			femm.r *= cols.albedo.r;
			femm.g *= cols.albedo.g;
			femm.b *= cols.albedo.b;

			Ray r;
			r.o = pos;
			femm *= settings.backward_ray_power * 128.0f * (1.0f / adjusted_settings.emission_density);

			for (int n = 0; n < nSamples; n++) {
				// send out emission bounce rays
				generate_random_hemi_unit_dir(r.d, *plane_normal);
				BF_process_texel_light_bounce(adjusted_settings.num_directional_bounces, r, femm); // always at least 1 emission ray
			}
		}
	}

	Color ctemp;
	texel_add.to(ctemp);

	ctemp *= adjusted_settings.antialias_samples_per_texel;

	r_col += ctemp;

	return true;
}

void LightMapper::AA_BF_process_texel(int p_tx, int p_ty) {
	const MiniList &ml = _image_tri_ids.get_item(p_tx, p_ty);
	if (!ml.num)
		return; // no triangles in this UV

	bool debug = false;
	if ((p_tx == 305) && (p_ty == 248))
		debug = true;

	int aa_size = adjusted_settings.antialias_samples_width;
	float step = 1.0f / (aa_size + 2);

	Color col;
	int samples_inside = 0;

	for (int y = 0; y < aa_size; y++) {
		float fy = p_ty + ((y + 0.5f) * step);

		for (int x = 0; x < aa_size; x++) {
			float fx = p_tx + ((x + 0.5f) * step);

			if (AA_BF_process_sub_texel(fx, fy, ml, col))
				samples_inside++;
		}
	}

	if (debug) {
		//print_line("samples inside " + itos(samples_inside) + ", col " + String(Variant(col)));
	}

	if (!samples_inside)
		return;

	col /= (float)samples_inside;

	// safe write
	FColor texel_add;
	texel_add.set(col);
	MT_safe_add_to_texel(p_tx, p_ty, texel_add);
}

void LightMapper::BF_process_texel(int tx, int ty) {
	AA_BF_process_texel(tx, ty);
	return;

	//		if ((tx == 13) && (ty == 284))
	//			print_line("testing");

	DEV_ASSERT(_image_L.is_within(tx, ty));

	uint32_t tri_id = 0;
	Vector3 pos;
	Vector3 normal;
	const Vector3 *bary = nullptr;
	const Vector3 *plane_normal = nullptr;
	if (!load_texel_data(tx, ty, tri_id, &bary, pos, normal, &plane_normal))
		return;

	/*
		// find triangle
		uint32_t tri_id = *_image_ID_p1.get(tx, ty);
		if (!tri_id)
			return;
		tri_id--; // plus one based

		// barycentric
		const Vector3 &bary = *_image_barycentric.get(tx, ty);
		Vector3 bary_clamped; // = bary;

		// new .. cap the barycentric to prevent edge artifacts
		const float clamp_margin = 0.0001f;
		const float clamp_margin_high = 1.0f - clamp_margin;

		bary_clamped.x = CLAMP(bary.x, clamp_margin, clamp_margin_high);
		bary_clamped.y = CLAMP(bary.y, clamp_margin, clamp_margin_high);
		bary_clamped.z = CLAMP(bary.z, clamp_margin, clamp_margin_high);

		// we will trace
		// FROM THE SURFACE TO THE LIGHT!!
		// this is very important, because the ray is origin and direction,
		// and there will be precision errors at the destination.
		// At the light end this doesn't matter, but if we trace the other way
		// we get artifacts due to precision loss due to normalized direction.
		Vector3 pos;
		_scene._tris[tri_id].interpolate_barycentric(pos, bary_clamped);

		// add epsilon to pos to prevent self intersection and neighbour intersection
		const Vector3 &plane_normal = _scene._tri_planes[tri_id].normal;
		pos += plane_normal * adjusted_settings.surface_bias;

		Vector3 normal;
		_scene._tri_normals[tri_id].interpolate_barycentric(normal, bary.x, bary.y, bary.z);
	*/
	//Vector2i tex_uv = Vector2i(x, y);

	// find the colors of this texel
	ColorSample cols;
	_scene.take_triangle_color_sample(tri_id, *bary, cols);

	/*
	Color albedo;
	Color emission;
	bool transparent;
	bool emitter;
	_scene.take_triangle_color_sample(tri_id, bary, albedo, emission, transparent, emitter);
	*/

	FColor texel_add;
	texel_add.set(0);

	//	FColor * pTexel = m_Image_L.Get(tx, ty);
	//	if (!pTexel)
	//		return;

	int num_samples = adjusted_settings.backward_num_rays;

	FColor temp;
	for (int l = 0; l < _lights.size(); l++) {
		BF_process_texel_light(cols.albedo, l, pos, *plane_normal, normal, temp, num_samples);
		texel_add += temp;
	}

	// sky (if present)
	if (BF_process_texel_sky(cols.albedo, pos, *plane_normal, normal, temp)) {
		// scale sky by number of samples in the normal lights
		float descale_to_match_normal_lights = num_samples / 1024.0;
		texel_add += temp * descale_to_match_normal_lights;
	}

	// add emission
	//	Color emission_tex_color;
	//	Color emission_color;
	//	if (m_Scene.FindEmissionColor(tri_id, bary, emission_tex_color, emission_color))
	if (cols.is_emitter) {
		// Glow determines how much the surface itself is lighted (and thus the ratio between glow and emission)
		// emission density determines the number of rays and lighting effect
		FColor femm;
		femm.set(cols.emission);

		// float power = settings.backward_ray_power * adjusted_settings.backward_num_rays * 32.0f;
		// power *= adjusted_settings.glow;
		// texel_add += femm * power;

		texel_add += femm * adjusted_settings.glow * adjusted_settings.emission_power;

		// only if directional bounces are being used (could use ambient bounces for emission)
		if (adjusted_settings.num_directional_bounces) {
			// needs to be adjusted according to size of texture .. as there will be more emissive texels
			int nSamples = adjusted_settings.backward_num_rays * 2 * adjusted_settings.emission_density;

			// apply the albedo to the emission color to get the color emanating
			femm.r *= cols.albedo.r;
			femm.g *= cols.albedo.g;
			femm.b *= cols.albedo.b;

			Ray r;
			r.o = pos;
			femm *= settings.backward_ray_power * 128.0f * (1.0f / adjusted_settings.emission_density);

			for (int n = 0; n < nSamples; n++) {
				// send out emission bounce rays
				generate_random_hemi_unit_dir(r.d, *plane_normal);
				BF_process_texel_light_bounce(adjusted_settings.num_directional_bounces, r, femm); // always at least 1 emission ray
			}
		}
	}

	// safe write
	MT_safe_add_to_texel(tx, ty, texel_add);
}

void LightMapper::process_emission_pixels() {
	for (int n = 0; n < _scene._emission_pixels.size(); n++) {
		const Vec2_i16 &coord = _scene._emission_pixels[n];
		process_emission_pixel(coord.x, coord.y);
	}

	while (!ray_bank_are_voxels_clear()) {
		ray_bank_process();
		ray_bank_flush();
	}
}

void LightMapper::process_emission_pixel(int32_t p_x, int32_t p_y) {
	DEV_ASSERT(_image_L.is_within(p_x, p_y));

	// find triangle
	/*
	uint32_t tri_id = *_image_ID_p1.get(p_x, p_y);
	if (!tri_id)
		return;
	tri_id--; // plus one based
	*/

	uint32_t tri_id = 0;
	Vector3 pos;
	Vector3 normal;
	const Vector3 *bary = nullptr;
	const Vector3 *plane_normal = nullptr;
	if (!load_texel_data(p_x, p_y, tri_id, &bary, pos, normal, &plane_normal))
		return;

	// find the colors of this texel
	ColorSample cols;
	_scene.take_triangle_color_sample(tri_id, *bary, cols);

	//	const Color &emission = cols.albedo;
	//	const Color &emission = cols.emission;

	Color emission = _scene._image_emission_done.get_item(p_x, p_y);

	emission *= adjusted_settings.emission_power;

	float luminosity = 0;
	for (int c = 0; c < 3; c++) {
		luminosity += emission.components[c];
	}
	// Not emitting enough to be worth it.
	if (luminosity < 0.1f)
		return;

		//	print_line("process_emission_pixel : color " + String(Variant(emission)) + " coords " +  itos(p_x) + ", " + itos(p_y) + " ... tri_id " + itos(tri_id) + " bary " + String(Variant(*bary)));

//#define LLIGHTMAP_FORWARD_EMISSION_PIXELS
#ifdef LLIGHTMAP_FORWARD_EMISSION_PIXELS
	//	uint32_t num_rays = 512;
	uint32_t num_rays = _num_rays * 64 * 16;

	Ray r;
	r.o = pos;

	for (uint32_t n = 0; n < num_rays; n++) {
		// Ensure ray goes outward from surface.
		generate_random_hemi_unit_dir(r.d, *plane_normal);

		FColor fcol;
		fcol.set(emission);
		//fcol.set(0, 1, 0);

		ray_bank_request_new_ray(r, 1, fcol, nullptr);
	}
#else
	process_emission_pixel_backward(p_x, p_y, emission, pos, *plane_normal);
#endif
}

void LightMapper::process_emission_pixel_backward(int32_t p_x, int32_t p_y, const Color &p_emission, const Vector3 &p_emission_pos, const Vector3 &p_emission_normal) {
	Plane emission_plane(p_emission_pos, p_emission_normal);

	for (int y = 0; y < _height; y++) {
		for (int x = 0; x < _width; x++) {
			// find triangle
			uint32_t tri_source = *_image_ID_p1.get(x, y);
			if (tri_source) {
				tri_source--; // plus one based

				if ((p_y == y) && (p_x == x))
					continue; // check this goes to right place

				// Find triangle info for this texel.
				uint32_t tri_id = 0;
				Vector3 pos;
				Vector3 vertex_normal;
				const Vector3 *bary = nullptr;
				const Vector3 *plane_normal = nullptr;
				if (!load_texel_data(x, y, tri_id, &bary, pos, vertex_normal, &plane_normal))
					continue;

				// If the tri is behind emission tri ignore.
				if (emission_plane.distance_to(pos) <= 0)
					continue;

				// If the emission is behind the test tri, ignore.
				Plane tri_plane(pos, *plane_normal);
				if (tri_plane.distance_to(p_emission_pos) <= 0)
					continue;

				// If emission tri normal and this tri normal are not facing, ignore.
				// TODO: possibly try the vertex interpolated normal if artifacts on edges.
				//				float dot = p_emission_normal.dot(*plane_normal);
				//				if (dot >= 0)
				//					continue;

				Ray r;
				r.o = pos;
				r.o += *plane_normal * adjusted_settings.surface_bias;

				Vector3 offset_from_emission = p_emission_pos - r.o;
				float dist_from_emission = offset_from_emission.length();

				if (dist_from_emission <= 0)
					continue;

				// Normalize direction.
				r.d = offset_from_emission / dist_from_emission;

				// Test ray. If it hits, no light arrives.
				if (_scene.test_intersect_ray(r, dist_from_emission, true))
					continue;

				// Calculate the power.
				//////////////////////////////
				// for backward tracing, first pass, this is a special case, because we DO
				// take account of distance to the light, and normal, in order to simulate the effects
				// of the likelihood of 'catching' a ray. In forward tracing this happens by magic.
				float power = inverse_square_drop_off(dist_from_emission);

				// take into account normal
				float dot = r.d.dot(vertex_normal);
				dot = fabs(dot);

				power *= dot;
				//////////////////////////////

				// Test directly add the emission.
				MT_safe_add_to_texel(x, y, p_emission * power);
			}
		}
	}
}

void LightMapper::process_emission_tris() {
	int num_sections = _num_rays_forward / RAYS_PER_SECTION;

	if (!num_sections)
		num_sections = 1;

	float fraction = 1.0f / num_sections;

	//	if (bake_begin_function) {
	//		bake_begin_function(num_sections);
	//	}

	for (int s = 0; s < num_sections; s++) {
		if ((s % 1) == 0) {
			if (bake_step_function) {
				_pressed_cancel = bake_step_function(s / (float)num_sections, String("Process Emission Section: ") + " (" + itos(s) + ")");
				if (_pressed_cancel) {
					if (bake_end_function)
						bake_end_function();
					return;
				}
			}
		}

		process_emission_tris_section(fraction);

		while (!ray_bank_are_voxels_clear())
		//for (int b=0; b<m_AdjustedSettings.m_Forward_NumBounces+1; b++)
		{
			ray_bank_process();
			ray_bank_flush();
		} // for bounce
	} // for section

	// left over
	//	{
	//		int num_leftover = m_iNumRays - (num_sections * m_iRaysPerSection);
	//		ProcessLight(n, num_leftover);

	//		for (int b=0; b<m_AdjustedSettings.m_Forward_NumBounces+1; b++)
	//		{
	//			RayBank_Process();
	//			RayBank_Flush();
	//		} // for bounce
	//	}

	if (bake_end_function) {
		bake_end_function();
	}
}

void LightMapper::process_emission_tris_section(float fraction_of_total) {
	for (int n = 0; n < _scene._emission_tris.size(); n++) {
		process_emission_tri(n, fraction_of_total);
	}
}

void LightMapper::process_emission_tri(int etri_id, float fraction_of_total) {
	const EmissionTri &etri = _scene._emission_tris[etri_id];
	int tri_id = etri.tri_id;

	// get the material

	// positions
	const Tri &tri_pos = _scene._tris[tri_id];

	// normals .. just use plane normal for now (no interpolation)
	Vector3 norm = _scene._tri_planes[tri_id].normal;

	Ray ray;
	ray.d = norm;

	// use the area to get number of samples
	float rays_per_unit_area = _num_rays_forward * adjusted_settings.emission_density * 0.12f * 0.5f;
	int nSamples = etri.area * rays_per_unit_area * fraction_of_total;

	// nSamples may be zero incorrectly for small triangles, maybe we need to adjust for this
	// NYI

	for (int s = 0; s < nSamples; s++) {
		// find a random barycentric coord
		Vector3 bary;
		generate_random_barycentric(bary);

		// find point on this actual triangle
		tri_pos.interpolate_barycentric(ray.o, bary);

		// shoot a ray from this pos and normal, using the emission color
		generate_random_unit_dir(ray.d);
		ray.d += norm;

		if (ray.d.dot(norm) < 0.0f)
			ray.d = -ray.d;

		ray.d.normalize();

		// get the albedo etc
		Color emission_tex_color;
		Color emission_color;
		_scene.find_emission_color(tri_id, bary, emission_tex_color, emission_color);

		FColor fcol;
		fcol.set(emission_tex_color);
		ray_bank_request_new_ray(ray, adjusted_settings.num_directional_bounces + 1, fcol);

		// special. For emission we want to also affect the emitting surface.
		// convert barycentric to uv coords in the lightmap
		Vector2 uv;
		_scene.find_uvs_barycentric(tri_id, uv, bary);

		// texel address
		int tx = uv.x * _width;
		int ty = uv.y * _height;

		// could be off the image
		if (!_image_L.is_within(tx, ty))
			continue;

		FColor *pTexelCol = _image_L.get(tx, ty);

		//		fcol.Set(emission_color * 0.5f);
		*pTexelCol += fcol * adjusted_settings.glow;
	}
}

void LightMapper::process_lights() {
	//	const int rays_per_section = 1024 * 16;

	int num_sections = _num_rays_forward / RAYS_PER_SECTION;

	//	if (bake_begin_function) {
	//		bake_begin_function(num_sections);
	//	}

	for (int n = 0; n < _lights.size(); n++) {
		for (int s = 0; s < num_sections; s++) {
			// double check the voxels are clear
#ifdef DEBUG_ENABLED
			//RayBank_CheckVoxelsClear();
#endif

			if ((s % 1) == 0) {
				if (bake_step_function) {
					_pressed_cancel = bake_step_function(s / (float)num_sections, String("Process Light Section: ") + " (" + itos(s) + ")");
					if (_pressed_cancel) {
						if (bake_end_function)
							bake_end_function();
						return;
					}
				}
			}

			process_light(n, RAYS_PER_SECTION);

			while (!ray_bank_are_voxels_clear())
			//for (int b=0; b<m_AdjustedSettings.m_Forward_NumBounces+1; b++)
			{
				ray_bank_process();
				ray_bank_flush();
			} // for bounce

			// if everything went correctly,
			// all the raybank should be empty
			//RayBank_AreVoxelsClear();

		} // for section

		// left over
		{
			int num_leftover = _num_rays_forward - (num_sections * RAYS_PER_SECTION);
			process_light(n, num_leftover);

			while (!ray_bank_are_voxels_clear())
			//for (int b=0; b<m_AdjustedSettings.m_Forward_NumBounces+1; b++)
			{
				ray_bank_process();
				ray_bank_flush();
			} // for bounce
		}

		// this is not really required, but is a protection against memory leaks
		ray_bank_reset(true);
	} // for light

	if (bake_end_function) {
		bake_end_function();
	}
}

void LightMapper::process_light(int light_id, int num_rays) {
	const LLight &light = _lights[light_id];

	Ray r;
	//	r.o = Vector3(0, 5, 0);

	//	float range = light.scale.x;
	//	const float range = 2.0f;

	// the power should depend on the volume, with 1x1x1 being normal power
	//	float power = light.scale.x * light.scale.y * light.scale.z;
	//	float power = light.energy;
	//	power *= settings.Forward_RayPower;

	//	light_color.r = power;
	//	light_color.g = power;
	//	light_color.b = power;

	// each ray

	//	num_rays = 1; // debug

	// new... allow the use of indirect energy to scale the number of samples
	num_rays *= light.indirect_energy;

	// compensate for the number of rays in terms of the power per ray
	float power = 0.01f; //settings.Forward_RayPower;

	if (light.indirect_energy > 0.0001f)
		power *= 1.0f / light.indirect_energy;

	// for directional, we need a load more rays for it to work well - it is expensive
	if (light.type == LLight::LT_DIRECTIONAL) {
		num_rays *= 2;
		//		float area = light.dl_tangent_range * light.dl_bitangent_range;
		//		num_rays = num_rays * area;
		// we will increase the power as well, because daylight more powerful than light bulbs typically.
		power *= 4.0f;

		if (light.dir.y >= 0.0f)
			return; // not supported
	}

	FColor light_color = light.color * power;

	for (int n = 0; n < num_rays; n++) {
		//		if ((n % 10000) == 0)
		//		{
		//			if (bake_step_function)
		//			{
		//				m_bCancel = bake_step_function(n, String("Process Rays: ") + " (" + itos(n) + ")");
		//				if (m_bCancel)
		//					return;
		//			}
		//		}

		r.o = light.pos;

		switch (light.type) {
			case LLight::LT_DIRECTIONAL: {
				bool hit_bound = false;
				while (!hit_bound) {
					hit_bound = true;

					// use the precalculated source plane stored in the llight
					r.o = light.dl_plane_pt;
					r.o += light.dl_tangent * Math::random(0.0f, light.dl_tangent_range);
					r.o += light.dl_bitangent * Math::random(0.0f, light.dl_bitangent_range);

					bool direction_ok = false;
					while (!direction_ok) {
						generate_random_unit_dir(r.d);
						r.d *= light.scale;
						// must point down - reverse hemisphere if pointing up
						if (light.dir.dot(r.d) < 0.0f) {
							r.d = -r.d;
						}

						r.d += (light.dir * 2.0f);
						r.d.normalize();

						if (r.d.y < 0.0f) {
							direction_ok = true;
						}
					}

					// now we are starting from destination, we must trace back to origin
					const AABB &bb = get_tracer().get_world_bound_expanded();

					// num units to top of map
					// plus a bit to make intersecting the world bound easier
					float units = (bb.size.y + 1.0f) / r.d.y;

					r.o += r.d * units;

					//				r.o.y = 3.2f;

					// hard code
					//				r.o = Vector3(0.2, 10, 0.2);
					//				r.d = Vector3(0, -1, 0);

					// special .. for dir light .. will it hit the AABB? if not, do a wraparound
					//					Vector3 clip;
					//					if (!GetTracer().IntersectRayAABB(r, GetTracer().m_SceneWorldBound_mid, clip))
					//					{
					//						hit_bound = false;
					//					}

					//					Vector3 ptHit = r.o + (r.d * 2.0f);
					//					Vector3 bb_min = bb.position;
					//					Vector3 bb_max = bb.position + bb.size;

					//					if (ptHit.x > bb_max.x)
					//						r.o.x -= bb.size.x;
					//					if (ptHit.x < bb_min.x)
					//						r.o.x += bb.size.x;
					//					if (ptHit.z > bb_max.z)
					//						r.o.z -= bb.size.z;
					//					if (ptHit.z < bb_min.z)
					//						r.o.z += bb.size.z;

					if (!get_tracer().get_world_bound_expanded().intersects_ray(r.o, r.d)) {
						hit_bound = false;
					}
				} // while hit bound
			} break;
			case LLight::LT_SPOT: {
				Vector3 offset;
				generate_random_unit_dir(offset);
				offset *= light.scale;
				r.o += offset;

				r.d = light.dir;

				// random axis
				Vector3 axis;
				generate_random_unit_dir(axis);

				float falloff_start = 0.5f; // this could be adjustable;

				float ang_max = light.spot_angle_radians;
				float ang_falloff = ang_max * falloff_start;
				float ang_falloff_range = ang_max - ang_falloff;

				// random angle giving equal circle distribution
				float a = Math::random(0.0f, 1.0f);

				// below a certain proportion are the central, outside is falloff
				a *= 2.0f;
				if (a > 1.0f) {
					// falloff
					a -= 1.0f;
					//angle /= 3.0f;

					//a *= a;

					a = ang_falloff + (a * ang_falloff_range);
				} else {
					// central
					a = sqrtf(a);
					a *= ang_falloff;
				}

				//				if (angle > ang_falloff)
				//				{
				//					// in the falloff zone, use different math.
				//					float r = Math::random(0.0f, 1.0f);
				//					r = r*r;
				//					r *= ang_falloff_range;
				//					angle = ang_falloff + r;
				//				}

				Quat rot;
				rot.set_axis_angle(axis, a);

				// TURNED OFF FOR TESTING
				r.d = rot.xform(r.d);

				// this is the radius of the cone at distance 1
				//				float radius_at_dist_one = Math::tan(Math::deg2rad(light.spot_angle));

				//				float spot_ball_size = radius_at_dist_one;

				//				Vector3 ball;
				//				RandomSphereDir(ball, spot_ball_size);

				//				r.d += ball;
				//				r.d.normalize();
			} break;
			default: {
				Vector3 offset;
				generate_random_unit_dir(offset);
				offset *= light.scale;
				r.o += offset;

				generate_random_unit_dir(r.d);
			} break;
		}
		//r.d.normalize();

		ray_bank_request_new_ray(r, adjusted_settings.num_directional_bounces + 1, light_color, 0);

		//		ProcessRay(r, 0, power);
	}

	//print_line("PASSED " + itos (passed) + ", MISSED " + itos(missed));
}

} // namespace LM
