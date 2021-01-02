
int _handle_get_tree_id(BVHHandle p_handle) const
{
	if (USE_PAIRS)
	{
		int tree = 0;
		if (_extra[p_handle.id()].pairable)
			tree = 1;
		return tree;
	}
	return 0;
}

public:
void _handle_sort(BVHHandle &p_ha, BVHHandle &p_hb) const
{
	if (p_ha.id() > p_hb.id())
	{
		BVHHandle temp = p_hb;
		p_hb = p_ha;
		p_ha = temp;
	}
}
private:

float _compute_cost_change_upward(uint32_t p_node_id, const BVH_ABB &p_aabb)
{
	const TNode &tnode = _nodes[p_node_id];

	BVH_ABB merged = p_aabb;
	merged.merge(tnode.aabb);

	float m_size = merged.get_area();

	float b_size = tnode.aabb.get_area();

	// change
	float change = m_size - b_size;

	// recurse upward
	if (tnode.parent_tnode_id_p1)
	{
		change += _compute_cost_change_upward(tnode.parent_tnode_id_p1-1, p_aabb);
	}

	return change;
}


// get the cost of a branch of the tree based on the cost of each non leaf node
float _compute_cost(uint32_t p_node_id)
{
	const TNode &tnode = _nodes[p_node_id];
	if (tnode.is_leaf())
		return 0.0f;

	float cost = 0.0f;
	cost += tnode.aabb.get_size();

	// children
	for (int n=0; n<tnode.num_items; n++)
	{
		cost += _compute_cost(tnode.get_item(n).item_ref_id);
	}

	return cost;
}

/*
float _aabb_surface_area(const AABB &p_aabb) const
{
	const Vector3 &d = p_aabb.size;
	return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
}

float _aabb_size(const AABB &p_aabb) const
{
	return _aabb_surface_area(p_aabb);
//	return p_aabb.size.x * p_aabb.size.y * p_aabb.size.z;
}

void _set_aabb_to_opposites(AABB &aabb)
{
	aabb.position = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
	aabb.size = Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
}
*/

// less change is better
float _compute_goodness_of_fit_merge(const BVH_ABB &p_new, const BVH_ABB &p_existing) const
{
	BVH_ABB merged = p_new;
	merged.merge(p_existing);

	float merged_size = merged.get_area();
	float existing_size = p_existing.get_area();

	return merged_size - existing_size;

//	AABB merged = p_a.merge(p_b);
//	float a_size = _aabb_size(p_a);
//	float b_size = _aabb_size(p_b);

//	// higher is better
//	return -(m_size - (a_size + b_size));
}


void create_root_node(int p_tree)
{
	// if there is no root node, create one
	if (_root_node_id[p_tree] == -1)
	{
		uint32_t root_node_id;
		TNode * node = _nodes.request(root_node_id);
		node->clear();
		_root_node_id[p_tree] = root_node_id;
		
		// make the root node a leaf
		uint32_t leaf_id;
		TLeaf * leaf = _leaves.request(leaf_id);
		leaf->clear();
		node->neg_leaf_id = -leaf_id;
	}
}

bool node_is_leaf_full(TNode &tnode) const
{
	const TLeaf * leaf = node_get_leaf(tnode);
	return leaf->is_full();
}

public:
TLeaf * node_get_leaf(TNode &tnode)
{
	CRASH_COND(!tnode.is_leaf());
	return &_leaves[tnode.get_leaf_id()];
}

const TLeaf * node_get_leaf(const TNode &tnode) const
{
	CRASH_COND(!tnode.is_leaf());
	return &_leaves[tnode.get_leaf_id()];
}

private:


