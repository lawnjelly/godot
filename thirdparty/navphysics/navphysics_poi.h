// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#pragma once

#include "navphysics_defines.h"
#include "navphysics_pointi.h"
#include "navphysics_rect.h"

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

class MeshInstance;

class Block {
	friend class MeshInstance;
	friend class Mesh;

	struct Data {
		freal width = 1;
		freal height = 1;
		freal yaw = 0;
		IRect2 bound;
		IPoint2 corners[4];
		LocalPoint3 local_point;
		np_handle handle_mesh_instance;
		np_handle handle_block;
		bool active = true;
	} data;

	void changed();
	void set_corners(Span<IPoint2> p_corners);

public:
#define NP_SET_GET(a, TYPE)                 \
	TYPE get_##a() const { return data.a; } \
	void set_##a(TYPE p_value) {            \
		data.a = p_value;                   \
		changed();                          \
	}

	NP_SET_GET(width, freal)
	NP_SET_GET(height, freal)

#undef NP_SET_GET

	void set_active(bool p_enable) {
		data.active = p_enable;
	}

	void init(np_handle p_handle) {
		data.handle_block = p_handle;
	}
};

} //namespace NavPhysics
