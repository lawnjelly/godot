#pragma once

#include "lambient_occlusion.h"

namespace LM {

class LightMapper : public AmbientOcclusion {
public:
	// main function called from the godot class
	bool lightmap_meshes(Spatial *pMeshesRoot, Spatial *pLR, Image *pIm_Lightmap, Image *pIm_AO, Image *pIm_Combined);
	bool uv_map_meshes(Spatial *pRoot);

private:
	bool _lightmap_meshes(Spatial *pMeshesRoot, const Spatial &light_root, Image &out_image_lightmap, Image &out_image_ao, Image &out_image_combined);
	void reset();
	void save_intermediates();

private:
	void save_tri_ids(const String &p_filename);
	bool load_tri_ids(const String &p_filename);
	template <class T>
	void save_intermediate(bool p_save, const String &p_filename, const LightImage<T> &p_lightimage);
	template <class T>
	bool load_intermediate(const String &p_filename, LightImage<T> &r_lightimage);

	// forward tracing
	void process_lights();
	void process_light(int light_id, int num_rays);

	// void process_emission_tris();
	// void process_emission_tris_section(float fraction_of_total);
	// void process_emission_tri(int etri_id, float fraction_of_total);

	void process_emission_pixels();
	void process_emission_pixel_MT(uint32_t p_pixel_id, int p_dummy);
	void process_emission_pixel(int32_t p_x, int32_t p_y);
	void process_emission_pixel_backward(int32_t p_x, int32_t p_y, const Color &p_emission, const Vector3 &p_emission_pos, const Vector3 &p_emission_normal);

	// ambient bounces
	void do_ambient_bounces();
	void process_texels_ambient_bounce(int section_size, int num_sections);
	void process_texels_ambient_bounce_line_MT(uint32_t offset_y, int start_y);
	FColor process_texel_ambient_bounce(int x, int y, uint32_t tri_source);
	bool process_texel_ambient_bounce_sample(const Vector3 &plane_norm, const Vector3 &ray_origin, FColor &total_col);

	// backward tracing
	void backward_process_texels();
	void backward_process_texel_line_MT(uint32_t offset_y, int start_y);

	// backward forward tracing
	void BF_process_texel(int tx, int ty);
	void BF_process_texel_light(const ColorSample &p_tri_color_sample, int light_id, const Vector3 &pt_source_pushed, const Vector3 &orig_face_normal, const Vector3 &orig_vertex_normal, Color &r_color, SubTexelSample &r_sub_texel_sample, bool debug = false); //, uint32_t tri_ignore);
	void BF_process_texel_light_bounce(int bounces_left, Ray r, Color ray_color);

	bool BF_process_texel_sky(const Color &orig_albedo, const Vector3 &ptSource, const Vector3 &orig_face_normal, const Vector3 &orig_vertex_normal, FColor &color);

	bool bounce_ray(Ray &r, const Vector3 &face_normal, bool apply_epsilon = true) {
		return bounce_ray_with_smoothness(r, face_normal, adjusted_settings.smoothness, apply_epsilon);
	}
	bool bounce_ray_with_smoothness(Ray &r, const Vector3 &face_normal, float p_smoothness, bool apply_epsilon = true);

	// Backward tracing with antialiasing.
	void AA_BF_process_texel(int p_tx, int p_ty);
	bool AA_BF_process_sub_texel(float p_fx, float p_fy, const MiniList &p_ml, Color &r_col);
	bool AA_BF_process_sub_texel_for_light(float p_fx, float p_fy, const MiniList &p_ml, Color &r_col, int p_light_id, SubTexelSample &r_sub_texel_sample, bool p_debug);
	bool _AA_BF_process_sub_texel_for_light(const Vector2 &p_st, const MiniList &p_ml, Color &r_col, int p_light_id, SubTexelSample &r_sub_texel_sample, bool p_debug);
	void _AA_create_kernel();
	void _AA_reclaim_texels();

	// Modulate original material with the lightmap (if we want to merge them).
	void process_orig_material();
	void _process_orig_material_texel(int32_t p_tx, int32_t p_ty);
	bool _process_orig_material_sub_texel(float p_fx, float p_fy, const MiniList &p_ml, Color &r_total_color);

