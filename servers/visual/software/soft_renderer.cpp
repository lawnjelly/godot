#include "soft_renderer.h"
#include "soft_mesh.h"
//#include "core/math/math_funcs.h"

//#define SOFTREND_USE_MULTITHREAD

uint32_t SoftRend::Node::tile_width = 128;
uint32_t SoftRend::Node::tile_height = 128;

void SoftRend::ScanConverter::get_x_min_max(int32_t p_y, int32_t &r_x_min, int32_t &r_x_max) const {
	p_y -= _y_min;
	DEV_ASSERT(p_y >= 0);
	DEV_ASSERT(p_y < _y_height);

	r_x_min = scan_buffer[p_y * 2];
	r_x_max = scan_buffer[(p_y * 2) + 1];

	DEV_ASSERT(r_x_min != INT32_MAX);
	DEV_ASSERT(r_x_max != INT32_MAX);
}

void SoftRend::ScanConverter::scan_convert_triangle(const Vector2 &p_min_y_vert, const Vector2 &p_mid_y_vert, const Vector2 &p_max_y_vert) {
	// Calculate handedness.. will change because of swapping due to y min.
	const Vector2 &b = p_max_y_vert;
	const Vector2 &c = p_mid_y_vert;
	const Vector2 &a = p_min_y_vert;

	float x1 = b.x - a.x;
	float y1 = b.y - a.y;

	float x2 = c.x - a.x;
	float y2 = c.y - a.y;

	float area = (x1 * y2 - x2 * y1);
	//float area = minYVert.TriangleAreaTimesTwo(maxYVert, midYVert);
	int handedness = area >= 0 ? 1 : 0;

	scan_convert_line(p_min_y_vert, p_max_y_vert, handedness);
	scan_convert_line(p_min_y_vert, p_mid_y_vert, 1 - handedness);
	scan_convert_line(p_mid_y_vert, p_max_y_vert, 1 - handedness);
}

void SoftRend::ScanConverter::scan_convert_line(const Vector2 &p_min_y_vert, const Vector2 &p_max_y_vert, int32_t p_which_side) {
	int32_t y_start = (int)Math::ceil(p_min_y_vert.y);
	int32_t y_end = (int)Math::ceil(p_max_y_vert.y);
	//int32_t x_start = (int)Math::ceil(p_min_y_vert.x);
	//int32_t x_end = (int)Math::ceil(p_max_y_vert.x);

	float y_dist = p_max_y_vert.y - p_min_y_vert.y;
	float x_dist = p_max_y_vert.x - p_min_y_vert.x;

	if (y_dist <= 0) {
		return;
	}

	float x_step = (float)x_dist / (float)y_dist;
	float y_prestep = y_start - p_min_y_vert.y;
	float curr_x = p_min_y_vert.x + y_prestep * x_step;

	// account for y below 0
	if (y_start < _y_min) {
		int32_t diff = _y_min - y_start;
		curr_x += x_step * diff;
		y_start = _y_min;
	}

	// account for bottom
	if (y_end > _y_max) {
		y_end = _y_max;
	}

	int j_start = y_start - _y_min;
	int j_end = y_end - _y_min;
	for (int j = j_start; j < j_end; j++) {
		scan_buffer[(j * 2) + p_which_side] = (int32_t)Math::ceil(curr_x);
		curr_x += x_step;
	}
}

SoftRend::Node::~Node() {
	for (uint32_t n = 0; n < 2; n++) {
		if (children[n]) {
			memdelete(children[n]);
			children[n] = nullptr;
		}
	}
}

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

	// Create tile scan converters
	for (uint32_t n = 0; n < _tiles.tiles.size(); n++) {
		_tiles.tiles[n].create();
	}
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

	state.frame_count += 1;
}

