float _compute_cost_change_upward(uint32_t p_node_id, const ABB &p_aabb)
{
	const TNode &tnode = _nodes[p_node_id];

	ABB merged = p_aabb;
	merged.merge(tnode.aabb);

	float m_size = merged.size();

	float b_size = tnode.aabb.size();

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
float _goodness_of_fit_merge(const ABB &p_new, const ABB &p_existing) const
{
	ABB merged = p_new;
	merged.merge(p_existing);

	float merged_size = merged.get_size();
	float existing_size = p_existing.get_size();

	return merged_size - existing_size;

//	AABB merged = p_a.merge(p_b);
//	float a_size = _aabb_size(p_a);
//	float b_size = _aabb_size(p_b);

//	// higher is better
//	return -(m_size - (a_size + b_size));
}

void refit_all()
{
	refit_downward(_root_node_id);
}

void create_root_node()
{
	// if there is no root node, create one
	if (_root_node_id == -1)
	{
		uint32_t root_node_id;
		TNode * node = _nodes.request(root_node_id);
		node->clear();
		_root_node_id = root_node_id;
	}
}

void refit_upward(uint32_t p_node_id)
{
	refit_all();
	return;

	TNode &tnode = _nodes[p_node_id];

	// update children AABBs
	if (!tnode.is_leaf())
	{
		for (int n=0; n<tnode.num_items; n++)
		{
			Item &item = tnode.get_item(n);
			const TNode &tchild = _nodes[item.item_ref_id];

			item.aabb = tchild.aabb;
		}
	}

	// update overall aabb from the children
	tnode.update_aabb_internal();

	if (tnode.parent_tnode_id_p1)
	{
		refit_upward(	tnode.parent_tnode_id_p1-1);
	}
}

ABB * refit_downward(uint32_t p_node_id)
{
	TNode &tnode = _nodes[p_node_id];

	// do children first
	if (!tnode.is_leaf())
	{
		for (int n=0; n<tnode.num_items; n++)
		{
			Item &item = tnode.get_item(n);
			item.aabb = *refit_downward(item.item_ref_id);
		}
	}

	tnode.update_aabb_internal();

	return &tnode.aabb;
}
