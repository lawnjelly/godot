#include "slow_hull.h"
#include "core/math/convex_hull.h"
#include "core/math/quick_hull.h"
#include "core/os/os.h"

#ifndef DEV_ASSERT
#define DEV_ASSERT(a) CRASH_COND(!(a))
#endif

#define SLOWHULL_DEVASSERT(a) DEV_ASSERT(a)

#ifdef GODOT_SLOWHULL_USE_LOGS
#define SLOWHULL_LOG(a) print_line(a)
#else
#define SLOWHULL_LOG(a)
#endif

#define GODOT_SLOWHULL_EXTRA_CHECKS

void SlowHull::HEdge::create(const HFace &p_face, int p_corn) {
	set(p_face.corn[p_corn], p_face.corn[(p_corn + 1) % 3]);
}

void SlowHull::HFace::debug_print() {
	String sz = "HFace : (";
	for (int n = 0; n < 3; n++) {
		sz += itos(corn[n]) + ", ";
	}
	sz += ")";
	print_line(sz);
}

void SlowHull::HEdgeList::debug_print() {
	String sz = "edgelist inds : (";
	for (int n = 0; n < inds.size(); n++) {
		sz += itos(inds[n]) + ", ";
	}
	print_line(sz + ")");
}

bool SlowHull::HEdgeList::try_insert(int p_which, uint32_t p_corn, const Vector3 &p_normal, const Vector3 *p_source_pts) {
	//	return false;

	real_t epsilon = -0.01; // -0.01

	// the potential point to add
	const Vector3 &pt = p_source_pts[p_corn];

	// get two previous points, and next 2
	const Vector3 &a = p_source_pts[get_wrapped(p_which - 1)];
	const Vector3 &b = p_source_pts[get_wrapped(p_which)];
	const Vector3 &c = p_source_pts[get_wrapped(p_which + 1)];
	const Vector3 &d = p_source_pts[get_wrapped(p_which + 2)];

	// test for on the right of ab
	Plane p(a, b, b + p_normal);
	real_t dist = p.distance_to(pt);
	if (dist > epsilon) {
		// concave
		return false;
	}

	p = Plane(c, d, d + p_normal);
	dist = p.distance_to(pt);
	if (dist > epsilon) {
		// concave
		return false;
	}

	// convex! we can add
	inds.insert(p_which + 1, p_corn);

	// rejig the start index of the face, so that we can ensure the same triangles are created from the compressed face
	// (otherwise the diagonals can change, and hence the topography)

	//edges.inds.insert(n+1, e.corn[1]);
	//added = true;
	//break;
	return true;
}

// attempt to form a triangle fan without changing topology
// (i.e. changing the diagonals of the original triangles)
// it is important not to change the topology because two triangles can be close in normal
// with one diagonal, but very different normals with another diagonal chosen.
bool SlowHull::HMultiFace::push_face(HFace *p_face, const Vector3 *p_source_pts) {
	// there are 4 cases.
	// First triangle, any order.
	// Two triangles .. chose either of the matching edge corners as 'base'
	// Three triangles, must rechoose the base so it matches for all three triangles
	// Four or more triangles must reuse the existing base, or they cannot fit.

	switch (exfaces.size()) {
		case 0: {
			// first triangle
			ExFace exf;
			exf.hface = p_face;
			exfaces.push_back(exf);
			base = p_face->a;
			flares.push_back(p_face->b);
			flares.push_back(p_face->c);
			//			edges.inds.push_back(p_face->corn[0]);
			//			edges.inds.push_back(p_face->corn[1]);
			//			edges.inds.push_back(p_face->corn[2]);
			return true;
		} break;
		case 1: {
			// second triangle .. either matching edge can be base.
			return push_face_2_tris(p_face);
		} break;
		case 2: {
			return push_face_3_tris(p_face);
		} break;
		default: {
			// four or more triangles
			return push_face_multi_tris(p_face);
		} break;
	}

	//	edges.debug_print();
	//	p_face->debug_print();

	//	int which_corn_index = -1;
	//	int insert_pos = edges.find_corn_insert_position(*p_face, which_corn_index);

	//	if (insert_pos != -1) {
	//		if (edges.try_insert(insert_pos, which_corn_index, plane.normal, p_source_pts)) {
	//			faces.push_back(p_face);
	//			return true;
	//		}
	//	}
	return false;
}

bool SlowHull::HMultiFace::push_face_multi_tris(HFace *p_face) {
	if (!p_face->contains_index(base)) {
		return false;
	}

	// with three triangles we have a last chance to rechoose the base so it matches all three
	LocalVector<uint32_t> new_flares = flares;

	// in either case we need to add the third flare
	if (!add_flare_to_fan(*p_face, base, new_flares)) {
		return false;
	}

	// check for convex NYI
	flares = new_flares;

	// push the new face
	ExFace exf;
	exf.hface = p_face;
	exfaces.push_back(exf);
	return true;
}

bool SlowHull::HMultiFace::push_face_3_tris(HFace *p_face) {
	// with three triangles we have a last chance to rechoose the base so it matches all three
	int new_base = -1;
	LocalVector<uint32_t> new_flares;

	HFace &fa = *exfaces[0].hface;
	HFace &fb = *exfaces[1].hface;
	HFace &fc = *p_face;

	// simplest case, the base can be maintained
	if (p_face->contains_index(base)) {
		new_base = base;
	} else {
		for (int n = 0; n < 3; n++) {
			uint32_t corn_a = fa.corn[n];

			for (int m = 0; m < 3; m++) {
				uint32_t corn_b = fb.corn[m];

				if (corn_a != corn_b)
					continue;

				for (int o = 0; o < 3; o++) {
					if (corn_a == fc.corn[o]) {
						new_base = fc.corn[o];
						break;
					}
				}
				if (new_base != -1)
					break;
			}
			if (new_base != -1)
				break;
		}
	}

	// no common base possible, cannot form a fan
	if (new_base == -1) {
		return false;
	}

	// start by adding flares for a and b
	// if the new base has changed we need to change the flares
	if (new_base != base) {
		int bi = fa.find_index(new_base);
		new_flares.push_back(fa.corn[(bi + 1) % 3]);
		new_flares.push_back(fa.corn[(bi + 2) % 3]);

		// the new face either goes on the front or the back of the new_flares
		if (!add_flare_to_fan(fb, new_base, new_flares)) {
			CRASH_COND("failed to add to fan");
			return false;
		}
	} else {
		new_flares = flares;
	}

	// in either case we need to add the third flare
	if (!add_flare_to_fan(fc, new_base, new_flares)) {
		return false;
	}

	// check for convex NYI
	base = new_base;
	flares = new_flares;

	// push the new face
	ExFace exf;
	exf.hface = p_face;
	exfaces.push_back(exf);

	return true;
}