void SoftRend::push_mesh(SoftMeshInstance &r_softmesh, const Transform &p_cam_transform, const CameraMatrix &p_cam_projection, const Transform &p_instance_xform) {
	Transform xcam = p_cam_transform.affine_inverse();

	// Combine instance and camera transform as cheaper per vert.
	Transform view_matrix = xcam * p_instance_xform;

	// Keep track of first list vert for each surface.
	uint32_t first_surface_list_vert = _vertices.size();

	const SoftMesh &mesh = meshes->get(r_softmesh.get_mesh_id());

	for (uint32_t s = 0; s < mesh.data.surfaces.size(); s++) {
		const SoftMesh::Surface &surf = mesh.data.surfaces[s];

		uint32_t num_verts = surf.positions.size();
		_vertices.resize(_vertices.size() + num_verts);

		// Push the surface item.
		Item it;
		it.mesh_instance = &r_softmesh;
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

			// Backface culling
#define SOFTREND_BACKFACE_CULLING
#ifdef SOFTREND_BACKFACE_CULLING
			if (!final_tri.is_front_facing()) {
				continue;
			}
#endif

#define SOFTREND_REMOVE_NEAR_TRIS
#ifdef SOFTREND_REMOVE_NEAR_TRIS
			bool ignore_near = false;
			for (uint32_t c = 0; c < 3; c++) {
				if (final_tri.coords[c].z <= 0) {
					ignore_near = true;
					break;
				}
				if (final_tri.coords[c].z > 1) {
					ignore_near = true;
					break;
				}
			}
			if (ignore_near) {
				continue;
			}
#endif

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

void SoftRend::flush_to_gbuffer_work(uint32_t p_tile_id, void *p_userdata) {
	Tile &tile = _tiles.tiles[p_tile_id];

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
		draw_tri_to_gbuffer(tile, final_tri, tri_id, tri.item_id + 1);
	}
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
#ifdef SOFTREND_USE_MULTITHREAD
		state.thread_cores = OS::get_singleton()->get_default_thread_pool_size() / 2;
		state.thread_cores = MAX(state.thread_cores, 1);
#else
		state.thread_cores = 1;
#endif

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

	state.render_target->get_image()->unlock();
	state.render_target->update();
}

void SoftRend::flush_final(uint32_t p_tile_id, uint32_t *p_frame_buffer_orig) {
	Tile &tile = _tiles.tiles[p_tile_id];
	const Rect2i &clip_rect = tile.clip_rect;

	uint32_t left = clip_rect.position.x;
	uint32_t right = clip_rect.position.x + clip_rect.size.x;
	uint32_t top = clip_rect.position.y;
	uint32_t bottom = clip_rect.position.y + clip_rect.size.y;

	// Special, for unhit tiles
	if (!tile.tri_list->size()) {
		// return;
		if (!tile.clear) {
			tile.clear = true;
			for (uint32_t y = top; y < bottom; y++) {
				uint32_t *frame_buffer = p_frame_buffer_orig + ((state.viewport_height - y - 1) * state.viewport_width) + left;
				for (uint32_t x = left; x < right; x++) {
					*frame_buffer++ = state.clear_rgba;
				}
			}
		}
		return;
	} else {
		tile.clear = false;
	}

	// Go through pixels and deferred render.
	for (uint32_t y = top; y < bottom; y++) {
		uint32_t *frame_buffer = p_frame_buffer_orig + ((state.viewport_height - y - 1) * state.viewport_width) + left;
		const SoftSurface::GData *gdata = &state.render_target->get_g(left, y);

		for (uint32_t x = left; x < right; x++) {
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
			uint32_t soft_material_id = item.mesh_instance->get_surface_material_id(item.surf_id);

			const SoftMaterial *curr_material = nullptr;
			if (soft_material_id != UINT32_MAX) {
				curr_material = &materials.get(soft_material_id);
			}

			//Color col;
			uint32_t rgba;
			if (curr_material) {
				rgba = curr_material->st_albedo.get8(Vector2(u, v));
			} else {
				// Black
				rgba = 255;
			}

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

void SoftRend::draw_tri_to_gbuffer(Tile &p_tile, const FinalTri &tri, uint32_t p_tri_id, uint32_t p_item_id_p1) {
	const Rect2i &p_clip_rect = p_tile.clip_rect;

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

	Bound16 bound16;
	bound16.set_safe(bound);

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

	//uint32_t tri_id_p1 = p_tri_id + 1;

	Vector3 bary_s[2];
	for (int i = 2; i--;) {
		bary_s[i].coord[0] = pts[2].coord[i] - pts[0].coord[i];
		bary_s[i].coord[1] = pts[1].coord[i] - pts[0].coord[i];

		// Just for safety, can be removed.
		bary_s[i].coord[2] = 0;
	}

	//uint32_t pixels_written = 0;

	Vector2 t0 = pts[0];
	Vector2 t1 = pts[1];
	Vector2 t2 = pts[2];

	// sort the vertices, t0, t1, t2 lower−to−upper (bubblesort yay!)
	if (t0.y > t1.y)
		SWAP(t0, t1);
	if (t0.y > t2.y)
		SWAP(t0, t2);
	if (t1.y > t2.y)
		SWAP(t1, t2);

	p_tile.scan_converter.scan_convert_triangle(t0, t1, t2);

	int32_t y_start = Math::ceil(t0.y);
	int32_t y_end = Math::ceil(t2.y);

	y_start = MAX(y_start, bound16.top);
	y_end = MIN(y_end, bound16.bottom);

	for (int y = y_start; y < y_end; y++) {
		int32_t x_start, x_end;
		p_tile.scan_converter.get_x_min_max(y, x_start, x_end);

		x_start = MAX(x_start, bound16.left);
		x_end = MIN(x_end, bound16.right);

		// get z buffer as one off per line
		float *z_iterator = state.render_target->get_z(x_start, y);

		for (int x = x_start; x < x_end; x++) {
			float *z = z_iterator;
			z_iterator++;

			if (!find_pixel_bary_optimized(bary_s, pts[0], Vector2(x, y), bc_screen))
				continue;

				// Use these for more accurate barycentric clipping
//#define SOFTREND_BARYCENTRIC_TRI_CLIPPING
#ifdef SOFTREND_BARYCENTRIC_TRI_CLIPPING
			if (bc_screen.x < 0.0f)
				continue;
			if (bc_screen.y < 0.0f)
				continue;
			if (bc_screen.z < 0.0f)
				continue;
#endif

			Vector3 pt3;
			find_point_barycentric(tri.coords, pt3, bc_screen.x, bc_screen.y, bc_screen.z);

			Vector3 bc_clip = Vector3(bc_screen.x / tri.hcoords[0].d, bc_screen.y / tri.hcoords[1].d, bc_screen.z / tri.hcoords[2].d);

			float clip_denom = bc_clip.x + bc_clip.y + bc_clip.z;
			if (clip_denom <= 0) {
				continue;
			}
			bc_clip = bc_clip / clip_denom;
			//float frag_depth = clipc[2]*bc_clip;

			if (pt3.z >= *z)
				continue; // failed z test

			//			if (pt3.z < 0)
			//				continue;

			if (texture_map_tri_to_gbuffer(x, y, tri, bc_clip, p_item_id_p1)) {
				// write z only if not discarded by alpha test
				*z = pt3.z;
			}
		}
	}
}

/*
	float total_height = t2.y - t0.y;

	int y_start = t0.y;
	int y_end = y_start + total_height;

	// TEST
	//i_start -= 1;
	y_end += 1;

	y_start = MAX(y_start, bound16.top);
	y_end = MIN(y_end, bound16.bottom);

	// OVERRIDE
	//i_start = bound16.top;
	//i_end = bound16.bottom;

	//i_start -= t0.y;
	//i_end -= t0.y;

	//	for (int i = 0; i < total_height; i++) {

	Vector2 t2_minus_t1 = t2 - t1;
	Vector2 t1_minus_t0 = t1 - t0;

	float segment_height_A = t2.y - t1.y;
	float segment_height_B = t1.y - t0.y;

	float second_half_comp_A = t1.y - t0.y;
	bool second_half_bool = t1.y == t0.y;

	for (int y = y_start; y < y_end; y++) {
		//	for (int i = i_start; i < i_end; i++) {
		//int y = i + t0.y;
		float i = y - t0.y;

		bool second_half = i > second_half_comp_A || second_half_bool;
		float segment_height = second_half ? segment_height_A : segment_height_B;

		float alpha = (float)i / total_height;
		float beta = (float)(i - (second_half ? segment_height_B : 0)) / segment_height; // be careful: with above conditions no division by zero here
		Vector2 A = t0 + (Vector2((t2 - t0)) * alpha);
		Vector2 B = second_half ? t1 + (t2_minus_t1 * beta) : t0 + ((t1_minus_t0)*beta);
		if (A.x > B.x)
			SWAP(A, B);

		int x_start = A.x;
		int x_end = B.x + 1;

		// TEST
		//x_start -= 1;
		//x_end += 1;

		x_start = MAX(x_start, bound16.left);
		x_end = MIN(x_end, bound16.right);

		// OVERRIDE
		//x_start = bound16.left;
		//x_end = bound16.right;

		// get z buffer as one off per line
		float *z_iterator = state.render_target->get_z(x_start, y);

		for (int x = x_start; x < x_end; x++) {
			//			SoftSurface::GData &gdata = state.render_target->get_g(x, y);
			//			gdata.item_id_p1 = p_item_id_p1;
			//			gdata.uv = Vector2(0, 0);

			float *z = z_iterator;
			z_iterator++;

			if (!find_pixel_bary_optimized(bary_s, pts[0], Vector2(x, y), bc_screen))
				continue;

				// Use these for more accurate barycentric clipping
//#define SOFTREND_BARYCENTRIC_TRI_CLIPPING
#ifdef SOFTREND_BARYCENTRIC_TRI_CLIPPING
			if (bc_screen.x < 0.0f)
				continue;
			if (bc_screen.y < 0.0f)
				continue;
			if (bc_screen.z < 0.0f)
				continue;
#endif

			Vector3 pt3;
			find_point_barycentric(tri.coords, pt3, bc_screen.x, bc_screen.y, bc_screen.z);

			Vector3 bc_clip = Vector3(bc_screen.x / tri.hcoords[0].d, bc_screen.y / tri.hcoords[1].d, bc_screen.z / tri.hcoords[2].d);

			float clip_denom = bc_clip.x + bc_clip.y + bc_clip.z;
			if (clip_denom <= 0) {
				continue;
			}
			bc_clip = bc_clip / clip_denom;
			//float frag_depth = clipc[2]*bc_clip;

			if (pt3.z >= *z)
				continue; // failed z test

			//			if (pt3.z < 0)
			//				continue;

			if (texture_map_tri_to_gbuffer(x, y, tri, bc_clip, p_item_id_p1)) {
				// write z only if not discarded by alpha test
				*z = pt3.z;
			}
		}
	}

}
*/

bool SoftRend::texture_map_tri_to_gbuffer(int x, int y, const FinalTri &tri, const Vector3 &p_bary, uint32_t p_item_id_p1) {
	float u, v;
	find_uv_barycentric(tri.uvs, u, v, p_bary.x, p_bary.y, p_bary.z);

	SoftSurface::GData &gdata = state.render_target->get_g(x, y);
	//gdata.tri_id_p1 = p_tri_id_p1;
	gdata.item_id_p1 = p_item_id_p1;
	gdata.uv = Vector2(u, v);
	//	gdata.uv = Vector2(0,0);
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
	meshes = memnew(SoftMeshes);
}

SoftRend::~SoftRend() {
	if (meshes) {
		memdelete(meshes);
		meshes = nullptr;
	}

#ifndef NO_THREADS
	state.thread_pool.finish();
#endif // !NO_THREADS

	if (state.tree) {
		memdelete(state.tree);
		state.tree = 0;
	}
}

/*

void SoftRend::draw_tri_to_gbuffer_OLD(const Rect2i &p_clip_rect, const FinalTri &tri, uint32_t p_tri_id, uint32_t p_item_id_p1) {
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


*/

/*
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
 */
