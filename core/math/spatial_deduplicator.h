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

	template <class T>
	bool deduplicate_verts_only(const uint32_t *p_in_inds, uint32_t p_num_in_inds, const Vector3 *p_in_verts, uint32_t p_num_in_verts, LocalVectori<Vector3> &r_out_verts, LocalVectori<uint32_t> &r_out_inds, const T &p_attribute_tester, real_t p_epsilon = 0.01) {
		LocalVectori<uint32_t> vert_map;
		uint32_t num_out_verts = 0;
		if (!deduplicate_map(p_in_inds, p_num_in_inds, p_in_verts, p_num_in_verts, vert_map, num_out_verts, r_out_inds, p_attribute_tester, p_epsilon))
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
	template <class T>
	bool deduplicate_map(const uint32_t *p_in_inds, uint32_t p_num_in_inds, const Vector3 *p_in_verts, uint32_t p_num_in_verts, LocalVectori<uint32_t> &r_vert_map, uint32_t &r_num_out_verts, LocalVectori<uint32_t> &r_out_inds, const T &p_attribute_tester, real_t p_epsilon = 0.01) {
		_epsilon = p_epsilon;

		//LocalVectori<uint32_t> vert_map;
		// vert map needs to be big enough for all the input verts,
		// it maps an input vert to an output vert after removing duplicates
		r_vert_map.resize(p_num_in_verts);

		LocalVectori<Vector3> out_verts;
		out_verts.clear();
		r_out_inds.clear();

		for (int n = 0; n < p_num_in_verts; n++) {
			const Vector3 &pt = p_in_verts[n];

			// find existing?
			bool found = false;

			for (int i = 0; i < out_verts.size(); i++) {
				if (out_verts[i].is_equal_approx(pt, _epsilon) && p_attribute_tester.deduplicate_test_similar_attributes(n, i)) {
					r_vert_map[n] = i;
					found = true;
					break;
				}
			}

			if (found)
				continue;

			// not found...
			r_vert_map[n] = out_verts.size();
			out_verts.push_back(pt);
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
};
