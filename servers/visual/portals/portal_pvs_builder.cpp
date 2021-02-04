/*************************************************************************/
/*  portal_pvs_builder.cpp                                               */
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

#include "portal_pvs_builder.h"
#include "core/print_string.h"
#include "portal_renderer.h"

bool PVSBuilder::_log_active = true;

void PVSBuilder::find_neighbours(LocalVector<Neighbours> &r_neighbours) {

	// first find the neighbours
	int num_rooms = _portal_renderer->get_num_rooms();

	//	LocalVector<Neighbours> neighbours;
	//	neighbours.resize(num_rooms);

	for (int n = 0; n < num_rooms; n++) {
		const VSRoom &room = _portal_renderer->get_room(n);

		// go through each portal
		int num_portals = room._portal_ids.size();

		for (int p = 0; p < num_portals; p++) {
			int portal_id = room._portal_ids[p];
			const VSPortal &portal = _portal_renderer->get_portal(portal_id);

			// everything depends on whether the portal is incoming or outgoing.
			// if incoming we reverse the logic.
			int outgoing = 1;

			int room_a_id = portal._linkedroom_ID[0];
			if (room_a_id != n) {
				outgoing = 0;
				DEV_ASSERT(portal._linkedroom_ID[1] == n);
			}

			// trace through this portal to the next room
			int linked_room_id = portal._linkedroom_ID[outgoing];

			// not relevant, portal doesn't go anywhere
			if (linked_room_id == -1)
				continue;

			r_neighbours[n].room_ids.push_back(linked_room_id);
		} // for p through portals

	} // for n through rooms

	// the secondary PVS is the primary PVS plus the neighbours
}

void PVSBuilder::create_secondary_pvs(int p_room_id, const LocalVector<Neighbours> &p_neighbours, BitFieldDynamic &r_bitfield_rooms) {
	VSRoom &room = _portal_renderer->get_room(p_room_id);
	room._secondary_pvs_first = _pvs->get_secondary_pvs_size();

	// go through each primary PVS room, and add the neighbours in the secondary pvs
	for (int r = 0; r < room._pvs_size; r++) {

		int pvs_entry = room._pvs_first + r;
		int pvs_room_id = _pvs->get_pvs_room_id(pvs_entry);

		// add the visible rooms first
		_pvs->add_to_secondary_pvs(pvs_room_id);
		room._secondary_pvs_size += 1;

		// now any neighbours of this that are not already added
		const Neighbours &neigh = p_neighbours[pvs_room_id];
		for (int n = 0; n < neigh.room_ids.size(); n++) {
			int neigh_room_id = neigh.room_ids[n];

			//log("\tconsidering neigh " + itos(neigh_room_id));

			if (r_bitfield_rooms.check_and_set(neigh_room_id)) {
				// add to the secondary pvs for this room
				_pvs->add_to_secondary_pvs(neigh_room_id);
				room._secondary_pvs_size += 1;
			} // neighbour room has not been added yet
		} // go through the neighbours
	} // go through each room in the primary pvs
}

void PVSBuilder::calculate_pvs(PortalRenderer &p_portal_renderer) {
	_portal_renderer = &p_portal_renderer;
	_pvs = &p_portal_renderer.get_pvs();

	int num_rooms = _portal_renderer->get_num_rooms();
	BitFieldDynamic bf;
	bf.create(num_rooms);

	LocalVector<Neighbours> neighbours;
	neighbours.resize(num_rooms);

	// find the immediate neighbours of each room -
	// this is needed to create the secondary pvs
	find_neighbours(neighbours);

	for (int n = 0; n < num_rooms; n++) {
		bf.blank();

		//_visible_rooms.clear();

		LocalVector<Plane, int32_t> dummy_planes;

		VSRoom &room = _portal_renderer->get_room(n);
		room._pvs_first = _pvs->get_pvs_size();

		log("pvs from room : " + itos(n));

		trace_rooms_recursive(n, n, -1, false, -1, dummy_planes, bf);

		create_secondary_pvs(n, neighbours, bf);

		if (_log_active) {
			String sz = "";
			for (int i = 0; i < room._pvs_size; i++) {
				int visible_room = _pvs->get_pvs_room_id(room._pvs_first + i);
				sz += itos(visible_room);
				sz += ", ";
			}

			log("\t" + sz);

			sz = "secondary : ";
			for (int i = 0; i < room._secondary_pvs_size; i++) {
				int visible_room = _pvs->get_secondary_pvs_room_id(room._secondary_pvs_first + i);
				sz += itos(visible_room);
				sz += ", ";
			}

			log("\t" + sz);
		}
	}
}

void PVSBuilder::log(String sz) {
	if (_log_active) {
		print_line(sz);
	}
}

