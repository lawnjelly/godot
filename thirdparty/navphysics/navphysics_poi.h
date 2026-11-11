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
	
	np_handle handle_mesh_instance;
	bool active = true;
	
	void init(np_handle p_handle) {
	}
};

class Obstacle
{
	struct Data
	{
		freal width = 1;
		freal height = 1;
	} data;
	
public:
	LocalPoint3 local_point;
	np_handle handle_mesh_instance;

#define NP_SET_GET(a, TYPE) TYPE get_##a() const {return data.a;}\
	void set_##a(TYPE p_value) {data.a = p_value;}
	
	NP_SET_GET(width, freal)
	NP_SET_GET(height, freal)
	
#undef NP_SET_GET	
	
	void init(np_handle p_handle) {
	}
};

} //namespace NavPhysics
