
#include "soft_renderer.h"
#include "soft_mesh.h"
//#include "core/math/math_funcs.h"

//#define SOFTREND_USE_MULTITHREAD

uint32_t SoftRend::Node::tile_width = 64;
uint32_t SoftRend::Node::tile_height = 64;

#ifdef VISUAL_SERVER_SOFTREND_ENABLED
SoftRend g_soft_rend;
#endif // softrend enabled

void SoftRend::ScanConverter::get_x_min_max(int32_t p_y, int32_t &r_x_min, int32_t &r_x_max) const {
	p_y -= _y_min;
	DEV_ASSERT(p_y >= 0);
	DEV_ASSERT(p_y < _y_height);

	r_x_min = scan_buffer[p_y * 2];
	r_x_max = scan_buffer[(p_y * 2) + 1];

	DEV_ASSERT(r_x_min != INT32_MAX);
	DEV_ASSERT(r_x_max != INT32_MAX);
}

void SoftRend::ScanConverter::scan_convert_triangle(Vector2 t0, Vector2 t1, Vector2 t2, int32_t &r_y_start, int32_t &r_y_end) {
	//	Vector2 t0 = p_verts[0];
	//	Vector2 t1 = p_verts[1];
	//	Vector2 t2 = p_verts[2];

	// sort the vertices, t0, t1, t2 lower−to−upper (bubblesort yay!)
	if (t0.y > t1.y)
		SWAP(t0, t1);
	if (t0.y > t2.y)
		SWAP(t0, t2);
	if (t1.y > t2.y)
		SWAP(t1, t2);

	_scan_convert_triangle(t0, t1, t2);

	r_y_start = Math::ceil(t0.y);
	r_y_end = Math::ceil(t2.y);
}

void SoftRend::ScanConverter::_scan_convert_triangle(const Vector2 &p_min_y_vert, const Vector2 &p_mid_y_vert, const Vector2 &p_max_y_vert) {
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

	print_line("SoftRender setting new render target size " + itos(width) + " x " + itos(height));

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
	//state.tree->debug_tree();

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
	//Transform xcam = p_cam_transform;

	// Combine instance and camera transform as cheaper per vert.
	Transform view_matrix = xcam * p_instance_xform;

	const SoftMesh &mesh = meshes->get(r_softmesh.get_mesh_id());

	for (uint32_t s = 0; s < mesh.data.surfaces.size(); s++) {
		// Keep track of first list vert for each surface.
		uint32_t first_surface_list_vert = _vertices.size();

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
			_vertices[first_surface_list_vert + v].set(hpos, pos, uv);

			//			final_tri.uvs[c] = surf.uvs[is[i + c]];

			//surf.xformed_hpositions[v] = hpos;
			//surf.xformed_positions[v] = pos;
		}

		uint32_t num_inds = surf.indices.size();
		uint32_t num_tris = num_inds / 3;

		const LocalVector<int> &is = surf.indices;

		//FinalTri final_tri;

		for (uint32_t t = 0; t < num_tris; t++) {
			uint32_t i = t * 3;

			//Tri tri;
			uint32_t corns[3];
			for (uint32_t c = 0; c < 3; c++) {
				corns[c] = is[i + c] + first_surface_list_vert;
			}

			//			for (uint32_t c = 0; c < 3; c++) {
			//				final_tri.coords[c] = _vertices.cam_space_coords[corns[c]];
			//			}

			// Backface culling
//#define SOFTREND_BACKFACE_CULLING
#ifdef SOFTREND_BACKFACE_CULLING

			Vector3 normal_camera_space;
			// backface cull
			//			const Vector3 &h0 = _vertices.hcoords[corns[0]].normal;
			//			const Vector3 &h1 = _vertices.hcoords[corns[1]].normal;
			//			const Vector3 &h2 = _vertices.hcoords[corns[2]].normal;
			const Vector3 &h0 = _vertices.cam_space_coords[corns[0]];
			const Vector3 &h1 = _vertices.cam_space_coords[corns[1]];
			const Vector3 &h2 = _vertices.cam_space_coords[corns[2]];
			normal_camera_space = (h2 - h0).cross(h1 - h0);
			if (normal_camera_space.z < 0) {
				//continue;
			}

//			if (!final_tri.is_front_facing()) {
//				continue;
//			}
#endif

			clip_tri(corns, it, item_id);
		}

		// set for next surface / mesh
		//first_surface_list_vert += num_verts;

		// Push surface item.
		_items.push_back(it);
	}
}

