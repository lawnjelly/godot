#pragma once

#include "llightmapper_base.h"
#include "llighttypes.h"
#include "lvector.h"

namespace LM {

struct RB_Voxel {
	LVector<FRay> rays;
};

class RayBank : public LightMapper_Base {
public:
	void ray_bank_reset(bool recreate = false);
	void ray_bank_create();

	// every time we want to queue a new ray for processing
	FRay *ray_bank_request_new_ray(Ray ray, int num_rays_left, const FColor &col, const Vec3i *pStartVoxel = nullptr);

	// multithread accelerated .. do intersection tests on rays, calculate hit points and new rays
	void ray_bank_process();

	// flush ray results to the lightmap
	void ray_bank_flush();

	bool ray_bank_are_voxels_clear();

private:
	// used for below multithread routine
	RB_Voxel *_p_current_thread_voxel;
	void ray_bank_process_ray_MT(uint32_t ray_id, int start_ray);
	void ray_bank_process_ray_MT_old(uint32_t ray_id, int start_ray);

	void ray_bank_flush_ray(RB_Voxel &vox, int ray_id);

	RB_Voxel &ray_bank_get_voxel_write(const Vec3i &pt) {
		int n = get_tracer().get_voxel_num(pt);
		return _data_RB.get_voxels_write()[n];
	}
	RB_Voxel &ray_bank_get_voxel_read(const Vec3i &pt) {
		int n = get_tracer().get_voxel_num(pt);
		return _data_RB.get_voxels_read()[n];
	}

public:
	LightTracer &get_tracer() { return _scene._tracer; }
	const LightTracer &get_tracer() const { return _scene._tracer; }

private:
	struct RayBank_Data {
		LVector<RB_Voxel> &get_voxels_read() { return _voxels[map_read]; }
		LVector<RB_Voxel> &get_voxels_write() { return _voxels[map_write]; }
		LVector<RB_Voxel> _voxels[2];
		void swap();
		int map_read;
		int map_write;
	} _data_RB;

protected:
	bool hit_back_face(const Ray &r, int tri_id, const Vector3 &bary, Vector3 &face_normal) const {
		const Tri &triangle_normal = _scene._tri_normals[tri_id];
		triangle_normal.interpolate_barycentric(face_normal, bary);
		face_normal.normalize(); // is this necessary as we are just checking a dot product polarity?

		float dot = face_normal.dot(r.d);
		if (dot >= 0.0f) {
			return true;
		}

		return false;
	}

	void calculate_transmittance(const Color &albedo, FColor &ray_color) {
		// rapidly converge to the surface color
		float surf_fraction = albedo.a * 2.0f;
		if (surf_fraction > 1.0f)
			surf_fraction = 1.0f;

		FColor mixed_color;
		mixed_color.set(albedo);
		mixed_color = mixed_color * ray_color;

		ray_color.lerp(mixed_color, surf_fraction);

		// darken
		float dark_fraction = albedo.a;
		dark_fraction -= 0.5f;
		dark_fraction *= 2.0f;

		if (dark_fraction < 0.0f)
			dark_fraction = 0.0f;

		ray_color *= 1.0f - dark_fraction;
	}
};

} // namespace LM
