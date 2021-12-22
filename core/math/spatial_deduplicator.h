#pragma once

#include "core/local_vector.h"
#include "core/math/vector3.h"

#define SPATIAL_DEDUPLICATOR_USE_REFERENCE_IMPL

class SpatialDeduplicator {
	real_t _epsilon = 0.01;

public:
	class DummyAttributeTest {
	public:
		bool deduplicate_test_similar_attributes(uint32_t p_ind_a, uint32_t p_ind_b) const {
			return true;
		}
	};

	// The spatial deduplication is done automatically, but the user can provide a template function following the form above
	// to detect whether a vertex is similar enough to be merged based on e.g. normal, UVs etc.
	template <class T>
	bool deduplicate(const uint32_t *p_inds, uint32_t p_num_inds, const Vector3 *p_verts, uint32_t p_num_verts, LocalVectori<Vector3> &r_verts, LocalVectori<uint32_t> &r_out_inds, const T &p_attribute_tester, real_t p_epsilon = 0.01) {
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
				if (r_verts[i].is_equal_approx(pt, _epsilon) && p_attribute_tester.deduplicate_test_similar_attributes(n, i)) {
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
};