bool SoftRend::is_inside_view_frustum(uint32_t p_ind) {
	const Plane &v = _vertices[p_ind].hcoord;
	float w = Math::absf(v.d);
	return ((Math::absf(v.normal.x) <= w) &&
			(Math::absf(v.normal.y) <= w) &&
			(Math::absf(v.normal.z) <= w));
}

void SoftRend::clip_tri(const uint32_t *p_inds, Item &r_item, uint32_t p_item_id) {
	bool inside[3];
	for (uint32_t c = 0; c < 3; c++) {
		inside[c] = is_inside_view_frustum(p_inds[c]);
	}

	if (inside[0] && inside[1] && inside[2]) {
		push_tri(p_inds, r_item, p_item_id);
		return;
	}

	//	if (!inside[0] && !inside[1] && !inside[2]) {
	//		return;
	//	}

	// If we got to here, we need to clip.
	FixedArray<uint32_t, 16> inds;
	FixedArray<uint32_t, 16> auxillary;

	inds.push_back(p_inds[0]);
	inds.push_back(p_inds[1]);
	inds.push_back(p_inds[2]);

	if (clip_polygon_axis(inds, auxillary, 0) &&
			clip_polygon_axis(inds, auxillary, 1) &&
			clip_polygon_axis(inds, auxillary, 2)) {
		uint32_t corns[3];
		corns[0] = inds[0];

		// display clipped
		//		for (uint32_t n = 0; n < inds.size(); n++) {
		//			Plane p = _vertices.hcoords[inds[n]];
		//			print_line(itos(n) + " ) " + itos(inds[n]) + " : " + String(Variant(p)) + " is " + String(Variant(p.normal / p.d)));
		//		}

		for (uint32_t i = 1; i < inds.size() - 1; i++) {
			corns[1] = inds[i];
			corns[2] = inds[i + 1];

			push_tri(corns, r_item, p_item_id);
		}
	} else {
		//print_line("clipping failed");
	}
}

bool SoftRend::clip_polygon_axis(FixedArray<uint32_t, 16> &inds, FixedArray<uint32_t, 16> &aux, int32_t component_index) {
	// debug
	//	for (uint32_t n=0; n<inds.size(); n++)
	//	{
	//		Plane p = _vertices.hcoords[inds[n]];
	//		print_line(itos(n) + " ) " + itos(inds[n]) + " : " + String(Variant(p)) + " is " + String(Variant(p.normal / p.d)));
	//	}

	clip_polygon_component(inds, aux, component_index, 1.0f);
	inds.clear();

	if (aux.is_empty()) {
		return false;
	}

	clip_polygon_component(aux, inds, component_index, -1.0f);
	aux.clear();

	return !inds.is_empty();
}

void SoftRend::clip_polygon_component(FixedArray<uint32_t, 16> &inds, FixedArray<uint32_t, 16> &result, int32_t component_index, float component_factor) {
	uint32_t prev_ind = inds[inds.size() - 1];
	Plane prev_vert = _vertices[prev_ind].hcoord;

	float prev_component = prev_vert.normal.coord[component_index] * component_factor;
	bool prev_inside = prev_component <= prev_vert.d;

	//	Vertex previousVertex = vertices.get(vertices.size() - 1);
	//	float previousComponent = previousVertex.Get(componentIndex) * componentFactor;
	//	boolean previousInside = previousComponent <= previousVertex.GetPosition().GetW();

	for (uint32_t i = 0; i < inds.size(); i++) {
		uint32_t curr_ind = inds[i];
		Plane curr_vert = _vertices[curr_ind].hcoord;
		float curr_component = curr_vert.normal.coord[component_index] * component_factor;
		bool curr_inside = curr_component <= curr_vert.d;

		if (curr_inside ^ prev_inside) {
			float f = (prev_vert.d - prev_component) /
					((prev_vert.d - prev_component) -
							(curr_vert.d - curr_component));

			//			if (curr_inside)
			//			{
			//				f *= 1.001f;
			//			}
			//			else
			//			{
			//				f *= 0.999f;
			//			}

			Vertex v;

			// construct a new vertex lerped
			v.hcoord = lerp_hcoord(prev_vert, curr_vert, f);
			v.coord_cam_space = v.hcoord.normal / v.hcoord.d;
			v.uv = _vertices[prev_ind].uv.linear_interpolate(_vertices[curr_ind].uv, f);

			_vertices.push_back(v);
			result.push_back(_vertices.size() - 1);
		}

		if (curr_inside) {
			result.push_back(curr_ind);
		}

		prev_vert = curr_vert;
		prev_ind = curr_ind;
		prev_component = curr_component;
		prev_inside = curr_inside;
	}

	/*
	Iterator<Vertex> it = vertices.iterator();
	while(it.hasNext())
	{
		Vertex currentVertex = it.next();
		float currentComponent = currentVertex.Get(componentIndex) * componentFactor;
		boolean currentInside = currentComponent <= currentVertex.GetPosition().GetW();

		if(currentInside ^ previousInside)
		{
			float lerpAmt = (previousVertex.GetPosition().GetW() - previousComponent) /
					((previousVertex.GetPosition().GetW() - previousComponent) -
							(currentVertex.GetPosition().GetW() - currentComponent));

			result.add(previousVertex.Lerp(currentVertex, lerpAmt));
		}

		if(currentInside)
		{
			result.add(currentVertex);
		}

		previousVertex = currentVertex;
		previousComponent = currentComponent;
		previousInside = currentInside;
	}
*/
}

