#pragma once

#include "core/list.h"
#include "core/local_vector.h"
#include "core/math/aabb.h"
#include "core/math/geometry.h"
#include "core/math/plane.h"
#include "core/math/vector3.h"
#include "core/reference.h"

#ifdef DEBUG_ENABLED
//#define GODOT_SLOWHULL_USE_LOGS
#endif

class SlowHull : public Reference {
	GDCLASS(SlowHull, Reference);

	struct HFace;

	struct HEdge {
		void set(uint32_t p_a, uint32_t p_b) {
			corn[0] = p_a;
			corn[1] = p_b;
		}
		bool operator==(const HEdge &p_edge) const {
			return (corn[0] == p_edge.corn[0]) && (corn[1] == p_edge.corn[1]);
		}
		HEdge sorted() const {
			HEdge e;
			e.set(corn[0], corn[1]);
			e.sort();
			return e;
		}
		void create(const HFace &p_face, int p_corn);
		uint32_t corn[2];
		void sort() {
			if (corn[0] > corn[1]) {
				SWAP(corn[0], corn[1]);
			}
		}
	};

	struct HFace {
		union {
			struct {
				uint32_t a;
				uint32_t b;
				uint32_t c;
			};
			uint32_t corn[3];
		};

		LocalVector<uint32_t> pts;
		Plane plane;
		HFace *neigh[3];

		bool faces_eye;

#ifdef GODOT_SLOWHULL_USE_LOGS
		int id;
#endif

		// outputted in final mesh
		bool done;
		int32_t final_face_id;
		uint32_t check_id;

		HFace() {
#ifdef GODOT_SLOWHULL_USE_LOGS
			id = -1;
#endif
			check_id = 0;
			faces_eye = false;
			done = false;
			final_face_id = -1;
			for (int n = 0; n < 3; n++) {
				corn[n] = 0;
				neigh[n] = nullptr;
			}
		}

		void debug_print();
		bool contains_index(uint32_t p_index) const {
			for (int n = 0; n < 3; n++) {
				if (corn[n] == p_index) {
					return true;
				}
			}
			return false;
		}

		int find_third(uint32_t p_index_a, uint32_t p_index_b) const {
			for (int n = 0; n < 3; n++) {
				uint32_t c = corn[n];
				if ((c != p_index_a) && (c != p_index_b))
					return n;
			}
			return -1;
		}
		uint32_t find_third_corn(uint32_t p_index_a, uint32_t p_index_b) const {
			int which = find_third(p_index_a, p_index_b);
			if (which != -1) {
				return corn[which];
			}
			return -1;
		}

		int find_index(uint32_t p_index) const {
			for (int n = 0; n < 3; n++) {
				if (corn[n] == p_index) {
					return n;
				}
			}
			return -1;
		}

		//		bool find_shared_edge(const HFace &p_o, HEdge &r_edge) const {
		//			int found = 0;
		//			for (int n = 0; n < 3; n++) {
		//				for (int m = 0; m < 3; m++) {
		//					if (corn[n] == p_o.corn[m]) {
		//						switch (found) {
		//							case 0: {
		//								r_edge.corn[0] = corn[n];
		//							} break;
		//							case 1: {
		//								r_edge.corn[1] = corn[n];
		//								return true;
		//							} break;
		//							default: {
		//								return false;
		//							} break;
		//						}
		//						found++;
		//					}
		//				}
		//			}
		//			return false;
		//		}

		bool remove_neighbour(HFace *p_neigh) {
			for (int n = 0; n < 3; n++) {
				if (neigh[n] == p_neigh) {
					neigh[n] = nullptr;
					return true;
				}
			}
			return false;
		}

		void calc_plane(const SlowHull &p_owner) {
			plane = Plane(p_owner._source_pts[corn[0]], p_owner._source_pts[corn[1]], p_owner._source_pts[corn[2]]);
		}
	};

