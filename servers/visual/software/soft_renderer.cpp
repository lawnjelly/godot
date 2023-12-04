#include "soft_renderer.h"
#include "soft_mesh.h"

uint32_t SoftRend::Node::tile_width = 128;
uint32_t SoftRend::Node::tile_height = 128;

SoftRend::Node::~Node() {
	for (uint32_t n = 0; n < 2; n++) {
		if (children[n]) {
			memdelete(children[n]);
			children[n] = nullptr;
		}
	}
}

/*
void SoftRend::Tiles::create(uint32_t p_viewport_width, uint32_t p_viewport_height) {
	//	tiles_x = 4;
	//	tiles_y = 4;
	//	tile_width = p_viewport_width / tiles_x;
	//	tile_height = p_viewport_height / tiles_y;

	tile_width = 128;
	tile_height = 128;
	tiles_x = p_viewport_width / tile_width;
	tiles_y = p_viewport_height / tile_height;

	tiles.resize(tiles_x * tiles_y);

	uint32_t count = 0;
	for (uint32_t y = 0; y < tiles_y; y++) {
		for (uint32_t x = 0; x < tiles_x; x++) {
			Tile &tile = tiles[count];
			count++;

			tile.clip_rect = Rect2i(x * tile_width, y * tile_height, tile_width, tile_height);
			tile.clip_rect = tile.clip_rect.clip(Rect2i(0, 0, p_viewport_width, p_viewport_height));
		}
	}
}
*/

void SoftRend::set_render_target(SoftSurface *p_soft_surface) {
	DEV_ASSERT(p_soft_surface);
	state.render_target = p_soft_surface;

	// Aliases
	uint32_t new_width = p_soft_surface->data.width;
	uint32_t new_height = p_soft_surface->data.height;
	int32_t &width = state.viewport_width;
	int32_t &height = state.viewport_height;

	// Noop?
	if ((new_width == width) && (new_height == height)) {
		return;
	}

	width = new_width;
	height = new_height;
	state.fviewport_width = width;
	state.fviewport_height = height;

	_tiles.tiles.clear();
	//_tiles.create(width, height);

	// Build the tree.
	if (state.tree) {
		memdelete(state.tree);
		state.tree = nullptr;
	}

	state.tree = memnew(Node);
	state.tree->clip_rect = Rect2i(0, 0, width, height);
	state.tree->build_tree_split();
	state.tree->debug_tree();

	link_leaf_nodes_to_tiles(state.tree);
}

void SoftRend::link_leaf_nodes_to_tiles(Node *p_node) {
	// Leaf node?
	if (!p_node->children[0]) {
		p_node->tile_id_p1 = _tiles.tiles.size() + 1;
		Tile t;
		t.clip_rect = p_node->clip_rect;
		t.tri_list = &p_node->tris;
		_tiles.tiles.push_back(t);
		return;
	}

	link_leaf_nodes_to_tiles(p_node->children[0]);
	link_leaf_nodes_to_tiles(p_node->children[1]);
}

void SoftRend::Node::build_tree_split() {
	// find longest axis and split on it
	uint32_t w = clip_rect.size.x / tile_width;
	uint32_t h = clip_rect.size.y / tile_height;

	if ((w <= 1) && (h <= 1)) {
		// leaf node
		return;
	}

	children[0] = memnew(Node);
	children[1] = memnew(Node);

	// Old rect
	Vector2i op = clip_rect.position;
	Vector2i os = clip_rect.size;

	if (w > h) {
		uint32_t split = (w / 2) * tile_width;
		children[0]->clip_rect = Rect2i(op.x, op.y, split, os.y);
		children[1]->clip_rect = Rect2i(op.x + split, op.y, os.x - split, os.y);
	} else {
		uint32_t split = (h / 2) * tile_height;
		children[0]->clip_rect = Rect2i(op.x, op.y, os.x, split);
		children[1]->clip_rect = Rect2i(op.x, op.y + split, os.x, os.y - split);
	}

	children[0]->build_tree_split();
	children[1]->build_tree_split();
}

