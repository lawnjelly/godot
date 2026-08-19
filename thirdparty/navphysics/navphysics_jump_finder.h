#pragma once

#include "navphysics_mesh.h"
#include "navphysics_pointf.h"

namespace NavPhysics {

class JumpFinder {
	struct JumpInfo {
		TVector<u32> to_wall_id;
		TVector<u32> to_poly_id;
	};

	struct Data {
		Vector<JumpInfo> jump_infos;
		TVector<u32> wall_jump_info;
		void clear() {
			jump_infos.clear();
			wall_jump_info.clear();
		}
	} data;

	void find_poly_jumps(Mesh &r_mesh, u32 p_wall_id, u32 p_poly_id);
	void find_wall_jumps(Mesh &r_mesh, u32 p_wall_id);
	bool jump_wall_within_range(const IPoint2 &p_a, const IPoint2 &p_b, const IPoint2 &p_c, const IPoint2 &p_d, freal p_range) const;
	void add_wall_jump(Mesh &r_mesh, u32 p_wall_id_from, u32 p_wall_id_to);
	void add_poly_jump(u32 p_wall_id_from, u32 p_poly_id_to);

public:
	void find_jumps(Mesh &r_mesh);
};

} //namespace NavPhysics
