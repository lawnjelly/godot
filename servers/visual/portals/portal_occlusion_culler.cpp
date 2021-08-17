/*************************************************************************/
/*  portal_occlusion_culler.cpp                                          */
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

#include "portal_occlusion_culler.h"

#include "core/engine.h"
#include "core/math/aabb.h"
#include "core/project_settings.h"
#include "portal_renderer.h"

void PortalOcclusionCuller::prepare_generic(PortalRenderer &p_portal_renderer, const LocalVector<uint32_t, uint32_t> &p_occluder_pool_ids, const Vector3 &pt_camera, const LocalVector<Plane> &p_planes) {
	// bodge to keep settings up to date
#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint() && ((Engine::get_singleton()->get_frames_drawn() % 16) == 0)) {
		_max_polys = GLOBAL_GET("rendering/misc/occlusion_culling/max_active_polys");
	}
#endif

	_pt_camera = pt_camera;

	// spheres
	_num_spheres = 0;
	real_t goodness_of_fit_sphere[MAX_SPHERES];
	for (int n = 0; n < _max_spheres; n++) {
		goodness_of_fit_sphere[n] = 0.0;
	}
	real_t weakest_fit_sphere = FLT_MAX;
	int weakest_sphere = 0;
	_sphere_closest_dist = FLT_MAX;

	// polys
	_num_polys = 0;
	for (int n = 0; n < _max_polys; n++) {
		_polys[n].goodness_of_fit = 0.0;
	}
	real_t weakest_fit_poly = 0.0;
	int weakest_poly_id = 0;

	// find occluders
	for (unsigned int o = 0; o < p_occluder_pool_ids.size(); o++) {
		int id = p_occluder_pool_ids[o];
		VSOccluder &occ = p_portal_renderer.get_pool_occluder(id);

		// is it active?
		// in the case of rooms, they will always be active, as inactive
		// are removed from rooms. But for whole scene mode, some may be inactive.
		if (!occ.active) {
			continue;
		}

		// TODO : occlusion cull spheres AGAINST themselves.
		// i.e. a sphere that is occluded by another occluder is no
		// use as an occluder...
		if (occ.type == VSOccluder::OT_SPHERE) {
			// make sure world space spheres are up to date
			p_portal_renderer.occluder_ensure_up_to_date_sphere(occ);

			// multiple spheres
			for (int n = 0; n < occ.list_ids.size(); n++) {
				const Occlusion::Sphere &occluder_sphere = p_portal_renderer.get_pool_occluder_sphere(occ.list_ids[n]).world;

				// is the occluder sphere culled?
				if (is_sphere_culled(occluder_sphere.pos, occluder_sphere.radius, p_planes)) {
					continue;
				}

				real_t dist = (occluder_sphere.pos - pt_camera).length();

				// keep a record of the closest sphere for quick rejects
				if (dist < _sphere_closest_dist) {
					_sphere_closest_dist = dist;
				}

				// calculate the goodness of fit .. smaller distance better, and larger radius
				// calculate adjusted radius at 100.0
				real_t fit = 100 / MAX(dist, 0.01);
				fit *= occluder_sphere.radius;

				// until we reach the max, just keep recording, and keep track
				// of the worst fit
				if (_num_spheres < _max_spheres) {
					_spheres[_num_spheres] = occluder_sphere;
					_sphere_distances[_num_spheres] = dist;
					goodness_of_fit_sphere[_num_spheres] = fit;

					if (fit < weakest_fit_sphere) {
						weakest_fit_sphere = fit;
						weakest_sphere = _num_spheres;
					}

					_num_spheres++;
				} else {
					// must beat the weakest
					if (fit > weakest_fit_sphere) {
						_spheres[weakest_sphere] = occluder_sphere;
						_sphere_distances[weakest_sphere] = dist;
						goodness_of_fit_sphere[weakest_sphere] = fit;

						// the weakest may have changed (this could be done more efficiently)
						weakest_fit_sphere = FLT_MAX;
						for (int s = 0; s < _max_spheres; s++) {
							if (goodness_of_fit_sphere[s] < weakest_fit_sphere) {
								weakest_fit_sphere = goodness_of_fit_sphere[s];
								weakest_sphere = s;
							}
						}
					}
				}
			}
		} // sphere

		if (occ.type == VSOccluder::OT_POLY) {
			// multiple spheres
			for (int n = 0; n < occ.list_ids.size(); n++) {
				const VSOccluder_Poly &opoly = p_portal_renderer.get_pool_occluder_poly(occ.list_ids[n]);
				const Occlusion::Poly &poly = opoly.poly;

				bool debug = false;
				if ((Engine::get_singleton()->get_frames_drawn() % 8) == 0) {
					if (n == 0) {
						debug = true;
					}
				}

				real_t fit;
				if (!calculate_poly_goodness_of_fit(debug, opoly, fit)) {
					continue;
				}

				//				if (debug) {
				//					print_line("fit 0 : " + rtos(1.0 / fit));
				//				}

				/*				
				if (is_poly_culled(opoly, p_planes)) {
					continue;
				}
*/
				// backface cull
				bool faces_camera = poly.plane.is_point_over(pt_camera);

				/*
				Vector3 offset = pt_camera - opoly.center;
				real_t dist = offset.length();

				// prevent divide by zero
				if (dist == 0.0) {
					continue;
				}

				// goodness of fit
				// depends on distance
				real_t fit = dist;
				// area
				real_t area_effect = fit / (opoly.area * 0.1);
				fit += area_effect;

				// normalize
				offset *= 1.0 / dist;
				real_t dot = offset.dot(poly.plane.normal);

				// 1 is a good match, 0 is no good match
				dot = 1.0 - dot;

				// add a bias depending on the dot product to the goodness of fit
				fit += (fit * dot);
*/
				if (_num_polys < _max_polys) {
					SortPoly &dest = _polys[_num_polys];
					dest.poly = poly;
					dest.faces_camera = faces_camera;
					dest.poly_source_id = n;
					dest.goodness_of_fit = fit;

					if (fit > weakest_fit_poly) {
						weakest_fit_poly = fit;
						weakest_poly_id = _num_polys;
					}

					_num_polys++;
				} else {
					// must beat the weakest
					if (fit < weakest_fit_poly) {
						SortPoly &dest = _polys[weakest_poly_id];
						dest.poly = poly;
						dest.faces_camera = faces_camera;
						dest.poly_source_id = n;
						dest.goodness_of_fit = fit;

						// the weakest may have changed (this could be done more efficiently)
						weakest_fit_poly = 0.0;
						for (int p = 0; p < _max_polys; p++) {
							real_t goodness_of_fit = _polys[p].goodness_of_fit;

							if (goodness_of_fit > weakest_fit_poly) {
								weakest_fit_poly = goodness_of_fit;
								weakest_poly_id = p;
							}
						}
					}

				} // polys full up, replace
			}
		}
	} // for o

	// flip polys so always facing camera
	//uint32_t last_checksum = _poly_checksum;
	//_poly_checksum = 0;
	for (int n = 0; n < _num_polys; n++) {
		if (!_polys[n].faces_camera) {
			_polys[n].poly.flip();
		}
		//_poly_checksum += _poly_source_id[n];
	}
	//	if (_poly_checksum != last_checksum) {
	//		// draw the active polys in the editor?
	//	}

	//	if ((Engine::get_singleton()->get_frames_drawn() % 8) == 0) {
	//		for (int n = 0; n < _num_polys; n++) {
	//			print_line(itos(n) + "\tfit : " + rtos(goodness_of_fit_poly[n]) + ", source : " + itos(_poly_source_id[n]) + ", facescam : " + String(Variant(_poly_faces_camera[n])) + ", plane : " + String(Variant(_polys[n].plane)));
	//		}
	//	}

	// force the sphere closest distance to above zero to prevent
	// divide by zero in the quick reject
	_sphere_closest_dist = MAX(_sphere_closest_dist, 0.001);

	// record whether to do any occlusion culling at all..
	_occluders_present = _num_spheres || _num_polys;
}

