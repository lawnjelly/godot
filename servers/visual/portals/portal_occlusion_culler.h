/*************************************************************************/
/*  portal_occlusion_culler.h                                            */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef PORTAL_OCCLUSION_CULLER_H
#define PORTAL_OCCLUSION_CULLER_H

class PortalRenderer;
#include "portal_types.h"

class PortalOcclusionCuller {
	enum {
		MAX_SPHERES = 64,
	};

	struct Metaball : public Occlusion::Sphere {
		void set(const Occlusion::Sphere &p_sphere, real_t p_distance, real_t p_globbiness, uint32_t p_glob_group) {
			pos = p_sphere.pos;
			radius = p_sphere.radius;
			distance = p_distance;
			globbiness = p_globbiness;
			glob_group = p_glob_group;
		}
		// from camera, or culling source point
		real_t distance = 0.0;
		// 0 if no globbing (i.e. not a metaball)
		// this is basically the occluder ID + 1, or zero.
		uint32_t glob_group = 0;
		real_t globbiness = 0.0;
	};

	// metaball groups are groups of metaballs that form a glob
	// these can be quick rejected with a bounding sphere around the whole
	// group
	struct MetaballGroup {
		Occlusion::Sphere sphere;
		uint32_t first_metaball = 0;
		uint32_t num_metaballs = 0;
		real_t dist_to_center = 0.0;
		real_t dist_to_sphere_start = 0.0;
	};

public:
	PortalOcclusionCuller();
	void prepare(PortalRenderer &p_portal_renderer, const VSRoom &p_room, const Vector3 &pt_camera, const LocalVector<Plane> &p_planes, const Plane *p_near_plane) {
		if (p_near_plane) {
			static LocalVector<Plane> local_planes;
			int size_wanted = p_planes.size() + 1;

			if ((int)local_planes.size() != size_wanted) {
				local_planes.resize(size_wanted);
			}
			for (int n = 0; n < (int)p_planes.size(); n++) {
				local_planes[n] = p_planes[n];
			}
			local_planes[size_wanted - 1] = *p_near_plane;

			prepare_generic(p_portal_renderer, p_room._occluder_pool_ids, pt_camera, local_planes);
		} else {
			prepare_generic(p_portal_renderer, p_room._occluder_pool_ids, pt_camera, p_planes);
		}
	}

	void prepare_generic(PortalRenderer &p_portal_renderer, const LocalVector<uint32_t, uint32_t> &p_occluder_pool_ids, const Vector3 &pt_camera, const LocalVector<Plane> &p_planes);
	bool cull_aabb(const AABB &p_aabb) const {
		if (!_num_spheres) {
			return false;
		}

		return cull_sphere(p_aabb.get_center(), p_aabb.size.length() * 0.5);
	}
	bool cull_sphere(const Vector3 &p_occludee_center, real_t p_occludee_radius, int p_ignore_sphere = -1) const;

private:
	bool _cull_sphere_metaball_groups(const Vector3 &p_occludee_center, real_t p_occludee_radius, const Vector3 &p_ray_dir, real_t p_dist_to_occludee) const;
	bool _cull_sphere_metaball_group(const MetaballGroup &p_group, const Vector3 &p_occludee_center, real_t p_occludee_radius, const Vector3 &p_ray_dir, real_t p_dist_to_occludee) const;

	bool _cull_sphere_metaballs(const Vector3 &p_occludee_center, real_t p_occludee_radius, const Vector3 &p_ray_dir, real_t p_dist_to_occludee) const;
	real_t _evaluate_metaball_group_SDF(const MetaballGroup &p_group, const Vector3 &p_pt) const;
	real_t _evaluate_metaball_SDF(const Vector3 &p_pt) const;
	real_t _SDF_smooth_union(real_t d1, real_t d2, real_t k) const;
	void _create_metaball_group(uint32_t p_first_metaball, uint32_t p_num_metaballs);

	// if a sphere is entirely in front of any of the culling planes, it can't be seen so returns false
	bool is_sphere_culled(const Vector3 &p_pos, real_t p_radius, const LocalVector<Plane> &p_planes) const {
		for (unsigned int p = 0; p < p_planes.size(); p++) {
			real_t dist = p_planes[p].distance_to(p_pos);
			if (dist > p_radius) {
				return true;
			}
		}

		return false;
	}

	bool is_aabb_culled(const AABB &p_aabb, const LocalVector<Plane> &p_planes) const {
		const Vector3 &size = p_aabb.size;
		Vector3 half_extents = size * 0.5;
		Vector3 ofs = p_aabb.position + half_extents;

		for (unsigned int i = 0; i < p_planes.size(); i++) {
			const Plane &p = p_planes[i];
			Vector3 point(
					(p.normal.x > 0) ? -half_extents.x : half_extents.x,
					(p.normal.y > 0) ? -half_extents.y : half_extents.y,
					(p.normal.z > 0) ? -half_extents.z : half_extents.z);
			point += ofs;
			if (p.is_point_over(point)) {
				return true;
			}
		}

		return false;
	}

	// only a number of the spheres in the scene will be chosen to be
	// active based on their distance to the camera, screen space etc.
	Metaball _spheres[MAX_SPHERES];
	real_t _sphere_closest_dist = 0.0;
	int _num_spheres = 0;
	int _max_spheres = 8;

	Metaball _metaballs[MAX_SPHERES];
	int _num_metaballs = 0;

	MetaballGroup _metaball_groups[MAX_SPHERES];
	int _num_metaball_groups = 0;

	Vector3 _pt_camera;
};

inline real_t PortalOcclusionCuller::_evaluate_metaball_group_SDF(const MetaballGroup &p_group, const Vector3 &p_pt) const {
	real_t res = FLT_MAX;
	uint32_t last_ball = p_group.first_metaball + p_group.num_metaballs;

	for (int s = p_group.first_metaball; s < last_ball; s++) {
		const Metaball &ball = _metaballs[s];

		Vector3 offset = p_pt - ball.pos;
		real_t d = offset.length();
		d -= ball.radius;
		res = _SDF_smooth_union(res, d, ball.globbiness);
	}

	return res;
}

inline real_t PortalOcclusionCuller::_evaluate_metaball_SDF(const Vector3 &p_pt) const {
	real_t res = FLT_MAX;
	real_t overall_res = FLT_MAX;
	uint32_t current_group = 0;

	for (int s = 0; s < _num_metaballs; s++) {
		const Metaball &ball = _metaballs[s];

		// new glob group?
		if (ball.glob_group != current_group) {
			overall_res = MIN(overall_res, res);
			current_group = ball.glob_group;
			res = FLT_MAX;
		}

		Vector3 offset = p_pt - ball.pos;
		real_t d = offset.length();

		d -= ball.radius;

		// res = _SDF_smooth_union(res, d, 10.0);
		res = _SDF_smooth_union(res, d, ball.globbiness);
	}

	// final
	overall_res = MIN(overall_res, res);

	return overall_res;
}

inline real_t PortalOcclusionCuller::_SDF_smooth_union(real_t d1, real_t d2, real_t k) const {
	real_t h = CLAMP(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0);
	real_t mix = d2 + ((d1 - d2) * h);
	return mix - (k * h * (1.0 - h));
}

#endif // PORTAL_OCCLUSION_CULLER_H