bool SlowHull::HMultiFace::push_face_2_tris(HFace *p_face) {
	//	HEdge edge;
	//	if (!faces[0].find_shared_edge(*p_face, edge))
	//	{
	//		return false;
	//	}

	int new_base = -1;
	LocalVector<uint32_t> new_flares;

	HFace &fa = *exfaces[0].hface;
	HFace &fb = *p_face;

	// simplest case, the base can be maintained
	if (p_face->contains_index(base)) {
		new_base = base;
	} else {
		for (int n = 0; n < 3; n++) {
			for (int m = 0; m < 3; m++) {
				if (fa.corn[n] == fb.corn[m]) {
					new_base = fa.corn[n];
					break;
				}
			}
			if (new_base != -1)
				break;
		}
	}

	CRASH_COND(new_base == -1);

	// if the new base has changed we need to change the flares
	if (new_base != base) {
		int bi = fa.find_index(new_base);
		new_flares.push_back(fa.corn[(bi + 1) % 3]);
		new_flares.push_back(fa.corn[(bi + 2) % 3]);
	} else {
		new_flares = flares;
	}

	// the new face either goes on the front or the back of the new_flares
	bool success = add_flare_to_fan(fb, new_base, new_flares);
	CRASH_COND(!success);

	// check for convex NYI
	base = new_base;
	flares = new_flares;

	// push the new face
	ExFace exf;
	exf.hface = p_face;
	exfaces.push_back(exf);

	return true;
}

bool SlowHull::HMultiFace::add_flare_to_fan(const HFace &p_face, uint32_t p_new_base, LocalVector<uint32_t> &r_new_flares) {
	// the new face either goes on the front or the back of the new_flares
	if (p_face.contains_index(r_new_flares[0])) {
		// front
		uint32_t new_corn = p_face.find_third_corn(p_new_base, r_new_flares[0]);
		r_new_flares.insert(0, new_corn);
	} else {
		// back
		uint32_t new_corn = p_face.find_third_corn(p_new_base, r_new_flares[r_new_flares.size() - 1]);
		if (new_corn == -1) {
			// can't add on the end, only the middle, not a valid fan
			return false;
		}
		r_new_flares.push_back(new_corn);
	}
	return true;
}

//////////////////////////////

void SlowHull::_bind_methods() {
	ClassDB::bind_method(D_METHOD("build_hull"), &SlowHull::build_hull);
}

void SlowHull::clear() {
	_source_pts = nullptr;
	_num_source_pts = 0;
	_faces.clear();
	_aabb = AABB();
	//_plane_epsilon = 0.0001;
}

void SlowHull::create_initial_simplex() {
	int longest_axis = _aabb.get_longest_axis_index();

	//first two vertices are the most distant
	int simplex[4] = { 0 };

	{
		real_t max = 0, min = 0;

		for (int i = 0; i < _num_source_pts; i++) {
			real_t d = _source_pts[i][longest_axis];
			if (i == 0 || d < min) {
				simplex[0] = i;
				min = d;
			}

			if (i == 0 || d > max) {
				simplex[1] = i;
				max = d;
			}
		}
	}

	//third vertex is one most further away from the line

	{
		real_t maxd = 0;
		Vector3 rel12 = _source_pts[simplex[0]] - _source_pts[simplex[1]];

		for (int i = 0; i < _num_source_pts; i++) {
			Vector3 n = rel12.cross(_source_pts[simplex[0]] - _source_pts[i]).cross(rel12).normalized();
			real_t d = Math::abs(n.dot(_source_pts[simplex[0]]) - n.dot(_source_pts[i]));

			if (i == 0 || d > maxd) {
				maxd = d;
				simplex[2] = i;
			}
		}
	}

	//fourth vertex is the one most further away from the plane

	{
		real_t maxd = 0;
		Plane p(_source_pts[simplex[0]], _source_pts[simplex[1]], _source_pts[simplex[2]]);

		for (int i = 0; i < _num_source_pts; i++) {
			real_t d = Math::abs(p.distance_to(_source_pts[i]));

			if (i == 0 || d > maxd) {
				maxd = d;
				simplex[3] = i;
			}
		}
	}

	//compute center of simplex, this is a point always warranted to be inside
	Vector3 center;

	for (int i = 0; i < 4; i++) {
		center += _source_pts[simplex[i]];
	}

	center /= 4.0;

	//add faces
	LocalVector<Plane> simplex_planes;

	for (int i = 0; i < 4; i++) {
		static const int face_order[4][3] = {
			{ 0, 1, 2 },
			{ 0, 1, 3 },
			{ 0, 2, 3 },
			{ 1, 2, 3 }
		};

		HFace f;
		for (int j = 0; j < 3; j++) {
			f.corn[j] = simplex[face_order[i][j]];
		}

		f.calc_plane(*this);

		if (f.plane.is_point_over(center)) {
			//flip face to clockwise if facing inwards
			SWAP(f.corn[0], f.corn[1]);
			f.plane = -f.plane;
		}

		_faces.push_back(f);

		simplex_planes.push_back(f.plane);
	}

	// seed the neighbouring faces
	find_face_neighbours();

	// first whittle the source inds in play to the simplex
	whittle_source_indices(simplex_planes, true);

	for (List<HFace>::Element *E = _faces.front(); E; E = E->next()) {
		HFace &f = E->get();

		fill_new_face(f);

		//		for (int n = 0; n < _; n++) {
		//			const Vector3 &pt = _source_pts[n];
		//			real_t dist = f.plane.distance_to(pt);

		//			if (dist > _plane_epsilon) {
		//				f.pts.push_back(n);
		//			}
		//		}
	}
}

void SlowHull::find_face_neighbours() {
	for (List<HFace>::Element *E = _faces.front(); E; E = E->next()) {
		HFace &f = E->get();

		for (List<HFace>::Element *O = E->next(); O; O = O->next()) {
			HFace &n = O->get();

			// each edge of f
			for (int c = 0; c < 3; c++) {
				HEdge edge;
				edge.set(f.corn[c], f.corn[(c + 1) % 3]);
				edge.sort();

				// each edge of n
				for (int d = 0; d < 3; d++) {
					HEdge edge2;
					edge2.set(n.corn[d], n.corn[(d + 1) % 3]);
					edge2.sort();

					// match?
					if (edge == edge2) {
						// set the neighbour pointers
						f.neigh[c] = &n;
						n.neigh[d] = &f;
					}
				}
			}
		}
	}

	// verify
	for (List<HFace>::Element *E = _faces.front(); E; E = E->next()) {
		const HFace &f = E->get();

		CRASH_COND(!f.neigh[0]);
		CRASH_COND(!f.neigh[1]);
		CRASH_COND(!f.neigh[2]);
	}
}

List<SlowHull::HFace>::Element *SlowHull::split_face(List<HFace>::Element *p_current_face) {
	// while the current face is done, move to the next
	while (!p_current_face->get().pts.size()) {
		SLOWHULL_LOG("face [" + itos(p_current_face->get().id) + "] done...");

		p_current_face = p_current_face->next();
		if (!p_current_face) {
			// finished the list! all done
			return nullptr;
		}
	}

	// find the furthest point from the plane
	const SlowHull::HFace &face = p_current_face->get();
	SLOWHULL_LOG("split_face : [" + itos(face.id) + "]");

	real_t furthest = -1.0;
	int furthest_index = -1;

	for (int n = 0; n < face.pts.size(); n++) {
		int point_index = face.pts[n];

		SLOWHULL_DEVASSERT(face.find_index(point_index) == -1);

		const Vector3 &pt = _source_pts[point_index];
		real_t dist = face.plane.distance_to(pt);
		if (dist > furthest) {
			furthest = dist;
			furthest_index = point_index;
		}
	}

	SLOWHULL_DEVASSERT(furthest_index >= 0);
	SLOWHULL_LOG("\tfurthest (" + itos(furthest_index) + ") {dist " + String(Variant(furthest)) + "}");

	split_face_implementation(face, furthest_index);

	return p_current_face->next();
}

