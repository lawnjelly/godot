#ifdef BVH_DEBUG_DRAW
	void draw_debug(ImmediateGeometry * p_im)
	{
#ifdef BVH_VERBOSE_FRAME
		_recursive_print_tree(_root_node_id);
#endif


		p_im->clear();
		//im->begin(Mesh::PRIMITIVE_TRIANGLES, NULL);
		p_im->begin(Mesh::PRIMITIVE_LINES, NULL);

		for (int n=0; n<NUM_TREES; n++)
		{
			if (_root_node_id[n] == -1)
				continue;

			_debug_draw_node(p_im, _root_node_id[n]);
		}
		//im->add_vertex(m_DebugPortalLightPlanes[n]);
		p_im->end();
	}

	void _debug_draw_AABB(ImmediateGeometry * p_im, const BVH_ABB &abb)
	{
		Vector3 mins, maxs;
		mins = -abb.neg_min;
		maxs = abb.max;

		Vector3 pts[8];
		pts[0] = Vector3(mins.x, mins.y, mins.z);
		pts[1] = Vector3(mins.x, maxs.y, mins.z);
		pts[2] = Vector3(maxs.x, maxs.y, mins.z);
		pts[3] = Vector3(maxs.x, mins.y, mins.z);

		pts[4] = Vector3(mins.x, mins.y, maxs.z);
		pts[5] = Vector3(mins.x, maxs.y, maxs.z);
		pts[6] = Vector3(maxs.x, maxs.y, maxs.z);
		pts[7] = Vector3(maxs.x, mins.y, maxs.z);


		p_im->add_vertex(pts[0]);
		p_im->add_vertex(pts[1]);
		p_im->add_vertex(pts[1]);
		p_im->add_vertex(pts[2]);
		p_im->add_vertex(pts[2]);
		p_im->add_vertex(pts[3]);
		p_im->add_vertex(pts[3]);
		p_im->add_vertex(pts[0]);

		p_im->add_vertex(pts[4+0]);
		p_im->add_vertex(pts[4+1]);
		p_im->add_vertex(pts[4+1]);
		p_im->add_vertex(pts[4+2]);
		p_im->add_vertex(pts[4+2]);
		p_im->add_vertex(pts[4+3]);
		p_im->add_vertex(pts[4+3]);
		p_im->add_vertex(pts[4+0]);


		p_im->add_vertex(pts[0]);
		p_im->add_vertex(pts[4+0]);
		p_im->add_vertex(pts[1]);
		p_im->add_vertex(pts[4+1]);
		p_im->add_vertex(pts[2]);
		p_im->add_vertex(pts[4+2]);
		p_im->add_vertex(pts[3]);
		p_im->add_vertex(pts[4+3]);

	}

	void _debug_draw_node(ImmediateGeometry * p_im, uint32_t p_node_id)
	{
		TNode &tnode = _nodes[p_node_id];

		p_im->set_color(Color(0.0f, 1.0f, 0.0f, 1.0f));
		_debug_draw_AABB(p_im, tnode.aabb);

		// children?
		if (!tnode.is_leaf())
		{
			for (int n=0; n<tnode.num_items; n++)
			{
				_debug_draw_node(p_im, tnode.items[n].item_ref_id);
			}
		}
		else
		{
			// items
			if (tnode.num_items > 1)
			{
				p_im->set_color(Color(1.0f, 0.2f, 0.2f, 1.0f));
				for (int n=0; n<tnode.num_items; n++)
				{
					_debug_draw_AABB(p_im, tnode.items[n].aabb);
				}
			}
		}
	}

#endif

#ifdef BVH_VERBOSE
	void _recursive_print_tree(int p_tree_id) const
	{
		if (_root_node_id[p_tree_id] != -1)
			_recursive_print_tree_node(_root_node_id[p_tree_id]);
	}

	String _aabb_to_string(const BVH_ABB &aabb) const
	{
		String sz = "(";
		sz += itos (-aabb.neg_min.x);
		sz += " ~ ";
		sz += itos (aabb.max.x);
		sz += ") (";

		sz += itos (-aabb.neg_min.y);
		sz += " ~ ";
		sz += itos (aabb.max.y);
		sz += ") (";

		sz += itos (-aabb.neg_min.z);
		sz += " ~ ";
		sz += itos (aabb.max.z);
		sz += ") ";

		Vector3 size = aabb.calculate_size();
		float vol = size.x * size.y * size.z;
		sz += "vol " + itos(vol);

		return sz;
	}

	void _recursive_print_tree_node(uint32_t p_node_id, int depth = 0) const
	{
		//return;
		const TNode &tnode = _nodes[p_node_id];

		String sz = "";
		for (int n=0; n<depth; n++)
		{
			sz += "\t";
		}
		sz += itos(p_node_id);

		if (tnode.is_leaf())
			sz += " L ";
		else
			sz += " N ";

		sz += "[";
		for (int n=0; n<tnode.num_items; n++)
		{
			if (n)
				sz += ", ";
			sz += itos(tnode.get_item(n).item_ref_id);
		}
		sz += "]  ";
//		sz += "(";
//		sz += itos(tnode.num_items);
//		sz += ") ";

		sz += _aabb_to_string(tnode.aabb);
		//sz += String(tnode.aabb);
		print_line(sz);

		if (!tnode.is_leaf())
		{
			for (int n=0; n<tnode.num_items; n++)
			{
				const Item &item = tnode.get_item(n);
				_recursive_print_tree_node(item.item_ref_id, depth+1);
			}
		}

	}
#endif

