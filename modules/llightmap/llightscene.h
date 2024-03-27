#pragma once

#include "llightimage.h"
#include "llighttracer.h"
#include "llighttypes.h"
#include "lmaterials.h"
#include "lvector.h"
#include "scene/3d/mesh_instance.h"

namespace LM {

class LightMapper_Base;

class LightScene {
	friend class LLightMapper;
	friend class LightTracer;

public:
	// each texel can have more than 1 triangle crossing it.
	// this is important for sub texel anti-aliasing, so we need a quick record
	// of all the triangles to test
	struct TexelTriList {
		enum { MAX_TRIS = 11 };
		uint32_t tri_ids[MAX_TRIS];
		uint32_t num_tris;
	};

	void reset();
	bool create(Spatial *pMeshesRoot, int width, int height, int voxel_density, int max_material_size, float emission_density);

	// returns triangle ID (or -1) and barycentric coords
	int find_intersect_ray(const Ray &ray, float &u, float &v, float &w, float &nearest_t, const Vec3i *pVoxelRange = nullptr); //, int ignore_tri_p1 = 0);
	int find_intersect_ray(const Ray &ray, Vector3 &bary, float &nearest_t, const Vec3i *pVoxelRange = nullptr) {
		return find_intersect_ray(ray, bary.x, bary.y, bary.z, nearest_t, pVoxelRange);
	}

	// simple test returns true if any collision
	bool test_intersect_ray(const Ray &ray, float max_dist, const Vec3i &voxel_range, bool bCullBackFaces = false);
	bool test_intersect_ray(const Ray &ray, float max_dist, bool bCullBackFaces = false);
	bool test_intersect_line(const Vector3 &a, const Vector3 &b, bool bCullBackFaces = false);

	// single triangle
	bool test_intersect_ray_triangle(const Ray &ray, float max_dist, int tri_id) const {
		float t;
		if (!ray.test_intersect_edge_form(_tris_edge_form[tri_id], t))
			return false;

		return t <= max_dist;
	}

	void find_uvs_barycentric(int tri, Vector2 &uvs, float u, float v, float w) const {
		_uv_tris[tri].find_uv_barycentric(uvs, u, v, w);
	}
	void find_uvs_barycentric(int tri, Vector2 &uvs, const Vector3 &bary) const {
		_uv_tris[tri].find_uv_barycentric(uvs, bary);
	}

	bool find_emission_color(int tri_id, const Vector3 &bary, Color &texture_col, Color &col);

	// single function
	bool take_triangle_color_sample(int tri_id, const Vector3 &bary, ColorSample &r_sample) const {
		Vector2 uvs;
		_uv_tris_primary[tri_id].find_uv_barycentric(uvs, bary.x, bary.y, bary.z);
		int mat_id_p1 = _tri_lmaterial_ids[tri_id];
		return _materials.find_colors(mat_id_p1, uvs, r_sample);
	}

	// setup
	struct RasterizeTriangleIDParams {
		LightMapper_Base *base = nullptr;
		LightImage<uint32_t> *im_p1 = nullptr;
		LightImage<Vector3> *im_bary = nullptr;
		uint32_t tile_width = 0;
		uint32_t tile_height = 0;
		uint32_t num_tiles_high = 0;
		uint32_t num_tiles_wide = 0;

		LightImage<LocalVector<uint32_t>> temp_image_tris;
	} _rasterize_triangle_id_params;

	void thread_rasterize_triangle_ids(uint32_t p_tile, uint32_t *p_dummy);
	bool rasterize_triangles_ids(LightMapper_Base &base, LightImage<uint32_t> &im_p1, LightImage<Vector3> &im_bary);
	int get_num_tris() const { return _uv_tris.size(); }

	int get_num_meshes() const { return _meshes.size(); }
	MeshInstance *get_mesh(int n) { return _meshes[n]; }

	void find_meshes(Spatial *pNode);

private:
	bool create_from_mesh(int mesh_id, int width, int height);
	bool create_from_mesh_surface(int mesh_id, int surf_id, Ref<Mesh> rmesh, int width, int height);

	void calculate_tri_texel_size(int tri_id, int width, int height);

	void transform_verts(const PoolVector<Vector3> &ptsLocal, PoolVector<Vector3> &ptsWorld, const Transform &tr) const;
	void transform_norms(const PoolVector<Vector3> &normsLocal, PoolVector<Vector3> &normsWorld, const Transform &tr) const;
	void process_voxel_hits(const Ray &ray, const PackedRay &pray, const Voxel &voxel, float &r_nearest_t, int &r_nearest_tri); // int ignore_triangle_id_p1);
	void process_voxel_hits_old(const Ray &ray, const Voxel &voxel, float &r_nearest_t, int &r_nearest_tri);

	bool test_voxel_hits(const Ray &ray, const PackedRay &pray, const Voxel &voxel, float max_dist, bool bCullBackFaces);

	//	PoolVector<Vector3> m_ptPositions;
	//	PoolVector<Vector3> m_ptNormals;
	//PoolVector<Vector2> m_UVs;
	//PoolVector<int> m_Inds;

	struct Rect2i {
		uint16_t min_x = 0;
		uint16_t min_y = 0;
		uint16_t max_x = 0;
		uint16_t max_y = 0;
	};

	LVector<Rect2> _tri_uv_aabbs;
	LVector<Rect2i> _tri_uv_bounds;
	LVector<AABB> _tri_pos_aabbs;
	LVector<MeshInstance *> _meshes;

	// precalculate these .. useful for finding cutting tris.
	LVector<float> _tri_texel_size_world_space;

	LMaterials _materials;

protected:
public:
	LightImage<Color> _image_emission_done;
	LVector<UVTri> _uv_tris;

	LightTracer _tracer;

	LVector<Tri> _tris;
	LVector<Tri> _tri_normals;

	LVector<Plane> _tri_planes;

	// we maintain a list of tris in the form of 2 edges plus a point. These
	// are precalculated as they are used in the intersection test.
	LVector<Tri> _tris_edge_form;

	// stuff for mapping to original meshes
	//	LVector<int> m_Tri_MeshIDs;
	//	LVector<uint16_t> m_Tri_SurfIDs;

	// these are plus 1
	LVector<uint16_t> _tri_lmaterial_ids;

	// a list of triangles that have emission materials
	LVector<EmissionTri> _emission_tris;
	LVector<Vec2_i16> _emission_pixels;
	LBitField_Dynamic _emission_tri_bitfield;
	Mutex _emission_pixels_mutex;

	// these are UVs in the first channel, if present, or 0.
	// This allows mapping back to the albedo etc texture.
	LVector<UVTri> _uv_tris_primary;

	Vec3i _voxel_range;

	bool _use_SIMD;
};

} // namespace LM