void SoftRend::Node::debug_tree(uint32_t p_depth) {
	String sz;
	for (uint32_t n = 0; n < p_depth; n++) {
		sz += String("\t");
	}
	sz += String(clip_rect);
	print_line(sz);

	if (children[0]) {
		children[0]->debug_tree(p_depth + 1);
		children[1]->debug_tree(p_depth + 1);
	}
}

void SoftRend::prepare() {
	_items.clear();
	_tris.clear();
	_vertices.clear();
	state.tris_rejected_by_edges = 0;
}

void SoftRend::push_mesh(SoftMesh &r_softmesh, const Transform &p_cam_transform, const CameraMatrix &p_cam_projection, const Transform &p_instance_xform) {
	Transform xcam = p_cam_transform.affine_inverse();

	// Combine instance and camera transform as cheaper per vert.
	Transform view_matrix = xcam * p_instance_xform;

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

		uint32_t item_id = _items.size();

		Vector3 pos;
		Plane hpos;
		for (uint32_t v = 0; v < num_verts; v++) {
			pos = surf.positions[v];

			//pos = p_instance_xform.xform(pos);
			//pos = xcam.xform(pos);
			pos = view_matrix.xform(pos);

			hpos = Plane(pos, 1.0f);
			hpos = p_cam_projection.xform4(hpos);
			//pos = p_cam_projection.xform(pos);

			//hpos.normal /= hpos.d;
			pos = hpos.normal / hpos.d;

			Vector2 uv = surf.uvs[v];
			_vertices.set(first_surface_list_vert + v, hpos, pos, uv);

			//			final_tri.uvs[c] = surf.uvs[is[i + c]];

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

			tri.bound.unset();
			tri.item_id = item_id;

			Vector2 pt_screen;
			for (uint32_t c = 0; c < 3; c++) {
				//final_tri.coords[c] = surf.xformed_positions[is[i + c]];
				GL_to_viewport_coords(final_tri.coords[c], pt_screen);

				int32_t x, y;
				x = pt_screen.x;
				y = pt_screen.y;
				x = CLAMP(x, INT16_MIN, INT16_MAX - 1);
				y = CLAMP(y, INT16_MIN, INT16_MAX - 1);
				tri.bound.expand_to_include(x, y);
				tri.bound.right += 1;
				tri.bound.bottom += 1;
			}

			//tri.uvs[c] = surf.uvs[is[i + c]];

			// The start index is needed for reading the UVs from the original
			// surface, because we may delete backfacing tris, so we can't just
			// use the triangle number to derive this.
			//tri.index_start = i;

			_tris.push_back(tri);
			it.num_tris += 1;
		}

		// set for next surface / mesh
		first_surface_list_vert += num_verts;

		// Push surface item.
		_items.push_back(it);
	}
}

/*
void SoftRend::flush_to_gbuffer(Rect2i p_clip_rect) {
	// Each surface item.
	for (uint32_t i = 0; i < _items.size(); i++) {
		const Item &item = _items[i];
		if (!item.num_tris)
			continue;

		FinalTri final_tri;

		for (uint32_t t = 0; t < item.num_tris; t++) {
			const Tri &tri = _tris[item.first_tri + t];

			for (uint32_t c = 0; c < 3; c++) {
				uint32_t vert_id = tri.corns[c];
				final_tri.coords[c] = _vertices.cam_space_coords[tri.corns[c]];
				final_tri.hcoords[c] = _vertices.hcoords[vert_id];
				final_tri.uvs[c] = _vertices.uvs[vert_id];
			}
			draw_tri_to_gbuffer(p_clip_rect, final_tri, item.first_tri + t, i + 1);
		}
	}
}
*/

void SoftRend::flush_to_gbuffer_work(uint32_t p_tile_id, void *p_userdata) {
	const Tile &tile = _tiles.tiles[p_tile_id];

	FinalTri final_tri;
	for (uint32_t t = 0; t < tile.tri_list->size(); t++) {
		uint32_t tri_id = (*tile.tri_list)[t];
		const Tri &tri = _tris[tri_id];

		for (uint32_t c = 0; c < 3; c++) {
			uint32_t vert_id = tri.corns[c];
			final_tri.coords[c] = _vertices.cam_space_coords[tri.corns[c]];
			final_tri.hcoords[c] = _vertices.hcoords[vert_id];
			final_tri.uvs[c] = _vertices.uvs[vert_id];
		}
		draw_tri_to_gbuffer(tile.clip_rect, final_tri, tri_id, tri.item_id + 1);
	}
	//flush_to_gbuffer(_tiles.tiles[p_tile_id].clip_rect);
}

