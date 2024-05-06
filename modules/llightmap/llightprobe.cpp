#include "llightprobe.h"
#include "core/math/aabb.h"
#include "llightmapper.h"
#include "llighttypes.h"

namespace LM {

int LightProbes::create(LightMapper &lm) {
	//	return;

	// save locally
	data.light_mapper = &lm;

	// get the aabb (smallest) of the world bound.
	AABB bb = lm.get_tracer().get_world_bound_contracted();

	// can't make probes for a non existant world
	if (bb.get_shortest_axis_size() <= 0.0f)
		return -1;

	// use the probe density to estimate the number of voxels
	data.dims = lm.get_tracer().estimate_voxel_dims(lm.data.params[LightMapper::PARAM_PROBE_DENSITY]);
	data.dim_x_times_y = (data.dims.x * data.dims.y);

	Vector3 voxel_dims;
	data.dims.to(voxel_dims);

	print_line("Probes voxel dims : " + String(Variant(voxel_dims)));

	// voxel dimensions and start point
	data.voxel_size = bb.size / voxel_dims;

	// start point is offset by half a voxel
	data.pt_min = bb.position + (data.voxel_size * 0.5f);

	data.probes.resize(data.dims.x * data.dims.y * data.dims.z);

	return data.dims.z;
}

void LightProbes::process(int stage) {
	int z = stage;
	//	for (int z=0; z<data.dims.z; z++)
	//	{
	for (int y = 0; y < data.dims.y; y++) {
		for (int x = 0; x < data.dims.x; x++) {
			calculate_probe(Vec3i(x, y, z));
		}
	}
	//	}
}

void LightProbes::save() {
	String filename = data.light_mapper->settings.combined_filename;
	filename = filename.get_basename();
	filename += ".probe";

	save(filename);
	//	Save("lightprobes.probe");
}

void LightProbes::calculate_probe_old(const Vec3i &pt) {
	Vector3 pos;
	get_probe_position(pt, pos);

	LightProbe *pProbe = get_probe(pt);
	DEV_ASSERT(pProbe);

	// do multiple tests per light
	const int nLightTests = 9;
	Vector3 ptLightSpacing = data.voxel_size / (nLightTests + 2);

	for (int l = 0; l < data.light_mapper->_lights.size(); l++) {
		const LightMapper_Base::LLight &light = data.light_mapper->_lights[l];

		// the start of the mini grid for each voxel
		Vector3 ptLightStart = pos - (ptLightSpacing * ((nLightTests - 1) / 2));

		int nHits = 0;

		for (int lt_z = 0; lt_z < nLightTests; lt_z++) {
			for (int lt_y = 0; lt_y < nLightTests; lt_y++) {
				for (int lt_x = 0; lt_x < nLightTests; lt_x++) {
					// ray from probe to light
					Ray r;
					r.o = ptLightStart;
					r.o.x += (lt_x * ptLightSpacing.x);
					r.o.y += (lt_y * ptLightSpacing.y);
					r.o.z += (lt_z * ptLightSpacing.z);

					Vector3 offset = light.pos - r.o;
					float dist_to_light = offset.length();

					// NYI
					if (dist_to_light == 0.0f)
						continue;

					r.d = offset / dist_to_light;

					// to start with just trace a ray to each light
					Vector3 bary;
					float nearest_t;
					int tri_id = data.light_mapper->_scene.find_intersect_ray(r, bary, nearest_t);

					// light blocked
					if ((tri_id != -1) && (nearest_t < dist_to_light))
						continue;

					nHits++;

				} // for lt_x
			} // for lt_y
		} // for lt_z

		if (nHits) {
			// light got through..
			// pProbe->m_fLight += 1.0f;
			LightProbe::Contribution *pCont = pProbe->contributions.request();
			pCont->light_id = l;

			// calculate power based on distance
			//pCont->power = CalculatePower(light, dist_to_light, pos);
			pCont->power = (float)nHits / (float)(nLightTests * nLightTests * nLightTests);

			//print_line("\tprobe " + pt.ToString() + " light " + itos (l) + " power " + String(Variant(pCont->power)));
		}
	}

	// indirect light
	pProbe->color_indirect = data.light_mapper->probe_calculate_indirect_light(pos);

	// apply gamma
	float gamma = 1.0f / (float)data.light_mapper->data.params[LightMapper_Base::PARAM_GAMMA];
	pProbe->color_indirect.r = powf(pProbe->color_indirect.r, gamma);
	pProbe->color_indirect.g = powf(pProbe->color_indirect.g, gamma);
	pProbe->color_indirect.b = powf(pProbe->color_indirect.b, gamma);

	Color col_ind;
	pProbe->color_indirect.to(col_ind);
	//print_line("\tprobe " + pt.ToString() + " indirect " + String(Variant(col_ind)));
}

void LightProbes::calculate_probe(const Vec3i &pt) {
	Vector3 pos;
	get_probe_position(pt, pos);

	LightProbe *pProbe = get_probe(pt);
	DEV_ASSERT(pProbe);

	// do multiple tests per light
	const int nSamples = (int)data.light_mapper->data.params[LightMapper::PARAM_PROBE_SAMPLES] / 8;

	for (int l = 0; l < data.light_mapper->_lights.size(); l++) {
		const LightMapper_Base::LLight &light = data.light_mapper->_lights[l];

		// for a spotlight, we can cull completely in a lot of cases.
		// and we MUST .. because otherwise the cone ray generator will have infinite loop.
		if (light.type == LightMapper_Base::LLight::LT_SPOT) {
			Ray r;
			r.o = light.spot_emanation_point;
			r.d = pos - r.o;
			r.d.normalize();
			float dot = r.d.dot(light.dir);
			//float angle = safe_acosf(dot);
			//if (angle >= light.spot_angle_radians)

			dot -= light.spot_dot_max;

			if (dot <= 0.0f)
				continue;
		}

		/*
		AABB bb;
		bb.position = data.pt_min;
		bb.position.x += data.voxel_size.x * pt.x;
		bb.position.y += data.voxel_size.y * pt.y;
		bb.position.z += data.voxel_size.z * pt.z;
		bb.size = data.voxel_size;
		*/

		//int nClear = 0;

		// store as a float for number of clear paths, because spotlights have a falloff for the cone
		float fClear = 0.0f;

		//bool LightMapper::Process_BackwardSample_Light(const LLight &light, const Vector3 &ptSource, const Vector3 &ptNormal, FColor &color, float power)

		// dummy not really needed
		//Vector3 ptNormal = Vector3(0, 1, 0);
		//FColor fcol;
		//fcol.Set(0.0f);

		// test single ray from probe to light centre
		//if (!data.light_mapper->m_Scene.TestIntersect_Line(pos, light.pos))
		//	fClear = nSamples;

		int sample_count = 0;

		for (int sample = 0; sample < nSamples; sample++) {
			// random start position in bounding box
			//			Vector3 ptStart = bb.position;
			//			ptStart.x += Math::randf() * bb.size.x;
			//			ptStart.y += Math::randf() * bb.size.y;
			//			ptStart.z += Math::randf() * bb.size.z;
			Vector3 ptStart = pos;

			float multiplier;
			bool disallow_sample;
			bool bClear = data.light_mapper->probe_sample_to_light(light, ptStart, multiplier, disallow_sample);

			// don't count samples if there is not path from the light centre to the light sample
			if (!disallow_sample)
				sample_count++;

			if (bClear) {
				fClear += multiplier;
			}

		} // for sample

		if (fClear > 0.0f) {
			// light got through..
			// pProbe->m_fLight += 1.0f;
			LightProbe::Contribution *pCont = pProbe->contributions.request();
			pCont->light_id = l;

			// calculate power based on distance
			//pCont->power = CalculatePower(light, dist_to_light, pos);
			DEV_ASSERT(sample_count);
			pCont->power = (float)fClear / (float)(sample_count);

			//print_line("\tprobe " + pt.ToString() + " light " + itos (l) + " power " + String(Variant(pCont->power)));
		}
	}

	// indirect light
	pProbe->color_indirect = data.light_mapper->probe_calculate_indirect_light(pos);

	// apply gamma
	float gamma = 1.0f / (float)data.light_mapper->data.params[LightMapper_Base::PARAM_GAMMA];
	pProbe->color_indirect.r = powf(pProbe->color_indirect.r, gamma);
	pProbe->color_indirect.g = powf(pProbe->color_indirect.g, gamma);
	pProbe->color_indirect.b = powf(pProbe->color_indirect.b, gamma);

	Color col_ind;
	pProbe->color_indirect.to(col_ind);
	//print_line("\tprobe " + pt.ToString() + " indirect " + String(Variant(col_ind)));
}

float LightProbes::calculate_power(const LightMapper_Base::LLight &light, float dist, const Vector3 &pos) const {
	// ignore light energy for getting more detail, we can apply the light energy at runtime
	//float power = data.light_mapper->LightDistanceDropoff(dist, light, 1.0f);

	// now we are calculating distance func in the shader, power can be used to represent how many rays hit the light,
	// and also the cone of a spotlight
	float power = 1.0f;
	return power;
}

void LightProbes::save_Vector3(FileAccess *f, const Vector3 &v) {
	f->store_float(v.x);
	f->store_float(v.y);
	f->store_float(v.z);
}

void LightProbes::normalize_indirect() {
	float max = 0.0f;

	for (int n = 0; n < data.probes.size(); n++) {
		FColor &col = data.probes[n].color_indirect;
		if (col.r > max)
			max = col.r;
		if (col.g > max)
			max = col.g;
		if (col.b > max)
			max = col.b;
	}

	// can't normalize
	if (max == 0.0f) {
		print_line("WARNING LightProbes::Normalize_Indirect : Can't normalize, range too small");
		return;
	}

	float mult = 1.0f / max;

	for (int n = 0; n < data.probes.size(); n++) {
		FColor &col = data.probes[n].color_indirect;
		col *= mult;
	}
}

void LightProbes::save(String pszFilename) {
	normalize_indirect();

	// try to open file for writing
	FileAccess *f = FileAccess::open(pszFilename, FileAccess::WRITE);
	ERR_FAIL_COND_MSG(!f, "Cannot create file at path '" + String(pszFilename) + "'.");

	// fourcc
	f->store_string("Prob");

	// version
	f->store_16(100);

	// first save dimensions
	f->store_16(data.dims.x);
	f->store_16(data.dims.y);
	f->store_16(data.dims.z);

	// minimum pos
	save_Vector3(f, data.pt_min);

	// voxel size
	save_Vector3(f, data.voxel_size);

	// num lights
	f->store_16(data.light_mapper->_lights.size());

	for (int n = 0; n < data.light_mapper->_lights.size(); n++) {
		// light info
		const LightMapper_Base::LLight &light = data.light_mapper->_lights[n];

		// type
		f->store_8(light.type);

		// pos, direction
		save_Vector3(f, light.pos);
		save_Vector3(f, light.dir);

		// energy
		f->store_float(light.energy);
		f->store_float(light.range);

		// color
		f->store_float(light.color.r);
		f->store_float(light.color.g);
		f->store_float(light.color.b);

		f->store_float(light.spot_angle_radians);
	}

	// now save voxels .. 2 passes to make easier to compress with zip.
	// the first pass will be easily compressable
	for (int z = 0; z < data.dims.z; z++) {
		for (int y = 0; y < data.dims.y; y++) {
			for (int x = 0; x < data.dims.x; x++) {
				LightProbe *probe = get_probe(Vec3i(x, y, z));
				probe->save(f);
			}
		}
	}

	for (int z = 0; z < data.dims.z; z++) {
		for (int y = 0; y < data.dims.y; y++) {
			for (int x = 0; x < data.dims.x; x++) {
				LightProbe *probe = get_probe(Vec3i(x, y, z));
				probe->save_secondary(f);
			}
		}
	}

	if (f) {
		f->close();
		memdelete(f);
	}
}

void LightProbe::save_secondary(FileAccess *f) {
	// color
	uint8_t r, g, b;
	color_indirect.to_u8(r, g, b, 1.0f);

	f->store_8(r);
	f->store_8(g);
	f->store_8(b);

	int nContribs = contributions.size();
	if (nContribs > 255)
		nContribs = 255;

	for (int n = 0; n < nContribs; n++) {
		const Contribution &c = contributions[n];

		//f->store_8(c.light_id);

		// convert power to linear 0 to 255
		float p = c.power;
		p *= 0.5f;
		p *= 255.0f;

		int i = p;
		i = CLAMP(i, 0, 255);
		f->store_8(i);
	}
}

void LightProbe::save(FileAccess *f) {
	// number of contributions
	int nContribs = contributions.size();
	if (nContribs > 255)
		nContribs = 255;

	// store number of contribs
	f->store_8(nContribs);

	for (int n = 0; n < nContribs; n++) {
		const Contribution &c = contributions[n];

		f->store_8(c.light_id);

		// convert power to linear 0 to 255
		//		float p = c.power;
		//		p *= 0.5f;
		//		p *= 255.0f;

		//		int i = p;
		//		i = CLAMP(i, 0, 255);
		//		f->store_8(i);
	}
}

} //namespace LM
