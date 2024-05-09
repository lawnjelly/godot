#include "lambient_occlusion.h"
#include "core/os/threaded_array_processor.h"

namespace LM {

void AmbientOcclusion::process_AO_clear_texel(int tx, int ty, AONearestResult &r_nearest) {
	if (!_image_tri_ids_p1.get_item(tx, ty))
		return;

	const MiniList &ml = _image_tri_minilists.get_item(tx, ty);

	if (AO_are_triangles_out_of_range(tx, ty, ml, adjusted_settings.AO_range, r_nearest)) {
		data_ao.bitimage_clear.set_pixel(tx, ty, true);
	}
}

void AmbientOcclusion::process_AO_texel(int tx, int ty, int qmc_variation) {
	//	if ((tx == 77) && (ty == 221))
	//		print_line("test");

	// Reclaimed?
	if (!_image_tri_ids_p1.get_item(tx, ty))
		return;

	const MiniList &ml = _image_tri_minilists.get_item(tx, ty);
	DEV_ASSERT(ml.num);
	//	if (!ml.num)
	//		return; // no triangles in this UV

	// could be off the image
	float *pfTexel = _image_AO.get(tx, ty);

	float power = 0.0f;

	// simple or complex? (are there cuts on this texel)
	if (1)
	//	if (!m_Image_Cuts.GetItem(tx, ty))
	{
		power = calculate_AO(tx, ty, qmc_variation, ml);
	} else {
		power = calculate_AO_complex(tx, ty, qmc_variation, ml);
	}

	*pfTexel = power;
}

void AmbientOcclusion::AO_load_or_calculate_bitimage_clear() {
	float loaded_range = 0;
	uint32_t loaded_bytes = 0;

	data_ao.black_image_valid = false;
	Error err = data_ao.bitimage_black.load(settings.AO_bitimage_black_filename, (uint8_t *)&loaded_range, &loaded_bytes, sizeof(float));
	if ((err == OK) && (loaded_range == adjusted_settings.AO_range)) {
		if ((data_ao.bitimage_black.get_width() == _width) && (data_ao.bitimage_black.get_height() == _height)) {
			data_ao.black_image_valid = true;
		}
	}
	if (!data_ao.black_image_valid) {
		data_ao.bitimage_black.create(_width, _height);
	}

	err = data_ao.bitimage_clear.load(settings.AO_bitimage_clear_filename, (uint8_t *)&loaded_range, &loaded_bytes, sizeof(float));
	Error err_middle = data_ao.bitimage_middle_only.load(settings.AO_bitimage_middle_filename);

	// If the middle file doesn't exist, abort and recalculate all.
	// Usually the clear file and the middle file will both be in sync.
	if (err_middle != OK || !data_ao.bitimage_middle_only.dimensions_match(_width, _height)) {
		err = ERR_BUG;
	}

	if ((err == OK) && (loaded_range == adjusted_settings.AO_range) && data_ao.bitimage_clear.dimensions_match(_width, _height)) {
		// All done!
		return;
	}

	// File didn't exist or was out of date etc.
	data_ao.bitimage_clear.create(_width, _height);
	data_ao.bitimage_middle_only.create(_width, _height);

#ifdef LAMBIENT_OCCLUSION_USE_THREADS
	//	int nCores = OS::get_singleton()->get_processor_count();
	int nSections = _height / 16; // 64
	if (!nSections)
		nSections = 1;
	int y_section_size = _height / nSections;
	int leftover_start = y_section_size * nSections;

	for (int s = 0; s < nSections; s++) {
		int y_section_start = s * y_section_size;

		if (bake_step_function) {
			_pressed_cancel = bake_step_function(y_section_start / (float)_height, String("Building Clear BitImage for Texels: ") + " (" + itos(y_section_start) + ")");
			if (_pressed_cancel) {
				if (bake_end_function) {
					bake_end_function();
				}
				return;
			}
		}

		thread_process_array(y_section_size, this, &AmbientOcclusion::process_AO_clear_line_MT, y_section_start);
	}

	// leftovers
	int nLeftover = _height - leftover_start;

	if (nLeftover)
		thread_process_array(nLeftover, this, &AmbientOcclusion::process_AO_clear_line_MT, leftover_start);
#else

	for (int y = 0; y < m_iHeight; y++) {
		if ((y % 10) == 0) {
			if (bake_step_function) {
				m_bCancel = bake_step_function(y, String("Building Clear BitImage for Texels: ") + " (" + itos(y) + ")");
				if (m_bCancel) {
					if (bake_end_function) {
						bake_end_function();
					}
					return;
				}
			}
		}

		ProcessAO_clear_LineMT(0, y);
	}
#endif

	// Save
	// Ensure 32 bit.
	float saved_range = adjusted_settings.AO_range;

	// Save clear
	err = data_ao.bitimage_clear.save(settings.AO_bitimage_clear_filename, (uint8_t *)&saved_range, sizeof(float));
	ERR_FAIL_COND_MSG(err != OK, "Error saving AO clear BitImage.");

	err = data_ao.bitimage_middle_only.save(settings.AO_bitimage_middle_filename);
	ERR_FAIL_COND_MSG(err != OK, "Error saving AO middle BitImage.");
}

void AmbientOcclusion::process_AO() {
	//	if (bake_begin_function) {
	//		bake_begin_function(m_iHeight);
	//	}
	_image_AO.create(_width, _height, 1);

	// find the max range in voxels. This can be used to speed up the ray trace
	const float range = adjusted_settings.AO_range;
	_scene._tracer.get_distance_in_voxels(range, _scene._voxel_range);

	data_ao.reset();

	AO_load_or_calculate_bitimage_clear();

	print_line("AO clear pixels: " + itos(data_ao.bitimage_clear.count()));
	print_line("AO middle pixels: " + itos(data_ao.middle));
	print_line("AO non-middle pixels: " + itos(data_ao.non_middle));

#ifdef LAMBIENT_OCCLUSION_USE_THREADS
	//	int nCores = OS::get_singleton()->get_processor_count();
	int nSections = _height / 16; // 64
	if (!nSections)
		nSections = 1;
	int y_section_size = _height / nSections;
	int leftover_start = y_section_size * nSections;

	for (int s = 0; s < nSections; s++) {
		int y_section_start = s * y_section_size;

		if (bake_step_function) {
			_pressed_cancel = bake_step_function(y_section_start / (float)_height, String("Process Texels: ") + " (" + itos(y_section_start) + ")");
			if (_pressed_cancel) {
				if (bake_end_function) {
					bake_end_function();
				}
				return;
			}
		}

		thread_process_array(y_section_size, this, &AmbientOcclusion::process_AO_line_MT, y_section_start);
	}

	// leftovers
	int nLeftover = _height - leftover_start;

	if (nLeftover)
		thread_process_array(nLeftover, this, &AmbientOcclusion::process_AO_line_MT, leftover_start);
#else

	for (int y = 0; y < m_iHeight; y++) {
		if ((y % 10) == 0) {
			if (bake_step_function) {
				m_bCancel = bake_step_function(y, String("Process Texels: ") + " (" + itos(y) + ")");
				if (m_bCancel) {
					if (bake_end_function) {
						bake_end_function();
					}
					return;
				}
			}
		}

		ProcessAO_LineMT(0, y);
	}
#endif

	// Save black
	if (!data_ao.black_image_valid) {
		float saved_range = adjusted_settings.AO_range;
		Error err = data_ao.bitimage_black.save(settings.AO_bitimage_black_filename, (uint8_t *)&saved_range, sizeof(float));
		if (err != OK) {
			ERR_PRINT("Error saving AO black BitImage.");
		}
	}

	print_line("AO aborts " + itos(data_ao.aborts) + " out of " + itos(_width * _height) + " pixels.");
	print_line("AO clear " + itos(data_ao.clear) + ", black " + itos(data_ao.black) + ", processed " + itos(data_ao.processed) + ".");

	if (bake_end_function) {
		bake_end_function();
	}
}

void AmbientOcclusion::process_AO_clear_line_MT(uint32_t y_offset, int y_section_start) {
	int ty = y_section_start + y_offset;

	// seed based on the line
	AONearestResult nearest_result;

	for (int x = 0; x < _width; x++) {
		process_AO_clear_texel(x, ty, nearest_result);
	}
}

void AmbientOcclusion::process_AO_line_MT(uint32_t y_offset, int y_section_start) {
	int ty = y_section_start + y_offset;

	// seed based on the line
	int qmc_variation = _QMC.random_variation();

	for (int x = 0; x < _width; x++) {
		qmc_variation = _QMC.get_next_variation(qmc_variation);
		process_AO_texel(x, ty, qmc_variation);
	}
}

bool AmbientOcclusion::_is_minilist_coplanar(const MiniList &p_ml) const {
	DEV_ASSERT(p_ml.num > 1);

	uint32_t first_tri_id = _minilist_tri_ids[p_ml.first];
	const Plane &first_plane = _scene._tri_planes[first_tri_id];

	for (uint32_t n = 1; n < p_ml.num; n++) {
		uint32_t tri_id = _minilist_tri_ids[p_ml.first + n];
		const Plane &plane = _scene._tri_planes[tri_id];

		if (!Math::is_equal_approx(plane.d, first_plane.d))
			return false;

		if (!first_plane.normal.is_equal_approx(plane.normal))
			return false;
	}

	return true;
}

bool AmbientOcclusion::AO_are_triangles_out_of_range(int p_tx, int p_ty, const MiniList &ml, float p_threshold_dist, AONearestResult &r_nearest) {
	bool nearest_was_valid = r_nearest.valid;
	r_nearest.valid = false;

	if (!ml.num) {
		return false;
	}

	// All triangles must be coplanar for this to work
	if (ml.num != 1) {
		if (!_is_minilist_coplanar(ml))
			return false;
		else {
			//print_line("ml is coplanar");
		}
	}

	uint32_t tri_id = 0;
	Vector3 pos;
	Vector3 normal;
	const Vector3 *bary = nullptr;
	const Vector3 *plane_normal = nullptr;
	if (!load_texel_data(p_tx, p_ty, tri_id, &bary, pos, normal, &plane_normal)) {
		return false;
	}

#define LLIGHTMAP_AO_CALCULATE_MIDDLE_IMAGE
#ifndef LLIGHTMAP_AO_CALCULATE_MIDDLE_IMAGE
	// Can we reuse the nearest?
	if (nearest_was_valid && (tri_id == r_nearest.tri_id)) {
		// Get the distance between this texel and the previous.
		// This is the maximum change in distance that could have occurred.
		float dist_change = (pos - r_nearest.pos).length();

		// If we are still way out of range, then no need to recalculate anything.
		if ((r_nearest.nearest_tri_distance - dist_change) > p_threshold_dist) {
			// This is the major win path, we are still out of range of already calculated tris.
			r_nearest.valid = true;
			return true;
		}

		// If we are still way within range, no need to recalculate, but we return false.
		if ((r_nearest.nearest_tri_distance + dist_change) < p_threshold_dist) {
			r_nearest.valid = true;
			return false;
		}

		// Else recalculate everything.
	}

	float threshold_dist_sq = p_threshold_dist * p_threshold_dist;
#endif

	Plane plane(pos + (*plane_normal * adjusted_settings.surface_bias), *plane_normal);

	int num_tris = _scene._tris.size();

	r_nearest.nearest_tri_distance = FLT_MAX;
	r_nearest.tri_id = UINT32_MAX;

	for (int n = 0; n < num_tris; n++) {
		if (n == tri_id)
			continue;

		const Tri &tri = _scene._tris[n];

		// Anything that is totally behind the texel triangle plane can be rejected.
		int front_count = 0;

		float plane_dist = FLT_MAX;

		for (int c = 0; c < 3; c++) {
			float dist = plane.distance_to(tri.pos[c]);
			plane_dist = MIN(plane_dist, dist);

			if (dist >= 0)
				front_count++;
		}

		if (front_count == 0) {
			continue;
		}

		// Anything further than the ambient threshold distance can be ignored.
		if (plane_dist > p_threshold_dist) {
			continue;
		}

		// Anything in the goldilocks zone, we must calculate the distance to the closest point on the triangle (expensive).
		Vector3 closest_pt = closest_point_in_triangle(pos, tri.pos[0], tri.pos[1], tri.pos[2]);
		float dist = pos.distance_squared_to(closest_pt);

		if (dist < r_nearest.nearest_tri_distance) {
			r_nearest.nearest_tri_distance = dist;
			r_nearest.tri_id = n;

#ifndef LLIGHTMAP_AO_CALCULATE_MIDDLE_IMAGE
			if (r_nearest.nearest_tri_distance < threshold_dist_sq) {
				r_nearest.valid = true;
				r_nearest.nearest_tri_distance = Math::sqrt(r_nearest.nearest_tri_distance);
				//r_nearest.tri_id = n;
				r_nearest.pos = pos;
				return false;
			}
#endif
		}
	}

	// Change distance from square to regular.
	r_nearest.nearest_tri_distance = Math::sqrt(r_nearest.nearest_tri_distance);

	//	if (p_tx == 64) {
	//		print_line("texel " + itos(p_ty) + ", nearest tri dist " + String(Variant(r_nearest.nearest_tri_distance)));
	//	}

#ifdef LLIGHTMAP_AO_CALCULATE_MIDDLE_IMAGE
	// Within middle distance?
	if (r_nearest.nearest_tri_distance > 0.01f) {
		data_ao.bitimage_middle_only.set_pixel(p_tx, p_ty, true);
		data_ao.middle++;
	} else {
		// not needed except for debugging
		//data_ao.bitimage_middle_only.set_pixel(p_tx, p_ty, false);
		data_ao.non_middle++;
	}

	if (r_nearest.nearest_tri_distance < p_threshold_dist) {
		r_nearest.valid = true;
		//r_nearest.nearest_tri_distance = closest_dist;
		//r_nearest.tri_id = tri_id;
		r_nearest.pos = pos;
		return false;
	}
#endif

	// If we got to here, the nearest is again valid.
	r_nearest.valid = false;
	//	r_nearest.nearest_tri_distance = closest_dist;
	//	r_nearest.tri_id = tri_id;
	//	r_nearest.pos = pos;

	return true;
}

float AmbientOcclusion::calculate_AO(int tx, int ty, int qmc_variation, const MiniList &ml) {
	// Debugging, fast version.
	//	Vector2i diff(500, 100);
	//	diff.x -= tx;
	//	diff.y -= ty;
	//	int64_t sl = (diff.x * diff.x) + (diff.y * diff.y);
	//	if (Math::sqrt((double)sl) > 200) {
	//		return 0;
	//	}

#if 1
	if (data_ao.bitimage_clear.get_pixel(tx, ty)) {
		//	if (AO_are_triangles_out_of_range(tx, ty, ml, adjusted_settings.AO_range, r_nearest)) {
		data_ao.clear++;
		return 1;
	}

	if (data_ao.black_image_valid && data_ao.bitimage_black.get_pixel(tx, ty)) {
		data_ao.black++;
		return 0;
	}
#endif

	data_ao.processed++;

	Ray r;
	int num_samples = adjusted_settings.AO_num_samples;
	int num_samples_per_repeat = num_samples;

	// find the max range in voxels. This can be used to speed up the ray trace
	Vec3i voxel_range = _scene._voxel_range;

	int num_hits = 0;
	int num_samples_inside = 0;
	float previous_metric = 0.0; // ensure always two repeats by setting to 2.0, or make it quicker by making zero hits case faster.

	int attempts = -1;

#define LLIGHTMAP_AO_USE_RESULT_METRIC

	bool fast_approx_test = false;

	LightScene::RayTestHit test_hit;

	/////////////////////////////////////////////////////////////
	bool middle_only = data_ao.bitimage_middle_only.get_pixel(tx, ty);

	Vector2 st;
	AO_random_texel_sample(st, tx, ty, 0);

	uint32_t tri_inside_id;
	Vector3 bary;
	Vector3 normal;
	if (!AO_find_texel_triangle(ml, st, tri_inside_id, bary)) {
		middle_only = false;
	} else {
		// calculate world position ray origin from barycentric
		_scene._tris[tri_inside_id].interpolate_barycentric(r.o, bary);

		// Add a bias along the tri plane.
		const Plane &tri_plane = _scene._tri_planes[tri_inside_id];
		r.o += tri_plane.normal * adjusted_settings.surface_bias;

		// calculate surface normal (should be use plane?)
		normal = tri_plane.normal;
	}

	/////////////////////////////////////////////////////////////
	while (true) {
		attempts++;
		//	for (int n = 0; n < num_samples; n++) {

		if (!middle_only) {
			AO_random_texel_sample(st, tx, ty, attempts);

			// find which triangle on the minilist we are inside (if any)
			if (!AO_find_texel_triangle(ml, st, tri_inside_id, bary))
				continue;

			num_samples_inside++;

			// calculate world position ray origin from barycentric
			_scene._tris[tri_inside_id].interpolate_barycentric(r.o, bary);

			// Add a bias along the tri plane.
			const Plane &tri_plane = _scene._tri_planes[tri_inside_id];
			r.o += tri_plane.normal * adjusted_settings.surface_bias;

			// calculate surface normal (should be use plane?)
			normal = tri_plane.normal;
		} else {
			num_samples_inside++;
		}

		// construct a random ray to test
		//AO_random_QMC_ray(r, normal = tri_plane.normal, attempts, qmc_variation);
		generate_random_hemi_unit_dir(r.d, normal);

		// test ray
		if (_scene.test_intersect_ray_single(r, adjusted_settings.AO_range, test_hit)) {
			num_hits++;
		} else {
			if (_scene.test_intersect_ray(r, adjusted_settings.AO_range, voxel_range, test_hit)) {
				num_hits++;
			} else {
				test_hit.voxel_id = UINT32_MAX;
			}
		}

		// fast zero hits
		if (fast_approx_test && (num_samples_inside == 64)) {
			fast_approx_test = false;

			if (!num_hits)
				return 1;

			if (num_hits >= (num_samples_inside - 8))
				return 0;
		}

		if (num_samples_inside >= num_samples) {
#ifdef LLIGHTMAP_AO_USE_RESULT_METRIC
			// Has it hit error metric?
			float metric = (float)num_hits / num_samples_inside;
			float metric_diff = Math::abs(metric - previous_metric);
			if (metric_diff <= adjusted_settings.AO_error_metric)
				break;

			// Not enough samples, keep going.
			num_samples += num_samples_per_repeat;
			previous_metric = metric;
#else
			break;
#endif
		}
#ifdef LLIGHTMAP_AO_USE_RESULT_METRIC
		if (attempts >= (num_samples_per_repeat * adjusted_settings.AO_abort_timeout)) {
			//			print_line("AO texel aborting after " + itos(attempts) + " attempts, with " + itos(num_hits) + " hits, ml contained " + itos(ml.num) + " tris, " + itos(num_samples_inside) + " were inside, coverage was " + itos(ml.kernel_coverage));
			data_ao.aborts++;
			break;
		}
#else
		if (attempts >= (num_samples_per_repeat * 8)) {
			break;
		}
#endif

	} // for samples

	float total = (float)num_hits / num_samples_inside;
	total = 1.0f - (total * 1.0f);

	if (total < 0)
		total = 0;

	if (!data_ao.black_image_valid && (num_hits == num_samples_inside)) {
		data_ao.bitimage_black.set_pixel(tx, ty, true);
	}

	return total;
}

float AmbientOcclusion::calculate_AO_complex(int tx, int ty, int qmc_variation, const MiniList &ml) {
	// first we need to identify some sample points
	AOSample samples[MAX_COMPLEX_AO_TEXEL_SAMPLES];
	int nSampleLocs = AO_find_sample_points(tx, ty, ml, samples);

	// if no good samples locs found
	if (!nSampleLocs) {
		// set to dilate
		_image_tri_ids_p1.get_item(tx, ty) = 0;
		return 0.5f; // 0.5 could use an intermediate value for texture filtering to look better?
	}

	int sample_counter = 0;
	int nHits = 0;
	Ray r;

	LightScene::RayTestHit test_hit;

	for (int n = 0; n < adjusted_settings.AO_num_samples; n++) {
		// get the sample to look from
		const AOSample &sample = samples[sample_counter++];

		// wraparound
		if (sample_counter == nSampleLocs)
			sample_counter = 0;

		r.o = sample.pos;

		// construct a random ray to test
		AO_random_QMC_direction(r.d, sample.normal, n, qmc_variation);

		// test ray
		if (_scene.test_intersect_ray(r, adjusted_settings.AO_range, _scene._voxel_range, test_hit)) {
			nHits++;
		}
	} // for n

	float fTotal = (float)nHits / adjusted_settings.AO_num_samples;
	fTotal = 1.0f - (fTotal * 1.0f);

	if (fTotal < 0.0f)
		fTotal = 0.0f;

	return fTotal;
}

int AmbientOcclusion::AO_find_sample_points(int tx, int ty, const MiniList &ml, AOSample samples[MAX_COMPLEX_AO_TEXEL_SAMPLES]) {
	int samples_found = 0;
	int attempts = adjusted_settings.AO_num_samples + 64;

	// scale number of sample points roughly to the user interface quality
	int num_desired_samples = adjusted_settings.AO_num_samples;
	if (num_desired_samples > MAX_COMPLEX_AO_TEXEL_SAMPLES)
		num_desired_samples = MAX_COMPLEX_AO_TEXEL_SAMPLES;

	Ray r;

	LightScene::RayTestHit test_hit;

	for (int n = 0; n < attempts; n++) {
		Vector2 st;
		AO_random_texel_sample(st, tx, ty, 10);

		// find which triangle on the minilist we are inside (if any)
		uint32_t tri_inside_id;
		Vector3 bary;

		if (!AO_find_texel_triangle(ml, st, tri_inside_id, bary))
			continue; // not inside a triangle, failed this attempt

		// calculate world position ray origin from barycentric
		Vector3 pos;
		_scene._tris[tri_inside_id].interpolate_barycentric(pos, bary);
		r.o = pos;

		// calculate surface normal (should be use plane?)
		const Vector3 &ptNormal = _scene._tri_planes[tri_inside_id].normal;

		// construct a random ray to test
		AO_find_samples_random_ray(r, ptNormal);

		// test ray
		if (_scene.test_intersect_ray(r, adjusted_settings.AO_range, _scene._voxel_range, test_hit, true))
			continue; // hit, not interested

		// no hit .. we can use this sample!
		AOSample &sample = samples[samples_found++];

		// be super careful with this offset.
		// the find test offsets BACKWARDS to find tris on the floor
		// the actual ambient test offsets FORWARDS to avoid self intersection.
		// the ambient test could also use the backface culling test, but this is slower.
		sample.pos = pos + (ptNormal * adjusted_settings.surface_bias); //0.005f);
		sample.uv = st;
		sample.normal = ptNormal;
		sample.tri_id = tri_inside_id;

		// finished?
		if (samples_found == num_desired_samples)
			return samples_found;
	}

	return samples_found;
}

} // namespace LM