	// new backward tracing experiment, by triangle
#if 0
	void backward_trace_triangles();
	void backward_trace_triangle(int tri_id);
	void backward_trace_sample(int tri_id);
#endif

	void MT_safe_add_fcolor_to_texel(int32_t p_x, int32_t p_y, const Color &p_col) {
		FColor fcol;
		fcol.set(p_col);
		MT_safe_add_fcolor_to_texel(p_x, p_y, fcol);
	}

	// multithread safe. This must prevent against several threads trying to access the same texel
	// at once. This will be rare but must be prevented.
	void MT_safe_add_fcolor_to_texel(int tx, int ty, const FColor &col) {
		// not yet thread safe
		FColor *pTexel = _image_main.get(tx, ty);
		if (!pTexel)
			return;

		_atomic.atomic_add_col(tx, *pTexel, col);
		//*pTexel += col;
	}

	void MT_safe_add_color_to_texel(int tx, int ty, const Color &col) {
		FColor f;
		f.set(col);
		MT_safe_add_fcolor_to_texel(tx, ty, f);
	}

	// light probes
	void process_light_probes();

public:
	// encapsulate a single backward trace to a light, because this will be useful for light probes as well as backward tracing
	bool probe_sample_to_light(const LLight &light, const Vector3 &ptProbe, float &multiplier, bool &disallow_sample);

	bool light_random_sample(const LLight &light, const Vector3 &ptSurf, Ray &ray, Vector3 &ptLight, float &ray_length, float &multiplier) const;

	//bool Process_BackwardSample_Light(const LLight &light, const Vector3 &ptSource, const Vector3 &ptNormal, FColor &color, float power, float &multiplier, bool bTestLightToSample);

