// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#pragma once

#include "navphysics_defines.h"
#include "navphysics_pointi.h"

namespace NavPhysics {

class POI {
public:
	u64 user_data = 0; // Allow storing e.g. a pointer on the POI.

	u32 poly_id = UINT32_MAX;
	u32 user_type = 0; // Allow the user to search by type.s

	freal height = 0;

	IPoint2 local_pos;

	FPoint3 local_pos3;
	bool active = true;
	
	void init(np_handle p_handle) {
	}
	
};

} //namespace NavPhysics
