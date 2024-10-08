#pragma once

#include "navphysics_loader.h"

namespace NavPhysics {

class Simplifier {
	struct Poly {
		const static int MAX_POINTS = 7;
		bool active = true;
		u32 num_points = 0;
		u32 points[MAX_POINTS];
		//u32 neighs[MAX_POINTS];
		FPoint3 normal;
		void init() {
			active = true;
			num_points = 0;
			for (u32 n = 0; n < MAX_POINTS; n++) {
				points[n] = UINT32_MAX;
				//neighs[n] = UINT32_MAX;
			}
			normal.zero();
		}
		bool add_point(u32 p_ind) {
			NP_DEV_ASSERT(num_points < MAX_POINTS);
			if (num_points < MAX_POINTS) {
				points[num_points++] = p_ind;
				return true;
			}
			return false;
		}
	};

	struct Pair {
		u32 poly_a, poly_b;
		f32 fit;
		void init() {
			poly_a = UINT32_MAX;
			poly_b = UINT32_MAX;
			fit = 0;
		}
	};

	struct Data {
		Vector<FPoint3> verts;
		Vector<Poly> polys;
		Vector<Pair> pairs;

		Vector<u32> final_inds;
		Vector<u32> final_num_poly_inds;
	} data;

	void load(const Loader::SourceMeshData &p_source);
	void find_pairs();
	void test_pair(u32 p_a, u32 p_b);
	bool match_edge(u32 p_a, u32 p_b, u32 p_edge_a, u32 p_edge_b);
	bool is_ok_to_merge(const Poly &p) const;
	bool is_concave(const FPoint3 p_pts[3]) const;
	void debug_poly(const Poly &p) const;
	void calc_poly_normal(Poly &p);

public:
	Loader::SourceMeshData simplify(const Loader::SourceMeshData &p_source);
};

} //namespace NavPhysics
