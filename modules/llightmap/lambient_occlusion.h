#pragma once

#include "lraybank.h"

namespace LM {

class AmbientOcclusion : public RayBank {
public:
protected:
	struct AOSample {
		Vector2 uv;
		uint32_t tri_id;
		Vector3 pos;
		Vector3 normal;
	};

	enum { MAX_COMPLEX_AO_TEXEL_SAMPLES = 16 };

	void process_AO();
	void process_AO_line_MT(uint32_t y_offset, int y_section_start);
	void process_AO_texel(int tx, int ty, int qmc_variation);
	float calculate_AO(int tx, int ty, int qmc_variation, const MiniList &ml);
	float calculate_AO_complex(int tx, int ty, int qmc_variation, const MiniList &ml);

	bool AO_find_texel_triangle(const MiniList &p_mini_list, const Vector2 &p_st, uint32_t &r_tri_inside, Vector3 &r_bary) const {
		// barycentric coords.
		const UVTri *uv_tri;

		// This needs to check face normals, because there can be a large number of tris
		// intersecting a single texel.
		for (uint32_t i = 0; i < p_mini_list.num; i++) {
			r_tri_inside = _minilist_tri_ids[p_mini_list.first + i];
			uv_tri = &_scene._uv_tris[r_tri_inside];

			// within?
			if (!uv_tri->find_barycentric_coords(p_st, r_bary))
				continue;

			if (barycentric_inside(r_bary)) {
				return true;
			}
		}

		// not inside any triangles
		return false;
	}

	bool AO_find_texel_triangle_facing_position(const MiniList &p_mini_list, const Vector2 &p_st, uint32_t &r_tri_inside, Vector3 &r_bary, bool &r_backfacing_hits, const Vector3 &p_facing_position, bool p_debug) const {
		// barycentric coords.
		const UVTri *pUVTri;

		uint32_t best = UINT32_MAX;
		float best_insideness = 0.51f; // 0.6
		r_backfacing_hits = false;

		//if (p_mini_list.num > 1)
		//	p_debug = true;

		// This needs to check face normals, because there can be a large number of tris
		// intersecting a single texel.
		for (uint32_t i = 0; i < p_mini_list.num; i++) {
			uint32_t tri_inside = _minilist_tri_ids[p_mini_list.first + i];
			pUVTri = &_scene._uv_tris[tri_inside];

			// Within?
			Vector3 bary;

			if (!pUVTri->find_barycentric_coords(p_st, bary))
				continue;

			float insideness = barycentric_insideness(bary);
			if (p_debug) {
				print_line("Tri " + itos(i) + " (" + itos(tri_inside) + ")\tbary : " + String(Variant(bary)) + "\tinsideness: " + String(Variant(insideness)));
			}

			if (insideness < best_insideness) {
				Vector3 tri_pos;
				_scene._tris[tri_inside].interpolate_barycentric(tri_pos, bary);
				Vector3 tri_to_light = p_facing_position - tri_pos;

				// Check whether facing the position.
				const Vector3 &face_normal = _scene._tri_planes[tri_inside].normal;

				float dot = face_normal.dot(tri_to_light);

				//				if (p_debug)
				//				{
				//					print_line("\tface normal: " + String(Variant(face_normal)) + "\tdot: " + String(Variant(dot)));
				//				}
				if (dot > 0) {
					best_insideness = insideness;
					best = i;
					r_bary = bary;
					r_tri_inside = tri_inside;
				} else {
					r_backfacing_hits = true;
				}
			}
		}

		//		if ((best != -1) && (best_insideness < 0.6f)) {
		if (best != -1) {
			return true;
		}

		// not inside any triangles
		return false;
	}

private:
	int AO_find_sample_points(int tx, int ty, const MiniList &ml, AOSample samples[MAX_COMPLEX_AO_TEXEL_SAMPLES]);

	void AO_random_texel_sample(Vector2 &st, int tx, int ty, int n) const {
		if (n) {
			st.x = Math::randf() + tx;
			st.y = Math::randf() + ty;
		} else {
			// fix first to centre of texel
			st.x = 0.5f + tx;
			st.y = 0.5f + ty;
		}

		// has to be ranged 0 to 1
		st.x /= _width;
		st.y /= _height;
	}

	void AO_random_QMC_direction(Vector3 &dir, const Vector3 &ptNormal, int n, int qmc_variation) const {
		Vector3 push = ptNormal * adjusted_settings.surface_bias; //0.005f;

		_QMC.QMC_random_unit_dir(dir, n, qmc_variation);

		// clip?
		float dot = dir.dot(ptNormal);
		if (dot < 0.0f)
			dir = -dir;

		// prevent parallel lines
		dir += push;

		// collision detect
		dir.normalize();
	}

	void AO_random_QMC_ray(Ray &r, const Vector3 &ptNormal, int n, int qmc_variation) const {
		Vector3 push = ptNormal * adjusted_settings.surface_bias; // 0.005f;

		// push ray origin
		r.o += push;

		//		RandomUnitDir(r.d);
		_QMC.QMC_random_unit_dir(r.d, n, qmc_variation);

		// clip?
		float dot = r.d.dot(ptNormal);
		if (dot < 0.0f) {
			// make dot always positive for calculations as to the weight given to this hit
			//dot = -dot;
			r.d = -r.d;
		}

		// prevent parallel lines
		r.d += push;
		//	r.d = ptNormal;

		// collision detect
		r.d.normalize();
	}

	void AO_find_samples_random_ray(Ray &r, const Vector3 &ptNormal) const {
		//Vector3 push = ptNormal * settings.AO_ReverseBias; //0.005f;
		Vector3 push = ptNormal * adjusted_settings.surface_bias; //0.005f;

		// push ray origin
		r.o -= push;

		generate_random_hemi_unit_dir(r.d, ptNormal);

		// clip?

		// prevent parallel lines
		r.d += push;
		//	r.d = ptNormal;

		// collision detect
		r.d.normalize();
	}
};

} //namespace LM