void SoftRend::fill_tree(Node *p_node, const LocalVector<uint32_t> &p_parent_tri_list) {
	DEV_ASSERT(p_node);
	p_node->tris.clear();

	// translate tile clip rect to 16
	Bound16 clip_rect;
	clip_rect.set(p_node->clip_rect);

	for (uint32_t n = 0; n < p_parent_tri_list.size(); n++) {
		uint32_t tri_id = p_parent_tri_list[n];
		const Tri &tri = _tris[tri_id];

		if (tri.bound.intersects(clip_rect)) {
			p_node->tris.push_back(tri_id);
		}
	}

	if (p_node->children[0]) {
		fill_tree(p_node->children[0], p_node->tris);
		fill_tree(p_node->children[1], p_node->tris);
	}
}

void SoftRend::flush() {
	// Seed tris
	DEV_ASSERT(state.tree);
	state.tree->tris.resize(_tris.size());
	for (uint32_t n = 0; n < _tris.size(); n++) {
		state.tree->tris[n] = n;
	}
	if (state.tree->children[0]) {
		fill_tree(state.tree->children[0], state.tree->tris);
		fill_tree(state.tree->children[1], state.tree->tris);
	}

	state.clear_color = Color(0.15, 0.3, 0.7, 1);
	state.clear_rgba = state.clear_color.to_abgr32();
	//data.image->fill(Color(0.15, 0.3, 0.7, 1));

#ifndef NO_THREADS
	if (!state.thread_cores && state.thread_pool.get_thread_count() == 0) {
		//state.thread_cores = OS::get_singleton()->get_default_thread_pool_size() / 2;
		state.thread_cores = 1;
		state.thread_cores = MAX(state.thread_cores, 1);

		if (state.thread_cores > 1) {
			state.thread_pool.init(state.thread_cores);
		}
	}
	if (state.thread_cores > 1) {
		state.thread_pool.do_work(
				_tiles.tiles.size(),
				this,
				&SoftRend::flush_to_gbuffer_work,
				nullptr);
	} else {
		for (uint32_t t = 0; t < _tiles.tiles.size(); t++) {
			flush_to_gbuffer_work(t, nullptr);
		}
	}
#else
	for (uint32_t t = 0; t < _tiles.tiles.size(); t++) {
		flush_to_gbuffer_work(t, nullptr);
	}
#endif // NO_THREADS

	state.render_target->get_image()->lock();

	PoolVector<uint8_t> &frame_buffer_pool = state.render_target->get_image()->get_data_pool();
	PoolVector<uint8_t>::Write write = frame_buffer_pool.write();
	uint32_t *frame_buffer_orig = (uint32_t *)write.ptr();

#ifndef NO_THREADS
	if (state.thread_cores > 1) {
		state.thread_pool.do_work(
				_tiles.tiles.size(),
				this,
				&SoftRend::flush_final,
				frame_buffer_orig);
	} else {
		for (uint32_t t = 0; t < _tiles.tiles.size(); t++) {
			flush_final(t, frame_buffer_orig);
		}
	}
#else
	for (uint32_t t = 0; t < _tiles.tiles.size(); t++) {
		flush_final(t, frame_buffer_orig);
	}
#endif // NO_THREADS

	/*
		const SoftSurface::GData *gdata = &state.render_target->get_g(0, 0);

		// Go through pixels and deferred render.
		for (uint32_t y = 0; y < state.viewport_height; y++) {
			uint32_t *frame_buffer = frame_buffer_orig + ((state.viewport_height - y - 1) * state.viewport_width);

			for (uint32_t x = 0; x < state.viewport_width; x++) {
				const SoftSurface::GData &g = *gdata++;

				// Background
				if (!g.item_id_p1) {
					*frame_buffer++ = state.clear_rgba;
					continue;
				}

				uint32_t item_id = g.item_id_p1 - 1;
				float u = g.uv.x;
				float v = g.uv.y;

				const Item &item = _items[item_id];

				SoftMesh::Surface &surf = item.mesh->data.surfaces[item.surf_id];

				state.curr_material = nullptr;
				if (surf.soft_material_id != UINT32_MAX) {
					state.curr_material = &materials.get(surf.soft_material_id);
				}

				uint32_t rgba;
				if (state.curr_material) {
					rgba = state.curr_material->st_albedo.get8(Vector2(u, v));
				} else {
					// Black
					rgba = 255;
				}

				*frame_buffer++ = rgba;
			}
		}
		*/

	state.render_target->get_image()->unlock();
	state.render_target->update();
}

