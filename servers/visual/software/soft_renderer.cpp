#include "soft_renderer.h"
#include "soft_mesh.h"

void SoftRend::set_render_target(SoftSurface *p_soft_surface) {
	DEV_ASSERT(p_soft_surface);
	state.render_target = p_soft_surface;
	state.viewport_width = p_soft_surface->data.width;
	state.viewport_height = p_soft_surface->data.height;
	state.iviewport_height = p_soft_surface->data.height;
}

void SoftRend::prepare() {
	_items.clear();
	_tris.clear();
	_vertices.clear();
}

void SoftRend::push_mesh(SoftMesh &r_softmesh, const Transform &p_cam_transform, const CameraMatrix &p_cam_projection, const Transform &p_instance_xform) {
	Transform xcam = p_cam_transform.affine_inverse();

	// Keep track of first list vert for each surface.
	uint32_t first_surface_list_vert = _vertices.size();

	for (uint32_t s = 0; s < r_softmesh.data.surfaces.size(); s++) {
		SoftMesh::Surface &surf = r_softmesh.data.surfaces[s];

		uint32_t num_verts = surf.positions.size();
		_vertices.resize(_vertices.size() + num_verts);

		// Push the surface item.
		Item it;
		it.mesh = &r_softmesh;
		it.first_list_vert = _vertices.size();
		it.first_tri = _tris.size();
		it.surf_id = s;

		Vector3 pos;
		Plane hpos;
		for (uint32_t v = 0; v < num_verts; v++) {
			pos = surf.positions[v];

			pos = p_instance_xform.xform(pos);

			pos = xcam.xform(pos);

			hpos = Plane(pos, 1.0f);
			hpos = p_cam_projection.xform4(hpos);
			//pos = p_cam_projection.xform(pos);

			//hpos.normal /= hpos.d;
			pos = hpos.normal / hpos.d;

			_vertices.set(first_surface_list_vert + v, hpos, pos);

			//surf.xformed_hpositions[v] = hpos;
			//surf.xformed_positions[v] = pos;
		}

		uint32_t num_inds = surf.indices.size();
		uint32_t num_tris = num_inds / 3;

		const LocalVector<int> &is = surf.indices;

		FinalTri final_tri;

		for (uint32_t t = 0; t < num_tris; t++) {
			uint32_t i = t * 3;

			Tri tri;
			for (uint32_t c = 0; c < 3; c++) {
				tri.corns[c] = is[i + c] + first_surface_list_vert;
			}

			for (uint32_t c = 0; c < 3; c++) {
				//final_tri.coords[c] = surf.xformed_positions[is[i + c]];
				final_tri.coords[c] = _vertices.cam_space_coords[tri.corns[c]];
			}

			if (!final_tri.is_front_facing()) {
				continue;
			}

			// The start index is needed for reading the UVs from the original
			// surface, because we may delete backfacing tris, so we can't just
			// use the triangle number to derive this.
			tri.index_start = i;

			_tris.push_back(tri);
			it.num_tris += 1;
		}

		// set for next surface / mesh
		first_surface_list_vert += num_verts;

		// Push surface item.
		_items.push_back(it);
	}
}

void SoftRend::flush() {
	state.render_target->get_image()->lock();

	// Each surface item.
	for (uint32_t i = 0; i < _items.size(); i++) {
		const Item &item = _items[i];
		if (!item.num_tris)
			continue;

		SoftMesh::Surface &surf = item.mesh->data.surfaces[item.surf_id];

		state.curr_material = nullptr;
		if (surf.soft_material_id != UINT32_MAX) {
			state.curr_material = &materials.get(surf.soft_material_id);
		}

		FinalTri final_tri;

		const LocalVector<int> &is = surf.indices;

		for (uint32_t t = 0; t < item.num_tris; t++) {
			const Tri &tri = _tris[item.first_tri + t];
			//uint32_t i = t * 3;
			uint32_t i = tri.index_start;

			for (uint32_t c = 0; c < 3; c++) {
				final_tri.coords[c] = _vertices.cam_space_coords[tri.corns[c]];
			}

			for (uint32_t c = 0; c < 3; c++) {
				final_tri.hcoords[c] = _vertices.hcoords[tri.corns[c]];
				final_tri.uvs[c] = surf.uvs[is[i + c]];
			}
			draw_tri(final_tri);
		}
	}

	state.render_target->get_image()->unlock();
	state.render_target->update();
}