void SlowHull::find_lit_faces(Vector<List<SlowHull::HFace>::Element *> &r_lit_elements, const Vector3 &p_pt_furthest) {
	// There is a massive potential problem with floating point error and finding lit faces.
	// If a lot of faces are coplanar, and the point is on that plane, then due to floating point
	// error you can get some faces lit and some not, and thus multiple horizon edges, which
	// leads to crashes later on generating the cone.
	// We have to explicitly deal with this. We use a 2 pronged approach .. firstly a negative epsilon
	// (i.e. we can afford to lose a few verts that are JUST over such a boundary .. this also simplifies such coplanar areas)
	// and secondly we do a check for a contiguous island.

	real_t epsilon = MAX(-_plane_epsilon, -0.001);

	List<HFace>::Element *face_ele = _faces.front();
	while (face_ele) {
		HFace &face = face_ele->get();
		real_t dist = face.plane.distance_to(p_pt_furthest);

		// record for each face whether it faces the eye .. this will be used to detect
		// the edge horizon, which is an edge between a face that does and does not face the eye

		// This epsilon is crucial, see above. DO NOT use zero, as this is more likely to produce
		// lit face 'acne' due to float error. The user should not be able to set this to zero
		face.faces_eye = dist >= epsilon;

		if (face.faces_eye) {
			r_lit_elements.push_back(face_ele);

			SLOWHULL_LOG("\t\tlit face [" + itos(face.id) + "] corns (" + itos(face.corn[0]) + ", " + itos(face.corn[1]) + ", " + itos(face.corn[2]) + ") {dist " + String(Variant(dist)) + "}");
		}

		face_ele = face_ele->next();
	}

	// ensure a contiguous lit island. This step can potentially be removed for more speed,
	// but this CAN occur in *very *rare cases, and will cause the cone to fail, and the hull to be not generated.
	// NYI
}

void SlowHull::find_edge_horizon(const Vector<List<SlowHull::HFace>::Element *> &p_lit_elements, LocalVector<HEdge> &r_edges) {
	LocalVector<HEdge> sorted_edges;

	for (int n = 0; n < p_lit_elements.size(); n++) {
		HFace &face = p_lit_elements[n]->get();

		for (int c = 0; c < 3; c++) {
			if (!face.neigh[c]->faces_eye) {
				// this is an edge horizon!
				HEdge edge;
				edge.set(face.corn[c], face.corn[(c + 1) % 3]);

				// does the edge already exist in the list?
				bool already_on_list = false;

				HEdge sedge = edge;
				sedge.sort();
				for (int i = 0; i < sorted_edges.size(); i++) {
					if (sorted_edges[i] == sedge) {
						already_on_list = true;
						break;
					}
				}

				if (!already_on_list) {
					SLOWHULL_LOG("horizon edge : (" + itos(sedge.corn[0]) + ", " + itos(sedge.corn[1]) + ")");
					r_edges.push_back(edge);
					sorted_edges.push_back(sedge);
				}
			}
		}
	}
}

void SlowHull::split_face_implementation(const HFace &p_face, int p_furthest_point_index) {
	verify_faces();

	const Vector3 &pt_furthest = _source_pts[p_furthest_point_index];

	LocalVector<HEdge> edges;
	Vector<List<SlowHull::HFace>::Element *> lit_elements;

	// find all faces that the point is ahead of. They will all be removed
	find_lit_faces(lit_elements, pt_furthest);

	// now go through the lit faces, and find the edge horizon
	find_edge_horizon(lit_elements, edges);

	// we now have a list of horizon edges, and faces to delete
	// first add the cone faces
	LocalVector<Plane> cone_face_planes;
	List<HFace>::Element *face_ele_before_cone = _faces.back();

	for (int n = 0; n < edges.size(); n++) {
		const HEdge &edge = edges[n];

		_faces.push_back(HFace());
		HFace &new_face = _faces.back()->get();
		new_face.corn[0] = edge.corn[0];
		new_face.corn[1] = edge.corn[1];
		new_face.corn[2] = p_furthest_point_index;
		new_face.calc_plane(*this);

		SLOWHULL_DEVASSERT(!((new_face.corn[0] == p_face.corn[0]) && (new_face.corn[1] == p_face.corn[1]) && (new_face.corn[2] == p_face.corn[2])));

		// we keep a small list of the cone face planes because we use this
		// to prune the the list of face points
		cone_face_planes.push_back(new_face.plane);

		// check orientation of face? NYI
		// if (new_face.plane.is_point_over(_aabb_center))
		// {
		// print_line("center is above plane..");
		// }

		// fill face points
		//fill_new_face(new_face, p_face);
	}

	// create a trimmed list of points
	//	LocalVector<uint32_t> trimmed_pts;
	//	for (int n = 0; n < p_face.pts.size(); n++) {
	//		uint32_t ind = p_face.pts[n];
	//		const Vector3 &pt = _source_pts[ind];

	//		bool trim = true;

	//		// if the point is INSIDE the new cone, it can be trimmed
	//		for (int c = 0; c < cone_face_planes.size(); c++) {
	//			real_t dist = cone_face_planes[c].distance_to(pt);
	//			if (dist >= 0.0) {
	//				trim = false;
	//				break;
	//			}
	//		}

	//		if (!trim) {
	//			trimmed_pts.push_back(ind);
	//		}
	//	}

	// important!! add a plane for the parent face, to limit the whittling
	// to the new exclusion zone. This is just in case the hull is temporarily concave.
	cone_face_planes.push_back(-p_face.plane);

	// whittle the source points
	whittle_source_indices(cone_face_planes);

	// go over the new faces again and fill their face points
	List<HFace>::Element *face_ele = face_ele_before_cone->next();
	while (face_ele) {
		HFace &new_face = face_ele->get();

		// fill face points
		//fill_new_face(new_face, p_face, trimmed_pts);
		fill_new_face(new_face);

		face_ele = face_ele->next();
	}

	// delete the old faces
	for (int n = 0; n < lit_elements.size(); n++) {
		HFace &face = lit_elements[n]->get();

		// remove from neighbours (mainly for debugging)
		for (int neigh = 0; neigh < 3; neigh++) {
			if (face.neigh[neigh]) {
				face.neigh[neigh]->remove_neighbour(&face);
			}
		}

		_faces.erase(lit_elements[n]);
	}

	verify_faces(false);

	// re-establish the neighbours .. kind of slow can be done better later
	if (!reestablish_neighbours()) {
		_error_condition = true;
		//debug_print_faces();
		//		CRASH_COND(!reestablish_neighbours());
		//reestablish_neighbours();
	}
}