void PVSBuilder::trace_rooms_recursive(int p_source_room_id, int p_room_id, int p_first_portal_id, bool p_first_portal_outgoing, int p_previous_portal_id, const LocalVector<Plane, int32_t> &p_planes, BitFieldDynamic &r_bitfield_rooms) {
	// has this room been done already?
	if (!r_bitfield_rooms.check_and_set(p_room_id)) {
		return;
	}

	// get the room
	const VSRoom &room = _portal_renderer->get_room(p_room_id);

	// set this room to be in the PVS of the source room
	//	_visible_rooms.push_back(p_room_id);

	// add to the room PVS of the source room
	VSRoom &source_room = _portal_renderer->get_room(p_source_room_id);
	_pvs->add_to_pvs(p_room_id);
	source_room._pvs_size += 1;

	// go through each portal
	int num_portals = room._portal_ids.size();

	for (int p = 0; p < num_portals; p++) {
		int portal_id = room._portal_ids[p];
		const VSPortal &portal = _portal_renderer->get_portal(portal_id);

		// everything depends on whether the portal is incoming or outgoing.
		// if incoming we reverse the logic.
		int outgoing = 1;

		int room_a_id = portal._linkedroom_ID[0];
		if (room_a_id != p_room_id) {
			outgoing = 0;
			DEV_ASSERT(portal._linkedroom_ID[1] == p_room_id);
		}

		// trace through this portal to the next room
		int linked_room_id = portal._linkedroom_ID[outgoing];

		// not relevant, portal doesn't go anywhere
		if (linked_room_id == -1)
			continue;

		// linked room done already?
		if (r_bitfield_rooms.get_bit(linked_room_id))
			continue;

		// is it culled by the planes?
		VSPortal::eClipResult overall_res = VSPortal::eClipResult::CLIP_INSIDE;

		// while clipping to the planes we maintain a list of partial planes, so we can add them to the
		// recursive next iteration of planes to check
		static LocalVector<int> partial_planes;
		partial_planes.clear();

		for (int32_t l = 0; l < p_planes.size(); l++) {
			VSPortal::eClipResult res = portal.clip_with_plane(p_planes[l]);

			switch (res) {
				case VSPortal::eClipResult::CLIP_OUTSIDE: {
					overall_res = res;
				} break;
				case VSPortal::eClipResult::CLIP_PARTIAL: {
					// if the portal intersects one of the planes, we should take this plane into account
					// in the next call of this recursive trace, because it can be used to cull out more objects
					overall_res = res;
					partial_planes.push_back(l);
				} break;
				default: // suppress warning
					break;
			}

			// if the portal was totally outside the 'frustum' then we can ignore it
			if (overall_res == VSPortal::eClipResult::CLIP_OUTSIDE)
				break;
		}

		// this portal is culled
		if (overall_res == VSPortal::eClipResult::CLIP_OUTSIDE) {
			continue;
		}

		// construct new planes
		LocalVector<Plane, int32_t> planes;

		if (p_first_portal_id != -1) {
			// add new planes
			const VSPortal &first_portal = _portal_renderer->get_portal(p_first_portal_id);
			portal.add_pvs_planes(first_portal, p_first_portal_outgoing, planes, outgoing != 0);

//#define GODOT_PVS_EXTRA_REJECT_TEST
#ifdef GODOT_PVS_EXTRA_REJECT_TEST
			// extra reject test for pvs - was the previous portal points outside the planes formed by the new portal?
			// not fully tested and not yet found a situation where needed, but will leave in in case testers find
			// such a situation.
			if (p_previous_portal_id != -1) {
				const VSPortal &prev_portal = _portal_renderer->get_portal(p_previous_portal_id);
				if (prev_portal._pvs_is_outside_planes(planes)) {
					continue;
				}
			}
#endif
		}

		// if portal is totally inside the planes, don't copy the old planes ..
		// i.e. we can now cull using the portal and forget about the rest of the frustum (yay)
		if (overall_res != VSPortal::eClipResult::CLIP_INSIDE) {
			// if it WASNT totally inside the existing frustum, we also need to add any existing planes
			// that cut the portal.
			for (uint32_t n = 0; n < partial_planes.size(); n++)
				planes.push_back(p_planes[partial_planes[n]]);
		}

		// hopefully the portal actually leads somewhere...
		if (linked_room_id != -1) {

			// we either pass on the first portal id, or we start
			// it here, because we are looking through the first portal
			int first_portal_id = p_first_portal_id;
			if (first_portal_id == -1) {
				first_portal_id = portal_id;
				p_first_portal_outgoing = outgoing != 0;
			}

			trace_rooms_recursive(p_source_room_id, linked_room_id, first_portal_id, p_first_portal_outgoing, portal_id, planes, r_bitfield_rooms);
		} // linked room is valid
	}
}