void SoftRend::set_pixel(float x, float y) {
	if ((x < 0) || (x >= state.viewport_width))
		return;
	if ((y < 0) || (y >= state.viewport_height))
		return;
	Color col = Color(1, 1, 1, 1);
	state.render_target->get_image()->set_pixel(x, y, col);
}

void SoftRend::draw_tri(const FinalTri &tri) {
	Vector2 pts[3];
	for (uint32_t c = 0; c < 3; c++) {
		GL_to_viewport_coords(tri.coords[c], pts[c]);
	}

	Rect2i bound;
	bound.position = Point2i(pts[0].x, pts[0].y);

	for (uint32_t n = 1; n < 3; n++) {
		bound.expand_to(Point2i(pts[n].x, pts[n].y));
	}
	bound.size += Vector2i(1, 1);

	bound = bound.clip(Rect2i(0, 0, state.viewport_width, state.viewport_height));
	if (bound.has_no_area())
		return;

	/*
	for (uint32_t n=0; n<3; n++)
	{
		float x = pts[n].x;
		float y = pts[n].y;
		//set_pixel(r_soft_surface, x, y);
		//print_line(itof(x) + ", " + itof(y));
	}
*/
	//print_line("\n");

	//	print_line(String(Variant(p_a)) + ", " + String(Variant(p_b)) + ", " + String(Variant(p_c)));

	Vector3 bc_screen;

	for (int y = bound.position.y; y < bound.position.y + bound.size.y; y++) {
		for (int x = bound.position.x; x < bound.position.x + bound.size.x; x++) {
			if (!find_pixel_bary(pts[0], pts[1], pts[2], Vector2(x, y), bc_screen))
				continue;

			if (bc_screen.x < 0.0f)
				continue;
			if (bc_screen.y < 0.0f)
				continue;
			if (bc_screen.z < 0.0f)
				continue;

			//				   // zbuffer
			Vector3 pt3;
			find_point_barycentric(tri.coords, pt3, bc_screen.x, bc_screen.y, bc_screen.z);
			//			tri.FindPoint_Barycentric(pt3, ptBary.x, ptBary.y, ptBary.z);
			//			float &z = m_pZBuffer->GetItem(x, y).g;

			Vector3 bc_clip = Vector3(bc_screen.x / tri.hcoords[0].d, bc_screen.y / tri.hcoords[1].d, bc_screen.z / tri.hcoords[2].d);

			float clip_denom = bc_clip.x + bc_clip.y + bc_clip.z;
			if (clip_denom <= 0) {
				continue;
			}
			bc_clip = bc_clip / clip_denom;
			//float frag_depth = clipc[2]*bc_clip;

			// CHECK Z BUFFER
			float *z = state.render_target->get_z(x, y);
			DEV_ASSERT(z);

			if (pt3.z >= *z)
				continue; // failed z test

			if (pt3.z < 0)
				continue;

			if (texture_map_tri(x, y, tri, bc_clip)) {
				//if (texture_map_tri(x, y, tri, bc_screen)) {
				// write z only if not discarded by alpha test
				*z = pt3.z;
			}
		}
	}
}

bool SoftRend::texture_map_tri(int x, int y, const FinalTri &tri, const Vector3 &p_bary) {
	float u, v;
	find_uv_barycentric(tri.uvs, u, v, p_bary.x, p_bary.y, p_bary.z);

	//set_pixel(r_soft_surface, x, y);
	Color col;
	if (state.curr_material) {
		col = state.curr_material->st_albedo.get_color(Vector2(u, v));
	} else {
		col = Color(u, v, 1, 1);
	}

	state.render_target->get_image()->set_pixel(x, state.iviewport_height - y - 1, col);

	return true;
}
