#pragma once

#include "servers/visual/visual_server_constants.h"

#include "core/fixed_array.h"
#include "core/math/camera_matrix.h"
#include "core/math/plane.h"
#include "core/math/vector2.h"
#include "core/os/thread_work_pool.h"
#include "servers/visual/software/soft_surface.h"
#include "soft_material.h"

class SoftMeshInstance;
class SoftMeshes;
//class Transform;
//struct CameraMatrix;

class SoftRend {
	class ScanConverter {
		LocalVector<int32_t> scan_buffer;
		int32_t _y_min = 0;
		int32_t _y_max = 0;
		uint32_t _y_height = 0;

		void _scan_convert_triangle(const Vector2 &p_min_y_vert, const Vector2 &p_mid_y_vert, const Vector2 &p_max_y_vert);

	public:
		void create(int32_t p_y_min, uint32_t p_height) {
			scan_buffer.resize(p_height * 2);
			scan_buffer.fill(INT32_MAX);
			_y_min = p_y_min;
			_y_height = p_height;
			_y_max = _y_min + _y_height;
		}

		void scan_convert_triangle(Vector2 t0, Vector2 t1, Vector2 t2, int32_t &r_y_start, int32_t &r_y_end);
		void scan_convert_line(const Vector2 &p_min_y_vert, const Vector2 &p_max_y_vert, int32_t p_which_side);
		void get_x_min_max(int32_t p_y, int32_t &r_x_min, int32_t &r_x_max) const;
	};

	struct Node {
		Rect2i clip_rect;
		LocalVector<uint32_t> tris;
		Node *children[2];

		// The corresponding tile in the tile list if a leaf node.
		uint32_t tile_id_p1 = 0;

		void build_tree_split();
		void debug_tree(uint32_t p_depth = 0);

		// Blank children using value initialization.
		Node() :
				children() {}
		~Node();

		static uint32_t tile_length;
		static uint32_t tile_num_pixels;
	};

	struct State {
		SoftSurface *render_target = nullptr;
		const SoftMaterial *curr_material = nullptr;

		float fviewport_width = 0;
		float fviewport_height = 0;
		int32_t viewport_width = 0;
		int32_t viewport_height = 0;

		Color clear_color;
		uint32_t clear_rgba = 0;
		uint32_t frame_count = UINT32_MAX;

		Node *tree = nullptr;

		// Stats
		uint32_t tris_rejected_by_edges = 0;

#ifndef NO_THREADS
		/// Pooled threads for computing steps
		ThreadWorkPool thread_pool;
		int thread_cores = 0;

		Mutex tri_mutex;

		uint32_t draw_calls_per_thread = 1;
		uint32_t tiles_per_thread = 1;

#endif // NO_THREADS
	} state;

	struct Vertex {
		//		void set(const Plane &p_hcoord, const Vector3 &p_coord, const Vector2 &p_uv, bool p_inside) {
		void set(const Plane &p_hcoord, const Vector3 &p_coord, const Vector2 &p_uv) {
			coord_cam_space = p_coord;
			hcoord = p_hcoord;
			uv = p_uv;
			//inside_frustum = p_inside;
		}
		Vector3 coord_cam_space;
		Plane hcoord;
		Vector2 coord_screen;
		Vector2 uv;
		//bool inside_frustum = true;
	};

	struct FinalTri {
		Vertex v[3];
	};

	// Source meshes have an item per surface,
	// every time the mesh is drawn.
	struct Item {
		// The source mesh.
		SoftMeshInstance *mesh_instance = nullptr;
		uint32_t surf_id = 0;

		// The first vertex in the pre-transformed list.
		uint32_t first_list_vert = 0;
		uint32_t first_tri = 0;
		uint32_t num_tris = 0;

		// The instance ID in the cull list in the visual server.
		uint32_t vs_instance_id = 0;
	};

#ifdef VISUAL_SERVER_SOFTREND_OCCLUSION_CULL
	struct CullState {
		bool visible = false;
	};
	LocalVector<CullState> _cull_states;
#endif

