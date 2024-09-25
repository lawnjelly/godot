#pragma once

#include "navphysics_mesh.h"
#include "navphysics_pointf.h"

namespace NavPhysics {
class Loader {
public:
	struct SourceMeshData {
		const FPoint3 *verts = nullptr;
		const u32 *indices = nullptr;

		const u32 *poly_num_indices = nullptr;

		u32 num_verts = 0;
		u32 num_indices = 0;
		u32 num_polys = 0;
	};

	bool load_mesh(const SourceMeshData &p_source_mesh, Mesh &r_mesh);
	//	SourceMeshData save_mesh(const Mesh &p_mesh);

private:
	bool load_polys(const SourceMeshData &p_mesh, Mesh &r_dest);

	u32 find_or_create_vert(Mesh &r_dest, const FPoint3 &p_pt);
	void plane_from_poly_newell(Mesh &r_dest, Poly &r_poly) const;

	void load_fixed_point_verts(Mesh &r_dest);
	void find_links(Mesh &r_dest);
	void find_walls(Mesh &r_dest);
	void find_bottlenecks(Mesh &r_dest);

	void find_index_nexts(Mesh &r_dest);
};

} // namespace NavPhysics