void SoftRend::push_tri(const uint32_t *p_inds, Item &r_item, uint32_t p_item_id) {
	Tri tri;
	memcpy(&tri.corns, p_inds, sizeof(uint32_t) * 3);
	tri.bound.unset();
	tri.item_id = p_item_id;

	tri.z_max = FLT_MIN;
	tri.z_min = FLT_MAX;

	if (_tris.size() == 93844) {
		print_line("test  tri");
	}

	Vector2 pt_screen[3];
	for (uint32_t c = 0; c < 3; c++) {
		const Vertex &vert = _vertices[p_inds[c]];
		const Vector3 &cam_coords = vert.coord_cam_space;

		tri.z_max = MAX(tri.z_max, cam_coords.z);
		tri.z_min = MIN(tri.z_min, cam_coords.z);
		//		tri.z_max = MAX(tri.z_max, vert.hcoord.normal.z);
		//		tri.z_min = MIN(tri.z_min, vert.hcoord.normal.z);

		GL_to_viewport_coords(cam_coords, pt_screen[c]);
		_vertices[p_inds[c]].coord_screen = pt_screen[c];

		int32_t x, y;
		x = pt_screen[c].x;
		y = pt_screen[c].y;
		x = CLAMP(x, INT16_MIN, INT16_MAX - 1);
		y = CLAMP(y, INT16_MIN, INT16_MAX - 1);
		tri.bound.expand_to_include(x, y);
	}
	tri.bound.right += 1;
	tri.bound.bottom += 1;

	// cull
	const Vector2 &c0 = pt_screen[0];
	const Vector2 &c1 = pt_screen[1];
	const Vector2 &c2 = pt_screen[2];

	//	tri.c_minus_a = c2 - c0;
	//	tri.b_minus_a = c1 - c0;

	float cross = (c1 - c0).cross(c2 - c0);
	//	float cross = tri.b_minus_a.cross(tri.c_minus_a);

	if (cross >= 0) {
		return;
	}

	_tris.push_back(tri);
	r_item.num_tris += 1;
}