	struct Bound16 {
		int16_t left;
		int16_t top;
		int16_t right;
		int16_t bottom;
		void set(const Rect2i &p_rect) {
			left = p_rect.position.x;
			top = p_rect.position.y;
			right = left + p_rect.size.x;
			bottom = top + p_rect.size.y;
		}
		void set_safe(const Rect2i &p_rect) {
			left = CLAMP(p_rect.position.x, INT16_MIN, INT16_MAX);
			top = CLAMP(p_rect.position.y, INT16_MIN, INT16_MAX);
			right = CLAMP(left + p_rect.size.x, INT16_MIN, INT16_MAX);
			bottom = CLAMP(top + p_rect.size.y, INT16_MIN, INT16_MAX);
		}
		bool intersects(const Bound16 &o) const {
			return !((left >= o.right) || (o.left >= right) || (top >= o.bottom) || (o.top >= bottom));
		}
		void expand_to_include(int16_t x, int16_t y) {
			if (x < left)
				left = x;
			if (x > right)
				right = x;
			if (y < top)
				top = y;
			if (y > bottom)
				bottom = y;
		}
		void unset() {
			left = INT16_MAX;
			right = INT16_MIN;
			top = INT16_MAX;
			bottom = INT16_MIN;
		}
	};

	struct Tri {
		uint32_t corns[3];
		uint32_t item_id;
		Bound16 bound;
		float z_min;
		float z_max;

		bool operator<(const Tri &p_tri) const { return z_min > p_tri.z_min; }
	};

	struct DrawCall {
		Transform view_matrix;
		CameraMatrix proj_matrix;
		uint32_t first_item;
		uint32_t num_items;
	};

	LocalVector<DrawCall> _drawcalls;
	LocalVector<Item> _items;
	LocalVector<Tri> _tris;

	LocalVector<Vertex, uint32_t, true> _vertices;

	struct GData {
		uint32_t tri_id_p1;
		float z;
		void blank() {
			tri_id_p1 = 0;
			z = FLT_MAX;
		}
	};

	struct Tile {
		Rect2i clip_rect;
		LocalVector<uint32_t> *tri_list = nullptr;

		LocalVector<uint32_t> priority_tri_list;

		LocalVector<GData> gbuffer;

		float full_tri_max_z = FLT_MAX;
		float debug_actual_max_z = FLT_MAX;

		ScanConverter scan_converter;
		void create() {
			scan_converter.create(clip_rect.position.y, clip_rect.size.y);

			gbuffer.resize(SoftRend::Node::tile_num_pixels);
			GData g;
			g.blank();
			gbuffer.fill(g);
		}

		void clear() {
			priority_tri_list.clear();
			full_tri_max_z = FLT_MAX;
			debug_actual_max_z = FLT_MAX;
			GData g;
			g.blank();
			gbuffer.fill(g);
		}

		// relative to viewport
		GData *get_g(int32_t p_x, int32_t p_y) {
			p_x -= clip_rect.position.x;
			p_y -= clip_rect.position.y;
			uint32_t pos = (p_y * Node::tile_length) + p_x;
			if (pos < gbuffer.size()) {
				return &gbuffer[pos];
			}
			return nullptr;
		}

		// Optimization...
		// If this tile is already clear, no need to reclear.
		bool background_clear = false;
	};

	struct Tiles {
		LocalVector<Tile> tiles;
		uint32_t tile_width = 0;
		uint32_t tile_height = 0;
		uint32_t tiles_x = 0;
		uint32_t tiles_y = 0;
	} _tiles;

	int which_side(const Vector2 &wall_a, const Vector2 &wall_vec, const Vector2 &pt) const;
	int tile_test_tri(Tile &p_tile, const Tri &tri);
	void draw_tri_to_gbuffer(Tile &p_tile, const FinalTri &tri, uint32_t p_tri_id, uint32_t p_item_id_p1, bool debug_flag_write, bool debug_full_tri);
	bool assert_tile_depth_is_at_least(Tile &p_tile, float p_depth);
	void set_pixel(float x, float y);

	//bool texture_map_tri_to_gbuffer(int x, int y, const FinalTri &tri, const Vector3 &p_bary, uint32_t p_item_id_p1, uint32_t p_tri_id_p1);
	void flush_to_gbuffer_work(uint32_t p_tile_id, void *p_userdata);

	void flush_final(uint32_t p_tile_id, uint32_t *p_frame_buffer_orig);
	void flush_tile_find_visible_instances(uint32_t p_tile_id);
	void flush_draw_calls();

