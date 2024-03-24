#include "lunmerger.h"
#include "lmerger.h"

namespace LM {

UnMerger::UnMerger() {
	set_unmerge_params(0.1f, 0.95f);
}

void UnMerger::set_unmerge_params(float thresh_dist, float thresh_dot) {
	_merge_params.threshold_dist = thresh_dist;
	_merge_params.threshold_dist_squared = thresh_dist * thresh_dist;
	_merge_params.threshold_dot = thresh_dot;
}

String UnMerger::LFace::to_string() const {
	String sz;

	for (int c = 0; c < 3; c++) {
		sz += String(Variant(pos[c]));
		sz += ", ";
	}

	sz += "norm : ";
	for (int c = 0; c < 3; c++) {
		sz += String(Variant(norm[c]));
		sz += ", ";
	}

	return sz;
}

bool UnMerger::LVert::approx_equal(const LVert &p_o) const {
	if (pos != p_o.pos)
		return false;
	if (norm != p_o.norm)
		return false;
	if (uv != p_o.uv)
		return false;
	if (uv2 != p_o.uv2)
		return false;

	return true;
}

bool UnMerger::unmerge(const Merger &p_merger, const MeshInstance &p_source_mi) {
	LMerged merged;
	if (!fill_merged_from_mesh(merged, p_source_mi))
		return false;

	for (int n = 0; n < p_merger._meshes.size(); n++) {
		if (!unmerge_mesh(*p_merger._meshes[n], merged))
			return false;
	}

	return true;
}

bool UnMerger::unmerge_mesh_surface(MeshInstance &p_mi, LMerged &r_merged, int p_surface_id, Ref<ArrayMesh> p_am_new) {
	Ref<Mesh> rmesh = p_mi.get_mesh();

	Array arrays = rmesh->surface_get_arrays(p_surface_id);
	PoolVector<Vector3> verts = arrays[VS::ARRAY_VERTEX];
	PoolVector<Vector3> norms = arrays[VS::ARRAY_NORMAL];
	PoolVector<Vector2> uv1s = arrays[VS::ARRAY_TEX_UV];
	PoolVector<int> inds = arrays[VS::ARRAY_INDEX];

	// if the mesh contains no indices, create some
	ensure_indices_valid(inds, verts);

	// we need to get the vert positions and normals from local space to world space to match up with the
	// world space coords in the merged mesh
	PoolVector<Vector3> world_verts;
	PoolVector<Vector3> world_norms;
	Transform trans = p_mi.get_global_transform();
	transform_verts(verts, world_verts, trans);
	transform_norms(norms, world_norms, trans);

	// these are the uvs to be filled in the sob
	PoolVector<Vector2> uv2s;
	uv2s.resize(verts.size());

	// for each face in the SOB, attempt to find matching face in the merged mesh
	int nFaces = inds.size() / 3;
	int iCount = 0;

	int nMergedFaces = r_merged.num_faces;

	iCount = 0;

	// the number of unique verts in the UV2 mapped mesh may be HIGHER
	// than the original mesh, because verts with same pos / norm / uv may now have
	// different UV2. So we will be recreating the entire
	// mesh data with a new set of unique verts
	LVector<LVert> UniqueVerts;
	PoolVector<int> UniqueIndices;

	for (int f = 0; f < nFaces; f++) {
		// create the original face in world space and standardize order
		LFace lf;
		// create a copy original face with model space coords
		LFace lf_modelspace;

		for (int c = 0; c < 3; c++) {
			int ind = inds[iCount++];
			lf.pos[c] = world_verts[ind];
			lf.norm[c] = world_norms[ind];
			lf.uvs[c] = uv1s[ind];

			lf.index[c] = ind;

			// model space .. needed for final reconstruction
			lf_modelspace.pos[c] = verts[ind];
			lf_modelspace.norm[c] = norms[ind];
		}

		lf.standardize_order();
		// DONT STANDARDIZE THE MODEL SPACE VERSION, it may need
		// to be rejigged when finally adding the final face

#ifdef LDEBUG_UNMERGE
		LPRINT(5, "lface : " + lf.ToString());
#endif

		// find matching face
		float best_goodness_of_fit = FLT_MAX;
		int best_matching_merged_face = -1;

		for (int mf = 0; mf < nMergedFaces; mf++) {
			// construct merged lface
			// no need to standardize, this is done as a pre-process now.
			LFace mlf = r_merged.lfaces[mf];

			// determine how good a match these 2 faces are
			float goodness_of_fit = do_faces_match(lf, mlf);

			// is it a better fit than the best match so far?
			if (goodness_of_fit < best_goodness_of_fit) {
				best_goodness_of_fit = goodness_of_fit;
				best_matching_merged_face = mf;

				// special case .. perfect goodness of fit
				if (goodness_of_fit == 0.0f)
					break;
			}

		} // for through merged faces

		// special case
		if (best_matching_merged_face == -1) {
			String sz = "\tface " + itos(f);
			sz += " no match";
			print_line(sz);
		} else {
			// standardized order is preprocessed
			LFace mlf = r_merged.lfaces[best_matching_merged_face];

			add_merged_triangle(lf, lf_modelspace, mlf, UniqueVerts, UniqueIndices);
		}

	} // for through original faces

	print_line("UnMerge MI : " + p_mi.get_name()); // + "\tFirstVert : " + itos(vert_count) + "\tNumUVs : " + itos(verts.size()));

	// rebuild the sob mesh, but now using the new uv2s

	//	LPRINT(2, "\t\tOrig Verts: " + itos(verts.size()) + ", Unique Verts: " + itos(UniqueVerts.size()) + ", Num Tris: " + itos(inds.size() / 3));

	// construct unique pool vectors to pass to mesh construction
	PoolVector<Vector3> unique_positions;
	PoolVector<Vector3> unique_norms;
	PoolVector<Vector2> unique_uv1s;
	PoolVector<Vector2> unique_uv2s;

	for (int n = 0; n < UniqueVerts.size(); n++) {
		const LVert &v = UniqueVerts[n];
		unique_positions.push_back(v.pos);
		unique_norms.push_back(v.norm);
		unique_uv1s.push_back(v.uv);
		unique_uv2s.push_back(v.uv2);
	}

	Array arr;
	arr.resize(Mesh::ARRAY_MAX);
	arr[Mesh::ARRAY_VERTEX] = unique_positions;
	arr[Mesh::ARRAY_NORMAL] = unique_norms;
	arr[Mesh::ARRAY_INDEX] = UniqueIndices;
	arr[Mesh::ARRAY_TEX_UV] = unique_uv1s;
	arr[Mesh::ARRAY_TEX_UV2] = unique_uv2s;

	//	am_new->surface_set_material(0, mat);

	// get the old material before setting the new mesh
	//Ref<Material> mat = mi.get_surface_material(p_surface_id);
	Ref<Material> mat = p_mi.get_active_material(p_surface_id);
	//	bool mat_on_mi = true;
	if (!mat.ptr()) {
		//		mat_on_mi = false;
		mat = rmesh->surface_get_material(0);
	}

	p_am_new->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arr);