void SoftRend::flush_to_gbuffer_work(uint32_t p_tile_id, void *p_userdata) {
	Tile &tile = _tiles.tiles[p_tile_id];
	tile.clear();

	for (int32_t t = 0; t < tile.tri_list->size(); t++) {
		uint32_t tri_id = (*tile.tri_list)[t];
		const Tri &tri = _tris[tri_id];

		//		if (tri.z_max > 0.9)
		//		{
		//			tile.tri_list->remove_unordered(t);
		//			t--; // repeat this element
		//			continue;

		//		}

		int res = tile_test_tri(tile, tri);
		switch (res) {
			default:
				break;
			case 1: {
				tile.tri_list->remove_unordered(t);
				t--; // repeat this element
			} break;
			case 2: {
				// Should we move full tri to high priority?
				// May be completely hidden by a previous full tri.
				if (true)
				//				if (tri.z_min <= tile.full_tri_max_z)
				{
					tile.priority_tri_list.push_back(tri_id);
					tile.full_tri_max_z = MIN(tile.full_tri_max_z, tri.z_max);

					FinalTri final_tri2;
					for (uint32_t c = 0; c < 3; c++) {
						final_tri2.v[c] = _vertices[tri.corns[c]];
					}
					draw_tri_to_gbuffer(tile, final_tri2, tri_id, tri.item_id + 1, false, true);

					if (!assert_tile_depth_is_at_least(tile, tri.z_max)) {
						print_line("Full tile failed!");
						print_line("Tile cliprect : " + String(tile.clip_rect));
						print_line("Tri : ");
						for (uint32_t c = 0; c < 3; c++) {
							print_line(String(Variant(final_tri2.v[c].coord_screen)));
						}
					}
				}

				tile.tri_list->remove_unordered(t);
				t--; // repeat this element
			} break;
		}
	}

	FinalTri final_tri;

	// High priority
	for (uint32_t t = 0; t < tile.priority_tri_list.size(); t++) {
		uint32_t tri_id = tile.priority_tri_list[t];
		const Tri &tri = _tris[tri_id];

		// Z reject
		//		if (tri.z_min > tile.full_tri_max_z)
		//		{
		//			continue;
		//		}

		for (uint32_t c = 0; c < 3; c++) {
			final_tri.v[c] = _vertices[tri.corns[c]];
		}

		draw_tri_to_gbuffer(tile, final_tri, tri_id, tri.item_id + 1, false, true);
	}

	// Low priority
	//	return;
	for (uint32_t t = 0; t < tile.tri_list->size(); t++) {
		uint32_t tri_id = (*tile.tri_list)[t];
		const Tri &tri = _tris[tri_id];

		for (uint32_t c = 0; c < 3; c++) {
			final_tri.v[c] = _vertices[tri.corns[c]];
		}

		// Z reject
		if (tri.z_min > tile.full_tri_max_z) {
			draw_tri_to_gbuffer(tile, final_tri, tri_id, tri.item_id + 1, true, false);
			continue;
		}

		draw_tri_to_gbuffer(tile, final_tri, tri_id, tri.item_id + 1, false, false);
	}

	DEV_ASSERT(tile.debug_actual_max_z <= tile.full_tri_max_z);
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
	// for debugging clipping, contract a bit

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
	if (!tile.tri_list->size() && !tile.priority_tri_list.size()) {
		// return;
		if (!tile.background_clear) {
			tile.background_clear = true;
			for (uint32_t y = top; y < bottom; y++) {
				uint32_t *frame_buffer = p_frame_buffer_orig + ((state.viewport_height - y - 1) * state.viewport_width) + left;
				for (uint32_t x = left; x < right; x++) {
					*frame_buffer++ = state.clear_rgba;
				}
			}
		}
		return;
	} else {
		tile.background_clear = false;
	}

	// Go through pixels and deferred render.
	for (uint32_t y = top; y < bottom; y++) {
		uint32_t *frame_buffer = p_frame_buffer_orig + ((state.viewport_height - y - 1) * state.viewport_width) + left;
		const SoftSurface::GData *gdata = &state.render_target->get_g(left, y);

		for (uint32_t x = left; x < right; x++) {
			const SoftSurface::GData &g = *gdata++;
			*frame_buffer++ = pixel_shader(x, y, g);
		}
	}
}

