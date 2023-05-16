// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#pragma once

#include "navphysics_pointi.h"
#include "navphysics_vector.h"

namespace NavPhysics {

class Mesh;

class BSP {
	struct BuildNode {
		u32 wall_id = UINT32_MAX;
		Vector<u32> poly_ids;

		u32 child[2] = {};
	};

	struct Node {
		u32 wall_id = UINT32_MAX;

		// Positive is a child, negative is a leaf
		i32 child[2];
	};

	struct Leaf {
		u32 first_poly = 0;
		u32 num_polys = 0;
	};

	static Vector<BuildNode> build_nodes;
	Vector<Node> nodes;
	Vector<Leaf> leaves;
	Vector<u32> leaf_poly_ids;

public:
	void build(const Mesh &p_mesh);
	void clear();
	const u32 *find_leaf(IPoint2 &p_pt, u32 &r_num_polys) const;
};

} //namespace NavPhysics
