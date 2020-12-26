
int _get_tree_id(BVHHandle p_handle) const
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
void _sort_handles(BVHHandle &p_ha, BVHHandle &p_hb) const
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
float _goodness_of_fit_merge(const BVH_ABB &p_new, const BVH_ABB &p_existing) const
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

void refit_all(int p_tree_id)
{
	refit_downward(_root_node_id[p_tree_id]);
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

void node_update_aabb(TNode &tnode)
{
	tnode.aabb.set_to_max_opposite_extents();
	tnode.height = 0;
	
	if (!tnode.is_leaf())
	{
		for (int n=0; n<tnode.num_children; n++)
		{
			uint32_t child_node_id = tnode.children[n];

			// merge with child aabb			
			const TNode &tchild = _nodes[child_node_id];
			tnode.aabb.merge(tchild.aabb);
			
			// do heights at the same time
			if (tchild.height > tnode.height)
				tnode.height = tchild.height;
		}
		
		// the height of a non leaf is always 1 bigger than the biggest child
		tnode.height ++;
		
#ifdef BVH_CHECKS
		if (!tnode.num_children)
		{
			// the 'blank' aabb will screw up parent aabbs
			WARN_PRINT("BVH_Tree::TNode no children, AABB is undefined");
		}
#endif
	}
	else
	{
		// leaf
		uint32_t leaf_id = tnode.get_leaf_id();
		const TLeaf &leaf = _leaves[leaf_id];
		
		for (int n=0; n<leaf.num_items; n++)
		{
			tnode.aabb.merge(leaf.items[n].aabb);
		}
#ifdef BVH_CHECKS
		if (!leaf.num_items)
		{
			// the 'blank' aabb will screw up parent aabbs
			WARN_PRINT("BVH_Tree::TLeaf no items, AABB is undefined");
		}
#endif
	}
	
}

BVH_ABB q3Combine(const BVH_ABB &a, const BVH_ABB &b)
{
	BVH_ABB c = a;
	c.merge(b);
	return c;
}


void refit_upward(uint32_t p_node_id)
{
	TNode &tnode = _nodes[p_node_id];

	// update overall aabb from the children
	node_update_aabb(tnode);

	if (tnode.parent_id != -1)
	{
		refit_upward(	tnode.parent_id);
	}
}

void refit_upward_and_balance(uint32_t p_node_id)
{
	while (p_node_id != -1)
	{
		uint32_t before = p_node_id;
		p_node_id = balance(p_node_id);
		
		if (before != p_node_id)
		{
			print_line("REBALANCED!");
		}
		
		TNode &tnode = _nodes[p_node_id];
		
		// update overall aabb from the children
		node_update_aabb(tnode);
	
		p_node_id = tnode.parent_id;
	}
}

void refit_downward(uint32_t p_node_id)
{
	TNode &tnode = _nodes[p_node_id];

	// do children first
	if (!tnode.is_leaf())
	{
		for (int n=0; n<tnode.num_children; n++)
		{
			refit_downward(tnode.children[n]);
		}
	}

	node_update_aabb(tnode);
}
