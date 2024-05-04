#include "lambient_occlusion.h"
#include "core/os/threaded_array_processor.h"

namespace LM {

void AmbientOcclusion::process_AO_texel(int tx, int ty, int qmc_variation) {
	//	if ((tx == 77) && (ty == 221))
	//		print_line("test");

	// Reclaimed?
	if (!_image_tri_ids_p1.get_item(tx, ty))
		return;

	const MiniList &ml = _image_tri_minilists.get_item(tx, ty);
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

void AmbientOcclusion::process_AO() {
	//	if (bake_begin_function) {
	//		bake_begin_function(m_iHeight);
	//	}
	_image_AO.create(_width, _height, 1);

	// find the max range in voxels. This can be used to speed up the ray trace
	const float range = adjusted_settings.AO_range;
	_scene._tracer.get_distance_in_voxels(range, _scene._voxel_range);

	data_ao.aborts = 0;

#define LAMBIENT_OCCLUSION_USE_THREADS
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

	print_line("AO aborts " + itos(data_ao.aborts) + " out of " + itos(_width * _height) + " pixels.");

	if (bake_end_function) {
		bake_end_function();
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

float AmbientOcclusion::calculate_AO(int tx, int ty, int qmc_variation, const MiniList &ml) {
	Ray r;
	int num_samples = adjusted_settings.AO_num_samples;
	int num_samples_per_repeat = num_samples;

	// find the max range in voxels. This can be used to speed up the ray trace
	Vec3i voxel_range = _scene._voxel_range;

	int num_hits = 0;
	int num_samples_inside = 0;
	float previous_metric = 0;

	int attempts = -1;

#define LLIGHTMAP_AO_USE_RESULT_METRIC

	while (true) {
		attempts++;
		//	for (int n = 0; n < num_samples; n++) {

		// pick a float position within the texel
		Vector2 st;
		AO_random_texel_sample(st, tx, ty, attempts);

		// find which triangle on the minilist we are inside (if any)
		uint32_t tri_inside_id;
		Vector3 bary;
		if (!AO_find_texel_triangle(ml, st, tri_inside_id, bary))
			continue;

		num_samples_inside++;

		// calculate world position ray origin from barycentric
		_scene._tris[tri_inside_id].interpolate_barycentric(r.o, bary);

		// Add a bias along the tri plane.
		const Plane &tri_plane = _scene._tri_planes[tri_inside_id];
		r.o += tri_plane.normal * adjusted_settings.surface_bias;

		// calculate surface normal (should be use plane?)
		Vector3 normal = tri_plane.normal;
		//_scene._tri_normals[tri_inside_id].interpolate_barycentric(normal = tri_plane.normal, bary);

		// construct a random ray to test
		//AO_random_QMC_ray(r, normal = tri_plane.normal, attempts, qmc_variation);
		generate_random_hemi_unit_dir(r.d, normal);

		// test ray
		if (_scene.test_intersect_ray(r, adjusted_settings.AO_range, voxel_range))
			num_hits++;

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
		if (_scene.test_intersect_ray(r, adjusted_settings.AO_range, _scene._voxel_range)) {
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
		if (_scene.test_intersect_ray(r, adjusted_settings.AO_range, _scene._voxel_range, true))
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