bool SlowHull::reestablish_neighbours() {
	// re-establish the neighbours .. kind of slow can be done better later
	List<HFace>::Element *face_ele = _faces.front();
	while (face_ele) {
		HFace &face = face_ele->get();

		for (int c = 0; c < 3; c++) {
			// needs refinding
			if (face.neigh[c] == nullptr) {
				bool found = false;
				//List<HFace>::Element *face2_ele = face_ele->next();
				List<HFace>::Element *face2_ele = _faces.front();

				HEdge edge_to_find;

				edge_to_find.set(face.corn[c], face.corn[(c + 1) % 3]);
				edge_to_find.sort(); // sort so the order is always consistent for neighbour finding

				while (face2_ele) {
					if (face2_ele == face_ele) {
						face2_ele = face2_ele->next();
						continue;
					}

					HFace &neigh_face = face2_ele->get();
					for (int nc = 0; nc < 3; nc++) {
						// only interested in blank
						if (neigh_face.neigh[nc]) {
							continue;
						}

						HEdge edge_to_compare;
						edge_to_compare.set(neigh_face.corn[nc], neigh_face.corn[(nc + 1) % 3]);
						edge_to_compare.sort();

						if (edge_to_find == edge_to_compare) {
							// match!! .. set the neighbours, then stop the search
							neigh_face.neigh[nc] = &face;
							face.neigh[c] = &neigh_face;
							found = true;
							break;
						}
					}

					if (found) {
						break;
					}
					face2_ele = face2_ele->next();
				}

				if (!found) {
					// fatal error condition .. this CRASH can be commented out, but still the
					// hull returned will be not be set.
					SLOWHULL_DEVASSERT(found);
					return false;
				}
			}
		}

		face_ele = face_ele->next();
	}

	return true;
}

void SlowHull::verify_faces(bool p_flag_null_neighbours) {
#ifdef GODOT_SLOWHULL_EXTRA_CHECKS

	List<HFace>::Element *face_ele = _faces.front();
	while (face_ele) {
		const HFace &face = face_ele->get();

		// check neighbours
		for (int n = 0; n < 3; n++) {
			const HFace *neigh = face.neigh[n];

			if (neigh) {
				// each neighbour should have a shared edge
				int shared_corns = 0;
				for (int c = 0; c < 3; c++) {
					int ind = face.corn[c];
					if (neigh->contains_index(ind))
						shared_corns++;
				}
				CRASH_COND(shared_corns != 2);

				bool found = false;
				for (int c = 0; c < 3; c++) {
					if (neigh->neigh[c] == &face) {
						found = true;
						break;
					}
				}
				CRASH_COND(!found);
			} else {
				if (p_flag_null_neighbours) {
					CRASH_COND(!neigh);
				}
			}
		}

		face_ele = face_ele->next();
	}
#endif
}

void SlowHull::verify_face(const HFace &p_face) {
#ifdef GODOT_SLOWHULL_EXTRA_CHECKS
	//	if (p_face.pts.size())
	//		return;

	for (int n = 0; n < _num_source_pts; n++) {
		const Vector3 &pt = _source_pts[n];

		real_t dist = p_face.plane.distance_to(pt);
		CRASH_COND(dist > _plane_epsilon);
	}
#endif
}

//void SlowHull::fill_new_face(HFace &p_new_face, const HFace &p_source_face, const LocalVector<uint32_t> &p_trimmed_pts) {
void SlowHull::fill_new_face(HFace &p_new_face) {
#ifdef GODOT_SLOWHULL_USE_LOGS
	p_new_face.id = _next_id++;
#endif

	// first make plane
	//	p_new_face.plane = Plane(_source_pts[p_new_face.corn[0]], _source_pts[p_new_face.corn[1]], _source_pts[p_new_face.corn[2]]);

	for (int n = 0; n < _source_indices_in_play.size(); n++) {
		int ind = _source_indices_in_play[n];

		//	for (int n = 0; n < p_trimmed_pts.size(); n++) {
		//		int ind = p_trimmed_pts[n];

		//#define GODOT_SLOW_HULL_FILL_FACE_ALL_POINTS
		//#ifdef GODOT_SLOW_HULL_FILL_FACE_ALL_POINTS
		//	for (int n = 0; n < _num_source_pts; n++) {
		//		int ind = n;
		//#else
		//	// add all the points that are above the new face
		//	for (int n = 0; n < p_source_face.pts.size(); n++) {
		//		int ind = p_source_face.pts[n];
		//#endif

		const Vector3 &pt = _source_pts[ind];
		real_t dist = p_new_face.plane.distance_to(pt);

		// epsilon..
		if (dist > _plane_epsilon) {
			// don't add if it is one of the new face corners
			if (!p_new_face.contains_index(ind)) {
				p_new_face.pts.push_back(ind);
			}
		}
	}

	/*	
	if (_use_all_points) {
		for (int n = 0; n < _num_source_pts; n++) {
			int ind = n;
			// don't add if it is one of the new face corners
			bool dont_add = false;

			for (int c = 0; c < 3; c++) {
				if (ind == p_new_face.corn[c]) {
					dont_add = true;
				}
			}
			if (dont_add) {
				continue;
			}

			const Vector3 &pt = _source_pts[ind];
			real_t dist = p_new_face.plane.distance_to(pt);

			// epsilon..
			if (dist > _plane_epsilon) {
				p_new_face.pts.push_back(ind);
			}
		}

	} else {
		for (int n = 0; n < _num_source_pts; n++) {
			int ind = n;
			// don't add if it is one of the new face corners
			bool dont_add = false;

			for (int c = 0; c < 3; c++) {
				if (ind == p_new_face.corn[c]) {
					dont_add = true;
				}
			}
			if (dont_add) {
				continue;
			}

			const Vector3 &pt = _source_pts[ind];
			real_t dist = p_new_face.plane.distance_to(pt);

			// epsilon..
			if (dist > _plane_epsilon) {
				p_new_face.pts.push_back(ind);
			}
		}
	}
*/
	//verify_face(p_new_face);
}

void SlowHull::whittle_source_indices(const LocalVector<Plane> &p_planes, bool p_initial_simplex) {
	// use a negative epsilon because we want to err on the side of deleting less points
	// it doesn't matter if there are few extra in the calculations.
	real_t epsilon = -0.01;

	// go through the source indices in play, and remove any that are BEHIND these planes
	// (the planes are either the initial simplex, or the new cone planes)
	if (p_initial_simplex) {
		_source_indices_in_play.clear();

		// do ALL the source points
		for (uint32_t ind = 0; ind < (uint32_t)_num_source_pts; ind++) {
			const Vector3 &pt = _source_pts[ind];

			bool trim = true;
			for (int p = 0; p < p_planes.size(); p++) {
				real_t dist = p_planes[p].distance_to(pt);
				if (dist > epsilon) {
					trim = false;
					break;
				}
			}

			if (!trim) {
				_source_indices_in_play.push_back(ind);
			}
		}

	} else {
		for (int n = 0; n < _source_indices_in_play.size(); n++) {
			uint32_t ind = _source_indices_in_play[n];
			const Vector3 &pt = _source_pts[ind];

			bool trim = true;
			for (int p = 0; p < p_planes.size(); p++) {
				real_t dist = p_planes[p].distance_to(pt);
				if (dist > epsilon) {
					trim = false;
					break;
				}
			}

			if (trim) {
				_source_indices_in_play.remove_unordered(n);
				n--; // we need to repeat this member because it will now be the last in the list next time through the loop
			}
		}
	}
}

