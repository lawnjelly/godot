// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#pragma once

#include "navphysics_defines.h"
#include "navphysics_pointi.h"


namespace NavPhysics {

// Can be baked.
class POI
{
public:
	FPoint3 local_pos3;
	IPoint2 local_pos;
	freal height = 0;
	u32 poly_id = UINT32_MAX;
};

class POI_Instance : public POI
{
public:
	bool active = true;
	
	// Allow the user to search by type.
	u32 user_type = 0;
	
	// Allow storing e.g. a pointer on the POI.
	u64 user_data = 0;
};


} //namespace NavPhysics