uint32_t SoftRend::pixel_shader(uint32_t x, uint32_t y, const SoftSurface::GData &g) const {
	// Background
	if (!g.tri_id_p1) {
		return state.clear_rgba;
	}

	// Get source triangle.
	//	const Tri &tri = _tris[g.tri_id_p1 - 1];
	const Tri &tri = (_tris.ptr())[g.tri_id_p1 - 1];

	//uint32_t item_id = g.item_id_p1 - 1;
	uint32_t item_id = tri.item_id;

	//	const Item &item = _items[item_id];
	const Item &item = (_items.ptr())[item_id];
	uint32_t soft_material_id = item.mesh_instance->get_surface_material_id(item.surf_id);

	//	float u = g.uv.x;
	//	float v = g.uv.y;

	const Vertex &v0 = (_vertices.ptr())[tri.corns[0]];
	const Vertex &v1 = (_vertices.ptr())[tri.corns[1]];
	const Vertex &v2 = (_vertices.ptr())[tri.corns[2]];

	//	Vector2 uvs[3];
	//	uvs[0] = v0.uv;
	//	uvs[1] = v1.uv;
	//	uvs[2] = v2.uv;

	//	for (uint32_t c = 0; c < 3; c++) {
	//		uvs[c] = _vertices[tri.corns[c]].uv;
	//	}

	Vector3 bary;
	//pixel_shader_calculate_bary(x, y, g, bary);
	pixel_shader_calculate_bary2(x, y, v0, v1, v2, bary);
	//	pixel_shader_calculate_bary(x, y, v0.coord_screen, tri.b_minus_a, tri.c_minus_a,Vector3(v0.hcoord.d, v1.hcoord.d, v2.hcoord.d),  bary);

	/*
	{
		find_pixel_bary(v0.coord_screen, v1.coord_screen, v2.coord_screen, Vector2(x, y), bary);

		Vector3 bc_clip = Vector3(bary.x / v0.hcoord.d, bary.y / v1.hcoord.d, bary.z / v2.hcoord.d);

		float clip_denom = bc_clip.x + bc_clip.y + bc_clip.z;
		if (clip_denom <= 0) {
			WARN_PRINT_ONCE("pixel_shader_calculate_bary clip_denom error");
		}
		bary = bc_clip / clip_denom;
	}
*/

	// LOCALLY
	Vector2 uv;
	find_uv_barycentric(v0.uv, v1.uv, v2.uv, uv, bary.x, bary.y, bary.z);

	const SoftMaterial *curr_material = nullptr;
	if (soft_material_id != UINT32_MAX) {
		curr_material = &materials.get(soft_material_id);
	}

	//Color col;
	uint32_t rgba;
	if (curr_material) {
		if (!curr_material->st_albedo.is_empty()) {
			rgba = curr_material->st_albedo.get8(uv);
		} else {
			// WHITE
			rgba = UINT32_MAX;
		}
	} else {
		// Black
		rgba = 255;
	}

	return rgba;
}

/*
void SoftRend::pixel_shader_calculate_bary(uint32_t x, uint32_t y, const Vector2 &a_screen, const Vector2 &b_minus_a, const Vector2 &c_minus_a, const Vector3 &W, Vector3 &r_bary) const
{
	Vector3 bc_screen;
	find_pixel_bary_optimized2(a_screen, b_minus_a, c_minus_a, Vector2(x, y), bc_screen);

	Vector3 bc_clip = Vector3(bc_screen.x / W.x, bc_screen.y / W.y, bc_screen.z / W.z);

	float clip_denom = bc_clip.x + bc_clip.y + bc_clip.z;
	if (clip_denom <= 0) {
		WARN_PRINT_ONCE("pixel_shader_calculate_bary clip_denom error");
	}
	r_bary = bc_clip / clip_denom;

}
*/

void SoftRend::pixel_shader_calculate_bary2(uint32_t x, uint32_t y, const Vertex &v0, const Vertex &v1, const Vertex &v2, Vector3 &r_bary) const {
	Vector3 bc_screen;
	find_pixel_bary(v0.coord_screen, v1.coord_screen, v2.coord_screen, Vector2(x, y), bc_screen);

	Vector3 bc_clip = Vector3(bc_screen.x / v0.hcoord.d, bc_screen.y / v1.hcoord.d, bc_screen.z / v2.hcoord.d);

	float clip_denom = bc_clip.x + bc_clip.y + bc_clip.z;
	if (clip_denom <= 0) {
		WARN_PRINT_ONCE("pixel_shader_calculate_bary clip_denom error");
	}
	r_bary = bc_clip / clip_denom;
}

void SoftRend::set_pixel(float x, float y) {
	if ((x < 0) || (x >= state.fviewport_width))
		return;
	if ((y < 0) || (y >= state.fviewport_height))
		return;
	Color col = Color(1, 1, 1, 1);
	state.render_target->get_image()->set_pixel(x, y, col);
}

int SoftRend::which_side(const Vector2 &wall_a, const Vector2 &wall_vec, const Vector2 &pt) const {
	Vector2 pt_vec = pt - wall_a;
	return (wall_vec.cross(pt_vec) > 0);
}