void SlowHull::debug_print_faces(bool p_print_positions) {
	int count = 0;
	print_line("");

	List<HFace>::Element *face_ele = _faces.front();
	while (face_ele) {
		const HFace &face = face_ele->get();

		String sz = "\tface " + itos(count) + " : ";
#ifdef GODOT_SLOWHULL_USE_LOGS
		sz += "[" + itos(face.id) + "] ";
#endif

		Vector3 pt;
		for (int n = 0; n < 3; n++) {
			if (p_print_positions) {
				pt = _source_pts[face.corn[n]];
				sz += itos(pt.x) + ", ";
				sz += itos(pt.y) + ", ";
				sz += itos(pt.z) + ", ";
			} else {
				sz += itos(face.corn[n]) + ", ";
			}
		}

		sz += " ... plane " + String(Variant(face.plane));

		// neighbours
#ifdef GODOT_SLOWHULL_USE_LOGS
		sz += "\tneighs : ";
		for (int n = 0; n < 3; n++) {
			if (face.neigh[n]) {
				sz += itos(face.neigh[n]->id);
			} else {
				sz += "-";
			}
			sz += ", ";
		}
#endif
		print_line(sz);

		face_ele = face_ele->next();

		count++;
	}
}

PoolVector3Array SlowHull::build_hull(PoolVector3Array p_points, int p_max_iterations, real_t p_simplify, int p_method) {
	Vector<Vector3> pts;
	pts.resize(p_points.size());
	for (int n = 0; n < pts.size(); n++) {
		pts.set(n, p_points[n]);
	}

	// compare speed against quickhull
	Geometry::MeshData md_a;

	uint64_t before = OS::get_singleton()->get_ticks_usec();
	QuickHull::build(pts, md_a);
	uint64_t after = OS::get_singleton()->get_ticks_usec();
	uint64_t time_quick_hull = after - before;

	for (int f = 0; f < md_a.faces.size(); f++) {
		Geometry::MeshData::Face face = md_a.faces[f];
		if (face.indices.size() > 3) {
			print_line("quickhull face " + itos(f) + " contains " + itos(face.indices.size()) + " indices");
		}
	}

	Geometry::MeshData md_b = md_a;
	uint64_t before2 = OS::get_singleton()->get_ticks_usec();
	build(pts, md_b, p_max_iterations);
	uint64_t after2 = OS::get_singleton()->get_ticks_usec();
	uint64_t time_slow_hull = after2 - before2;

	if (p_simplify > 0.0) {
		ConvexMeshSimplify3D simp;
		before = OS::get_singleton()->get_ticks_usec();
		simp.simplify(md_b, p_simplify);
		after = OS::get_singleton()->get_ticks_usec();
		print_line("simplification time : " + itos(after - before));
	}

	// bullet
	Geometry::MeshData md_c;
	uint64_t before3 = OS::get_singleton()->get_ticks_usec();
	ConvexHullComputer::convex_hull(pts, md_c);
	uint64_t after3 = OS::get_singleton()->get_ticks_usec();
	uint64_t time_bullet_hull = after3 - before3;

	print_line("bullet hull : " + itos(time_bullet_hull) + ", num points : " + itos(md_c.vertices.size()) + ", num faces " + itos(md_c.faces.size()));
	print_line("quick hull : " + itos(time_quick_hull) + ", num points : " + itos(md_a.vertices.size()) + ", num faces " + itos(md_a.faces.size()));
	print_line("slow hull : " + itos(time_slow_hull) + ", num points : " + itos(md_b.vertices.size()) + ", num faces " + itos(md_b.faces.size()));

	const Geometry::MeshData *md;
	switch (p_method) {
		default: {
			md = &md_b;
		} break;
		case 1: {
			md = &md_a;
		} break;
		case 2: {
			md = &md_c;
		} break;
	}

	PoolVector3Array output;
	// translate output
	for (int f = 0; f < md->faces.size(); f++) {
		Geometry::MeshData::Face face = md->faces[f];

		for (int t = 0; t < face.indices.size() - 2; t++) {
			int i0 = face.indices[0];
			int i1 = face.indices[t + 1];
			int i2 = face.indices[t + 2];

			const Vector3 &a = md->vertices[i0];
			const Vector3 &b = md->vertices[i1];
			const Vector3 &c = md->vertices[i2];

			output.push_back(a);
			output.push_back(b);
			output.push_back(c);

			//			print_line("tri " + itos(t));
			//			print_line("\t" + itos(i0) + " : " + String(Variant(a)) + ", " + itos(i1) + " : " + String(Variant(b)) + ", " + itos(i2) + " : " + String(Variant(c)));
		}
	}

	// check for same result
	//	_use_all_points = false;
	//	Geometry::MeshData md_a;
	//	build(pts, md_a);
	//	_use_all_points = true;
	//	Geometry::MeshData md_b;
	//	build(pts, md_b);

	//	if (md_a.vertices.size() != md_b.vertices.size()) {
	//		print_line("verts not equal");
	//	}

	return output;
}

bool SlowHull::build(const Vector<Vector3> &p_pts, Geometry::MeshData &r_mesh, int p_max_iterations, real_t p_plane_epsilon) {
	//	ConvexMeshSimplify3D simp;
	//	return simp.simplify(r_mesh);

	clear();
	_plane_epsilon = p_plane_epsilon;

	// just for safety, who knows what is in the mesh data before this is called
	r_mesh.vertices.clear();
	r_mesh.faces.clear();
	r_mesh.edges.clear();

	if (!p_pts.size()) {
		return false;
	}

	_source_pts = &p_pts[0];
	_num_source_pts = p_pts.size();

	_aabb.create_from_points(p_pts);
	_aabb_center = _aabb.get_center();

	create_initial_simplex();

	int count = 0;
	List<HFace>::Element *face_ele = _faces.front();
	while (face_ele) {
		//debug_print_faces();
		face_ele = split_face(face_ele);
		count++;

		if (count == p_max_iterations) {
			break;
		}

		if (_error_condition) {
			break;
		}

		if ((count % 100) == 0) {
			print_line("split_faces " + itos(count));
		}
	}

	if (_error_condition) {
		return false;
	}

	// output
	LocalVector<uint32_t> out_inds;

	face_ele = _faces.front();
	while (face_ele) {
		HFace &face = face_ele->get();

		face.corn[0] = get_or_create_output_index(out_inds, face.corn[0]);
		face.corn[1] = get_or_create_output_index(out_inds, face.corn[1]);
		face.corn[2] = get_or_create_output_index(out_inds, face.corn[2]);

		face_ele = face_ele->next();
	}

	// write output vertices
	r_mesh.vertices.resize(out_inds.size());
	for (int n = 0; n < out_inds.size(); n++) {
		r_mesh.vertices.set(n, _source_pts[out_inds[n]]);
	}

	// write faces

	verify_faces();

	// a coplanar face compromising multiple HFaces...
	// these must form a CONVEX face.
	HMultiFace multiface;

	face_ele = _faces.front();
	while (face_ele) {
		HFace &face = face_ele->get();
		// CRASH_COND(face.pts.size());
		// verify_face(face);

		//		if (true) {
		if (!face.done) {
			multiface.reset();
			_check_id++;

			find_multiface_recursive(multiface, face, &r_mesh.vertices[0]);
			fill_output_from_multiface(r_mesh, multiface);

			//			Geometry::MeshData::Face f;
			//			fill_output_face_recursive(r_mesh.faces.size(), f, face);
			//			r_mesh.faces.push_back(f);
		}

		// naive edges
		//		Geometry::MeshData::Edge e;

		//		e.a = face.corn[0];
		//		e.b = face.corn[1];
		//		r_mesh.edges.push_back(e);
		//		e.a = face.corn[1];
		//		e.b = face.corn[2];
		//		r_mesh.edges.push_back(e);
		//		e.a = face.corn[2];
		//		e.b = face.corn[0];
		//		r_mesh.edges.push_back(e);

		face_ele = face_ele->next();
	}

	// merge coplanar faces BEFORE creating edges
	//merge_coplanar_faces(r_mesh);

	/*
	for (int n = 0; n < r_mesh.faces.size(); n++) {
		Geometry::MeshData::Face face = r_mesh.faces[n];

		// the face plane now needs averaging
		//		int num_face_tris = face.indices.size() - 2;
		//		if (num_face_tris > 1) {
		//			face.plane.normal.normalize();
		//			face.plane.d /= real_t(num_face_tris);
		//		}
		if (face.indices.size() > 3) {
			make_poly_convex(face.plane.normal, face.indices, r_mesh.vertices);

			// sort the indices according to the standard winding order
			//sort_polygon_winding(face.plane.normal, face.indices, r_mesh.vertices);
		}

		r_mesh.faces.set(n, face);
	}
*/

	// do a final reduce of the number of output vertices (to account for convex faces that deleted verts)
	if (_param_final_reduce) {
		out_inds.reset();

		for (int n = 0; n < r_mesh.faces.size(); n++) {
			Geometry::MeshData::Face face = r_mesh.faces[n];

			for (int i = 0; i < face.indices.size(); i++) {
				int old_ind = face.indices[i];
				int new_ind = get_or_create_output_index(out_inds, old_ind);
				face.indices.set(i, new_ind);
			}

			r_mesh.faces.set(n, face);
		}

		// rejig the final vertices
		Vector<Vector3> temp_verts = r_mesh.vertices;
		r_mesh.vertices.clear();
		r_mesh.vertices.resize(out_inds.size());
		for (int n = 0; n < out_inds.size(); n++) {
			r_mesh.vertices.set(n, temp_verts[out_inds[n]]);
		}
	} // final reduce

	// finally edges
	for (int n = 0; n < r_mesh.faces.size(); n++) {
		Geometry::MeshData::Face face = r_mesh.faces[n];

		for (int c = 0; c < face.indices.size(); c++) {
			Geometry::MeshData::Edge e;
			e.a = face.indices[c];
			e.b = face.indices[(c + 1) % face.indices.size()];

			r_mesh.edges.push_back(e);
		}
	}

	//	if (_param_simplify) {
	//		ConvexMeshSimplify3D simp;
	//		return simp.simplify(r_mesh);
	//	}

	return true;
}