bool PortalOcclusionCuller::calculate_poly_goodness_of_fit(bool debug, const VSOccluder_Poly &p_opoly, real_t &r_fit) const {
	// transform each of the poly points, find the area in screen space
	Occlusion::Poly xpoly;
	int num_verts = p_opoly.poly.num_verts;

	//Transform tr = _matrix_camera.operator Transform();
	//tr.affine_invert();
	//Vector3 euler = tr.basis.get_rotation_euler();

	int off_left = 0;
	int off_right = 0;
	int off_top = 0;
	int off_bottom = 0;
	int off_near = 0;

	for (int n = 0; n < num_verts; n++) {
		Vector3 &dest = xpoly.verts[n];
		dest = _matrix_camera.xform(p_opoly.poly.verts[n]);

		if (debug) {
			print_line(itos(n) + " : " + String(Variant(dest)));
		}

		if (dest.x <= -1.0) {
			off_left++;
		} else if (dest.x >= 1.0) {
			off_right++;
		}
		if (dest.y <= -1.0) {
			off_bottom++;
		} else if (dest.y >= 1.0) {
			off_top++;
		}
		if (dest.z > 1.0) {
			off_near++;
		}

		// make 2d
		dest.z = 0.0;
	}

	if (off_left == num_verts)
		return false;
	if (off_right == num_verts)
		return false;
	if (off_top == num_verts)
		return false;
	if (off_bottom == num_verts)
		return false;
	if (off_near == num_verts)
		return false;

	// find screen space area
	real_t area = Geometry::find_polygon_area(&xpoly.verts[0], num_verts);

	if (debug)
		print_line("Area : " + rtos(area));
	//print_line("Area : " + rtos(area) + " euler : " + String(Variant(euler)));

	if (area < 0.01) {
		return false;
	}

	r_fit = 1.0 / area;
	return true;
}