	// mesh instance has the material?
	//	Ref<Material> mat = mi.get_surface_material(0);
	//	if (mat_on_mi)
	//	{
	if (!mat.ptr()) {
		print_line("\t\tno material found");
	}

	//mi.set_surface_material(0, mat);
	p_am_new->surface_set_material(p_surface_id, mat);

	//	}
	//	else
	//	{
	// mesh has the material?
	//	Ref<Material> smat = rmesh->surface_get_material(0);
	//mi.set_surface_material(0, smat);
	//	}

	return true;
}

bool UnMerger::unmerge_mesh(MeshInstance &mi, LMerged &merged) {
	//LPRINT(2, "UnMerge_SOB " + mi.get_name());

	Ref<Mesh> rmesh = mi.get_mesh();

	int num_surfaces = rmesh->get_surface_count();

	if (!num_surfaces) {
		WARN_PRINT_ONCE("UnMerger::UnMerge_Mesh mesh has no surfaces");
		return false;
	}

	Ref<ArrayMesh> am_new;
	am_new.instance();

	for (int s = 0; s < num_surfaces; s++) {
		unmerge_mesh_surface(mi, merged, s, am_new);
	}

	// hopefully the old mesh will be ref count freed? ???
	mi.set_mesh(am_new);

	return true;
}

// we now have all the info needed in the orig face and the merged face
void UnMerger::add_merged_triangle(const LFace &forig, const LFace &forig_modelspace, const LFace &fmerged, LVector<LVert> &UniqueVerts, PoolVector<int> &UniqueIndices) {
	for (int c = 0; c < 3; c++) {
		// the model space tuple may be offset
		int t = c + (forig.offset);
		t %= 3;

		// construct the unique vert
		LVert uvert;
		//int orig_ind = forig.m_index[c];
		uvert.pos = forig_modelspace.pos[t];
		uvert.norm = forig_modelspace.norm[t];

		// first UV from the original
		uvert.uv = forig.uvs[c];

		// 2nd UV from the merged
		uvert.uv2 = fmerged.uvs[c];

		// find it or add to the list
		int ind_uni_vert = find_or_add_vert(UniqueVerts, uvert);

		// add the index to form the triangle list
		UniqueIndices.push_back(ind_uni_vert);
	}
}