void SoftRend::flush_final(uint32_t p_tile_id, uint32_t *p_frame_buffer_orig) {
	const Rect2i &clip_rect = _tiles.tiles[p_tile_id].clip_rect;

	uint32_t left = clip_rect.position.x;
	uint32_t right = clip_rect.position.x + clip_rect.size.x;
	uint32_t top = clip_rect.position.y;
	uint32_t bottom = clip_rect.position.y + clip_rect.size.y;

	// Go through pixels and deferred render.
	for (uint32_t y = top; y < bottom; y++) {
		uint32_t *frame_buffer = p_frame_buffer_orig + ((state.viewport_height - y - 1) * state.viewport_width) + left;
		const SoftSurface::GData *gdata = &state.render_target->get_g(left, y);

		for (uint32_t x = left; x < right; x++) {
			//const SoftSurface::GData &g = state.render_target->get_g(x, y);
			const SoftSurface::GData &g = *gdata++;

			// Background
			if (!g.item_id_p1) {
				*frame_buffer++ = state.clear_rgba;
				continue;
			}

			uint32_t item_id = g.item_id_p1 - 1;
			float u = g.uv.x;
			float v = g.uv.y;

			const Item &item = _items[item_id];

			SoftMesh::Surface &surf = item.mesh->data.surfaces[item.surf_id];

			const SoftMaterial *curr_material = nullptr;
			//state.curr_material = nullptr;
			if (surf.soft_material_id != UINT32_MAX) {
				curr_material = &materials.get(surf.soft_material_id);
			}

			//Color col;
			uint32_t rgba;
			if (curr_material) {
				//col = state.curr_material->st_albedo.get_color(Vector2(u, v));
				rgba = curr_material->st_albedo.get8(Vector2(u, v));
				//rgba = (255 << 24) | 255;
			} else {
				//col = Color(u, v, 1, 1);
				// Black
				rgba = 255;
			}

			//state.render_target->get_image()->set_pixel(x, state.viewport_height - y - 1, col);
			*frame_buffer++ = rgba;
		}
	}
}

void SoftRend::set_pixel(float x, float y) {
	if ((x < 0) || (x >= state.fviewport_width))
		return;
	if ((y < 0) || (y >= state.fviewport_height))
		return;
	Color col = Color(1, 1, 1, 1);
	state.render_target->get_image()->set_pixel(x, y, col);
}

bool SoftRend::which_side(const Vector2 &wall_a, const Vector2 &wall_vec, const Vector2 &pt) const {
	Vector2 pt_vec = pt - wall_a;
	return wall_vec.cross(pt_vec) > 0;
}