bool PortalOcclusionCuller::cull_aabb_to_polys(const AABB &p_aabb) const {
	if (!_num_polys) {
		return false;
	}

	Plane plane;

	for (int n = 0; n < _num_polys; n++) {
		const Occlusion::Poly &poly = _polys[n].poly;

		// test against each edge of the poly, and expand the edge
		bool hit = true;

		// must handle polys in both directions, so the winding order is reversed depending whether it faces the camera
		//bool faces_camera = _poly_faces_camera[n];

		// occludee must be on opposite side to camera
		real_t omin, omax;
		p_aabb.project_range_in_plane(poly.plane, omin, omax);

		//		if (!faces_camera) {
		//			if (omin < 0.0) {
		//				continue;
		//			}
		//		} else {
		if (omax > -0.2) {
			continue;
		}
		//		}

		for (int e = 0; e < poly.num_verts; e++) {
			//if (faces_camera) {
			plane = Plane(_pt_camera, poly.verts[e], poly.verts[(e + 1) % poly.num_verts]);
			//			} else {
			//				plane = Plane(_pt_camera, poly.verts[(e + 1) % poly.num_verts], poly.verts[e]);
			//			}

			p_aabb.project_range_in_plane(plane, omin, omax);

			if (omax > 0.0) {
				hit = false;
				break;
			}
		}

		// hit?
		if (hit) {
			return true;
		}
	}

	return false;
}

