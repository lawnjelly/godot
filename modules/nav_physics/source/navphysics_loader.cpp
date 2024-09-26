#include "navphysics_loader.h"
#include "navphysics_rect.h"

#include <cstring>

namespace NavPhysics {

void Loader::find_index_nexts(Mesh &r_dest) {
	for (u32 p = 0; p < r_dest.get_num_polys(); p++) {
		const Poly &poly = r_dest.get_poly(p);

		u32 num_poly_inds = poly.num_inds;

		for (u32 i = 0; i < num_poly_inds; i++) {
			u32 next = (i + 1) % num_poly_inds;
			next += poly.first_ind;

			next = r_dest.get_ind(next);
			r_dest._inds_next.push_back(next);
		} // for i
	} // for p
}

u32 Loader::find_or_create_vert(Mesh &r_dest, const FPoint3 &p_pt) {
	Vector<FPoint3> &dverts = r_dest._fverts3;

	// Slow for now...
	for (u32 n = 0; n < dverts.size(); n++) {
		if (dverts[n].is_equal_approx(p_pt)) {
			return n;
		}
	}

	// Not found .. add.
	dverts.push_back(p_pt);
	r_dest._fverts.push_back(FPoint2::make(p_pt.x, p_pt.z));
	return dverts.size() - 1;
}

void Loader::plane_from_poly_newell(Mesh &r_dest, Poly &r_poly) const {
	int num_points = r_poly.num_inds;

	if (num_points < 3) {
		r_poly.plane.zero();
		return;
	}

	FPoint3 normal;
	FPoint3 center;

	for (int i = 0; i < num_points; i++) {
		int j = (i + 1) % num_points;

		u32 ind_i = r_dest._inds[r_poly.first_ind + i];
		u32 ind_j = r_dest._inds[r_poly.first_ind + j];

		const FPoint3 &pi = r_dest._fverts3[ind_i];
		const FPoint3 &pj = r_dest._fverts3[ind_j];

		center += pi;

		normal.x += (((pi.z) + (pj.z)) * ((pj.y) - (pi.y)));
		normal.y += (((pi.x) + (pj.x)) * ((pj.z) - (pi.z)));
		normal.z += (((pi.y) + (pj.y)) * ((pj.x) - (pi.x)));
	}

	normal.normalize();
	center /= num_points;

	// point and normal
	r_poly.plane.set(center, normal);
}

bool Loader::load_polys(const SourceMeshData &p_mesh, Mesh &r_dest) {
	if (!p_mesh.num_verts)
		return false;
	if (!p_mesh.num_indices)
		return false;

	// Assuming polys are tris.
	u32 num_polys = p_mesh.num_indices / 3;

	r_dest._polys.resize(num_polys);
	r_dest._polys_extra.resize(num_polys);

	u32 index_count = 0;

	for (u32 n = 0; n < num_polys; n++) {
		Poly &dpoly = r_dest._polys[n];
		dpoly.init();

		dpoly.first_ind = r_dest._inds.size();
		dpoly.center3.zero();

		u32 num_inds = p_mesh.poly_num_indices[n];

		for (u32 v = 0; v < num_inds; v++) {
			u32 ind = p_mesh.indices[index_count++];

			NP_DEV_ASSERT(ind < p_mesh.num_verts);
			const FPoint3 &pt = p_mesh.verts[ind];

			r_dest._inds.push_back(find_or_create_vert(r_dest, pt));
			dpoly.center3 += pt;
		}

		dpoly.num_inds = num_inds;
		if (num_inds) {
			dpoly.center3 /= (float)num_inds;
		}

		// Clockwise flag not yet dealt with...
		plane_from_poly_newell(r_dest, dpoly);
	}

	return true;
}

void Loader::load_fixed_point_verts(Mesh &r_dest) {
	const Vector<FPoint2> &sverts = r_dest._fverts;

	// first find the offset and scale
	u32 num_verts = sverts.size();

	if (!num_verts) {
		return;
	}

	Rect2 rect;
	rect.position = sverts[0];

	for (u32 i = 1; i < num_verts; i++) {
		rect.expand_to(sverts[i]);
	}

	log(String("Internal Map AABB is ") + String(rect.position) + ", " + String(rect.size));

	r_dest._f32_to_fp_scale = FPoint2::make(FPoint2::FP_RANGE / rect.size.x, FPoint2::FP_RANGE / rect.size.y);
	r_dest._f32_to_fp_offset = rect.position;

	r_dest._fp_to_f32_offset = rect.position;
	r_dest._fp_to_f32_scale = FPoint2::make(rect.size.x / FPoint2::FP_RANGE, rect.size.y / FPoint2::FP_RANGE);

	for (u32 i = 0; i < num_verts; i++) {
		const FPoint2 &v = sverts[i];
		IPoint2 vfp = r_dest.float_to_fixed_point_2(v);
		// print("vert " + str(i) + " : " + vfp.sz())
		r_dest._verts.push_back(vfp);

		// var verify = _dmap._fpvec2_to_float(vfp)
		// var l = (verify- v).length()
		// print("length " + str(l))
	}

	// find the poly fixed point centers
	Vector<Poly> &dpolys = r_dest._polys;

	for (u32 n = 0; n < dpolys.size(); n++) {
		Poly &dpoly = dpolys[n];
		dpoly.center = r_dest.float_to_fixed_point_2(FPoint2::make(dpoly.center3.x, dpoly.center3.z));
	}
}

void Loader::find_links(Mesh &r_dest) {
}

void Loader::find_walls(Mesh &r_dest) {
}

void Loader::find_bottlenecks(Mesh &r_dest) {
}

//Loader::SourceMeshData Loader::save_mesh(const Mesh &p_mesh) {
//	SourceMeshData d;
//	const MeshSourceData &s = p_mesh._source_data;

//	d.num_verts = s.verts.size();
//	d.verts = s.verts.ptr();

//	d.num_indices = s.inds.size();
//	d.indices = s.inds.ptr();

//	d.num_polys = s.poly_num_indices.size();
//	d.poly_num_indices = s.poly_num_indices.ptr();

//	return d;
//}

bool Loader::load_mesh(const SourceMeshData &p_source_mesh, Mesh &r_mesh) {
	r_mesh.clear();

	//	MeshSourceData &d = r_mesh._source_data;
	//	const SourceMeshData &s = p_source_mesh;

	if (!p_source_mesh.verts)
		return false;
	if (!p_source_mesh.indices)
		return false;

	if (!load_polys(p_source_mesh, r_mesh)) {
		goto failed;
	}

	find_index_nexts(r_mesh);
	load_fixed_point_verts(r_mesh);
	find_links(r_mesh);
	find_walls(r_mesh);
	find_bottlenecks(r_mesh);

	log("NavPhysics loaded mesh:");
	log(String("\tpolys: ") + r_mesh.get_num_polys());
	log(String("\tverts: ") + r_mesh.get_num_verts());
	log(String("\tinds: ") + r_mesh.get_num_inds());

	// Backup source data
	//	d.verts.resize(s.num_verts);
	//	memcpy(d.verts.ptr(), s.verts, s.num_verts * sizeof(FPoint3));

	//	d.inds.resize(s.num_indices);
	//	memcpy(d.inds.ptr(), s.indices, s.num_indices * sizeof(u32));

	//	d.poly_num_indices.resize(s.num_polys);
	//	memcpy(d.poly_num_indices.ptr(), s.poly_num_indices, s.num_polys * sizeof(u32));

	return true;

failed:
	r_mesh.clear();
	return false;
}

} // namespace NavPhysics