void SoftRend::draw_tri_to_gbuffer(const Rect2i &p_clip_rect, const FinalTri &tri, uint32_t p_tri_id, uint32_t p_item_id_p1) {
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

	bound = bound.clip(p_clip_rect);
	if (bound.has_no_area())
		return;

	// Do a more accurate cull of each triangle edge.
	for (uint32_t e = 0; e < 3; e++) {
		const Vector2 &a = pts[e];
		const Vector2 &b = pts[(e + 1) % 3];

		Vector2 wall_vec = b - a;

		if (which_side(a, wall_vec, p_clip_rect.position) &&
				which_side(a, wall_vec, p_clip_rect.position + Vector2i(p_clip_rect.size.x, 0)) &&
				which_side(a, wall_vec, p_clip_rect.position + Vector2i(0, p_clip_rect.size.y)) &&
				which_side(a, wall_vec, p_clip_rect.position + p_clip_rect.size)) {
#ifdef DEV_ENABLED
			state.tris_rejected_by_edges += 1;
#endif
			return;
		}
	}

	Vector3 bc_screen;

	uint32_t tri_id_p1 = p_tri_id + 1;

	Vector3 bary_s[2];
	for (int i = 2; i--;) {
		bary_s[i].coord[0] = pts[2].coord[i] - pts[0].coord[i];
		bary_s[i].coord[1] = pts[1].coord[i] - pts[0].coord[i];

		// Just for safety, can be removed.
		bary_s[i].coord[2] = 0;
	}

	//uint32_t pixels_written = 0;

	for (int y = bound.position.y; y < bound.position.y + bound.size.y; y++) {
		// get z and g buffer as one off per line
		float *z_iterator = state.render_target->get_z(bound.position.x, y);

		for (int x = bound.position.x; x < bound.position.x + bound.size.x; x++) {
			float *z = z_iterator;
			z_iterator++;

			//			if (!find_pixel_bary(pts[0], pts[1], pts[2], Vector2(x, y), bc_screen))
			if (!find_pixel_bary_optimized(bary_s, pts[0], Vector2(x, y), bc_screen))
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
			//float *z = state.render_target->get_z(x, y);
			//DEV_ASSERT(z);

			//pixels_written++;

			if (pt3.z >= *z)
				continue; // failed z test

			if (pt3.z < 0)
				continue;

			if (texture_map_tri_to_gbuffer(x, y, tri, bc_clip, tri_id_p1, p_item_id_p1)) {
				//if (texture_map_tri(x, y, tri, bc_screen)) {
				// write z only if not discarded by alpha test
				*z = pt3.z;
			}
		}
	}
}

/*
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

	bound = bound.clip(Rect2i(0, 0, state.fviewport_width, state.fviewport_height));
	if (bound.has_no_area())
		return;

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
*/

bool SoftRend::texture_map_tri_to_gbuffer(int x, int y, const FinalTri &tri, const Vector3 &p_bary, uint32_t p_tri_id_p1, uint32_t p_item_id_p1) {
	float u, v;
	find_uv_barycentric(tri.uvs, u, v, p_bary.x, p_bary.y, p_bary.z);

	SoftSurface::GData &gdata = state.render_target->get_g(x, y);
	//gdata.tri_id_p1 = p_tri_id_p1;
	gdata.item_id_p1 = p_item_id_p1;
	gdata.uv = Vector2(u, v);
	/*
		   //set_pixel(r_soft_surface, x, y);
	Color col;
	if (state.curr_material) {
		col = state.curr_material->st_albedo.get_color(Vector2(u, v));
	} else {
		col = Color(u, v, 1, 1);
	}

	state.render_target->get_image()->set_pixel(x, state.iviewport_height - y - 1, col);
	*/
	return true;
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

	state.render_target->get_image()->set_pixel(x, state.viewport_height - y - 1, col);

	return true;
}

SoftRend::SoftRend() {
}

SoftRend::~SoftRend() {
#ifndef NO_THREADS
	state.thread_pool.finish();
#endif // !NO_THREADS

	if (state.tree) {
		memdelete(state.tree);
		state.tree = 0;
	}
}

/*
void SoftRend::flush_OLD() {
	flush_to_gbuffer();

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

			   //const LocalVector<int> &is = surf.indices;

		for (uint32_t t = 0; t < item.num_tris; t++) {
			const Tri &tri = _tris[item.first_tri + t];
			//uint32_t i = tri.index_start;


			for (uint32_t c = 0; c < 3; c++) {
				uint32_t vert_id =tri.corns[c];
				final_tri.coords[c] = _vertices.cam_space_coords[tri.corns[c]];
				final_tri.hcoords[c] = _vertices.hcoords[vert_id];
				//final_tri.uvs[c] = surf.uvs[is[i + c]];
				//Vector2 uv_test = _vertices.uvs[vert_id];
				final_tri.uvs[c] = _vertices.uvs[vert_id];
				//DEV_ASSERT(uv_test == final_tri.uvs[c]);
			}
			draw_tri(final_tri);
		}
	}

	state.render_target->get_image()->unlock();
	state.render_target->update();
}
*/
