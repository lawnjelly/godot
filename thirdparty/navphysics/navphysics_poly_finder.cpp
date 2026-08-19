// Copyright 2026-present Lawnjelly
// SPDX-License-Identifier: MIT

#include "navphysics_poly_finder.h"
#include "navphysics_mesh.h"

namespace NavPhysics {

#if 0
void PolyFinder::build(const FPoint3 *p_verts, uint32_t p_num_verts, uint32_t * p_indices, uint32_t p_num_tris)
{
	clear();
	NP_ERR_FAIL_NULL(p_verts);
	NP_ERR_FAIL_NULL(p_indices);
	NP_ERR_FAIL_COND(!p_num_verts);
	NP_ERR_FAIL_COND(!p_num_tris);
	
	// Seed bounds
	Vector<IRect2> bounds;
	bounds.resize(p_num_tris);
	
	_boundary.position = p_verts[0];
	_boundary.size.zero();
	
	for (u32 n=0; n<p_num_tris; n++)
	{
		IRect2 &bound = bounds[n];
		bound.position = p_verts[p_indices[n * 3]];
		bound.expand_to_fast(p_verts[p_indices[(n * 3)+1]];
		bound.expand_to_fast(p_verts[p_indices[(n * 3)+2]];
		bound.increment_size();
	}
	
	
}
#endif

void PolyFinder::build(const Mesh &p_mesh, bool p_ceiling) {
	clear();

	const Mesh::SubMesh *sm = p_ceiling ? &p_mesh.ceiling : &p_mesh.floor;

	// First find boundary in integer space
	u32 count = sm->get_num_polys();

	if (!count) {
		return;
	}

	_boundary = sm->poly_bounds[0];

	for (u32 n = 1; n < count; n++) {
		_boundary.expand_to(sm->poly_bounds[n]);
	}

	u64 total_area = (u64)_boundary.size.x * (u64)_boundary.size.y;

	// The separation should depend on the poly count, for efficiency.
	u32 tiles = MAX(1, count / 8);
	u64 area_per_tile = MAX(1, total_area / tiles);

	_cell_size = MAX(1, ::sqrt((f64)area_per_tile));
	_width_cells = (_boundary.size.x / _cell_size) + 1;
	_height_cells = (_boundary.size.y / _cell_size) + 1;

	_cells.resize(_width_cells * _height_cells);
	_build_cells.resize(_width_cells * _height_cells);

	// Place each poly into appropriate cells.
	// (This should be faster than going by cell.)
	for (u32 p = 0; p < count; p++) {
		IRect2 poly_bound = sm->poly_bounds[p];

		NP_DEV_ASSERT(poly_bound.size.x >= 0);
		NP_DEV_ASSERT(poly_bound.size.y >= 0);

		// Polys with zero size should not be found anyway..
		if (poly_bound.get_area() == 0) {
			log("NavPoly with zero bound detected.");
			continue;
		}

		// convert to cell coords.
		IPoint2 pbegin = poly_bound.position;
		NP_DEV_ASSERT(pbegin.x >= _boundary.position.x);
		NP_DEV_ASSERT(pbegin.y >= _boundary.position.y);

		pbegin -= _boundary.position;

		IPoint2 pend = pbegin + poly_bound.size;

		NP_DEV_ASSERT(pend.x <= _boundary.size.x);
		NP_DEV_ASSERT(pend.y <= _boundary.size.y);

		pbegin.x /= _cell_size;
		pbegin.y /= _cell_size;

		i32 mod_x = pend.x % _cell_size;
		i32 mod_y = pend.y % _cell_size;
		pend.x /= _cell_size;
		pend.y /= _cell_size;

		NP_DEV_ASSERT(pbegin.x >= 0);
		NP_DEV_ASSERT(pbegin.y >= 0);
		NP_DEV_ASSERT(pend.x < _width_cells);
		NP_DEV_ASSERT(pend.y < _height_cells);

		if (mod_x) {
			pend.x += 1;
		}
		if (mod_y) {
			pend.y += 1;
		}

		IPoint2 psize = pend - pbegin;
		NP_DEV_ASSERT(psize.x >= 0);
		NP_DEV_ASSERT(psize.y >= 0);

		for (i32 ty = pbegin.y; ty < pend.y; ty++) {
			for (i32 tx = pbegin.x; tx < pend.x; tx++) {
				u32 which = (ty * _width_cells) + tx;
				BuildCell &cell = _build_cells[which];
				cell.poly_ids.push_back(p);
			}
		}
	}

	// Convert to final cells.
	for (u32 y = 0; y < _height_cells; y++) {
		for (u32 x = 0; x < _width_cells; x++) {
			Cell &cell = get_cell(x, y);

			u32 which = (y * _width_cells) + x;
			const BuildCell &build_cell = _build_cells[which];

			cell.first_id = _poly_ids.size();
			cell.num_ids = build_cell.poly_ids.size();

			_poly_ids.resize(cell.first_id + cell.num_ids);

			// Fill
			for (u32 n = 0; n < cell.num_ids; n++) {
				_poly_ids[cell.first_id + n] = build_cell.poly_ids[n];
			}
		}
	}

	// Memory no longer required.
	_build_cells.clear();
}

void PolyFinder::find_cells(const IRect2 &p_rect, Vector<CellResult> &r_cells) {
	r_cells.clear();

	// If the polyfinder hasn't been built yet.
	if (!_cell_size) {
		NP_WARN_PRINT_ONCE("PolyFinder contains no cells.");
		return;
	}

	IRect2 rect = p_rect;
	rect.position -= _boundary.position;

	IPoint2 rect_end = rect.end();
	// Off map.
	if ((rect_end.x < 0) || (rect_end.y < 0)) {
		return;
	}

	IPoint2 begin = rect.position;
	begin /= (i32)_cell_size;

	if ((begin.x >= _width_cells) || (begin.y >= _height_cells)) {
		return;
	}

	IPoint2 end = rect_end / (i32)_cell_size;
	end.x += 1;
	end.y += 1;
	end.x = MIN(end.x, _width_cells);
	end.y = MIN(end.y, _height_cells);

	begin.x = MAX(begin.x, 0);
	begin.y = MAX(begin.y, 0);

	for (u32 y = begin.y; y < end.y; y++) {
		for (u32 x = begin.x; x < end.x; x++) {
			const Cell &cell = get_cell(x, y);

			if (cell.num_ids) {
				CellResult &cr = r_cells.request();
				cr.poly_ids = &_poly_ids[cell.first_id];
				cr.num_polys = cell.num_ids;
			}
		}
	}
}

const u32 *PolyFinder::find_leaf(const IPoint2 &p_pt, u32 &r_num_polys) const {
	// Find the cell corresponding to this point (if any).
	IPoint2 pt = p_pt;
	pt -= _boundary.position;
	if ((pt.x < 0) || (pt.y < 0)) {
		// Off map.
		return nullptr;
	}

	pt.x /= _cell_size;
	pt.y /= _cell_size;

	if ((pt.x >= _width_cells) || (pt.y >= _height_cells)) {
		return nullptr;
	}

#if 0
#ifdef NP_DEV_ENABLED
	IPoint2 pt_debug = (pt * (i32) _cell_size) + _boundary_offset;
	log(String("cell covers ") + pt_debug + " to " + (pt_debug + IPoint2::make(_cell_size, _cell_size) ));
#endif
#endif

	const Cell &cell = get_cell(pt.x, pt.y);
	r_num_polys = cell.num_ids;

	if (!cell.num_ids) {
		return nullptr;
	}

	return &_poly_ids[cell.first_id];
}

} //namespace NavPhysics