int SoftRend::tile_test_tri(Tile &p_tile, const Tri &tri) {
	const Rect2i &clip_rect = p_tile.clip_rect;

	Vector2 ps[3];
	for (uint32_t n = 0; n < 3; n++) {
		ps[n] = _vertices[tri.corns[n]].coord_screen;
	}

	// Expand the tile rect to account for off by one errors in the scan conversion.
	Vector2i rpos = clip_rect.position - Vector2i(2, 2);
	Vector2i rsize = clip_rect.size + Vector2i(4, 4);

	int inside = Geometry::is_point_in_triangle(rpos, ps[0], ps[1], ps[2]);
	inside += (Geometry::is_point_in_triangle(rpos + Vector2i(rsize.x, 0), ps[0], ps[1], ps[2]));
	inside += (Geometry::is_point_in_triangle(rpos + Vector2i(0, rsize.y), ps[0], ps[1], ps[2]));
	inside += (Geometry::is_point_in_triangle(rpos + rsize, ps[0], ps[1], ps[2]));

	if (inside == 4) {
		return 2;
	}
	return 0;

	uint32_t tile_inside_edge_count = 0;

	for (uint32_t e = 0; e < 3; e++) {
		const Vector2 &a = ps[e];
		const Vector2 &b = ps[(e + 1) % 3];

		Vector2 wall_vec = b - a;

		//		if (false) {
		if (tile_inside_edge_count == e) {
			//			if (which_side(a, wall_vec, clip_rect.position) &&
			//					which_side(a, wall_vec, clip_rect.position + Vector2i(clip_rect.size.x, 0)) &&
			//					which_side(a, wall_vec, clip_rect.position + Vector2i(0, clip_rect.size.y)) &&
			//					which_side(a, wall_vec, clip_rect.position + clip_rect.size)) {
			//				return 1;
			//			}

			int inside = which_side(a, wall_vec, clip_rect.position);
			inside += which_side(a, wall_vec, clip_rect.position + Vector2i(clip_rect.size.x, 0));
			inside += which_side(a, wall_vec, clip_rect.position + Vector2i(0, clip_rect.size.y));
			inside += which_side(a, wall_vec, clip_rect.position + clip_rect.size);

			if (inside == 4) {
#ifdef DEV_ENABLED
				state.tris_rejected_by_edges += 1;
#endif
				//				DEV_ASSERT (which_side(a, wall_vec, clip_rect.position) &&
				//						which_side(a, wall_vec, clip_rect.position + Vector2i(clip_rect.size.x, 0)) &&
				//						which_side(a, wall_vec, clip_rect.position + Vector2i(0, clip_rect.size.y)) &&
				//						which_side(a, wall_vec, clip_rect.position + clip_rect.size));

				return 1;
			}
			if (!inside) {
				tile_inside_edge_count++;
			}

		} else {
			if (which_side(a, wall_vec, clip_rect.position) &&
					which_side(a, wall_vec, clip_rect.position + Vector2i(clip_rect.size.x, 0)) &&
					which_side(a, wall_vec, clip_rect.position + Vector2i(0, clip_rect.size.y)) &&
					which_side(a, wall_vec, clip_rect.position + clip_rect.size)) {
#ifdef DEV_ENABLED
				state.tris_rejected_by_edges += 1;
#endif
				return 1;
			}
		}
	}
	if (tile_inside_edge_count == 3) {
		//print_line("inside!");

		// Expand the tile rect to account for off by one errors in the scan conversion.
		Vector2i rpos = clip_rect.position - Vector2i(1, 1);
		Vector2i rsize = clip_rect.size + Vector2i(2, 2);

		DEV_ASSERT(Geometry::is_point_in_triangle(rpos, ps[0], ps[1], ps[2]));
		DEV_ASSERT(Geometry::is_point_in_triangle(rpos + Vector2i(rsize.x, 0), ps[0], ps[1], ps[2]));
		DEV_ASSERT(Geometry::is_point_in_triangle(rpos + Vector2i(0, rsize.y), ps[0], ps[1], ps[2]));
		DEV_ASSERT(Geometry::is_point_in_triangle(rpos + rsize, ps[0], ps[1], ps[2]));

		return 2;
	}

	return 0;
}

bool SoftRend::assert_tile_depth_is_at_least(Tile &p_tile, float p_depth) {
	const Rect2i &p_clip_rect = p_tile.clip_rect;
	int32_t y_start = p_clip_rect.position.y;
	int32_t y_end = y_start + p_clip_rect.size.y;

	int32_t x_start = p_clip_rect.position.x;
	int32_t x_end = x_start + p_clip_rect.size.x;

	float max_depth_diff = 0;

	for (int32_t y = y_start; y < y_end; y++) {
		for (int32_t x = x_start; x < x_end; x++) {
			float *z = state.render_target->get_z(x, y);
			//DEV_ASSERT(*z <= p_depth);
			if (*z > p_depth) {
				max_depth_diff = MAX(max_depth_diff, *z - p_depth);
			}
		}
	}

	if (max_depth_diff < CMP_EPSILON) {
		return true;
	}

	return false;
}

