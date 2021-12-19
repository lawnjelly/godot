#include "mesh_simplify.h"

bool MeshSimplify::simplify(const LocalVectori<uint32_t> &p_inds, const LocalVectori<Vector3> &p_verts, LocalVectori<uint32_t> &r_inds) {
	_verts = p_verts;

	return true;
}
