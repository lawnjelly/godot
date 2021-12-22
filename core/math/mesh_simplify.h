#pragma once

#include "core/local_vector.h"
#include "core/math/plane.h"
#include "core/math/vector3.h"

// The optional callback can test any triangle before allowing a merge, allowing testing normals, UVs etc to prevent
// merging where the change in the attribute is too high.
// This could alternatively be written as a template, which would be more efficient but require everything in the header.
typedef bool (*MeshSimplifyCallback)(void *p_userdata, const uint32_t p_tri_from[3], const uint32_t p_tri_to[3]);

class MeshSimplify {
	struct Edge {
		Edge() {
			a = UINT32_MAX;
			b = UINT32_MAX;
		}
		void sort() {
			if (b > a) {
				SWAP(a, b);
			}
		}
		bool operator==(const Edge &p_o) const { return (a == p_o.a) && (b == p_o.b); }
		uint32_t a, b;
	};

	struct Tri {
		Tri() {
			for (int n = 0; n < 3; n++) {
				corn[n] = UINT32_MAX;
				neigh[n] = UINT32_MAX;
			}
		}
		// corner indices
		uint32_t corn[3];

		Edge edge[3];

		Plane plane;

		// neighboursing triangles
		uint32_t neigh[3];
		uint32_t num_neighs = 0;

		bool active = true;
	};

	struct Vert {
		void link_tri(uint32_t p_id) {
			int64_t res = tris.find(p_id);
			if (res == -1) {
				tris.push_back(p_id);
			}
		}

		Vector3 pos;
		LocalVectori<uint32_t> tris;
		bool edge_vert = false;
		bool active = false;
	};

public:
	// returns number of indices
	uint32_t simplify(const uint32_t *p_inds, uint32_t p_num_inds, const Vector3 *p_verts, uint32_t p_num_verts, uint32_t *r_inds, Vector3 *r_deduped_verts, uint32_t &r_num_deduped_verts, real_t p_threshold = 0.01, MeshSimplifyCallback p_callback = nullptr, void *p_userdata = nullptr);

	uint32_t simplify_map(const uint32_t *p_in_inds, uint32_t p_num_in_inds, const Vector3 *p_in_verts, uint32_t p_num_in_verts, uint32_t *r_out_inds, LocalVectori<uint32_t> &r_vert_map, uint32_t &r_num_out_verts, real_t p_threshold = 0.01, MeshSimplifyCallback p_callback = nullptr, void *p_userdata = nullptr);

private:
	bool _simplify();
	bool _simplify_vert(uint32_t p_vert_id);
	void _create_tris(const uint32_t *p_inds, uint32_t p_num_inds);
	void _establish_neighbours(int p_tri0, int p_tri1);
	uint32_t _choose_vert_to_merge(uint32_t p_start_from) const;
	bool _find_vert_to_merge_to(uint32_t p_vert_from);
	void _adjust_tri(uint32_t p_tri_id, uint32_t p_vert_from, uint32_t p_vert_to);
	void _resync_tri(uint32_t p_tri_id);
	bool _calculate_plane(uint32_t p_corns[3], Plane &r_plane) const;
	bool _allow_collapse(uint32_t p_tri_id, uint32_t p_vert_from, uint32_t p_vert_to) const;

	LocalVectori<Vert> _verts;
	LocalVectori<Tri> _tris;

	LocalVectori<uint32_t> _merge_vert_ids;
	real_t _threshold_dist = 0.01;
	MeshSimplifyCallback _callback = nullptr;
	void *_callback_userdata = nullptr;
};