bool SlowHull::is_multiface_convex(const HMultiFace &p_multiface, const HFace &p_face) {
	return false;
}

void SlowHull::find_multiface_recursive(HMultiFace &r_multiface, HFace &p_face, const Vector3 *p_source_pts) {
	// mark as hit in this multiface check
	p_face.check_id = _check_id;

	if (!r_multiface.exfaces.size()) {
		r_multiface.plane = p_face.plane;
		r_multiface.push_face(&p_face, _source_pts);
		//return;
	} else {
		// see if we can add
		// do the faces match?
		const Plane &pa = r_multiface.plane;
		const Plane &pb = p_face.plane;

		real_t dist_diff = Math::abs(pa.d - pb.d);
		if (dist_diff > _coplanar_dist_epsilon) {
			return;
		}

		real_t dot = pa.normal.dot(pb.normal);

		if (dot < _coplanar_dot_epsilon) {
			return;
		}

		// only do if not in the list already
		// this is now already done with the check id
		//		for (int n = 0; n < r_multiface.faces.size(); n++) {
		//			if (&p_face == r_multiface.faces[n]) {
		//				return;
		//			}
		//		}

		// if we failed to push, is not convex or will not form a triangle fan
		// without changing topology
		if (!r_multiface.push_face(&p_face, p_source_pts)) {
			return;
		}
	}

	// try neighbours
	for (int n = 0; n < 3; n++) {
		HFace *neigh = p_face.neigh[n];
		if (neigh && (neigh->check_id != _check_id) && !neigh->done) {
			find_multiface_recursive(r_multiface, *neigh, p_source_pts);
		}
	}
}

// return true if succeeded, false, if didn't include the first face (the initial face)
bool SlowHull::try_add_multiface_convex(Geometry::MeshData &r_md, HMultiFace &r_multiface) {
	//void make_poly_convex(const Vector3 &p_poly_normal, Vector<int> &r_indices, const Vector<Vector3> &p_vertices) const;

	return true;
}

void SlowHull::fill_output_from_multiface(Geometry::MeshData &r_mesh, const HMultiFace &p_multiface) {
	Geometry::MeshData::Face f;
	f.plane = p_multiface.plane;

	f.indices.push_back(p_multiface.base);

	for (int n = 0; n < p_multiface.flares.size(); n++) {
		f.indices.push_back(p_multiface.flares[n]);
	}

	r_mesh.faces.push_back(f);

	// mark as done
	for (int n = 0; n < p_multiface.exfaces.size(); n++) {
		p_multiface.exfaces[n].hface->done = true;
	}
}

void SlowHull::fill_output_face_recursive(int p_final_face_id, Geometry::MeshData::Face &r_out_face, HFace &p_first_face, HFace *p_parent_face, HFace *p_face) {
	if (!p_face) {
		r_out_face.plane = p_first_face.plane;
		r_out_face.indices.push_back(p_first_face.corn[0]);
		r_out_face.indices.push_back(p_first_face.corn[1]);
		r_out_face.indices.push_back(p_first_face.corn[2]);
		p_first_face.done = true;

		DEV_ASSERT(p_first_face.final_face_id == -1);
		p_first_face.final_face_id = p_final_face_id;

		// seed from the first face
		p_face = &p_first_face;

		if (!_param_recursive_face)
			return;
	} else {
		// is coplanar?
		// do the faces match?
		const Plane &pa = p_first_face.plane;
		const Plane &pb = p_face->plane;

		real_t dist_diff = Math::abs(pa.d - pb.d);
		if (dist_diff > _coplanar_dist_epsilon) {
			return;
		}

		real_t dot = pa.normal.dot(pb.normal);

		if (dot < _coplanar_dot_epsilon) {
			return;
		}

		// they match, add the extra point in neigh face that isn't in the parent
		DEV_ASSERT(p_parent_face);

		// mark as done (i.e. added to a face, and present in the final mesh)
		p_face->done = true;
		p_face->final_face_id = p_final_face_id;

		// add any indices not already in the output face
		for (int n = 0; n < 3; n++) {
			int ind = p_face->corn[n];
			if (r_out_face.indices.find(ind) == -1) {
				r_out_face.indices.push_back(ind);
			}
		}
		/*
		// first assess how many neighbour are already part of the final (output) face
		uint32_t neigh_done_bitfield = 0;
		int neighs_done_already = 0;
		int not_done_neigh = 0; // just a handy way of recording which of 3 is not done
		for (int n = 0; n < 3; n++) {
			if (p_face->neigh[n]->final_face_id == p_final_face_id) {
				neigh_done_bitfield |= (1 << n);
				neighs_done_already++;
			} else {
				not_done_neigh = n;
			}
		}

		// now we need to deal with 3 conditions.
		// 1 existing neighbour that is part of the final face .. add the missing corner
		if (neighs_done_already == 1) {
			for (int n = 0; n < 3; n++) {
				int ind = p_face->corn[n];
				if (p_parent_face->find_index(ind) == -1) {
					// this is a new index, add
					r_out_face.indices.push_back(ind);
				}
			}
		}

		// 2 existing neighbours that are part of the final face .. remove the shared corner
		if (neighs_done_already == 2) {
			int corn_to_remove = (not_done_neigh + 2) % 3;
			int ind = p_face->corn[corn_to_remove];

			// remove index from out face
			r_out_face.indices.erase(ind);
		}

		// 3 existing neighbours that are part of the final face (may not occur) .. remove ALL the corners
		if (neighs_done_already == 3) {
			for (int n = 0; n < 3; n++) {
				int ind = p_face->corn[n];

				// remove index from out face
				r_out_face.indices.erase(ind);
			}
		}
		*/

		// add to the plane to get an average plane
		//		r_out_face.plane.normal += p_face->plane.normal;
		//		r_out_face.plane.d += p_face->plane.d;
	}

	// try the neighbours recursively
	DEV_ASSERT(p_face);
	for (int n = 0; n < 3; n++) {
		HFace *neigh = p_face->neigh[n];
		if (neigh && !neigh->done) {
			// p_neigh_face now becomes the parent on the next call
			fill_output_face_recursive(p_final_face_id, r_out_face, p_first_face, p_face, neigh);
		}
	}
}

