#pragma once

#include "navphysics_pointf.h"

namespace NavPhysics {

class Mesh;

class MeshFuncs {
	u32 find_nearest_wall(const Mesh &p_mesh, const FPoint3 &p_from, const FPoint3 &p_to);
	void get_closest_points_between_segments(const FPoint3 &p_p0, const FPoint3 &p_p1, const FPoint3 &p_q0, const FPoint3 &p_q1, FPoint3 &r_ps, FPoint3 &r_qt);
	freal get_closest_distance_between_segments(const FPoint3 &p_p0, const FPoint3 &p_p1, const FPoint3 &p_q0, const FPoint3 &p_q1);

public:
	void editor_toggle_wall_connection(Mesh &r_mesh, const FPoint3 &p_from, const FPoint3 &p_to);
};

} //namespace NavPhysics