	struct HEdgeList {
		LocalVector<uint32_t> inds;
		//uint32_t operator[](int p_index) const {return inds[p_index];}
		int size() const { return inds.size(); }
		uint32_t get_wrapped(int p_index) const { return inds[(p_index + size()) % size()]; }
		//		bool push_edge(int p_a, int p_b)
		//		{
		//			HEdge edge;
		//			edge.set(p_a, p_b);
		//			edge.sort();
		//			if (!contains(edge))
		//			{
		//				edges.push_back(edge);
		//				return true;
		//			}
		//			return false;
		//		}
		void clear() { inds.clear(); }
		void debug_print();

		// if two of the corns are shared, the third is to be inserted and is returned in which corn.
		// the insertion position is returned. (or -1)
		int find_corn_insert_position(const HFace &p_face, int &r_corn_to_add_id) {
			for (int n = 0; n < size(); n++) {
				int central = inds[n];

				for (int c = 0; c < 3; c++) {
					// found one!
					if (central == p_face.corn[c]) {
						int before = get_wrapped(n - 1);
						int after = get_wrapped(n + 1);

						for (int d = 0; d < 3; d++) {
							int insert_position = -1;

							if (before == p_face.corn[d]) {
								// insert after the before
								insert_position = (n - 1 + size()) % size();
							}
							if (after == p_face.corn[d]) {
								// inssert after the central
								insert_position = n;
							}

							if (insert_position != -1) {
								r_corn_to_add_id = p_face.corn[3 - (c + d)];
								return insert_position;
							}
						}
					}
				}
			}

			return -1;
		}

		bool contains(HEdge p_edge) {
			for (int n = 0; n < inds.size(); n++) {
				if ((inds[n] == p_edge.corn[0]) && (get_wrapped(n + 1) == p_edge.corn[1])) {
					return true;
				}
			}
			return false;
		}

		bool try_insert(int p_which, uint32_t p_corn, const Vector3 &p_normal, const Vector3 *p_source_pts);
	};

	struct HMultiFace {
		struct ExFace {
			ExFace() {
				hface = nullptr;
				base = 0;
			}
			HFace *hface;
			int base; // which corner the fan base is on
			int get_corn(int c) const { return hface->corn[(base + c) % 3]; }
		};

		void reset() {
			exfaces.clear();
			//edges.clear();
			base = -1;
			flares.clear();
		}
		bool push_face(HFace *p_face, const Vector3 *p_source_pts);
		bool push_face_2_tris(HFace *p_face);
		bool push_face_3_tris(HFace *p_face);
		bool push_face_multi_tris(HFace *p_face);
		bool add_flare_to_fan(const HFace &p_face, uint32_t p_new_base, LocalVector<uint32_t> &r_new_flares);

		LocalVector<ExFace> exfaces;
		//HEdgeList edges;
		Plane plane;

		// shared base of the fan
		uint32_t base;
		// each point the fan forms from the base to a flare, two flares forms a triangle in the fan
		LocalVector<uint32_t> flares;
	};

public:
	bool build(const Vector<Vector3> &p_pts, Geometry::MeshData &r_mesh, int p_max_iterations = -1, real_t p_plane_epsilon = 0.0001);
	void clear();

	PoolVector3Array build_hull(PoolVector3Array p_points, int p_max_iterations, real_t p_simplify, int p_method);

	//bool _use_all_points = false;

private:
	void create_initial_simplex();
	List<SlowHull::HFace>::Element *split_face(List<HFace>::Element *p_current_face);
	void split_face_implementation(const HFace &p_face, int p_furthest_point_index);

	void find_lit_faces(Vector<List<SlowHull::HFace>::Element *> &r_lit_elements, const Vector3 &p_pt_furthest);
	void find_edge_horizon(const Vector<List<SlowHull::HFace>::Element *> &p_lit_elements, LocalVector<HEdge> &r_edges);
	bool reestablish_neighbours();

