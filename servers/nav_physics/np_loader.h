#pragma once

#include "core/local_vector.h"
#include "core/reference.h"
#include <stdint.h>

//class NavMap;
//class NavMesh;
struct Vector3;
class Plane;
class NavigationMesh;

namespace gd {
struct Polygon;
}

namespace NavPhysics {

class Map;
class Mesh;
struct Poly;

class Loader {
public:
	//bool load(const NavMap &p_navmap, NavPhysics::Map &r_map);
	bool load_mesh(Ref<NavigationMesh> p_navmesh, NavPhysics::Mesh &r_mesh);
	//bool unload_mesh(NavPhysics::Map &r_map, uint32_t p_navphysics_mesh_id);

	// the mesh bounds can be calculated to form a grid for collision detection
	Rect2 calculate_mesh_bound(const NavPhysics::Mesh &p_mesh) const;

private:
	bool load_polys(Ref<NavigationMesh> p_mesh, NavPhysics::Mesh &r_dest);
	void load_fixed_point_verts(NavPhysics::Mesh &r_dest);
	void find_links(NavPhysics::Mesh &r_dest);
	void find_walls(NavPhysics::Mesh &r_dest);
	void find_index_nexts(NavPhysics::Mesh &r_dest);
	uint32_t find_linked_poly(NavPhysics::Mesh &r_dest, uint32_t p_poly_from, uint32_t p_ind_a, uint32_t p_ind_b, uint32_t &r_linked_poly) const;

	void wall_add_neighbour_wall(NavPhysics::Mesh &r_dest, uint32_t p_a, uint32_t p_b);
	uint32_t find_or_create_vert(NavPhysics::Mesh &r_dest, const Vector3 &p_pt);
	void plane_from_poly_newell(NavPhysics::Mesh &r_dest, NavPhysics::Poly &r_poly) const;
};

} //namespace NavPhysics
