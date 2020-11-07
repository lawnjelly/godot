int cull_segment(const Vector3 &p_from, const Vector3 &p_to, T **p_result_array, int p_result_max, int *p_subindex_array, uint32_t p_mask)
{
	if (_root_node_id == -1)
		return 0;

	CullParams params;
	params.result_count = 0;
	params.result_max = p_result_max;
	params.result_array = p_result_array;
	params.subindex_array = p_subindex_array;
	params.mask = p_mask;

	params.segment.from = p_from;
	params.segment.to = p_to;

	_cull_segment_recursive(_root_node_id, params);

	return params.result_count;
}

int cull_aabb(const AABB &p_aabb, T **p_result_array, int p_result_max, int *p_subindex_array, uint32_t p_mask) {

	if (_root_node_id == -1)
		return 0;

	CullParams params;
	params.result_count = 0;
	params.result_max = p_result_max;
	params.result_array = p_result_array;
	params.subindex_array = p_subindex_array;
	params.mask = p_mask;

	ABB abb;
	abb.from(p_aabb);

	_cull_aabb_recursive(_root_node_id, abb, params);

	return params.result_count;
}

bool _cull_hit(uint32_t p_ref_id, CullParams &p) const
{
	const ItemExtra &ex = _extra[p_ref_id];

	// take into account masks etc
	// this would be more efficient to do before plane checks,
	// but done here for ease to get started
	if (USE_PAIRS)
	{
		if (!(p.mask & ex.pairable_type))
			return true;
	}

	p.result_array[p.result_count] = ex.userdata;

	if (p.subindex_array)
		p.subindex_array[p.result_count] = ex.subindex;

	p.result_count += 1;

	// filled up all the results
	if (p.result_count >= p.result_max)
		return false;

	return true;
}

bool _cull_segment_recursive(uint32_t p_node_id, CullParams &r_params) const
{
	TNode &tnode = _nodes[p_node_id];

	if (tnode.is_leaf())
	{
		// test children individually
		for (int n=0; n<tnode.num_items; n++)
		{
			const Item &item = tnode.get_item(n);
			if (item.aabb.intersects_segment(r_params.segment))
			{
				uint32_t child_id = item.item_ref_id;

				// full up with results?
				if (!_cull_hit(child_id, r_params))
					return false;
			}
		}
	}
	else
	{
		// test children individually
		for (int n=0; n<tnode.num_items; n++)
		{
			const Item &item = tnode.get_item(n);
			if (item.aabb.intersects_segment(r_params.segment))
			{
				uint32_t child_id = item.item_ref_id;
				if (!_cull_segment_recursive(child_id, r_params))
					return false;
			}
		}
	}

	return true;
}


bool _cull_aabb_recursive(uint32_t p_node_id, const ABB &p_aabb, CullParams &r_params, bool p_fully_within = false) const
{
	TNode &tnode = _nodes[p_node_id];

	if (tnode.is_leaf())
	{
		// test children individually
		for (int n=0; n<tnode.num_items; n++)
		{
			const Item &item = tnode.get_item(n);
			if (p_fully_within || item.aabb.intersects(p_aabb))
			{
				uint32_t child_id = item.item_ref_id;

				// full up with results?
				if (!_cull_hit(child_id, r_params))
					return false;
			}
		}
	}
	else
	{
		if (!p_fully_within)
		{
			// test children individually
			for (int n=0; n<tnode.num_items; n++)
			{
				const Item &item = tnode.get_item(n);
				if (item.aabb.intersects(p_aabb))
				{
					// is the node totally within the aabb?
					bool fully_within = p_aabb.is_within(item.aabb);

					uint32_t child_id = item.item_ref_id;
					if (!_cull_aabb_recursive(child_id, p_aabb, r_params, fully_within))
						return false;
				}
			}
		}
		else
		{
			for (int n=0; n<tnode.num_items; n++)
			{
				uint32_t child_id = tnode.get_item(n).item_ref_id;
				if (!_cull_aabb_recursive(child_id, p_aabb, r_params, p_fully_within))
					return false;
			}
		}

	}

	return true;
}

bool _cull_convex_recursive(uint32_t p_node_id, CullParams &r_params, bool p_fully_within = false) const
{
//	if (aabb.intersects_convex_shape(p_cull->planes, p_cull->plane_count, p_cull->points, p_cull->point_count)) {
	const TNode &tnode = _nodes[p_node_id];

	// is the node totally within the aabb?
//	bool fully_within = tnode.aabb.is_within(p_aabb);

	if (tnode.is_leaf())
	{
		// test children individually
		for (int n=0; n<tnode.num_items; n++)
		{
			const Item &item = tnode.get_item(n);
			if (p_fully_within || (item.aabb.intersects_convex(r_params.hull) != ABB::IR_MISS))
			{
				uint32_t child_id = item.item_ref_id;

				// full up with results?
				if (!_cull_hit(child_id, r_params))
					return false;
			}
		}
	}
	else
	{
		if (!p_fully_within)
		{
			// test children individually
			for (int n=0; n<tnode.num_items; n++)
			{
				const Item &item = tnode.get_item(n);
				typename ABB::IntersectResult res = item.aabb.intersects_convex(r_params.hull);

				if (res != ABB::IR_MISS)
				{
					uint32_t child_id = item.item_ref_id;
					if (!_cull_convex_recursive(child_id, r_params, res == ABB::IR_FULL))
						return false;
				}
			}
		}
		else
		{
			for (int n=0; n<tnode.num_items; n++)
			{
				uint32_t child_id = tnode.get_item(n).item_ref_id;
				if (!_cull_convex_recursive(child_id, r_params, p_fully_within))
					return false;
			}
		}

	}

	return true;
}

int cull_convex(const Plane * p_planes, int p_num_planes, T **p_result_array, int p_result_max, uint32_t p_mask)
{
	if (!p_num_planes || (_root_node_id == -1))
		return 0;

	Vector<Vector3> convex_points = Geometry::compute_convex_mesh_points(p_planes, p_num_planes);
	if (convex_points.size() == 0)
		return 0;

	CullParams params;
	params.result_count = 0;
	params.result_max = p_result_max;
	params.result_array = p_result_array;
	params.subindex_array = nullptr;
	params.mask = p_mask;

	params.hull.planes = p_planes;
	params.hull.num_planes = p_num_planes;
	params.hull.points = &convex_points[0];
	params.hull.num_points = convex_points.size();

	_cull_convex_recursive(_root_node_id, params);

	return params.result_count;
}