	void thread_draw_call_clip(uint32_t p_drawcall, uint32_t *p_dummy);
	void thread_draw_call_transform(uint32_t p_drawcall_batch, uint32_t *p_dummy);

	void thread_do_tile_work(uint32_t p_tile_batch, uint32_t *p_frame_buffer_orig) {
		uint32_t first_tile = p_tile_batch * state.tiles_per_thread;
		for (uint32_t n = 0; n < state.tiles_per_thread; n++) {
			uint32_t tile_id = first_tile + n;
			if (tile_id >= _tiles.tiles.size())
				break;

			flush_to_gbuffer_work(tile_id, nullptr);
#ifdef VISUAL_SERVER_SOFTREND_OCCLUSION_CULL
			flush_tile_find_visible_instances(tile_id);
#else
			flush_final(tile_id, p_frame_buffer_orig);
#endif
		}
	}

	String itof(float f) { return String(Variant(f)); }

	// Nodes
	void link_leaf_nodes_to_tiles(Node *p_node);
	void fill_tree(Node *p_node, const LocalVector<uint32_t> &p_parent_tri_list);

	void clip_tri(const uint32_t *p_inds, Item &r_item, uint32_t p_item_id);
	void push_tri(const uint32_t *p_inds, Item &r_item, uint32_t p_item_id);
	bool is_inside_view_frustum(uint32_t p_ind);
	bool is_hcoord_inside_view_frustum(const Plane &p_hcoord) const;
	bool clip_polygon_axis(FixedArray<uint32_t, 16> &inds, FixedArray<uint32_t, 16> &aux, int32_t component_index);
	void clip_polygon_component(FixedArray<uint32_t, 16> &inds, FixedArray<uint32_t, 16> &result, int32_t component_index, float component_factor);

public:
	void set_render_target(SoftSurface *p_soft_surface);
	void prepare();
	void flush();
	void push_mesh(SoftMeshInstance &r_softmesh, const Transform &p_cam_transform, const CameraMatrix &p_cam_projection, const Transform &p_instance_xform, uint32_t p_vs_instance_id);
#ifdef VISUAL_SERVER_SOFTREND_OCCLUSION_CULL
	void set_num_instances(uint32_t p_count) {
		_cull_states.resize(p_count);
		_cull_states.fill(CullState());
	}
	bool is_instance_visible(uint32_t p_index) const {
		return _cull_states[p_index].visible;
	}
#endif

	SoftMaterials materials;
	SoftMeshes *meshes = nullptr;

	SoftRend();
	~SoftRend();

private:
	void GL_to_viewport_coords(const Vector3 &p_v, Vector2 &r_pt) const {
		r_pt.x = ((p_v.x + 1.0f) * 0.5f) * state.fviewport_width;
		r_pt.y = ((p_v.y + 1.0f) * 0.5f) * state.fviewport_height;
	}

