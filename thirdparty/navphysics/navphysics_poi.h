// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#pragma once

#include "navphysics_defines.h"
#include "navphysics_pointi.h"

namespace NavPhysics {

class POI {
public:
	LocalPoint3 local_point;
	
	u32 user_type = 0; // Allow the user to search by type.s
	u64 user_data = 0; // Allow storing e.g. a pointer on the POI.
	
	np_handle handle_mesh_instance = UINT32_MAX;
	bool active = true;
	
	void init(np_handle p_handle) {
	}
};

class Obstacle
{
public:
	LocalPoint3 local_point;
	np_handle handle_mesh_instance = UINT32_MAX;
	
	void init(np_handle p_handle) {
	}
};

} //namespace NavPhysics