void SlowHull::make_poly_convex(const Vector3 &p_poly_normal, Vector<int> &r_indices, const Vector<Vector3> &p_vertices) const {
	// can't do
	if (r_indices.size() < 3) {
		return;
	}

	LocalVector<int> inds;
	Vector<int> remaining_inds = r_indices;

	bool found = false;

	// first find a start edge
	for (int n = 0; n < r_indices.size(); n++) {
		uint32_t ind_a = r_indices[n];

		for (int m = n + 1; m < r_indices.size(); m++) {
			uint32_t ind_b = r_indices[m];

			if (is_poly_edge(ind_a, ind_b, p_poly_normal, r_indices, p_vertices)) {
				inds.push_back(ind_a);
				inds.push_back(ind_b);
				found = true;
				remaining_inds.erase(ind_b);
				break;
			}

			if (is_poly_edge(ind_b, ind_a, p_poly_normal, r_indices, p_vertices)) {
				inds.push_back(ind_b);
				inds.push_back(ind_a);
				found = true;
				remaining_inds.erase(ind_a);
				break;
			}
		}

		// done
		if (found) {
			break;
		}
	}

	// now go through the remaining indices looking for the next edge until we reach the first again
	bool finished = false;
	while (!finished) {
		int tail_ind = inds[inds.size() - 1];
		int join_ind = -1;

		for (int n = 0; n < remaining_inds.size(); n++) {
			int ind = remaining_inds[n];
			if (is_poly_edge(tail_ind, ind, p_poly_normal, r_indices, p_vertices)) {
				// found a new edge
				join_ind = ind;
				break;
			}
		}

		CRASH_COND(join_ind == -1);
		if (join_ind != -1) {
			// end condition
			if (join_ind == inds[0]) {
				finished = true;
				break;
			} else {
				inds.push_back(join_ind);
				remaining_inds.erase(join_ind);
			}
		}
	}

	// by this point inds should contain a new list of indices, sorted in winding order
	int removed = r_indices.size() - inds.size();
	if (removed) {
		print_line("make_poly_convex removed " + itos(removed) + " indices.");
	}

	r_indices = inds;
}

bool SlowHull::is_poly_edge(int p_ind_a, int p_ind_b, const Vector3 &p_poly_normal, Vector<int> &r_indices, const Vector<Vector3> &p_vertices) const {
	if (p_ind_a == p_ind_b) {
		return false;
	}

	// construct a plane for the edge
	const Vector3 &a = p_vertices[p_ind_a];
	const Vector3 &b = p_vertices[p_ind_b];
	const Vector3 c = a + p_poly_normal;

	Plane plane = Plane(a, b, c);

	// if all the points are on one side of the plane (aside from a and b) then it forms an edge
	for (int n = 0; n < r_indices.size(); n++) {
		int ind = r_indices[n];
		if ((ind == p_ind_a) || (ind == p_ind_b)) {
			continue;
		}

		const Vector3 &pt = p_vertices[ind];

		real_t dist = plane.distance_to(pt);

		if (dist > 0.0)
			return false;
	}

	return true;
}

void SlowHull::sort_polygon_winding(const Vector3 &p_poly_normal, Vector<int> &r_indices, const Vector<Vector3> &p_vertices) const {
	// cannot sort less than 3 verts
	if (r_indices.size() < 3) {
		return;
	}

	// find centroid
	int num_points = r_indices.size();
	Vector3 pt_center_world = Vector3(0, 0, 0);

	//	Vector<Vector3> pts;
	//	pts.resize(r_indices.size());

	for (int n = 0; n < num_points; n++) {
		//		pts.set(n,  p_vertices[r_indices[n]]);
		pt_center_world += p_vertices[r_indices[n]];
	}
	pt_center_world /= num_points;
	/////////////////////////////////////////

	// now algorithm
	for (int n = 0; n < num_points - 2; n++) {
		const Vector3 &pt = p_vertices[r_indices[n]];
		Vector3 a = pt - pt_center_world;
		a.normalize();

		Plane plane = Plane(pt, pt_center_world, pt_center_world + p_poly_normal);

		double smallest_angle = -1;
		int smallest = -1;

		for (int m = n + 1; m < num_points; m++) {
			Vector3 pt_m = p_vertices[r_indices[m]];

			if (plane.distance_to(pt_m) > 0.0) {
				Vector3 b = pt_m - pt_center_world;
				b.normalize();

				double angle = a.dot(b);

				if (angle > smallest_angle) {
					smallest_angle = angle;
					smallest = m;
				}
			} // which side

		} // for m

		// swap smallest and n+1 vert
		if (smallest != -1) {
			int temp = r_indices[smallest];
			r_indices.set(smallest, r_indices[n + 1]);
			r_indices.set(n + 1, temp);

			//			Vector3 temp2 = pts[smallest];
			//			pts.set(smallest, pts[n+1]);
			//			pts.set(n+1, temp2);
		}
	} // for n

	// the vertices are now sorted, but may be in the opposite order to that wanted.
	// we detect this by calculating the normal of the poly, then flipping the order if the normal is pointing
	// the wrong way.
	Plane plane = Plane(p_vertices[r_indices[0]], p_vertices[r_indices[1]], p_vertices[r_indices[2]]);

	if (p_poly_normal.dot(plane.normal) < 0.0f) {
		// reverse winding order of verts
		r_indices.invert();
	}
}

uint32_t SlowHull::get_or_create_output_index(LocalVector<uint32_t> &r_out_inds, uint32_t p_in_index) {
	// exists already?
	for (uint32_t n = 0; n < r_out_inds.size(); n++) {
		if (r_out_inds[n] == p_in_index) {
			return n;
		}
	}

	// add
	r_out_inds.push_back(p_in_index);
	return r_out_inds.size() - 1;
}

