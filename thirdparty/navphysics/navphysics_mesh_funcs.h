#pragma once

#include "navphysics_pointf.h"

namespace NavPhysics {

class Mesh;
struct IPoint2;

class MeshFuncs {
	u32 find_nearest_wall(const Mesh &p_mesh, const FPoint3 &p_from, const FPoint3 &p_to);
	void get_closest_points_between_segments(const FPoint3 &p_p0, const FPoint3 &p_p1, const FPoint3 &p_q0, const FPoint3 &p_q1, FPoint3 &r_ps, FPoint3 &r_qt);

public:
	freal get_closest_distance_between_segments(const FPoint3 &p_p0, const FPoint3 &p_p1, const FPoint3 &p_q0, const FPoint3 &p_q1);
	freal get_closest_distance_between_segments(const IPoint2 &p_p0, const IPoint2 &p_p1, const IPoint2 &p_q0, const IPoint2 &p_q1);
	static FPoint3 get_closest_point_to_segment(const FPoint3 &p_point, const FPoint3 *p_segment);

	void editor_toggle_wall_connection(Mesh &r_mesh, const FPoint3 &p_from, const FPoint3 &p_to, bool p_external_or_internal);
};

} //namespace NavPhysics
