#include "spatial_deduplicator.h"
#include "core/print_string.h"
#include "core/variant.h"

void SpatialDeduplicator::Grid::calc_bound(const Vector3 *p_verts, uint32_t p_num_verts) {
	if (!p_num_verts) {
		return;
	}

	Rect2 rt;
	rt.position = vec3_xy(p_verts[0]);

	for (uint32_t n = 0; n < p_num_verts; n++) {
		rt.expand_to(vec3_xy(p_verts[n]));
	}

	_bound_min = rt.position;
	_bound_mult = Vector2();
	if (rt.size.x > 0.0) {
		_bound_mult.x = GRID_SIZE / rt.size.x;
	}
	if (rt.size.y > 0.0) {
		_bound_mult.y = GRID_SIZE / rt.size.y;
	}
}

void SpatialDeduplicator::Grid::add(const Vector3 &p_pos, uint32_t p_id) {
	Vector2i loc = get_cell_xy(p_pos);
	Vert v;
	v.id = p_id;
	v.pos = p_pos;

	for (int y = loc.y - 1; y <= loc.y + 1; y++) {
		for (int x = loc.x - 1; x <= loc.x + 1; x++) {
			if (is_on_map(x, y)) {
				Cell &cell = get_cell(x, y);
				cell.verts.push_back(v);
			}
		}
	}
}

bool SpatialDeduplicator::Grid::find(const Vector3 &p_pos, real_t p_epsilon, LocalVectori<uint32_t> &r_ids) const {
	r_ids.clear();

	const Cell &cell = get_cell_at_pos(p_pos);
	for (int n = 0; n < cell.verts.size(); n++) {
		const Vert &v = cell.verts[n];
		if (p_pos.is_equal_approx(v.pos, p_epsilon)) {
			//return v.id;
			r_ids.push_back(v.id);
		}
	}

	return r_ids.size() != 0;
}

bool SpatialDeduplicator::deduplicate_verts_only(const uint32_t *p_in_inds, uint32_t p_num_in_inds, const Vector3 *p_in_verts, uint32_t p_num_in_verts, LocalVectori<Vector3> &r_out_verts, LocalVectori<uint32_t> &r_out_inds, real_t p_epsilon) {
	LocalVectori<uint32_t> vert_map;
	uint32_t num_out_verts = 0;
	if (!deduplicate_map(p_in_inds, p_num_in_inds, p_in_verts, p_num_in_verts, vert_map, num_out_verts, r_out_inds, p_epsilon))
		return false;

	// create new verts list
	r_out_verts.resize(num_out_verts);

	for (int n = 0; n < p_num_in_verts; n++) {
		uint32_t new_vert_id = vert_map[n];
		DEV_ASSERT(new_vert_id < num_out_verts);

		r_out_verts[new_vert_id] = p_in_verts[n];
	}

	return true;
}

// The spatial deduplication is done automatically, but the user can provide a template function following the form above
// to detect whether a vertex is similar enough to be merged based on e.g. normal, UVs etc.
bool SpatialDeduplicator::deduplicate_map(const uint32_t *p_in_inds, uint32_t p_num_in_inds, const Vector3 *p_in_verts, uint32_t p_num_in_verts, LocalVectori<uint32_t> &r_vert_map, uint32_t &r_num_out_verts, LocalVectori<uint32_t> &r_out_inds, real_t p_epsilon) {
	_epsilon = p_epsilon;

	//LocalVectori<uint32_t> vert_map;
	// vert map needs to be big enough for all the input verts,
	// it maps an input vert to an output vert after removing duplicates
	r_vert_map.resize(p_num_in_verts);

	LocalVectori<Vector3> out_verts;
	LocalVectori<uint32_t> out_vert_sources;
	out_verts.clear();
	r_out_inds.clear();

#define GODOT_DEDUPLICATOR_USE_GRID
#ifdef GODOT_DEDUPLICATOR_USE_GRID
	Grid grid;
	grid.calc_bound(p_in_verts, p_num_in_verts);
	LocalVectori<uint32_t> found_ids;
#endif

	for (int n = 0; n < p_num_in_verts; n++) {
		const Vector3 &pt = p_in_verts[n];

		//print_line("in_vert " + itos(n) + " : ( " + String(Variant(pt)) + " ) ");

		// find existing?
		bool found = false;

#ifdef GODOT_DEDUPLICATOR_USE_GRID
		grid.find(pt, _epsilon, found_ids);

		for (int idc = 0; idc < found_ids.size(); idc++) {
			int i = found_ids[idc];
			{
#else
		for (int i = 0; i < out_verts.size(); i++) {
			//print_line("\tout_vert " + itos(i) + " : ( " + String(Variant(out_verts[i])) + " ) ");

			if (out_verts[i].is_equal_approx(pt, _epsilon)) {
#endif
				// attribute tests
				int out_vert_orig_id = out_vert_sources[i];

				bool allow = true;
				for (int t = 0; t < _attributes.size(); t++) {
					const Attribute &attr = _attributes[t];
					switch (attr.type) {
						case Attribute::AT_POSITION: {
							//DEV_ASSERT(attr.vec3s[out_vert_orig_id] == pt);
							const Vector3 &a = attr.vec3s[n];
							const Vector3 &b = attr.vec3s[out_vert_orig_id];
							real_t sl = (b - a).length_squared();
							if (sl > attr.epsilon_dedup) {
								allow = false;
								break;
							}
						} break;
						case Attribute::AT_NORMAL:
						default: {
							const Vector3 &a = attr.vec3s[n];
							const Vector3 &b = attr.vec3s[out_vert_orig_id];
							real_t dot = a.dot(b);
							if (dot < attr.epsilon_dedup) {
								allow = false;
								//print_line("disallowing merge at " + String(Variant(pt)));
								break;
							}
						} break;
						case Attribute::AT_UV: {
							const Vector2 &a = attr.vec2s[n];
							const Vector2 &b = attr.vec2s[out_vert_orig_id];
							real_t sl = (b - a).length_squared();
							if (sl > attr.epsilon_dedup) {
								allow = false;
								break;
							}
						} break;
					}
				}

				if (allow) {
					r_vert_map[n] = i;
					found = true;
					break;
				}
			}
		}

		if (found)
			continue;

		// not found...
		r_vert_map[n] = out_verts.size();

#ifdef GODOT_DEDUPLICATOR_USE_GRID
		grid.add(pt, out_verts.size());
#endif

		out_verts.push_back(pt);
		out_vert_sources.push_back(n);
	}

	// pass back the number of final verts
	r_num_out_verts = out_verts.size();

	// sort output data
	for (int n = 0; n < p_num_in_inds; n++) {
		uint32_t ind = p_in_inds[n];
		DEV_ASSERT(ind < p_num_in_verts);

		uint32_t new_ind = r_vert_map[ind];
		DEV_ASSERT(new_ind < out_verts.size());

		r_out_inds.push_back(new_ind);
	}

	return true;
}