void SoftRend::draw_tri_to_gbuffer(Tile &p_tile, const FinalTri &tri, uint32_t p_tri_id, uint32_t p_item_id_p1, bool debug_flag_write, bool debug_full_tri) {
	const Rect2i &p_clip_rect = p_tile.clip_rect;

	//	Vector2 pts[3];
	//	for (uint32_t c = 0; c < 3; c++) {
	//		GL_to_viewport_coords(tri.v[c].coord_cam_space, pts[c]);
	//	}

	Rect2i bound;
	bound.position = tri.v[0].coord_screen;

	for (uint32_t n = 1; n < 3; n++) {
		bound.expand_to(tri.v[n].coord_screen);
	}
	bound.size += Vector2i(1, 1);

	bound = bound.clip(p_clip_rect);
	if (bound.has_no_area())
		return;

	Bound16 bound16;
	bound16.set_safe(bound);

	// Do a more accurate cull of each triangle edge.
	/*
		uint32_t tile_inside_edge_count = 0;

		for (uint32_t e = 0; e < 3; e++) {
			const Vector2 &a = tri.v[e].coord_screen;
			const Vector2 &b = tri.v[(e + 1) % 3].coord_screen;

			Vector2 wall_vec = b - a;

			if (tile_inside_edge_count == e) {
				int inside = which_side(a, wall_vec, p_clip_rect.position);
				inside += which_side(a, wall_vec, p_clip_rect.position + Vector2i(p_clip_rect.size.x, 0));
				inside += which_side(a, wall_vec, p_clip_rect.position + Vector2i(0, p_clip_rect.size.y));
				inside += which_side(a, wall_vec, p_clip_rect.position + p_clip_rect.size);

				if (inside == 4) {
	#ifdef DEV_ENABLED
					state.tris_rejected_by_edges += 1;
	#endif
					return;
				}
				if (!inside) {
					tile_inside_edge_count++;
				}

			} else {
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
		}
		if (tile_inside_edge_count == 3) {
			//print_line("inside!");
		}
		*/

	Vector3 bc_screen;

	const Vector2 &ps0 = tri.v[0].coord_screen;
	const Vector2 &ps1 = tri.v[1].coord_screen;
	const Vector2 &ps2 = tri.v[2].coord_screen;

	Vector3 bary_s[2];
	for (int i = 2; i--;) {
		bary_s[i].coord[0] = ps2.coord[i] - ps0.coord[i];
		bary_s[i].coord[1] = ps1.coord[i] - ps0.coord[i];

		// Just for safety, can be removed.
		bary_s[i].coord[2] = 0;
	}

	int32_t y_start, y_end;
	p_tile.scan_converter.scan_convert_triangle(ps0, ps1, ps2, y_start, y_end);

	y_start = MAX(y_start, bound16.top);
	y_end = MIN(y_end, bound16.bottom);

	if (debug_full_tri) {
		if (y_start != bound16.top) {
			print_line("full tri y start is off by " + itos(y_start - bound16.top));
		}
		if (y_end != bound16.bottom) {
			print_line("full tri y end is off by " + itos(y_end - bound16.bottom));
		}
		//DEV_ASSERT(y_start == bound16.top);
		//DEV_ASSERT(y_end == bound16.bottom);
	}

	uint32_t tri_id_p1 = p_tri_id + 1;

	for (int y = y_start; y < y_end; y++) {
		int32_t x_start, x_end;
		p_tile.scan_converter.get_x_min_max(y, x_start, x_end);

		x_start = MAX(x_start, bound16.left);
		x_end = MIN(x_end, bound16.right);

		if (debug_full_tri) {
			if (x_start != bound16.left) {
				print_line("full tri x start is off by " + itos(x_start - bound16.left));
			}
			if (x_end != bound16.right) {
				print_line("full tri x end is off by " + itos(x_end - bound16.right));
			}
			//DEV_ASSERT(y_start == bound16.top);
			//DEV_ASSERT(y_end == bound16.bottom);
		}

		///////////////////
		// TEST
		const Tri &orig_tri = _tris[p_tri_id];
		//		float z_min = orig_tri.z_min;
		//		float z_max = orig_tri.z_max;
		//		Vector3 pt3;
		//		find_pixel_bary_optimized(bary_s, ps0, ps0, bc_screen);
		//		find_point_barycentric(tri.v[0].coord_cam_space, tri.v[1].coord_cam_space, tri.v[2].coord_cam_space, pt3, bc_screen.x, bc_screen.y, bc_screen.z);
		//		DEV_ASSERT(pt3.z >= z_min);
		//		DEV_ASSERT(pt3.z <= z_max);

		//		find_pixel_bary_optimized(bary_s, ps0, ps1, bc_screen);
		//		find_point_barycentric(tri.v[0].coord_cam_space, tri.v[1].coord_cam_space, tri.v[2].coord_cam_space, pt3, bc_screen.x, bc_screen.y, bc_screen.z);
		//		DEV_ASSERT(pt3.z >= z_min);
		//		DEV_ASSERT(pt3.z <= z_max);

		//		find_pixel_bary_optimized(bary_s, ps0, ps2, bc_screen);
		//		find_point_barycentric(tri.v[0].coord_cam_space, tri.v[1].coord_cam_space, tri.v[2].coord_cam_space, pt3, bc_screen.x, bc_screen.y, bc_screen.z);
		//		DEV_ASSERT(pt3.z >= z_min);
		//		DEV_ASSERT(pt3.z <= z_max);
		//////////////////////

		// get z buffer as one off per line
		float *z_iterator = state.render_target->get_z(x_start, y);

		for (int x = x_start; x < x_end; x++) {
			float *z = z_iterator;
			z_iterator++;

			if (!find_pixel_bary_optimized(bary_s, ps0, Vector2(x, y), bc_screen))
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

			/*
			Vector3 bc_clip = Vector3(bc_screen.x / tri.hcoords[0].d, bc_screen.y / tri.hcoords[1].d, bc_screen.z / tri.hcoords[2].d);

			float clip_denom = bc_clip.x + bc_clip.y + bc_clip.z;
			if (clip_denom <= 0) {
				continue;
			}
			bc_clip = bc_clip / clip_denom;
			//float frag_depth = clipc[2]*bc_clip;
*/

			//			Vector3 bary_test;
			//			SoftSurface::GData gtest;
			//			gtest.blank();
			//			gtest.tri_id_p1 = tri_id_p1;
			//			pixel_shader_calculate_bary(x, y, gtest, bary_test);

			// PURELY for z coordinate?
			Vector3 pt3;
			//find_point_barycentric(tri.coords, pt3, bc_screen.x, bc_screen.y, bc_screen.z);
			find_point_barycentric(tri.v[0].coord_cam_space, tri.v[1].coord_cam_space, tri.v[2].coord_cam_space, pt3, bc_screen.x, bc_screen.y, bc_screen.z);

			if (pt3.z >= *z)
				continue; // failed z test

			//			if (pt3.z < 0)
			//				continue;

			//if (texture_map_tri_to_gbuffer(x, y, tri, bc_clip, p_item_id_p1, tri_id_p1)) {
			if (texture_map_tri_to_gbuffer(x, y, p_item_id_p1, tri_id_p1)) {
				// write z only if not discarded by alpha test
				*z = pt3.z;

				DEV_ASSERT(pt3.z <= (orig_tri.z_max + CMP_EPSILON));

				float tile_z_diff = p_tile.full_tri_max_z - pt3.z;
				DEV_ASSERT(tile_z_diff >= -CMP_EPSILON);

				p_tile.debug_actual_max_z = MIN(p_tile.debug_actual_max_z, pt3.z);
				DEV_ASSERT(!debug_flag_write);
			}
		}
	}
}

bool SoftRend::texture_map_tri_to_gbuffer(int x, int y, uint32_t p_item_id_p1, uint32_t p_tri_id_p1) {
	//bool SoftRend::texture_map_tri_to_gbuffer(int x, int y, const FinalTri &tri, const Vector3 &p_bary, uint32_t p_item_id_p1, uint32_t p_tri_id_p1) {
	//float u, v;
	//find_uv_barycentric(tri.uvs, u, v, p_bary.x, p_bary.y, p_bary.z);

	SoftSurface::GData &gdata = state.render_target->get_g(x, y);
	gdata.tri_id_p1 = p_tri_id_p1;
	//gdata.item_id_p1 = p_item_id_p1;
	//gdata.uv = Vector2(u, v);

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
