#pragma once

#include "navphysics_mesh.h"
#include "navphysics_pointf.h"

namespace NavPhysics {

class MeshExtender {
	void extend_edge(Mesh &r_mesh, u32 p_edge_id);

	struct EdgeWall {
		u32 connecting_wall_id = UINT32_MAX;
		u32 poly_id = UINT32_MAX;
		IPoint2 normal;

		// The locations of the wall verts,
		// pre-adjusted by the vertex swap.
		IPoint2 a;
		IPoint2 b;

		// Pre-swapped.
		u32 ind_a = UINT32_MAX;
		u32 ind_b = UINT32_MAX;

		// The edge wall can have exactly
		// one vertex, which may be shared.
		// The index into _edge_pts.
		u32 ind_c = UINT32_MAX;

		// The final index into the vertex in the mesh.
		u32 ind_c_final = UINT32_MAX;

		// Each edge wall can be external or internal.
		bool external = true;
	};

	struct Edge {
		u32 leading_pt = UINT32_MAX;
		bool merge_leading_to_c = false;
		bool merge_c_to_leading = false;

		TVector<EdgeWall> walls;
		bool loop = false;
		//		u32 first() const { return cwalls[0]; }
		//		u32 last() const { return *cwalls.last(); }
	};

	Vector<Edge> _edges;

	struct EdgePoint {
		IPoint2 pos;
		u32 final_vert_id = UINT32_MAX;
	};

	Vector<EdgePoint> _edge_pts;

	void push_extended_connection_new(Mesh &r_mesh, const EdgeWall &p_edge_wall, u32 p_ind_a, u32 p_ind_b);

	void find_edges(const Mesh &p_mesh);
	void add_cwall_to_edges(u32 p_cwall_id, const Mesh &p_mesh);
	void try_join_edge(u32 p_edge_id, const Mesh &p_mesh);
	i32 can_join_walls(const EdgeWall &p_wall_a, const EdgeWall &p_wall_b) const;
	void identify_loop_edges(const Mesh &p_mesh);

	bool calculate_edge_wall(Edge &r_edge, u32 p_wall_id, const Mesh &p_mesh);
	void finalize_edge_wall(Edge &r_edge, u32 p_wall_id, Mesh &r_mesh);
	void _add_edge_point(const Mesh &p_mesh, const IPoint2 &p_pt);

	u32 create_final_mesh_vert(const EdgeWall &p_wall, Mesh &r_mesh, u32 p_edge_point);

	//void orientate_edges(const Mesh &p_mesh);
	//const IPoint2 &get_wall_normal(const Mesh &p_mesh, const Edge &p_edge, u32 p_which) const;
	//const Wall &get_wall(const Mesh &p_mesh, const Edge &p_edge, u32 p_which) const;

	void push_explicit_internal_wall_pairs(Mesh &r_mesh);

	void llog(String p_sz);

public:
	bool extend_mesh(Mesh &r_mesh);
	bool unextend_mesh(Mesh &r_mesh);

	static bool fill_poly(const Mesh &p_mesh, const Mesh::SubMesh &p_submesh, Poly &r_poly);
	static void calculate_poly_bound(Mesh::SubMesh &r_submesh, u32 p_poly_id);

	static bool plane_from_poly_newell(const TVector<FPoint3> &p_verts, const TVector<u32> &p_inds, Poly &r_poly);
};

} //namespace NavPhysics