bool PortalOcclusionCuller::cull_sphere_to_polys(const Vector3 &p_occludee_center, real_t p_occludee_radius) const {
	if (!_num_polys) {
		return false;
	}

	Plane plane;

	for (int n = 0; n < _num_polys; n++) {
		const Occlusion::Poly &poly = _polys[n].poly;

		// test against each edge of the poly, and expand the edge
		bool hit = true;

		// must handle polys in both directions, so the winding order is reversed depending whether it faces the camera
		//bool faces_camera = _poly_faces_camera[n];

		// occludee must be on opposite side to camera
		real_t dist = poly.plane.distance_to(p_occludee_center);

		//		if (!faces_camera) {
		//			dist = -dist;
		//		}

		if (dist > -p_occludee_radius) {
			continue;
		}

		for (int e = 0; e < poly.num_verts; e++) {
			//if (faces_camera) {
			plane = Plane(_pt_camera, poly.verts[e], poly.verts[(e + 1) % poly.num_verts]);
			//			} else {
			//				plane = Plane(_pt_camera, poly.verts[(e + 1) % poly.num_verts], poly.verts[e]);
			//			}

			// de-expand
			plane.d -= p_occludee_radius;

			if (plane.is_point_over(p_occludee_center)) {
				hit = false;
				break;
			}
		}

		// hit?
		if (hit) {
			return true;
		}
	}

	return false;
}

bool PortalOcclusionCuller::cull_sphere_to_spheres(const Vector3 &p_occludee_center, real_t p_occludee_radius, const Vector3 &p_ray_dir, real_t p_dist_to_occludee) const {
	// maybe not required
	if (!_num_spheres) {
		return false;
	}

	// prevent divide by zero, and the occludee cannot be occluded if we are WITHIN
	// its bounding sphere... so no need to check
	if (p_dist_to_occludee < _sphere_closest_dist) {
		return false;
	}

	// this can probably be done cheaper with dot products but the math might be a bit fiddly to get right
	for (int s = 0; s < _num_spheres; s++) {
		//  first get the sphere distance
		real_t occluder_dist_to_cam = _sphere_distances[s];
		if (p_dist_to_occludee < occluder_dist_to_cam) {
			// can't occlude
			continue;
		}

		// the perspective adjusted occludee radius
		real_t adjusted_occludee_radius = p_occludee_radius * (occluder_dist_to_cam / p_dist_to_occludee);

		const Occlusion::Sphere &occluder_sphere = _spheres[s];
		real_t occluder_radius = occluder_sphere.radius - adjusted_occludee_radius;

		if (occluder_radius > 0.0) {
			occluder_radius = occluder_radius * occluder_radius;

			// distance to hit
			real_t dist;

			if (occluder_sphere.intersect_ray(_pt_camera, p_ray_dir, dist, occluder_radius)) {
				if (dist < p_dist_to_occludee) {
					// occluded
					return true;
				}
			}
		} // expanded occluder radius is more than 0
	}

	return false;
}

bool PortalOcclusionCuller::cull_sphere(const Vector3 &p_occludee_center, real_t p_occludee_radius, bool p_cull_to_polys) const {
	if (!_occluders_present) {
		return false;
	}

	// ray from origin to the occludee
	Vector3 ray_dir = p_occludee_center - _pt_camera;
	real_t dist_to_occludee_raw = ray_dir.length();

	// account for occludee radius
	real_t dist_to_occludee = dist_to_occludee_raw - p_occludee_radius;

	// ignore occlusion for closeup, and avoid divide by zero
	if (dist_to_occludee_raw < 0.1) {
		return false;
	}

	// normalize ray
	// hopefully by this point, dist_to_occludee_raw cannot possibly be zero due to above check
	ray_dir *= 1.0 / dist_to_occludee_raw;

	if (cull_sphere_to_spheres(p_occludee_center, p_occludee_radius, ray_dir, dist_to_occludee)) {
		return true;
	}

	if (p_cull_to_polys && cull_sphere_to_polys(p_occludee_center, p_occludee_radius)) {
		return true;
	}

	return false;
}

PortalOcclusionCuller::PortalOcclusionCuller() {
	_max_spheres = GLOBAL_GET("rendering/misc/occlusion_culling/max_active_spheres");
	_max_polys = GLOBAL_GET("rendering/misc/occlusion_culling/max_active_polys");
}