	FColor probe_calculate_indirect_light(const Vector3 &pos);

private:
	void refresh_process_state();
	//	const int m_iRaysPerSection = 1024 * 1024 * 4; // 64
	const int RAYS_PER_SECTION = 1024 * 64; // 64
	// 1024*1024 is 46 megs
};

bool LightMapper::probe_sample_to_light(const LLight &light, const Vector3 &ptProbe, float &multiplier, bool &disallow_sample) {
	Ray r;
	Vector3 ptLight;
	float ray_length;
	if (!light_random_sample(light, ptProbe, r, ptLight, ray_length, multiplier))
		return false;

	// if there is no clear path from the light to the the light sample point, we disallow the sample
	// but not for directional lights as only the direction matters for these, not the origin
	if ((light.type != LLight::LT_DIRECTIONAL) && _scene.test_intersect_line(light.pos, ptLight)) {
		disallow_sample = true;
		return false;
	}

	disallow_sample = false;

	// just an intersect test for now
	if (_scene.test_intersect_line(ptProbe, ptLight))
		return false;

	return true;
}

/*
// encapsulate a single backward trace to a light, because this will be useful for light probes as well as backward tracing
// returns true if path to light
bool LightMapper::Process_BackwardSample_Light(const LLight &light, const Vector3 &ptSource, const Vector3 &ptNormal, FColor &color, float power, float &multiplier, bool bTestLightToSample)
{
	Ray r;
	Vector3 ptDest = light.pos;

	// allow falloff for cones
	multiplier = 1.0f;

	switch (light.type)
	{
	case LLight::LT_DIRECTIONAL:
		{
			r.o = ptSource;
			//r.d = -light.dir;

			// perturb direction depending on light scale
			//Vector3 ptTarget = light.dir * -2.0f;

			Vector3 offset;
			RandomUnitDir(offset);
			offset *= light.scale;

			offset += (light.dir * -2.0f);
			r.d = offset.normalized();

			// disallow zero length (should be rare)
			if (r.d.length_squared() < 0.00001f)
				return false;

			// don't allow from opposite direction
			if (r.d.dot(light.dir) > 0.0f)
				r.d = -r.d;
		}
		break;
	case LLight::LT_SPOT:
		{
			// NOTE that for spotlights you must do a broad check before this function, otherwise you
			// get infinite loops looking for a ray within the spot

			// source
			float dot;

			while (true)
			{
				Vector3 offset;
				RandomUnitDir(offset);
				offset *= light.scale;

				r.o = light.pos;
				r.o += offset;

				// offset from origin to destination texel
				r.d = ptSource - r.o;
				r.d.normalize();

				dot = r.d.dot(light.dir);
				//float angle = safe_acosf(dot);
				//if (angle >= light.spot_angle_radians)

				dot -= light.spot_dot_max;

				// if within cone, it is ok
				if (dot > 0.0f)
					break;
			}

			// there must be a clear path from the light to the light sample
			if (bTestLightToSample && m_Scene.TestIntersect_Line(light.pos, r.o))
				return false;

			// reverse ray for precision reasons
			r.d = -r.d;
			r.o = ptSource;

			dot *= 1.0f / (1.0f - light.spot_dot_max);
			multiplier = dot * dot;
			multiplier *= multiplier;



			// direction
			//				r.d = light.dir;
			//				float spot_ball = 0.2f;
			//				float x = Math::random(-spot_ball, spot_ball);
			//				float y = Math::random(-spot_ball, spot_ball);
			//				float z = Math::random(-spot_ball, spot_ball);
			//				r.d += Vector3(x, y, z);
			//				r.d.normalize();
		}
		break;
	default:
		{
			Vector3 offset;
			RandomUnitDir(offset);
			offset *= light.scale;
			ptDest += offset;

			// there must be a clear path from the light to the light sample
			if (bTestLightToSample && m_Scene.TestIntersect_Line(light.pos, ptDest))
				return false;

			// offset from origin to destination texel
			r.o = ptSource;
			r.d = ptDest - r.o;
			r.d.normalize();

		}
		break;
	}


	Vector3 ray_origin = r.o;
	FColor sample_color = light.color;

	int panic_count = 32;

	while (true)
	{
		// collision detect
		float u, v, w, t;

		m_Scene.m_Tracer.m_bUseSDF = true;
		int tri = m_Scene.FindIntersect_Ray(r, u, v, w, t);
		//		m_Scene.m_Tracer.m_bUseSDF = false;
		//		int tri2 = m_Scene.IntersectRay(r, u, v, w, t, m_iNumTests);
		//		if (tri != tri2)
		//		{
		//			// repeat SDF version
		//			m_Scene.m_Tracer.m_bUseSDF = true;
		//			int tri = m_Scene.IntersectRay(r, u, v, w, t, m_iNumTests);
		//		}


		// nothing hit
		if (tri == -1)
			//if ((tri == -1) || (tri == (int) tri_ignore))
		{
			// for backward tracing, first pass, this is a special case, because we DO
			// take account of distance to the light, and normal, in order to simulate the effects
			// of the likelihood of 'catching' a ray. In forward tracing this happens by magic.
			float local_power;

			// no drop off for directional lights
			float dist = (ptDest - ray_origin).length();
			local_power = LightDistanceDropoff(dist, light, power);

			// take into account normal
			float dot = r.d.dot(ptNormal);
			dot = fabs(dot);

			local_power *= dot;

			// cone falloff
			local_power *= multiplier;

			// total color
			color += sample_color * local_power;

			return true;
		}
		else
		{
			// hit something, could be transparent

			// back face?
			Vector3 face_normal;
			bool bBackFace = HitBackFace(r, tri, Vector3(u, v, w), face_normal);


			// first get the texture details
			Color albedo;
			bool bTransparent;
			m_Scene.FindPrimaryTextureColors(tri, Vector3(u, v, w), albedo, bTransparent);

			if (bTransparent)
			{
				bool opaque = bTransparent && (albedo.a > 0.999f);

				// push ray past the surface
				if (!opaque)
				{
					// hard code
					//						if (albedo.a > 0.0f)
					//							albedo.a = 0.3f;

					// position of potential hit
					Vector3 pos;
					const Tri &triangle = m_Scene.m_Tris[tri];
					triangle.InterpolateBarycentric(pos, u, v, w);

					float push = -settings.SurfaceBias;
					if (bBackFace) push = -push;

					r.o = pos + (face_normal * push);

					// apply the color to the ray
					CalculateTransmittance(albedo, sample_color);

					// this shouldn't happen, but if it did we'd get an infinite loop if we didn't break out
					panic_count--;
					if (!panic_count)
						break;

					continue;
				}
			}
		}

		break; // only trace once usually .. use a continue to trace again
	} // while keep tracing

	return false;
}
*/

} // namespace LM