	bool find_pixel_bary_optimized(Vector3 *s, const Vector2 &A, const Vector2 &P, Vector3 &ptBary) const {
		for (int i = 2; i--;) {
			s[i].coord[2] = A.coord[i] - P.coord[i];
		}
		Vector3 u = s[0].cross(s[1]);
		if (Math::absf(u.coord[2]) > 0.01f) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
		{
			ptBary.x = 1.f - (u.x + u.y) / u.z;
			ptBary.y = u.y / u.z;
			ptBary.z = u.x / u.z;
			return true;
		}

		ptBary = Vector3(-1, 1, 1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
		return false;
	}

	bool find_pixel_bary_optimized2(const Vector2 &A, const Vector2 &B_MINUS_A, const Vector2 &C_MINUS_A, const Vector2 &P, Vector3 &ptBary) const {
		Vector3 s[2];
		s[0].x = C_MINUS_A.x;
		s[0].y = B_MINUS_A.x;
		s[1].x = C_MINUS_A.y;
		s[1].y = B_MINUS_A.y;
		for (int i = 2; i--;) {
			s[i].coord[2] = A.coord[i] - P.coord[i];
		}
		Vector3 u = s[0].cross(s[1]);
		if (Math::absf(u.coord[2]) > 0.01f) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
		{
			ptBary.x = 1.f - (u.x + u.y) / u.z;
			ptBary.y = u.y / u.z;
			ptBary.z = u.x / u.z;
			return true;
		}

		ptBary = Vector3(-1, 1, 1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
		return false;
	}

	bool find_pixel_bary(const Vector2 &A, const Vector2 &B, const Vector2 &C, const Vector2 &P, Vector3 &ptBary) const {
		Vector3 s[2];

		for (int i = 2; i--;) {
			s[i].coord[0] = C.coord[i] - A.coord[i];
			s[i].coord[1] = B.coord[i] - A.coord[i];
			s[i].coord[2] = A.coord[i] - P.coord[i];
		}
		Vector3 u = s[0].cross(s[1]);
		if (Math::absf(u.coord[2]) > 0.01f) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
		{
			ptBary.x = 1.f - (u.x + u.y) / u.z;
			ptBary.y = u.y / u.z;
			ptBary.z = u.x / u.z;
			return true;
		}

		ptBary = Vector3(-1, 1, 1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
		return false;
	}

	void find_point_barycentric(const Vector3 &v0, const Vector3 &v1, const Vector3 &v2, Vector3 &pt, float u, float v, float w) const {
		pt.x = (v0.x * u) + (v1.x * v) + (v2.x * w);
		pt.y = (v0.y * u) + (v1.y * v) + (v2.y * w);
		pt.z = (v0.z * u) + (v1.z * v) + (v2.z * w);
	}
	//	void find_point_barycentric(const Vector3 *p_tri, Vector3 &pt, float u, float v, float w) const {
	//		pt.x = (p_tri[0].x * u) + (p_tri[1].x * v) + (p_tri[2].x * w);
	//		pt.y = (p_tri[0].y * u) + (p_tri[1].y * v) + (p_tri[2].y * w);
	//		pt.z = (p_tri[0].z * u) + (p_tri[1].z * v) + (p_tri[2].z * w);
	//	}

	void find_point4_barycentric(const Plane *p_tri, Vector3 &pt, float u, float v, float w) const {
		pt.x = (p_tri[0].normal.x * u) + (p_tri[1].normal.x * v) + (p_tri[2].normal.x * w);
		pt.y = (p_tri[0].normal.y * u) + (p_tri[1].normal.y * v) + (p_tri[2].normal.y * w);
		pt.z = (p_tri[0].normal.z * u) + (p_tri[1].normal.z * v) + (p_tri[2].normal.z * w);
	}

	void find_normal_barycentric(const Vector3 *p_tri, Vector3 &norm, float u, float v, float w) const {
		norm.x = (p_tri[0].x * u) + (p_tri[1].x * v) + (p_tri[2].x * w);
		norm.y = (p_tri[0].y * u) + (p_tri[1].y * v) + (p_tri[2].y * w);
		norm.z = (p_tri[0].z * u) + (p_tri[1].z * v) + (p_tri[2].z * w);
		norm.normalize();
	}

	void find_uv_barycentric(const Vector2 &uv0, const Vector2 &uv1, const Vector2 &uv2, Vector2 &uv, float u, float v, float w) const {
		uv.x = (uv0.x * u) + (uv1.x * v) + (uv2.x * w);
		uv.y = (uv0.y * u) + (uv1.y * v) + (uv2.y * w);
	}

	Plane lerp_hcoord(const Plane &a, const Plane &b, float f) const {
		Plane lerped;
		lerped.normal = a.normal + ((b.normal - a.normal) * f);
		lerped.d = a.d + ((b.d - a.d) * f);
		return lerped;
	}

	uint32_t pixel_shader(uint32_t x, uint32_t y, const GData &g) const;

	void pixel_shader_calculate_bary2(uint32_t x, uint32_t y, const Vertex &v0, const Vertex &v1, const Vertex &v2, Vector3 &r_bary) const {
		Vector3 bc_screen;
		find_pixel_bary(v0.coord_screen, v1.coord_screen, v2.coord_screen, Vector2(x, y), bc_screen);

		Vector3 bc_clip = Vector3(bc_screen.x / v0.hcoord.d, bc_screen.y / v1.hcoord.d, bc_screen.z / v2.hcoord.d);

		float clip_denom = bc_clip.x + bc_clip.y + bc_clip.z;
		if (clip_denom <= 0) {
			WARN_PRINT_ONCE("pixel_shader_calculate_bary clip_denom error");
		}
		r_bary = bc_clip / clip_denom;
	}
};

#ifdef VISUAL_SERVER_SOFTREND_ENABLED
extern SoftRend g_soft_rend;
#endif
