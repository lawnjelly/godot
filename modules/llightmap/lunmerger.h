#pragma once

#include "lvector.h"
#include "scene/3d/mesh_instance.h"

namespace LM {

class UnMerger {
	struct LFace {
		Vector3 pos[3];
		Vector3 norm[3];
		int index[3];

		// todo .. we only need to store one set of UVs
		// (the orig face and the merged face store UV and UV2 respectively)
		Vector2 uvs[3];

		// when standardized, a face has an offset
		// to get back to the original format
		int offset;

		bool vector3_less(const Vector3 &a, const Vector3 &b) const {
			if (a.x < b.x)
				return true;
			if (a.y < b.y)
				return true;
			if (a.z < b.z)
				return true;
			return false;
		}

		template <class T>
		void shift(int p_shift, T *p) {
			switch (p_shift) {
				case 1: {
					T temp = p[0];
					p[0] = p[1];
					p[1] = p[2];
					p[2] = temp;
				} break;
				case 2: {
					T temp = p[0];
					p[0] = p[2];
					p[2] = p[1];
					p[1] = temp;
				} break;
			}
		}

		// to make faces comparable
		int standardize_order() {
			int first = 0;
			Vector3 biggest = pos[0];
			if (vector3_less(biggest, pos[1])) {
				biggest = pos[1];
				first = 1;
			}
			if (vector3_less(biggest, pos[2])) {
				biggest = pos[2];
				first = 2;
			}

			// resync
			shift(first, pos);
			shift(first, norm);
			shift(first, index);
			shift(first, uvs);

			// save the standardize offset
			offset = first;

			return first;
		}

		String to_string() const;
	};

	// unique vert
	struct LVert {
		Vector3 pos;
		Vector3 norm;
		Vector2 uv;
		Vector2 uv2;

		bool approx_equal(const LVert &p_o) const;
	};

	// the merged mesh data to be passed for unmerging meshes
	struct LMerged {
		PoolVector<Vector3> verts;
		PoolVector<Vector3> norms;
		PoolVector<Vector2> uv2s;
		PoolVector<int> inds;

		int num_faces;

		// precreate LFaces
		LVector<LFace> lfaces;
	};

	// these are now not crucial because the algorithm finds the best fit.
	// These are only used for quick reject.
	struct LMergeParams {
		float threshold_dist;
		float threshold_dist_squared;
		float threshold_dot;
	} _merge_params;

public:
	UnMerger();
	bool unmerge(const Merger &p_merger, const MeshInstance &p_source_mi);
	void set_unmerge_params(float thresh_dist, float thresh_dot);

private:
	bool fill_merged_from_mesh(LMerged &merged, const MeshInstance &mesh);
	bool unmerge_mesh_surface(MeshInstance &p_mi, LMerged &r_merged, int p_surface_id, Ref<ArrayMesh> p_am_new);
	bool unmerge_mesh(MeshInstance &mi, LMerged &merged);

	int find_or_add_vert(LVector<LVert> &uni_verts, const LVert &vert) const;

	// returns goodness of fit
	float do_faces_match(const LFace &a, const LFace &b) const;
	void add_merged_triangle(const LFace &forig, const LFace &forig_modelspace, const LFace &fmerged, LVector<LVert> &UniqueVerts, PoolVector<int> &UniqueIndices);

	void transform_verts(const PoolVector<Vector3> &p_pts_local, PoolVector<Vector3> &p_pts_world, const Transform &p_tr) const;
	void transform_norms(const PoolVector<Vector3> &p_norms_local, PoolVector<Vector3> &p_norms_world, const Transform &p_tr) const;
};

} //namespace LM