bool UnMerger::fill_merged_from_mesh(LMerged &merged, const MeshInstance &mesh) {
	Ref<Mesh> rmesh = mesh.get_mesh();

	if (rmesh->get_surface_count() == 0) {
		WARN_PRINT("UnMerger::FillMergedFromMesh no surfaces");
		return false;
	}

	Array arrays = rmesh->surface_get_arrays(0);

	if (arrays.size() <= 1) {
		WARN_PRINT("UnMerger::FillMergedFromMesh no arrays");
		return false;
	}

	merged.verts = arrays[VS::ARRAY_VERTEX];
	merged.norms = arrays[VS::ARRAY_NORMAL];
	merged.uv2s = arrays[VS::ARRAY_TEX_UV2];
	merged.inds = arrays[VS::ARRAY_INDEX];
	//	PoolVector<Vector2> p_UV1s = arrays[VS::ARRAY_TEX_UV];

	// special case, if no indices, create some
	if (!merged.inds.size()) {
		WARN_PRINT("UnMerger::FillMergedFromMesh no indices");
	}
	//	if (!EnsureIndicesValid(merged.m_Inds, merged.m_Verts))
	//	{
	//		print_line("invalid triangles due to duplicate indices detected in " + mesh.get_name());
	//	}

	merged.num_faces = merged.inds.size() / 3;

	if (merged.uv2s.size() == 0) {
		print_line("Merged mesh has no secondary UVs, using primary UVs");

		merged.uv2s = arrays[VS::ARRAY_TEX_UV];

		if (merged.uv2s.size() == 0) {
			print_line("Merged mesh has no UVs");
			return false;
		}
	}

	int miCount = 0;
	for (int mf = 0; mf < merged.num_faces; mf++) {
		// construct merged lface
		LFace mlf;
		for (int c = 0; c < 3; c++) {
			int ind = merged.inds[miCount++];
			mlf.pos[c] = merged.verts[ind];
			mlf.norm[c] = merged.norms[ind].normalized();
			mlf.index[c] = ind;

			mlf.uvs[c] = merged.uv2s[ind];
		}

		// we precalculate the order of the merged faces .. these will be gone through lots of times
		mlf.standardize_order();

		merged.lfaces.push_back(mlf);
	}

	return true;
}

void UnMerger::transform_verts(const PoolVector<Vector3> &p_pts_local, PoolVector<Vector3> &p_pts_world, const Transform &p_tr) const {
	for (int n = 0; n < p_pts_local.size(); n++) {
		Vector3 pt_world = p_tr.xform(p_pts_local[n]);
		p_pts_world.push_back(pt_world);
	}
}

void UnMerger::transform_norms(const PoolVector<Vector3> &p_norms_local, PoolVector<Vector3> &p_norms_world, const Transform &p_tr) const {
	for (int n = 0; n < p_norms_local.size(); n++) {
		// hacky way for normals, we should use transpose of inverse matrix, dunno if godot supports this
		Vector3 pt_norm_a = Vector3(0, 0, 0);
		Vector3 pt_norm_world_a = p_tr.xform(pt_norm_a);
		Vector3 pt_norm_world_b = p_tr.xform(p_norms_local[n]);

		Vector3 pt_norm = pt_norm_world_b - pt_norm_world_a;

		pt_norm = pt_norm.normalized();

		p_norms_world.push_back(pt_norm);
	}
}

// returns -1 if no match, or the offset 0 1 or 2
float UnMerger::do_faces_match(const LFace &a, const LFace &b) const {
	//	const float thresh_diff = m_MergeParams.m_fThresholdDist;
	const float thresh_diff_squared = _merge_params.threshold_dist_squared;

	// now we can compare by tuple
	float goodness_of_fit = 0.0f;

	for (int n = 0; n < 3; n++) {
		// positions
		Vector3 pos_diff = b.pos[n] - a.pos[n];
		float sl = pos_diff.length_squared();

		// quick reject
		if (sl > thresh_diff_squared)
			return FLT_MAX;

		goodness_of_fit += sl;

		// normals
		// CHECKING THE NORMALS was resulting in some incorrect results. xatlas must change them.
		// But in fact now the pos checks are accurate, with vertex ordering, back faces will not be selected
		// so normals don't need checking.

		//		Vector3 norm_a = a.m_Norm[n];
		//		Vector3 norm_b = b.m_Norm[n];

		//		norm_a.normalize();
		//		norm_b.normalize();

		// quick reject
		//float dot = norm_a.dot(norm_b);
		//if (dot < m_MergeParams.m_fThresholdDot)
		//	return FLT_MAX;
	}

	// if we got to here they matched to some degree
	return goodness_of_fit;
}

int UnMerger::find_or_add_vert(LVector<LVert> &uni_verts, const LVert &vert) const {
	for (int n = 0; n < uni_verts.size(); n++) {
		if (uni_verts[n].approx_equal(vert))
			return n;
	}

	// not found .. add to list
	uni_verts.push_back(vert);

	return uni_verts.size() - 1;
}

} //namespace LM
