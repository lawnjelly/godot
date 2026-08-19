#include "navphysics_bsp.h"
#include "navphysics_mesh.h"

namespace NavPhysics {

Vector<BSP::BuildNode> BSP::build_nodes;

void BSP::build(const Mesh &p_mesh) {
	clear();

	// First fill first build node.
	build_nodes.resize(1);
	BuildNode *bn = &builds_nodes[0];
	u32 count = p_mesh.get_num_polys();
	bn->poly_ids.resize(count);

	for (u32 n = 0; n < count; n++) {
		bn->poly_ids[n] = n;
	}

	// Don't use recursive here...
	while (true) {
		find_splitting_wall(bn);
	}

	build_nodes.clear();
}

void BSP::clear() {
	nodes.clear();
	leaves.clear();
	leaf_poly_ids.clear();
}

const u32 *BSP::find_leaf(IPoint2 &p_pt, u32 &r_num_polys) const {
	return nullptr;
}

} //namespace NavPhysics