///////////////////////////////////////////////////////////////////
// main simplification function for a convex mesh
bool ConvexMeshSimplify3D::simplify(Geometry::MeshData &r_mesh, real_t p_simplification) {
	// we can't have no simplification, because otherwise we get multiple planes
	// in exact same position, and all the maths mucks up. The error goes up exponentially
	// the closer to each other than planes are allowed, so most stable behaviour
	// is with about 0.14 or more simplification.
	real_t s = CLAMP(p_simplification, 0.01, 1.0);
	s *= s;
	s = 1.0 - s;
	_coplanar_dot_epsilon = s;

	// just for reference in case we later want to use degrees...
	// _plane_simplify_dot = Math::cos(Math::deg2rad(_plane_simplify_degrees));

	print_line("simplify dot is " + String(Variant(_coplanar_dot_epsilon)));

	// first stage is to extract similar planes. each plane will form a final face
	fill_initial_planes(r_mesh);

	create_verts();

	if (!_verts.size()) {
		return false;
	}

	// now sort the face verts winding order
	sort_winding_order();

	// output
	output_mesh(r_mesh);

	return true;
}

void ConvexMeshSimplify3D::output_mesh(Geometry::MeshData &r_mesh) {
	// clear the mesh
	r_mesh.vertices.clear();
	r_mesh.edges.clear();
	r_mesh.faces.clear();

	r_mesh.vertices = _verts;

	for (int n = 0; n < _faces.size(); n++) {
		CFace &cface = _faces[n];
		Geometry::MeshData::Face face;
		face.plane = cface.plane;
		face.indices = cface.inds;
		r_mesh.faces.push_back(face);

		// edges (not unique yet)
		for (int n = 0; n < cface.inds.size(); n++) {
			Geometry::MeshData::Edge edge;
			edge.a = cface.inds[n];
			edge.b = cface.inds[(n + 1) % cface.inds.size()];

			r_mesh.edges.push_back(edge);
		}

		//		if (n >= 0)
		//			break;
	}
}

void ConvexMeshSimplify3D::sort_winding_order() {
	for (int n = 0; n < _faces.size(); n++) {
		CFace &face = _faces[n];
		sort_polygon_winding(face.plane.normal, face.inds, &_verts[0], _verts.size());
	}
}

int ConvexMeshSimplify3D::find_or_add_vert(const Vector3 &p_pos) {
	for (int n = 0; n < _verts.size(); n++) {
		if (vertex_approx_equal(_verts[n], p_pos)) {
			return n;
		}
	}

	int ind = _verts.size();
	_verts.push_back(p_pos);
	return ind;
}

void ConvexMeshSimplify3D::create_verts() {
	// now create verts from 3 plane intersections
	int plane_count = _faces.size();

	// Iterate through every unique combination of any three planes.
	Vector3 pt;

	const real_t epsilon = 0.0001;

	for (int i = 0; i < plane_count; i++) {
		for (int j = i + 1; j < plane_count; j++) {
			for (int k = j + 1; k < plane_count; k++) {
				const Plane &pi = _faces[i].plane;
				const Plane &pj = _faces[j].plane;
				const Plane &pk = _faces[k].plane;

				if (pi.intersect_3(pj, pk, &pt)) {
					// See if any *other* plane excludes this point because it's
					// on the wrong side.
					bool excluded = false;
					for (int n = 0; n < plane_count; n++) {
						real_t dist = _faces[n].plane.distance_to(pt);
						if (dist > epsilon) {
							if (n != i && n != j && n != k) {
								excluded = true;
								break;
							}
						}
					}

					// Only add the point if it passed all tests.
					if (!excluded) {
						int ind = find_or_add_vert(pt);

						// save the point to each face
						_faces[i].push_index(ind);
						_faces[j].push_index(ind);
						_faces[k].push_index(ind);
					}
				}

			} // for k
		} // for j
	} // for i
}

void ConvexMeshSimplify3D::fill_initial_planes(Geometry::MeshData &r_mesh) {
	for (int n = 0; n < r_mesh.faces.size(); n++) {
		const Geometry::MeshData::Face &face = r_mesh.faces[n];
		const Plane &pa = face.plane;

		//Plane test = Plane(r_mesh.vertices[face.indices[0]], r_mesh.vertices[face.indices[1]], r_mesh.vertices[face.indices[2]]);

		bool found = false;

		// try this plane
		for (int f = 0; f < _faces.size(); f++) {
			CFace &cface = _faces[f];
			const Plane &pb = cface.plane;

			real_t dist_diff = Math::abs(pa.d - pb.d);
			if (dist_diff > _coplanar_dist_epsilon)
				continue;

			real_t dot = pa.normal.dot(pb.normal);

			if (dot < _coplanar_dot_epsilon)
				continue;

			// match!
			cface.running_plane.normal += pa.normal;
			cface.running_plane.d += pa.d;
			cface.running_tris += 1;
			found = true;
			break;
		}

		// first occurrence of this plane
		if (!found) {
			_faces.push_back(CFace());
			CFace &cface = _faces[_faces.size() - 1];

			cface.plane = pa;
			cface.running_plane = pa;
			cface.running_tris = 1;
		}
	}

	// average the planes
	for (int n = 0; n < _faces.size(); n++) {
		CFace &face = _faces[n];
		face.plane.normal = face.running_plane.normal.normalized();
		face.plane.d = face.running_plane.d /= face.running_tris;

		//		print_line("simplify plane " + itos(n) + " is " + String(Variant(face.plane)));
	}
}

bool ConvexMeshSimplify3D::sort_polygon_winding(const Vector3 &p_poly_normal, Vector<int> &r_indices, const Vector3 *p_vertices, int p_num_verts) const {
	// cannot sort less than 3 verts
	if (r_indices.size() < 3) {
		return false;
	}

	// find centroid
	int num_points = r_indices.size();
	Vector3 pt_center_world = Vector3(0, 0, 0);

	//	Vector<Vector3> pts;
	//	pts.resize(r_indices.size());

	for (int n = 0; n < num_points; n++) {
		//		pts.set(n,  p_vertices[r_indices[n]]);
		CRASH_COND(r_indices[n] > p_num_verts);
		pt_center_world += p_vertices[r_indices[n]];
	}
	pt_center_world /= num_points;
	/////////////////////////////////////////

	// now algorithm
	for (int n = 0; n < num_points - 2; n++) {
		const Vector3 &pt = p_vertices[r_indices[n]];
		Vector3 a = pt - pt_center_world;
		a.normalize();

		Plane plane = Plane(pt, pt_center_world, pt_center_world + p_poly_normal);

		double smallest_angle = -1;
		int smallest = -1;

		for (int m = n + 1; m < num_points; m++) {
			Vector3 pt_m = p_vertices[r_indices[m]];

			if (plane.distance_to(pt_m) > 0.0) {
				Vector3 b = pt_m - pt_center_world;
				b.normalize();

				double angle = a.dot(b);

				if (angle > smallest_angle) {
					smallest_angle = angle;
					smallest = m;
				}
			} // which side

		} // for m

		// swap smallest and n+1 vert
		if (smallest != -1) {
			int temp = r_indices[smallest];
			r_indices.set(smallest, r_indices[n + 1]);
			r_indices.set(n + 1, temp);

			//			Vector3 temp2 = pts[smallest];
			//			pts.set(smallest, pts[n+1]);
			//			pts.set(n+1, temp2);
		}
	} // for n

	// the vertices are now sorted, but may be in the opposite order to that wanted.
	// we detect this by calculating the normal of the poly, then flipping the order if the normal is pointing
	// the wrong way.
	Plane plane = Plane(p_vertices[r_indices[0]], p_vertices[r_indices[1]], p_vertices[r_indices[2]]);

	if (p_poly_normal.dot(plane.normal) < 0.0f) {
		// reverse winding order of verts
		r_indices.invert();
	}

	return true;
}
