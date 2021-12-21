#include "spatial_deduplicator.h"

bool SpatialDeduplicator::deduplicate(const uint32_t *p_inds, uint32_t p_num_inds, const Vector3 *p_verts, uint32_t p_num_verts, LocalVectori<Vector3> &r_verts, LocalVectori<uint32_t> &r_out_inds, real_t p_epsilon) {
	_epsilon = p_epsilon;

	LocalVectori<uint32_t> vert_map;
	vert_map.resize(p_num_verts);

	r_verts.clear();
	r_out_inds.clear();

	for (int n = 0; n < p_num_verts; n++) {
		const Vector3 &pt = p_verts[n];

		// find existing?
		bool found = false;

		for (int i = 0; i < r_verts.size(); i++) {
			if (r_verts[i].is_equal_approx(pt, _epsilon)) {
				vert_map[n] = i;
				found = true;
				break;
			}
		}

		if (found)
			continue;

		// not found...
		vert_map[n] = r_verts.size();
		r_verts.push_back(pt);
	}

	// sort output data
	for (int n = 0; n < p_num_inds; n++) {
		uint32_t ind = p_inds[n];
		DEV_ASSERT(ind < p_num_verts);

		uint32_t new_ind = vert_map[ind];
		DEV_ASSERT(new_ind < r_verts.size());

		r_out_inds.push_back(new_ind);
	}

	return true;
}
