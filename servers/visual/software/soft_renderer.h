#pragma once

#include "core/math/plane.h"
#include "core/math/vector2.h"
#include "core/os/thread_work_pool.h"
#include "servers/visual/software/soft_surface.h"
#include "soft_material.h"

class SoftMesh;
class Transform;
struct CameraMatrix;

class SoftRend {
	class ScanConverter {
		LocalVector<int32_t> scan_buffer;
		int32_t _y_min = 0;
		int32_t _y_max = 0;
		uint32_t _y_height = 0;

	public:
		void create(int32_t p_y_min, uint32_t p_height) {
			scan_buffer.resize(p_height * 2);
			scan_buffer.fill(INT32_MAX);
			_y_min = p_y_min;
			_y_height = p_height;
			_y_max = _y_min + _y_height;
		}

		void scan_convert_triangle(const Vector2 &p_min_y_vert, const Vector2 &p_mid_y_vert, const Vector2 &p_max_y_vert);
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

		static uint32_t tile_width;
		static uint32_t tile_height;
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
#endif // NO_THREADS
	} state;

	struct FinalTri {
		Plane hcoords[3];
		Vector3 coords[3];
		Vector2 uvs[3];
		bool is_front_facing() const {
			Vector3 normal_camera_space;
			// backface cull
			normal_camera_space = (coords[2] - coords[1]).cross((coords[1] - coords[0]));
			return normal_camera_space.z >= 0;
		}
	};

	// Source meshes have an item per surface,
	// every time the mesh is drawn.
	struct Item {
		// The source mesh.
		SoftMesh *mesh = nullptr;
		uint32_t surf_id = 0;

		// The first vertex in the pre-transformed list.
		uint32_t first_list_vert = 0;
		uint32_t first_tri = 0;
		uint32_t num_tris = 0;
	};

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
	};

	LocalVector<Item> _items;
	LocalVector<Tri> _tris;

	struct Vertices {
		void clear() {
			cam_space_coords.clear();
			hcoords.clear();
			uvs.clear();
		}
		void resize(uint32_t p_size) {
			cam_space_coords.resize(p_size);
			hcoords.resize(p_size);
			uvs.resize(p_size);
		}
		void set(uint32_t p_index, const Plane &p_hcoord, const Vector3 &p_coord, const Vector2 &p_uv) {
			cam_space_coords[p_index] = p_coord;
			hcoords[p_index] = p_hcoord;
			uvs[p_index] = p_uv;
		}
		uint32_t size() const { return hcoords.size(); }
		LocalVector<Vector3> cam_space_coords;
		LocalVector<Plane> hcoords;
		LocalVector<Vector2> uvs;
	} _vertices;

	struct Tile {
		Rect2i clip_rect;
		const LocalVector<uint32_t> *tri_list = nullptr;
		ScanConverter scan_converter;
		void create() {
			scan_converter.create(clip_rect.position.y, clip_rect.size.y);
		}

		// Optimization...
		// If this tile is already clear, no need to reclear.
		bool clear = false;
	};

	struct Tiles {
		LocalVector<Tile> tiles;
		uint32_t tile_width = 0;
		uint32_t tile_height = 0;
		uint32_t tiles_x = 0;
		uint32_t tiles_y = 0;
	} _tiles;

	bool which_side(const Vector2 &wall_a, const Vector2 &wall_vec, const Vector2 &pt) const;
	void draw_tri_to_gbuffer(Tile &p_tile, const FinalTri &tri, uint32_t p_tri_id, uint32_t p_item_id_p1);
	void set_pixel(float x, float y);
	bool texture_map_tri(int x, int y, const FinalTri &tri, const Vector3 &p_bary);

	bool texture_map_tri_to_gbuffer(int x, int y, const FinalTri &tri, const Vector3 &p_bary, uint32_t p_item_id_p1);
	void flush_to_gbuffer_work(uint32_t p_tile_id, void *p_userdata);

	void flush_final(uint32_t p_tile_id, uint32_t *p_frame_buffer_orig);

	String itof(float f) { return String(Variant(f)); }

	// Nodes
	void link_leaf_nodes_to_tiles(Node *p_node);
	void fill_tree(Node *p_node, const LocalVector<uint32_t> &p_parent_tri_list);

public:
	void set_render_target(SoftSurface *p_soft_surface);
	void prepare();
	void flush();
	void push_mesh(SoftMesh &r_softmesh, const Transform &p_cam_transform, const CameraMatrix &p_cam_projection, const Transform &p_instance_xform);

	SoftMaterials materials;

	SoftRend();
	~SoftRend();

private:
	void GL_to_viewport_coords(const Vector3 &p_v, Vector2 &r_pt) const {
		r_pt.x = ((p_v.x + 1.0f) * 0.5f) * state.fviewport_width;
		r_pt.y = ((p_v.y + 1.0f) * 0.5f) * state.fviewport_height;
	}

	bool find_pixel_bary_optimized(Vector3 *s, const Vector2 &A, const Vector2 &P, Vector3 &ptBary) const {
		//Vector3 s[2];

		for (int i = 2; i--;) {
			//s[i].coord[0] = C.coord[i] - A.coord[i];
			//s[i].coord[1] = B.coord[i] - A.coord[i];
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

	void find_point_barycentric(const Vector3 *p_tri, Vector3 &pt, float u, float v, float w) const {
		pt.x = (p_tri[0].x * u) + (p_tri[1].x * v) + (p_tri[2].x * w);
		pt.y = (p_tri[0].y * u) + (p_tri[1].y * v) + (p_tri[2].y * w);
		pt.z = (p_tri[0].z * u) + (p_tri[1].z * v) + (p_tri[2].z * w);
	}

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

	void find_uv_barycentric(const Vector2 *p_uvs, float &fU, float &fV, float u, float v, float w) const {
		fU = (p_uvs[0].x * u) + (p_uvs[1].x * v) + (p_uvs[2].x * w);
		fV = (p_uvs[0].y * u) + (p_uvs[1].y * v) + (p_uvs[2].y * w);
	}
};
