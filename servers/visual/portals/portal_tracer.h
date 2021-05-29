/*************************************************************************/
/*  portal_tracer.h                                                      */
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

#ifndef PORTAL_TRACER_H
#define PORTAL_TRACER_H

#include "core/bitfield_dynamic.h"
#include "core/local_vector.h"
#include "portal_types.h"

#ifdef TOOLS_ENABLED
// use this for checking for instance lifetime errors, disable normally
//#define PORTAL_RENDERER_STORE_MOVING_RIDS
#endif

class PortalRenderer;
struct VSRoom;

class PortalTracer {
public:
	// a bitfield for which statics have been hit this time,
	// and a list of showing statics
	class TraceResult {
	public:
		void create(int p_num_statics) {
			bf_visible_statics.create(p_num_statics);
#ifdef GODOT_PORTALS_USE_SCISSOR
			static_xportal_ids.resize(p_num_statics);
			for (int n = 0; n < p_num_statics; n++) {
				static_xportal_ids[n] = 0;
			}
#endif
		}
		void clear() {
			bf_visible_statics.blank();
			visible_static_ids.clear();
			visible_roamer_pool_ids.clear();
#ifdef GODOT_PORTALS_USE_SCISSOR
			//xportal_rects.clear();
			// add one dummy
			xportal_rects.resize(1);
			//xportal_rects[0].set_inactive();
#endif
		}

		BitFieldDynamic bf_visible_statics;
		LocalVector<uint32_t> visible_static_ids;
		LocalVector<uint32_t> visible_roamer_pool_ids;

#ifdef GODOT_PORTALS_USE_SCISSOR
		// the transformed portal that a static is seen through or 0 for dummy xportal
		// if not seen through a portal, or seen through more than 1 portal
		LocalVector<uint16_t> static_xportal_ids;
		LocalVector<Rect2i> xportal_rects;
#endif
	};

	struct TraceParams {
		// optional camera transform to get portal into screen space
		const CameraMatrix *xform;
		bool use_pvs;
		uint8_t *decompressed_room_pvs;
	};

	// The recursive visibility function needs to allocate lists of planes each time a room is traversed.
	// Instead of doing this allocation on the fly we will use a pool which should be much faster and nearer
	// constant time.

	// Note this simple pool isn't super optimal but should be fine for now.
	class PlanesPool {
	public:
		// maximum number of vectors in the pool
		const static int POOL_MAX = 32;

		void reset();

		// request a new vector of planes .. returns the pool id, or -1 if pool is empty
		unsigned int request();

		// return pool id to the pool
		void free(unsigned int ui);

		LocalVector<Plane> &get(unsigned int ui) { return _planes[ui]; }

		PlanesPool();

	private:
		LocalVector<Plane> _planes[POOL_MAX];

		// list of pool ids that are free and can be allocated
		uint8_t _freelist[POOL_MAX];
		uint32_t _num_free;
	};

	// for debugging, instead of doing a normal trace, show the objects that are sprawled from the current room
	void trace_debug_sprawl(PortalRenderer &p_portal_renderer, const Vector3 &p_pos, int p_start_room_id, TraceResult &r_result);

	// trace statics, dynamics and roaming
	void trace(PortalRenderer &p_portal_renderer, const Vector3 &p_pos, const CameraMatrix *p_xform, const LocalVector<Plane> &p_planes, int p_start_room_id, TraceResult &r_result);

	// globals are handled separately as they don't care about the rooms
	int trace_globals(const LocalVector<Plane> &p_planes, VSInstance **p_result_array, int first_result, int p_result_max, uint32_t p_mask);

private:
	// main tracing function is recursive
#ifdef GODOT_PORTALS_USE_SCISSOR
	void trace_recursive(const TraceParams &p_params, int p_depth, int p_room_id, const LocalVector<Plane> &p_planes, uint32_t p_xportal_id = 0);
#else
	void trace_recursive(const TraceParams &p_params, int p_depth, int p_room_id, const LocalVector<Plane> &p_planes);
#endif

	// use pvs to cull instead of dynamically using portals
	// this is a faster trace but less accurate. Only possible if PVS has been generated.
	void trace_pvs(int p_source_room_id, const LocalVector<Plane> &p_planes);

	// debug version
	void trace_debug_sprawl_recursive(int p_depth, int p_room_id);

#ifdef GODOT_PORTALS_USE_SCISSOR
	void cull_statics(const VSRoom &p_room, const LocalVector<Plane> &p_planes, uint32_t p_xportal_id);
	uint32_t create_xportal(const VSPortal &p_portal, const CameraMatrix *p_xform, uint32_t p_parent_xportal_id);
#else
	void cull_statics(const VSRoom &p_room, const LocalVector<Plane> &p_planes);
#endif
	void cull_statics_debug_sprawl(const VSRoom &p_room);
	void cull_roamers(const VSRoom &p_room, const LocalVector<Plane> &p_planes);

	// if an aabb is in front of any of the culling planes, it can't be seen so returns false
	bool test_cull_inside(const AABB &p_aabb, const LocalVector<Plane> &p_planes, bool p_test_explicit_near_plane = true) const {
		for (unsigned int p = 0; p < p_planes.size(); p++) {
			real_t r_min, r_max;
			p_aabb.project_range_in_plane(p_planes[p], r_min, r_max);

			if (r_min > 0.0) {
				return false;
			}
		}

		if (p_test_explicit_near_plane) {
			real_t r_min, r_max;
			p_aabb.project_range_in_plane(_near_and_far_planes[0], r_min, r_max);

			if (r_min > 0.0) {
				return false;
			}
		}

		return true;
	}

	// local versions to prevent passing around the recursive functions
	PortalRenderer *_portal_renderer = nullptr;
	Vector3 _trace_start_point;
	TraceResult *_result = nullptr;
	Plane _near_and_far_planes[2];

	PlanesPool _planes_pool;

	// keep a tick count for each trace, to avoid adding a visible
	// object to the hit list more than once per tick
	// (this makes more sense than bitfield for moving objects)
	uint32_t _tick = 0;
};

#endif
