// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#pragma once

//#include "navphysics_structs.h"
#include "navphysics_pointi.h"
#include "navphysics_rect.h"
#include "navphysics_vector.h"

namespace NavPhysics {

class Mesh;

class PolyFinder {
	TVector<u32> _poly_ids;
	IRect2 _boundary;
	u32 _cell_size = 0;
	//IPoint2 _boundary_offset;
	u32 _width_cells = 0;
	u32 _height_cells = 0;

	struct Cell {
		u32 first_id = 0;
		u32 num_ids = 0;
	};

	struct BuildCell {
		TVector<u32> poly_ids;
	};

	TVector<Cell> _cells;
	Vector<BuildCell> _build_cells;

	void clear() {
		_poly_ids.clear();
		_boundary.zero();
		_cell_size = 0;
		_width_cells = 0;
		_height_cells = 0;
		_cells.clear();
		_build_cells.clear();
	}

	Cell &get_cell(u32 p_x, u32 p_y) {
		u32 which = (p_y * _width_cells) + p_x;
		return _cells[which];
	}
	const Cell &get_cell(u32 p_x, u32 p_y) const {
		u32 which = (p_y * _width_cells) + p_x;
		return _cells[which];
	}

public:
	void build(const Mesh &p_mesh, bool p_ceiling);
	//void build(const FPoint3 *p_verts, uint32_t p_num_verts, uint32_t * p_indices, uint32_t p_num_tris);

	// Find the leaf for a single point.
	const u32 *find_leaf(const IPoint2 &p_pt, u32 &r_num_polys) const;

	struct CellResult {
		const u32 *poly_ids = nullptr;
		u32 num_polys = 0;
	};

	// Find a list of cells that cover a rect, used for finding jump polys.
	void find_cells(const IRect2 &p_rect, Vector<CellResult> &r_cells);
};

} //namespace NavPhysics
