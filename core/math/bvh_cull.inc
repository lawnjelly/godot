void _translate_cull_hits(CullParams &p)
{
	int num_hits = _cull_hits.size();
	int left = p.result_max - p.result_count_overall;

	if (num_hits > left)
		num_hits = left;

	int out_n = p.result_count_overall;

	for (int n=0; n<num_hits; n++)
	{
		uint32_t ref_id = _cull_hits[n];

		const ItemExtra &ex = _extra[ref_id];

		// take into account masks etc
		// this would be more efficient to do before plane checks,
		// but done here for ease to get started
//		if (USE_PAIRS)
//		{
//			const ItemExtra &ex = _extra[p_ref_id];

//			if (!(p.mask & ex.pairable_type))
//				return true;
//		}

		p.result_array[out_n] = ex.userdata;

		if (p.subindex_array)
			p.subindex_array[out_n] = ex.subindex;

		out_n++;
	}

	p.result_count = num_hits;
	p.result_count_overall += num_hits;
}

//int cull_convex(const Plane * p_planes, int p_num_planes, T **p_result_array, int p_result_max, uint32_t p_mask, bool p_translate_hits = true)
int cull_convex(CullParams &r_params, bool p_translate_hits = true)
{
//	Vector<Vector3> convex_points = Geometry::compute_convex_mesh_points(p_planes, p_num_planes);
//	if (convex_points.size() == 0)
//		return 0;

	_cull_hits.clear();
	r_params.result_count = 0;

//	CullParams params;
//	params.result_count = 0;
//	params.result_max = p_result_max;
//	params.result_array = p_result_array;
//	params.subindex_array = nullptr;
//	params.mask = p_mask;

//	params.hull.planes = p_planes;
//	params.hull.num_planes = p_num_planes;
//	params.hull.points = &convex_points[0];
//	params.hull.num_points = convex_points.size();

	for (int n=0; n<NUM_TREES; n++)
	{
		if (_root_node_id[n] == -1)
			continue;

		_cull_convex_recursive(_root_node_id[n], r_params);
	}

	if (p_translate_hits)
		_translate_cull_hits(r_params);

	return r_params.result_count;
}

//int cull_segment(const Vector3 &p_from, const Vector3 &p_to, T **p_result_array, int p_result_max, int *p_subindex_array, uint32_t p_mask, bool p_translate_hits = true)
int cull_segment(CullParams &r_params, bool p_translate_hits = true)
{
	_cull_hits.clear();
	r_params.result_count = 0;


	for (int n=0; n<NUM_TREES; n++)
	{
		if (_root_node_id[n] == -1)
			continue;

		_cull_segment_recursive(_root_node_id[n], r_params);
	}

	if (p_translate_hits)
		_translate_cull_hits(r_params);

	return r_params.result_count;
}

//int cull_point(const Vector3 &p_point, T **p_result_array, int p_result_max, int *p_subindex_array, uint32_t p_mask, bool p_translate_hits = true)
int cull_point(CullParams &r_params, bool p_translate_hits = true)
{

	_cull_hits.clear();
	r_params.result_count = 0;

	for (int n=0; n<NUM_TREES; n++)
	{
		if (_root_node_id[n] == -1)
			continue;

		_cull_point_recursive(_root_node_id[n], r_params);
	}

	if (p_translate_hits)
		_translate_cull_hits(r_params);

	return r_params.result_count;
}

//int cull_aabb(const AABB &p_aabb, T **p_result_array, int p_result_max, int *p_subindex_array, uint32_t p_mask, bool p_translate_hits = true)
int cull_aabb(CullParams &r_params, bool p_translate_hits = true)
{
	_cull_hits.clear();
	r_params.result_count = 0;


	for (int n=0; n<NUM_TREES; n++)
	{
		if (_root_node_id[n] == -1)
			continue;

		_cull_aabb_recursive(_root_node_id[n], r_params);
	}


	if (p_translate_hits)
		_translate_cull_hits(r_params);

	return r_params.result_count;
}

bool _cull_hit(uint32_t p_ref_id, CullParams &p)
{
	//const ItemExtra &ex = _extra[p_ref_id];

	// take into account masks etc
	// this would be more efficient to do before plane checks,
	// but done here for ease to get started
	if (USE_PAIRS)
	{
		const ItemExtra &ex = _extra[p_ref_id];

		if (!(p.mask & ex.pairable_type))
			return true;
	}

	_cull_hits.push_back(p_ref_id);

	return true;
}

bool _cull_segment_recursive(uint32_t p_node_id, CullParams &r_params)
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

bool _cull_point_recursive(uint32_t p_node_id, CullParams &r_params)
{
	TNode &tnode = _nodes[p_node_id];

	if (tnode.is_leaf())
	{
		// test children individually
		for (int n=0; n<tnode.num_items; n++)
		{
			const Item &item = tnode.get_item(n);
			if (item.aabb.intersects_point(r_params.point))
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
			if (item.aabb.intersects_point(r_params.point))
			{
				uint32_t child_id = item.item_ref_id;
				if (!_cull_point_recursive(child_id, r_params))
					return false;
			}
		}
	}

	return true;
}


bool _cull_aabb_recursive(uint32_t p_node_id, CullParams &r_params, bool p_fully_within = false)
{
	TNode &tnode = _nodes[p_node_id];

	if (tnode.is_leaf())
	{
		// test children individually
		for (int n=0; n<tnode.num_items; n++)
		{
			const Item &item = tnode.get_item(n);
			if (p_fully_within || item.aabb.intersects(r_params.abb))
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
				if (item.aabb.intersects(r_params.abb))
				{
					// is the node totally within the aabb?
					bool fully_within = r_params.abb.is_within(item.aabb);

					uint32_t child_id = item.item_ref_id;
					if (!_cull_aabb_recursive(child_id, r_params, fully_within))
						return false;
				}
			}
		}
		else
		{
			for (int n=0; n<tnode.num_items; n++)
			{
				uint32_t child_id = tnode.get_item(n).item_ref_id;
				if (!_cull_aabb_recursive(child_id, r_params, p_fully_within))
					return false;
			}
		}

	}

	return true;
}

bool _cull_convex_recursive(uint32_t p_node_id, CullParams &r_params, bool p_fully_within = false)
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

