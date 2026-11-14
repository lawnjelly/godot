// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#include "navphysics_poi.h"
#include "navphysics_map.h"

namespace NavPhysics {

void Block::changed() {
	if (!data.handle_mesh_instance.is_valid()) {
		return;
	}
	MeshInstance *mi = NPWORLD.safe_get_mesh_instance(data.handle_mesh_instance);
	NP_ERR_FAIL_NULL(mi);
	
	mi->block_update(*this);
}

void Block::set_corners(Span<IPoint2> p_corners) {
	NP_DEV_ASSERT(p_corners.size() == 4);

	log("Setting corners to:");
	for (u32 n = 0; n < 4; n++) {
		data.corners[n] = p_corners[n];
		log(String("\t") + p_corners[n]);
	}

	// Find bound.
	data.bound.zero();
	data.bound.position = p_corners[0];
	for (u32 n = 1; n < 4; n++) {
		data.bound.expand_to_fast(p_corners[n]);
	}
	data.bound.increment_size();
}

} //namespace NavPhysics