	void fill_output_from_multiface(Geometry::MeshData &r_mesh, const HMultiFace &p_multiface);
	void fill_output_face_recursive(int p_final_face_id, Geometry::MeshData::Face &r_out_face, HFace &p_first_face, HFace *p_parent_face = nullptr, HFace *p_face = nullptr);
	void find_multiface_recursive(HMultiFace &r_multiface, HFace &p_face, const Vector3 *p_source_pts);
	bool try_add_multiface_convex(Geometry::MeshData &r_md, HMultiFace &r_multiface);
	bool is_multiface_convex(const HMultiFace &p_multiface, const HFace &p_face);

	void fill_new_face(HFace &p_new_face);
	uint32_t get_or_create_output_index(LocalVector<uint32_t> &r_out_inds, uint32_t p_in_index);
	void verify_face(const HFace &p_face);
	void find_face_neighbours();

	void verify_faces(bool p_flag_null_neighbours = true);

	//	void merge_coplanar_faces(Geometry::MeshData &r_mesh);
	void sort_polygon_winding(const Vector3 &p_poly_normal, Vector<int> &r_indices, const Vector<Vector3> &p_vertices) const;
	void make_poly_convex(const Vector3 &p_poly_normal, Vector<int> &r_indices, const Vector<Vector3> &p_vertices) const;
	bool is_poly_edge(int p_ind_a, int p_ind_b, const Vector3 &p_poly_normal, Vector<int> &r_indices, const Vector<Vector3> &p_vertices) const;

	void debug_print_faces(bool p_print_positions = false);
	void whittle_source_indices(const LocalVector<Plane> &p_planes, bool p_initial_simplex = false);

	// the original source points passed in
	const Vector3 *_source_pts = nullptr;
	int _num_source_pts = 0;

	// gradually the total list of source points is whittled down
	// making each split faster
	LocalVector<uint32_t> _source_indices_in_play;

	AABB _aabb;
	Vector3 _aabb_center;

	List<HFace> _faces;

	// epsilons
	real_t _plane_epsilon = 0.0;

	real_t _coplanar_dot_epsilon = 0.9;
	real_t _coplanar_dist_epsilon = 0.08;

	// params
	bool _param_final_reduce = true;
	bool _param_recursive_face = false;
	bool _param_simplify = true;

	bool _error_condition = false;

	// each multirect check we use a new check id to mark already searched faces
	uint32_t _check_id = 1;

#ifdef GODOT_SLOWHULL_USE_LOGS
	int _next_id = 0;
#endif

protected:
	static void _bind_methods();
};

/////////////////////////////////////////////
class ConvexMeshSimplify3D {
	struct CFace {
		Plane plane;
		Vector<int> inds; // vertex indices on this plane

		// these are kept to give a more accurate plane when averaging all tris
		Plane running_plane;
		int running_tris = 0;

		void push_index(int p_ind) {
			for (int n = 0; n < inds.size(); n++) {
				if (inds[n] == p_ind) {
					return;
				}
			}
			inds.push_back(p_ind);
		}
	};

public:
	bool simplify(Geometry::MeshData &r_mesh, real_t p_simplification);

private:
	void output_mesh(Geometry::MeshData &r_mesh);
	void fill_initial_planes(Geometry::MeshData &r_mesh);
	void create_verts();
	void sort_winding_order();
	bool sort_polygon_winding(const Vector3 &p_poly_normal, Vector<int> &r_indices, const Vector3 *p_vertices, int p_num_verts) const;
	int find_or_add_vert(const Vector3 &p_pos);

	bool vertex_approx_equal(const Vector3 &p_a, const Vector3 &p_b, real_t p_epsilon = 0.01) {
		real_t d = Math::abs(p_b.x - p_a.x);
		if (d > p_epsilon)
			return false;
		d = Math::abs(p_b.y - p_a.y);
		if (d > p_epsilon)
			return false;
		d = Math::abs(p_b.z - p_a.z);
		if (d > p_epsilon)
			return false;

		return true;
	}

	real_t _coplanar_dist_epsilon = 0.05; // 1.0
	real_t _coplanar_dot_epsilon = 0.995; // 0.99

	LocalVector<Vector3> _verts;
	LocalVector<CFace> _faces;
};
