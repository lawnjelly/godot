#include "navphysics_simplifier.h"

namespace NavPhysics {

Loader::SourceMeshData Simplifier::simplify(const Loader::SourceMeshData &p_source) {
	load(p_source);
	find_pairs();

	// Convert back.
	//Vector<u32> inds;
	//Vector<u32> num_poly_inds;
	Loader::SourceMeshData md = p_source;

	for (u32 n = 0; n < data.polys.size(); n++) {
		const Poly &source = data.polys[n];
		if (!source.active)
			continue;

		for (u32 i = 0; i < source.num_points; i++) {
			data.final_inds.push_back(source.points[i]);
		}

		data.final_num_poly_inds.push_back(source.num_points);
	}

	md.num_indices = data.final_inds.size();
	md.indices = data.final_inds.ptr();
	md.num_polys = data.final_num_poly_inds.size();
	md.poly_num_indices = data.final_num_poly_inds.ptr();

	return md;
}

void Simplifier::find_pairs() {
	for (u32 i = 0; i < data.polys.size(); i++) {
		const Poly &a = data.polys[i];
		if (!a.active)
			continue;

		for (u32 j = i + 1; j < data.polys.size(); j++) {
			test_pair(i, j);
		}
	}
}

void Simplifier::test_pair(u32 p_a, u32 p_b) {
	const Poly &a = data.polys[p_a];
	const Poly &b = data.polys[p_b];

	if (!a.active || !b.active)
		return;

	// Dot normal test?
	// Must be approx flat.
	float dot = a.normal.dot(b.normal);
	if (dot < 0.99f)
		return;

	// Can't combine if more than max points.
	if (((a.num_points + b.num_points) - 2) > Poly::MAX_POINTS)
		return;

	// shared edge?
	for (u32 i = 0; i < a.num_points; i++) {
		u32 x = a.points[i];
		u32 y = a.points[(i + 1) % a.num_points];

		if (x > y) {
			SWAP(x, y);
		}

		for (u32 j = 0; j < b.num_points; j++) {
			u32 z = b.points[j];
			u32 w = b.points[(j + 1) % b.num_points];

			if (z > w) {
				SWAP(z, w);
			}

			if ((x == z) && (y == w)) {
				if (match_edge(p_a, p_b, i, j))
					return;
			}
		}
	}
}

bool Simplifier::match_edge(u32 p_a, u32 p_b, u32 p_edge_a, u32 p_edge_b) {
	log(String("matched edge: ") + p_a + " to " + p_b + " ( " + p_edge_a + ", " + p_edge_b + " )");

	const Poly &a = data.polys[p_a];
	const Poly &b = data.polys[p_b];

	// Construct a test poly made of both
	Poly p;
	p.init();

	// Start of A
	for (u32 ac = 0; ac < a.num_points; ac++) {
		p.add_point(a.points[ac]);
		log(ac);

		// Time to add B?
		if (ac == p_edge_a) {
			for (u32 bc = 0; bc < (b.num_points - 2); bc++) {
				u32 b_actual = (p_edge_b + 2 + bc) % b.num_points;
				log(b_actual);
				p.add_point(b.points[b_actual]);
			}
		}
	}

	//	for (u32 n = 0; n < p_edge_a; n++) {
	//		p.add_point(a.points[n]);
	//	}

	//	// B (except the edge)
	//	u32 count = b.num_points - 2;
	//	for (u32 n=0; n<count; n++) {
	////	for (u32 n = (p_edge_b + 2); n != p_edge_b; n++) {
	//		// wraparound
	//		u32 i = (p_edge_b + 2 + n) % b.num_points;
	//		//n %= b.num_points;
	//		log(i);

	//		p.add_point(b.points[i]);
	//	}

	//	// End of A
	//	for (u32 n = p_edge_a + 1; n < a.num_points; n++) {
	//		p.add_point(a.points[n]);
	//	}

	if (is_ok_to_merge(p)) {
		// merge...
		log("Merging");
		calc_poly_normal(p);
		data.polys[p_a] = p;
		data.polys[p_b].active = false;
		return true;
	}

	return false;
}

bool Simplifier::is_ok_to_merge(const Poly &p) const {
	debug_poly(p);

	for (u32 e = 0; e < p.num_points; e++) {
		u32 e2 = (e + 1) % p.num_points;
		u32 e3 = (e + 2) % p.num_points;

		FPoint3 pts[3];
		pts[0] = data.verts[e];
		pts[1] = data.verts[e2];
		pts[2] = data.verts[e3];

		if (is_concave(pts)) {
			return false;
		}
	}

	return true;
}

void Simplifier::debug_poly(const Poly &p) const {
	log(String("Poly"));
	for (u32 n = 0; n < p.num_points; n++) {
		const FPoint3 &pt = data.verts[p.points[n]];
		log(String("\tpt ") + n + " : " + pt);
	}
}

void Simplifier::calc_poly_normal(Poly &p) {
	int num_points = p.num_points;

	if (num_points < 3) {
		p.normal.zero();
		return;
	}

	FPoint3 normal;
	FPoint3 center;

	for (int i = 0; i < num_points; i++) {
		int j = (i + 1) % num_points;

		u32 ind_i = p.points[i];
		u32 ind_j = p.points[j];

		const FPoint3 &pi = data.verts[ind_i];
		const FPoint3 &pj = data.verts[ind_j];

		center += pi;

		normal.x += (((pi.z) + (pj.z)) * ((pj.y) - (pi.y)));
		normal.y += (((pi.x) + (pj.x)) * ((pj.z) - (pi.z)));
		normal.z += (((pi.y) + (pj.y)) * ((pj.x) - (pi.x)));
	}

	normal.normalize();
	center /= num_points;

	// point and normal
	//r_poly.plane.set(center, normal);
	p.normal = normal;
}

bool Simplifier::is_concave(const FPoint3 p_pts[3]) const {
	FPoint3 a = p_pts[1] - p_pts[0];
	FPoint3 b = p_pts[2] - p_pts[0];

	float cross = (a.x * b.z) - (a.z * b.x);
	log(String("cross ") + cross);
	//return cross > -0.01f;
	return cross > 0;
}

void Simplifier::load(const Loader::SourceMeshData &p_source) {
	data.verts.resize(p_source.num_verts);
	data.polys.resize(p_source.num_polys);

	for (u32 n = 0; n < p_source.num_verts; n++) {
		data.verts[n] = p_source.verts[n];
	}

	u32 index_count = 0;

	for (u32 n = 0; n < p_source.num_polys; n++) {
		Poly &poly = data.polys[n];
		poly.init();

		u32 num_inds = p_source.poly_num_indices[n];

		for (u32 p = 0; p < num_inds; p++) {
			poly.add_point(p_source.indices[index_count++]);
		}

		calc_poly_normal(poly);
	}
}

} //namespace NavPhysics
